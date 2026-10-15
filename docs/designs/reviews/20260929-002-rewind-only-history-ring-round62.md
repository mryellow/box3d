---
title: Codex review - rewind-only history ring, round 62
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round61.md
mode: full pass, same prompt and scope as round 40, on revision 59
---

# Round 62

## Exact request sent to Codex

Identical to round 61 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T23:23:24+10:00 to 2026-10-15T23:31:49+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13fbb-5951-7be2-888c-8cbed5e55865`.

## Review report (Codex final message)

## 1. Summary

The current source supports the document’s account of the four broad-phase trees and the sorted continuous and explosion candidates. The journal ownership and restore order I traced are consistent, but the proposed hot shape accessor leaves a gap in the claimed compile-time enforcement of material journaling.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §§5.2, 7.3 | The hot accessor returns a writable whole `b3Shape`. Although `materials` has a const pointee, that type still permits reassignment of the pointer and `materialCount` without `b3Shape_WriteMaterial`. The image does not copy material-block contents, and §9 excludes their pointer and count from record restoration. Thus the stated compiler boundary cannot guarantee that an awake shape’s material block is journaled (`src/shape.h`, `src/shape.c`). | Give hot code writable access only to the shape fields it updates, such as `aabb` and `fatAABBs`; keep material ownership and count writable only inside the material mutator. |

## 3. Checked, no change

- The live broad phase has separate awake and sleeping dynamic trees and a migration queue; the design accounts for them (`src/broad_phase.c`, `src/broad_phase.h`).
- Pair discovery, continuous candidates, and explosion hits use the ordering described in §3 (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The traced pool, sensor, contact, island, solver-set, and hull ownership sequences are compatible with the proposed reverse journal walk (`src/id_pool.c`, `src/sensor.c`, `src/contact.c`, `src/island.c`, `src/solver_set.c`, `src/shape.c`, `src/physics_world.c`).
- The phase-0 adaptations identify the serializer’s missing migration queue, `userData`, event cleanup, and `bodyMoveIndex` handling (`src/world_snapshot.c`).

## 4. Proposed edits

Revise §§5.2 and 7.3 so the hot shape interface cannot write material pointers or counts, and state the resulting compile-time boundary explicitly.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (M) the hot accessor's writable shape record exposes `materials` and `materialCount`:
  confirmed, applied.** §5.2 had the hot accessor return "a record of an awake owner" for a shape
  as for a body or contact. A shape's `materials` field is pointer-to-const, which stops writes
  through it, not assignment of it, and `materialCount` is a plain int; the image carries the
  shape record without them (§9 step 3) and never the block's bytes, so a hot-path assignment of
  either would be unjournaled and unimaged, inside the boundary §5.2 claims the compiler closes.
  The step writes only a shape's `aabb` and its `fatAABBs` entry (`solver.c`), which §5.2 already
  named. The hot accessor now returns an awake shape's `aabb` and `fatAABBs` entry and never its
  record, and the sentence that said the accessor restricts the owner, not the field, says so for
  a shape. A contact's record is still returned whole: its manifold block is imaged and the narrow
  phase allocates and frees it in place, as §5.2's owned-blocks paragraph states. §7.3's claim
  stands with the shape view narrowed; no §7.3 edit.

Raw log: the reviewer read no round record or changelog.

## Status

One finding, Medium, in §5.2, which also produced round 61's finding: two rounds running, one
short of the U-S4 stop. Both are single sentences that named the wrong array or the wrong width
of access, not machinery added; the fix is a clause (U-T3). Not a consequence of round 61's fix
(the joint-sim array), nor of revision 57's. Fix folded as revision 60. A fresh clean round and
its confirmation are required on revision 60 (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). §5.2 two rounds running (61, 62); the
finding is not a consequence of the previous round's fix (U-T2); the reviewer read no round record
or changelog. Doc length 1195 -> 1197 lines; added lines for the fix: two (U-T3). Over the split
check (U-T6): unchanged, the tree and ring decisions already delegated.

Round 62: 1 findings (0H-1M-0L), 1 applied, 0 declined
Series total: 123 findings (25H-60M-38L) across 62 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 1 (0H-0M-1L) -> 1 (1H-0M-0L) -> 1 (0H-0M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 2 (0H-1M-1L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L)
