---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 7), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 7

## Review report (Codex final message)

## Summary

The hot-image and cold-journal design is broadly consistent with the source tree. I found two gaps: the proposed state hash can miss simulation-relevant joint ordering, and the geometry-buffer lifetime contract omits buffers supplied through a setter.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12 | The proposed hash includes joint sims "in id order" but does not explicitly include `b3Joint` records or ordered joint arrays. A joint's `localIndex` and `islandIndex` live in its record; overflow joints are solved in array order, and sleep transitions iterate island joints in stored order. Two states with different relevant joint order could therefore have the same stated hash. See `src/joint.h`, `src/solver.c`, `src/solver_set.c`. | Hash joint records, including their indices, or hash the relevant joint arrays in stored order. Cover an order change in the hash tests. |
| 2 | Low | §10 | The caller-owned geometry rule describes buffers "passed at shape creation," but `b3Shape_SetMesh` also stores a caller-supplied `b3MeshData*`. A rewind can restore a pointer to geometry that was later replaced, so the old buffer must remain alive while a retained tick can reference it. See `src/shape.c`, `include/box3d/box3d.h`. | Extend the lifetime rule to buffers supplied by geometry setters, including buffers replaced before the shape is destroyed. |

## Checked, no change

- The existing state hash covers body transforms and velocities only; the broader hash in §12 is needed. See `src/recording.c`.
- Sensor processing sorts and deduplicates overlaps, and its change bit is set when overlap content differs, supporting §5.2's journal trigger. See `src/sensor.c`.
- The tree provides moved-bit clearing and proxy marking operations for §7.4's reconstruction approach. See `src/dynamic_tree.c`.
- The snapshot serializer covers the six id pools, solver sets, sparse records, sensors, islands, trees, pair set, and graph colours claimed for Phase 0. See `src/world_snapshot.c`.

## Proposed edits

1. In §12, specify which joint indices or ordered arrays the hash covers, and add a test where overflow or island joint order changes while joint sim values remain the same.
2. In §10, replace "passed at shape creation" with wording covering creation **and geometry setters**, and state that replaced buffers remain live until no retained tick can restore their pointers.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:01:20+10:00, end 20:04:48+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 4 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–6 (eight items, including the sharpened wording for item 1
after round 6's dispute of it) was included in the prompt; Codex did not re-raise or dispute any
of them.

Each finding independently re-verified against source (via a forked sub-agent):

- **#1 (state hash omits joint-array storage order, so two states with the same joint sim values
  in different overflow/island order could hash equal), DECLINED — verified not a defect.**
  Confirmed `b3Joint` and `b3JointSim` are distinct structs (`joint.h`), that `b3Joint` carries
  `localIndex`/`islandIndex`, and that an overflow joint array exists (`B3_OVERFLOW_INDEX`,
  `constraint_graph.h`) whose serial solve (`b3SolveJoints_Overflow`, `joint.c`) is genuinely
  order-sensitive in floating point — the same channel §11.2(4) already documents for overflow
  contacts. But the finding models restore as a per-id *reconstruction* of the joint array, which
  is not how §9 step 3 restores hot state: `colors[].jointSims` (overflow included — it is just
  the array's last colour slot) is restored by a flat memcpy of the captured image bytes, not
  rebuilt field-by-field from journaled per-id writes. A memcpy has no freedom to place matching
  values at a different array position: if two images hash equal, they are the same bytes, so
  storage order is identical too, by construction — the hash needs to cover the *values* the
  image contains, which it already does via "joint sims... in id order" reading each captured
  record, not the physical byte layout that produced them. For a retained-but-unimaged tick, §6
  replays forward from the nearest image, and §3's determinism guarantee (same inputs reproduce
  the same run) makes that replay reproduce identical order, not merely identical values. Not
  applied; no doc change needed.

- **#2 (caller-contract lifetime rule for geometry buffers omits setter-supplied buffers),
  CONFIRMED, applied.** Confirmed `b3Shape_SetMesh( shapeId, const b3MeshData* meshData, b3Vec3
  scale )` exists (`box3d.h`, `shape.c`) and stores the caller pointer directly
  (`shape->mesh.data = meshData`), an ordinary `b3Shape_Set*` write already covered by §5.2's
  unconditional journaling of non-`aabb`/`fatAABBs` shape fields including geometry — so the old
  pointer *is* journaled and *will* be restored by a rewind, but §10's caller-obligation wording
  said only "the pointer passed at shape creation," never mentioning a setter-replaced buffer. A
  caller that frees an old mesh buffer right after calling `SetMesh` (reasonable, since the shape
  no longer references it) would leave a dangling pointer live in the journal, restorable by a
  later rewind. Fixed by extending §10's wording to cover a pointer installed by a later geometry
  setter, not just shape creation, and to trigger the caller's keep-alive obligation on the
  pointer's replacement as well as the shape's destruction.

## Status

Round 7 of an ongoing series (rounds 1–6 committed). One of two findings applied; finding rate (8,
8, 8, 7, 8, 3, 1 applied) continues to fall after round 6's spike, and unlike round 6's findings,
neither of this round's findings was a live-state-corrupting algorithm bug — one was a
documentation-completeness gap (a caller obligation that undercounted which buffers it covers),
the other a hash-coverage question that resolved to "already correctly covered" once the actual
restore mechanism (exact-copy for hot state, not per-field reconstruction) was traced precisely.
This is the lowest-severity round of the series so far. Still warrants at least one more
full-scope round before treating the series as converged, since a single low round doesn't yet
establish a trend distinct from round 5's own dip before round 6's spike.
