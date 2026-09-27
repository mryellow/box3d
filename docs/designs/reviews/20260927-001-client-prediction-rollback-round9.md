---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 9)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e08e-8da5-7f00-a84b-8de2b1d2dbf3
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 9)

## Review report (Codex final message)

## Summary

Round 9 closes both **round 8 design gaps**. The new pre-collide rebuild reaches narrow phase
before its full-world gather, and the post-collide rebuild reaches solve after contact and island
changes. The separate pending split slot resolves the relay conflict. **The design has
converged:** the remaining points below are narrower implementation details for the prototype, not
reasons for another design round.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Low | §9.2 | Replacing the gather still leaves `b3Collide` clearing, merging, and scanning contact-state bitsets sized by **world contact ID capacity** (`physics_world.c:881`, `bitset.c:26`). Thus the literal O(scope + boundary) claim needs one more pass-local bookkeeping change. This is narrower than round 8's missing collide input. | Use a sparse, scoped changed-contact collection during resim. |
| 2 | Low | §9.2 | Checking whether a pending island ID is live does not alone establish that it still identifies the *same* island: destruction frees the ID, and allocation can reuse it (`island.c:49`, `id_pool.c:19`). This is a local identity guard, narrower than round 8's missing storage mechanism. | Clear the pending field on island destruction, or retain an identity token that detects reuse. |

## Checked, no change

The pre-collide walk sees contacts created by pair discovery (`physics_world.c:1095`) before
`b3Collide` (`physics_world.c:1134`); the post-collide walk sees touch transitions and waking
before `b3Solve` (`physics_world.c:968`, `physics_world.c:1141`). A newly admitted sleeping
neighbor's other contacts need not have been in that step's pre-collide list: ordinary collide
also gathers before the wake, while wake transfers its existing touching contacts into the graph
for solve (`solver_set.c:129`). Narrow phase reaches those other contacts on the next replayed
step. The list builder should select awake contacts when walking incident edges, since sleeping
contacts also remain on those edges; that is a routine filter, not a further design gap.

## Round 8 findings: disposition

1. **Closed** — The pre-collide list replaces the full gather that round 8 identified
   (`design:367`, `physics_world.c:830`); sparse state-change bookkeeping remains prototype work.
2. **Closed** — Moving the non-scope candidate to `pendingSplitIslandId` frees the ordinary relay,
   and synchronous End handling avoids a two-candidate collision (`design:450`, `solver.c:1717`,
   `solver.c:2200`). Island ID reuse needs the local guard noted above.
