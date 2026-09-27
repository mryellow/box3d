---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 8)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e089-762d-7d21-968a-9a0df2f65345
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 8)

## Review report (Codex final message)

## Summary

Round 8 fixes the stale **solver** list and the mixed-island sleep rule. The after-collide hook
is real, and admitting a sleeping neighbor does not bypass the all-scope sleep gate. **The design
has not converged yet:** it still needs a list to drive scoped collide *before* collide runs, and
the deferred-split rule needs a way to coexist with newly scheduled splits.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §9.2 | The new rebuild point is correct for solve, but it is too late to supply collide's input. Collide gathers its contacts and launches work inside `b3Collide` (`physics_world.c:830`), before the proposed rebuild at the `b3Collide`/`b3Solve` boundary (`physics_world.c:1134`). With no earlier subset specified, collide still scans the full awake contact collection, contrary to the scoped collide mechanism in §9.2 (`design:367`). | Build a pre-collide subset after pair discovery, then rebuild the solver subset after collide and sleeping-neighbor admission. |
| 2 | Medium | §9.2 | The deferred-split rule prevents an unrelated split, but "preserve as-is" cannot simply use the current single `splitIslandId` slot. Solve clears that slot, asserts it is empty before collecting a new candidate, and writes a candidate into it (`solver.c:1861`, `solver.c:2201`). A deferred mixed-island candidate and a new all-scope candidate can coexist. Collide can also merge away the deferred island, which clears its id (`island.c:166`, `island.c:49`). | Specify separate handling for deferred and newly scheduled candidates, and discard or retarget a deferred candidate whose island no longer exists. |

## Checked, no change

- **The hook is unambiguous.** `b3Collide` completes its parallel contact work and serial state
  changes before returning; `b3World_Step` then calls `b3Solve` (`physics_world.c:893`,
  `physics_world.c:938`, `physics_world.c:1134`). A solver-list rebuild fits between those calls.
- **Sleeping-neighbor admission composes with the sleep gate.** Linking the new contact wakes the
  sleeping set and merges islands (`island.c:206`, `island.c:226`). If any member remains outside
  scope, the resulting island is mixed and the all-scope gate (`design:431`) excludes it from
  replay sleep processing. Admission creates no additional sleep exception.
- The revised `validSinceTick` comparison is conservative, and `b3Body_SetMassData`
  (`body.c:1856`) is now named. The claim that a later mutation always writes a *larger* stamp is
  imprecise during a rewind; the new event can receive a lower tick number while still correctly
  defining the current state. That wording does not establish a false-positive join.

## Round 7 findings: disposition

1. **Reopened differently** — The after-collide rebuild closes stale solver input, but leaves
   scoped collide without a pre-collide list.
2. **Closed** — Only an all-scope island may enter replay sleep processing, including after
   sleeping-neighbor admission.
3. **Reopened differently** — The scope gate stops the unrelated split; preserving its candidate
   needs explicit handling alongside new candidates and island invalidation.
4. **Closed** — The persistent-stamp rationale and `SetMassData` audit entry are corrected; the
   "larger" wording above is editorial.
