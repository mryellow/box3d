---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 7)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e082-5352-7ab2-8812-b704ab80b587
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 7)

## Review report (Codex final message)

## Summary

Round 7 closes the joint mass, tick boundary, and Begin/End guard findings. The edge walk also
stays proportional to scope bodies and their incident constraints: following `nextKey` does not
traverse a boundary partner's own edge list. **The design has not converged yet.** Two correctness
rules still fail when checked against the order of a real step and the engine's island sleep
behavior. I recommend one targeted documented revision before the prototype. This was a read-only
review; no files were modified.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §9.2 | **One rebuild at the start of a step is already stale before solve.** Pair discovery creates contacts after that point; collision then destroys contacts and moves them between the non-touching array and graph colours before `b3Solve` runs (`physics_world.c:1095`, `physics_world.c:1134`, `physics_world.c:963`, `physics_world.c:995`). The lists specified at `design:375` can therefore miss a new contact in collision or solve, or carry a removed contact into solve. | Build the collision subset after pair discovery, then rebuild or reconcile the solver subsets after collision's state changes and before solve. Admit eligible sleeping neighbors at that boundary. |
| 2 | High | §9.1–9.2 | **A scope member does not make an entire mixed island safe to sleep.** Linking a contact wakes a sleeping neighbor's whole solver set and merges islands (`island.c:193`, `island.c:226`). Finalization visits only scope bodies, so the proposed "has a scope-body member" gate can call `b3TrySleepIsland` when the scope body's timer expires even though an unvisited non-scope member should keep the island awake. That call transfers *every* island body to sleep (`solver.c:816`, `solver.c:2230`, `solver_set.c:216`). This breaks the frozen-body contract, including the sleeping-neighbor join case. | Define mixed-island sleep behavior explicitly. For example, defer sleeping a mixed island during replay, while allowing ordinary sleep transitions for all-scope islands. |
| 3 | Medium | §9.2 | **A pending island split can still process an unrelated island.** `b3Solve` schedules `world->splitIslandId` without a scope check, and splitting rewrites island membership (`solver.c:1710`, `island.c:574`). A candidate left by the preceding ordinary step can therefore change a non-scope island during replay, despite the proposed sleep-pass gate. | Specify how an unrelated pending split is deferred and preserved across the bracket; run scope-relevant splits when needed. |
| 4 | Low | §9.2 | The `historyTick + 1` timing is sound for a mutation before the next capture and for sleep entry. The accompanying claim that stamps from different brackets cannot coexist is false: `validSinceTick` is persistent per body (`design:435`, `design:454`). I found no resulting false-positive join, but that sentence cannot serve as the comparability argument. | State the conservative comparison argument across later rewinds. In the implementation audit, include existing sleeping-body mutators such as `SetMassData`, not just the named examples (`body.c:1856`). |

## Checked, no change

- Rebuilding from each scope body's `headContactKey` and `headJointKey` walks that body's incident
  edges; the selected edge's `nextKey` remains on the same body's chain (`physics_world.c:3556`,
  `physics_world.c:3581`). With deduplication, this is O(scope bodies + incident contacts and
  joints), including boundary constraints.
- The compact colour subsets and pass-local parallel buffers are coherent in principle. The solver
  does require subset counts, spans, packing, and overflow work, as round 7 now says
  (`solver.c:1514`, `solver.c:1664`). Joint stages must still reach the **live** `b3JointSim`
  objects, which preparation and solving mutate (`solver.c:225`, `solver.c:279`); that is a
  prototype implementation requirement, not another ownership design problem.

## Round 6 findings: disposition

1. **Closed** — Round 7 requires zero inverse mass and inertia before joint effective-mass
   calculations and keeps contact index overrides pass-local (`revolute_joint.c:279`,
   `contact_solver.c:86`).
2. **Reopened differently** — Unrelated islands are no longer swept indiscriminately, but
   sleeping a mixed island still transfers non-scope members (finding 2).
3. **Still open** — Per-step rebuilding fixes stale entries across steps, but the chosen rebuild
   point misses changes *within* a step (finding 1).
4. **Closed** — The required subset counts, spans, packed buffers, and serial overflow path are
   now explicitly acknowledged as prototype work.
5. **Closed** — `historyTick + 1` fixes the reported tick-boundary error; the persistent-stamp
   rationale needs the limited correction in finding 4.
6. **Closed** — `hasPendingRestore`, `pendingRestoreTick`, and the saved `resimTargetTick` make
   the Begin and End guards concrete.
