---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 24), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 24

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the current source tree. I found two gaps at the boundary between restored physics state and caller-visible behavior. Both can affect what a game observes after rewind even when the restored state hash matches.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §7.4, §10 | The document acknowledges that closest-ray ties involving mesh or height-field shapes remain tree-order dependent, but §10 does not put that result under the caller contract. The proposed callback tie-break cannot run when an internal cast rejects a hit equal to its current limit. A game using the returned shape to choose its next input can therefore diverge after a tree rebuild. See `src/physics_world.c`, `src/mesh.c`, and `src/height_field.c`. | Make tied closest-ray results deterministic through the internal casts, or explicitly limit the bit-exact caller contract and test that case. |
| 2 | Medium | §9 | Restore preserves a live `userShape` debug handle whenever the shape generation matches. Geometry setters destroy that handle without changing the generation; a later draw can create a handle for the new geometry. Rewinding past the setter can then retain that newer handle while restoring the older geometry. See `src/shape.c` and `src/physics_world.c`. | Invalidate the handle when restored geometry differs from the live geometry, then recreate it lazily. Cover a geometry change, draw, and rewind in a test. |

## Checked, no change

- The existing backward-seek test compares replayed transform and velocity hashes; the document correctly limits what that test demonstrates. See `test/test_recording.c` and `src/recording.c`.
- Sleeping-set wake transfers bodies, contacts, and joints in dense-array order, supporting the design's decision to preserve those arrays. See `src/solver_set.c`.
- A non-awake body transform setter can rewrite shape bounds and move a proxy, as covered by the cold-state inventory. See `src/body.c`.
- Sensor overlap changes are signalled after comparison, and the resulting overlap list is sorted and deduplicated. See `src/sensor.c`.

## Proposed edits

- **§7.4 and §10:** State the supported behavior of `b3World_CastRayClosest` for exact mesh and height-field ties. If deterministic query results are required, specify the internal-cast change and a restore/replay test.
- **§9:** Replace generation-only debug-handle reuse with a check that also accounts for geometry changes within the same shape generation.

## Unresolved / disagreements

- §12 should clarify whether body, shape, and joint `userData` pointers participate in the public hash. Their values can matter to callbacks, while raw pointer values are unsuitable for a cross-process hash. The intended hashing rule is not explicit enough for me to classify this as a separate finding.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-29T06:24:20+10:00, end 06:27:10+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 3 minutes. Raw log had the final message duplicated (a
`codex exec` streaming artifact); one copy is kept above. Codex ran read-only; the working tree was
untouched by it going in (round 23's edits were already applied to the working file, confirmed via
`git status` before the call). The curated declined-findings list carried into this batch (eight
items: sleep→wake AABB bounds, proxy numeric-id reconstruction, the allocation-free-restore half of
`maxBytes`, state-hash joint/solver array storage order, §11.1 figures "unverifiable" under review
scope, the CCD "Change:" paragraph's two equivalent phrasings, and pool `nextIndex` hash coverage —
the ninth, round 3's declined world-`userData` item, was dropped from this batch's list since round
21 already resolved it by imaging world `userData`) was included in the prompt; Codex did not
re-raise or dispute any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, §7.4's
Candidate-set/Presence/Properties restructuring (added across rounds 21–23) was re-read specifically
for internal self-consistency, since it has had three consecutive rounds each fixing a bug in the
immediately preceding round's own fix to that exact section — the highest-risk area for a fourth.
Traced Presence and Properties' division of responsibility (Presence alone decides existence;
Properties only touches a proxy Presence left in place) against every trigger the Candidate-set
bullet lists (awake-at-T, per-shape journaled entries, disabled-transfer, body-type change):
confirmed self-consistent — a body-type change with an already-live proxy correctly falls through
Presence's existence-only check to Properties' own explicit cross-tree-move clause, and a
proxy-reset case behaves the same way. One soft ambiguity noted but not force-fixed: the
Candidate-set clause "a journaled body-type change" doesn't specify how restore distinguishes a
type-changing body-record write from any other non-awake body-record write in the same generic
journal entry kind — left for a fresh reviewer's independent judgment rather than speculatively
resolved, since it wasn't clearly a defect (a full-scope round did in fact run afterward and did not
flag it, for what that's worth on this specific question). This self-check surfaced no gap requiring
a doc edit ahead of Codex's own pass.

Both findings, and the unresolved item, verified directly against source:

- **#1 (§7.4 already documents that a closest-ray tie against a mesh or height-field shape remains
  tree-order dependent — round 22's own finding — but §10, the caller-contract section, never lists
  it alongside the two residual dependencies it does enumerate there), CONFIRMED, applied.**
  Re-read §10's callback-purity bullet: it names exactly two order-dependent cases (CCD-triggered
  `preSolve`, and cast/overlap/mover callback traversal order) as the caller's actual contract
  obligations, while §7.4's own text (added in round 22) already states this is a *third* residual
  order dependence, on top of "the two §10 already documents" — meaning §7.4 itself already
  correctly counts three, but §10, the section a caller actually reads for its obligations, still
  only lists two. A real caller-contract omission, not a restore-algorithm defect. Fixed by adding
  one sentence to §10's callback bullet, immediately after the existing two-case list, naming the
  closest-ray mesh/height-field tie gap and its consequence for a caller that feeds the result back
  as input — matching §7.4's own already-correct count of three.

- **#2 (a geometry setter (`b3Shape_SetSphere`/`SetCapsule`/`SetHull`/`SetMesh`) destroys a shape's
  live debug-draw handle without incrementing `shape->generation`, so §9's generation-only
  handle-invalidation check leaves a stale, geometry-mismatched handle live across a rewind that
  crosses such a setter), CONFIRMED, applied.** Traced every caller of
  `b3DestroyShapeAllocationForShapeChange` (`shape.c`): exactly five — the shape-destroy path and
  the four geometry setters named above — confirming this destroy-and-null the live `userShape`
  handle without any of the other four setter families (`SetUserData`, `SetName`, `SetDensity`,
  `SetFriction`/`SetRestitution`/`SetSurfaceMaterial`/`SetMeshMaterial`, `SetFilter`) touching it.
  Constructed the exact scenario: shape S has geometry G1 and a live handle H1 at tick T; a
  geometry-setter call at T+1 destroys H1 without changing S's generation; a draw call at T+2
  lazily creates H2 for the new geometry G2; `Rewind(T)` restores S's fields to G1, but §9's rule
  (as worded before this round) checks only generation — which still matches, since the shape was
  never destroyed — and leaves H2 untouched, so the next draw uses a valid, non-dangling, but
  *visually wrong* handle (G2's shape drawn where the restored physics state says G1). Distinct
  from round 17's original fix for this same restore step, which targeted a *dangling*-pointer case
  (shape destroyed and a different shape reusing the id) — this is a live, valid-but-stale handle,
  a different failure mode the generation check alone can't catch because generation is defined to
  track shape *identity*, not shape *geometry*. Fixed by extending §9 step 5's condition to also
  destroy the handle when the walked range includes a journaled geometry-changing write for that
  shape, naming the four setters precisely (since journaling itself doesn't distinguish a
  geometry-changing generic record write from any other field's, the same imprecision noted in the
  pre-prompt self-check for the unrelated body-type-change case, but here resolved by naming the
  exact, small, closed set of setters rather than leaving it implicit).

- **(unresolved: does the public hash cover body/shape/joint `userData`?), CONFIRMED as a real
  ambiguity, applied — smaller than Codex's own framing.** Re-read §12 point 1's enumeration: the
  "shapes" entry already explicitly lists only `(filter, material, geometry)`, correctly and
  implicitly excluding shape `userData` by omission — no ambiguity there. But the "bodies" entry
  reads "bodies (excluding `bodyMoveIndex`...)", naming only one exclusion from what would otherwise
  read as the whole `b3Body` record (which does include `userData`, confirmed `body.h`), and the
  "joint records" entry names no exclusions at all — genuinely ambiguous whether joint/body
  `userData` participates, unlike the shapes entry's unambiguous enumeration style. Since round 21
  already established the precedent that a `userData` field is caller-opaque and excluded from the
  hash for exactly this reason (world `userData`, same section), the consistent, minimal fix is to
  state that same exclusion for the body and joint owners too, rather than inventing a new
  hashing rule for them. Fixed by folding all four owner types' `userData` exclusion into one
  clause covering bodies, shapes, joints, and the world together, replacing the world-only
  exclusion clause the sentence previously carried alone.

## Status

Round 24 of an ongoing series (rounds 1–23 committed, most recently as `ab5442d`). Two findings from
Codex, plus one self-identified ambiguity from its own Unresolved section that further inspection
showed to be a real, fixable gap once traced precisely — all three applied, none declined. Unlike
rounds 21–23, none of this round's findings are in §7.4's restore-algorithm core (the pre-prompt
self-check on that section's three-rounds-of-self-correction risk found nothing to report, and
Codex's own pass didn't touch it either) — this round's findings are instead at the *edges* of the
design: a caller-contract section not yet updated to match a residual-dependence count another
section already got right (§10 trailing §7.4), a debug-draw convenience mechanism with an
invalidation rule narrower than the set of live operations that can invalidate it, and a
public-hash enumeration whose exclusion style is inconsistent across owner types. All three are the
same general shape — a fix made correctly in one place (§7.4's residual-dependence list, round 17's
generation-based handle rule, round 21's world-`userData` exclusion) not fully propagated to every
place the same fact needs restating — rather than a newly-discovered mechanism gap. This is a
different failure signature than rounds 21–23's "each round fixes a bug in the immediately preceding
round's own fix," and may indicate the core restore mechanism (§7.4, §9's structural logic) is
nearing settled while the document's cross-referential consistency still has edges to find. Two more
full-scope rounds remain in the currently-requested batch; continuing at the same full scope, not
narrower, since edge-consistency gaps are exactly what a from-scratch full-scope pass catches and a
narrowed one would define away.
