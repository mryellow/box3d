---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 12), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 12

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible, but two state changes are missing from its restore rules. One breaks the sparse-array invariants after rewinding a newly allocated id; the other makes the proposed raw-tree fallback incomplete.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1, §9 | Undoing an id-pool allocation does not undo growth of its sparse array. New ids append to `bodies`, `shapes` and `fatAABBs`, `contacts`, `joints`, `islands`, or `solverSets`. The journal inventories pool state and record writes, but no length change for these arrays. After rewinding a first-time allocation, the restored pool capacity can disagree with the array count, violating `b3ValidateSolverSets` and changing later allocation behavior. See `body.c`, `shape.c`, `contact.c`, `joint.c`, `island.c`, `solver_set.c`, `physics_world.c`, `id_pool.c`. | Journal sparse-array growth and restore each array's count in both directions, including the paired `shapes`/`fatAABBs` counts. Specify how redo recreates a removed tail slot before applying its record write. |
| 2 | Medium | §7.4, §13 | The raw static-tree fallback says the static tree needs re-imaging only for static shape create/destroy or explicit rebuild because a static proxy never moves. `b3Body_SetTransform` can move a static shape's proxy, and shape setters can reset its proxy. Both can change raw tree state without those listed triggers. See `body.c`, `shape.c`, `broad_phase.c`. | For the phase-1 fallback, capture the static tree after every operation that mutates its proxies or topology; list those operations explicitly. |

## Checked, no change

- The serializer captures all six id pools, solver sets, sparse arrays, sensors, islands, trees, and the constraint graph (`world_snapshot.c`).
- The awake contact inventory correctly includes both graph contacts and the awake set's non-touching contact indices (`constraint_graph.c`, `solver_set.c`, `physics_world.c`).
- Sensor overlap changes are detected through the sensor task's event bit after comparing the current and previous overlap lists (`sensor.c`).
- The documented CCD traversal and eight-sensor-hit dependencies exist in the current code (`solver.c`).
- Static-body contact links can change during contact creation and destruction, so their records need the unconditional structural journaling described in §5.2 (`contact.c`).

## Proposed edits

Add sparse-array append and tail-removal entries to §7.1, name every affected array in §5.2, and include count restoration in §9's journal walk. Amend §7.4's fallback and §13's phase-1 description to cover static proxy moves and resets as raw-tree capture triggers.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:42:15+10:00, end 20:46:33+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 5 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–7 (nine items) was included in the prompt; Codex did not
re-raise or dispute any of them.

Both findings independently re-verified against source directly:

- **#1 (undoing a bump-index id-pool allocation doesn't shrink the paired sparse array back down,
  leaving pool capacity and array count out of sync), CONFIRMED, applied.** Traced `id_pool.c`:
  `b3AllocId` distinguishes a free-list pop (reusing an existing slot) from a bump-index
  allocation (`pool->nextIndex += 1`, a genuinely new id). Traced `b3CreateBody` (`body.c`): a
  bump-index allocation grows `world->bodies` via `b3Array_Push` exactly when `bodyId ==
  world->bodies.count`; otherwise it reuses an existing slot. Confirmed the invariant this
  depends on is explicitly asserted: `b3ValidateSolverSets` (`physics_world.c`) begins with `B3_ASSERT(
  b3GetIdCapacity( &world->bodyIdPool ) == world->bodies.count )` (and the same for contacts,
  joints, islands, solver sets) — pool capacity and array count are required to be exactly equal.
  §9 step 6 already runs this exact validation after every restore in validation builds. Since the
  existing `pool alloc/free` entry kind only mirrors the id pool's own bump/free-list source on
  undo (per its current wording) and says nothing about the paired array, undoing a bump
  allocation would decrement the pool's capacity while leaving the array at its old, larger size —
  directly violating this assertion the very next restore. This is a real, validation-build-visible
  defect, not merely a latent inconsistency. Fixed by extending the existing `pool alloc/free`
  entry kind (not adding a new one — a bump-index allocation's array-length change is always
  exactly ±1, in lockstep with the pool's own bump index, so no additional payload is needed) to
  also grow or shrink the paired sparse array on redo/undo, naming every affected array
  (`bodies`, `shapes`/`fatAABBs`, `contacts`, `joints`, `islands`, `solverSets`).

- **#2 (the static-tree fallback's claim "a static proxy never moves" is false), CONFIRMED,
  applied.** Traced `b3Body_SetTransform` (`body.c`): unlike `b3Body_SetLinearVelocity`/
  `SetAngularVelocity`, which both explicitly early-return for `body->type == b3_staticBody`,
  `SetTransform` has no such exclusion — it unconditionally recomputes every owned shape's `aabb`
  and calls `b3BroadPhase_MoveProxy` when the fat bound no longer contains it, for a static
  body's shapes exactly the same as any other. Confirmed `b3ResetProxy` (`shape.c`) is similarly
  generic, operating on `shape->bodyId`'s body regardless of type. So §7.4/§13's stated re-imaging
  triggers for the raw-tree fallback (create, destroy, explicit rebuild) miss two real triggers:
  an ordinary `SetTransform` on a static body, and any shape setter that resets a static shape's
  proxy. Fixed by naming both as additional re-imaging triggers.

## Status

Round 12 of an ongoing series (rounds 1–11 committed). Both findings genuine and applied. Finding
#1 continues the pattern round 11 found (an array-shape/count invariant left untracked by a
mechanism that only journals individual elements' content) — this time for sparse, id-addressed
arrays rather than dense per-set arrays, confirming the round 11 status note's suspicion that this
class of gap wasn't fully closed by that round's fix alone. Finding #2 is a narrower "stated
trigger list omits a real trigger" gap, the same shape as several earlier rounds' hash/contract
completeness findings. Both were caught with direct source verification this round (no separate
fork dispatch needed for confirmation, given how directly the assertions and code settled each
question) — worth noting for pacing future rounds. Finding rate (8, 8, 8, 7, 8, 3, 1, 2, 1, 1, 2, 2
applied) has now found genuine, source-confirmed defects in every one of the last four rounds
after round 7's low point, arguing against convergence. Given the recurring "array shape/count
invariant not tracked by per-element journaling" pattern across rounds 9, 11, and 12, another
full-scope round is warranted, with specific attention to any other place the doc relies on
per-element or per-id journaling to reconstruct an aggregate (count, order, total) property it
never separately tracks.
