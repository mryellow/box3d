---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 3)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e050-77ab-7220-8330-302165afa782
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 3)

## Review report (Codex final message)

## Summary

Round 3 removes the need to rewind global ID pools or reproduce graph slots bit for bit. It does
not yet establish a consistent scoped restore and replay. The main gaps are how a saved scope is
separated from *current* island topology, how recorded contacts are seeded without violating live
contact invariants, and how an active solver set avoids work in global step stages.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §9.2 | The active-set change is larger than replacing `b3_awakeSet` checks. The proposed migration path cannot move a body with contacts: `b3TransferBody` asserts it has none (`src/solver_set.c:567`). Set IDs above the awake set are treated as sleeping by wake and linking code (`src/body.c:340`, `src/island.c:206`); the solver and contact paths use the single world graph (`src/solver.c:1465`, `src/physics_world.c:833`). | Specify distinct active and sleeping set types, migration of bodies **with** contacts, graph routing, and the required changes to set-aware helpers before calling this a mechanical refactor. |
| 2 | High | §§6, 8–9 | Scope membership at T need not match an island at "now." A later touching contact merges islands (`src/island.c:218`); an existing contact or joint across the intended scope boundary is already linked. Suppressing *new* links during replay cannot partition that live island, and connectivity validation expects touching contacts to share their body's island (`src/physics_world.c:3552`). | Define the replay scope from saved body identities, then specify how existing boundary constraints and islands are partitioned or handled without changing frozen bodies. |
| 3 | High | §8.4 | Seeding a recreated **non-touching** contact with a recorded manifold immediately violates the engine's invariant. Creation puts it in a set's non-touching array (`src/contact.c:292`); validation requires such contacts to have no manifold (`src/physics_world.c:4048`, `src/physics_world.c:4068`). This also contradicts the claim that restore never leaves an inconsistent world. | Keep saved impulses in a side table until narrow phase establishes touching, or transition the contact through the full normal linking and graph path during restore. Validate immediately after restore. |
| 4 | High | §§7, 9–10 | Replay will not overwrite slots T+1…now as written. `stepIndex` advances in `b3Solve` (`src/solver.c:1444`); §8 does not rewind it. After restoring T while the live world is at "now," replay steps receive later indices. | Define a separate logical replay tick or explicitly manage `stepIndex` and ring-slot replacement across restore, replay, and exit. |
| 5 | High | §9.1 | A body sleeping **now** is not necessarily valid at T. It may have moved between T and the later tick when it slept; sleep migration copies its then-current simulation state (`src/solver_set.c:216`). Letting it join replay from "now" mixes timelines just as the frozen-awake rule seeks to avoid. | Track when an outside body last entered sleep and permit joining only when its state is known valid at T; otherwise keep it frozen or require it in history. |
| 6 | High | §§2, 9.2–9.3 | Making the solver active set variable does not make each `b3World_Step` O(scope). Moved scope proxies trigger global pair discovery, including a scan through the dynamic tree's nodes (`src/broad_phase.c:649`, `src/broad_phase.c:83`). Sensor overlap processing iterates every sensor (`src/sensor.c:281`). The §9.3 body-fraction estimates therefore omit potentially dominant work and unrelated sensor events. | Design scoped pair and sensor processing, or weaken the cost and event contracts and measure these stages separately in §13. |
| 7 | Medium | §9.1 | The dummy-body path is not sufficient for frozen bodies attached by joints. For example, distance-joint preparation reads both bodies' live inverse masses and inertias before choosing a dummy state (`src/distance_joint.c:265`, `src/distance_joint.c:280`). A frozen dynamic body would contribute mass to the effective constraint while receiving no state update. | Define boundary-joint behavior and zero the frozen side's effective mass and inertia throughout joint preparation and solving. |
| 8 | Medium | §8.4 | Clearing `b3_relativeTransformValid` only disables contact recycling (`src/physics_world.c:653`). Convex narrow phase still consumes the stored SAT or simplex cache (`src/contact.c:488`, `src/contact.c:519`); mesh contacts can reuse a cached triangle query when its old bounds contain the new bounds (`src/mesh_contact.c:75`). The promised "full query" does not follow from clearing that flag. | Specify cache invalidation for convex and mesh contacts, including mesh query bounds and triangle cache, or qualify the full-query claim. |
| 9 | Medium | §7.1, §8.4 | The compact warm-start format lacks fields needed for mesh matching. Mesh narrow phase matches old manifolds by normal, then points by both `featureId` **and** `triangleIndex` (`src/mesh_contact.c:1017`, `src/mesh_contact.c:1069`). The record includes neither normal nor triangle index, so seeding it cannot reproduce the described carry-over for mesh contacts. | Add mesh matching fields, define a mesh-specific matching method, or state that mesh warm starts are discarded on restore. |
| 10 | Medium | §§7–8, 11 | Body generation alone does not validate saved contact and joint records. Shape-pair keys encode shape indices and child index, not shape generations (`src/table.h:47`); reused shapes and joints advance their own generations (`src/shape.c:166`, `src/joint.c:220`). The restore sequence does not say to check the saved joint generation before writing its live sim. Joint settings can also change during the window (`src/distance_joint.c:15`). | Validate shape and joint generations and compatible types before seeding; say whether joint-setting mutations are recorded or must be replayed by the caller. |
| 11 | Medium | §§6, 8.2 | Waking one sleeping scope island may wake unrelated islands. A sleeping solver set can contain multiple islands (`src/solver_set.c:157`), and `b3WakeSolverSet` transfers every body and island in the set (`src/solver_set.c:45`, `src/solver_set.c:161`). It also resets their sleep timers (`src/solver_set.c:55`). | Account for and test whole-set wake side effects, or provide an island-only wake path. |
| 12 | Low | §§6, 8.3 | The kinematic-partner caveat can be resolved more precisely: enabled kinematic bodies get islands (`src/body.c:317`), and touching contacts merge the two non-static bodies' islands (`src/island.c:218`). A touching kinematic partner is therefore already in the selected island. Also, the cited `b3Body_SetTransform` path moves a proxy only when its new AABB escapes the stored fat AABB (`src/body.c:1143`). | Clarify that kinematic inclusion is already supplied by island membership and describe the conditional proxy move accurately. |
| 13 | Low | §13 | The quality criterion says divergence must stay within "a stated tolerance," but gives no tolerance or comparison window. The robustness checks also need to run in a validation build; the validator bodies are compiled under `B3_ENABLE_VALIDATION` (`src/physics_world.c:3534`). | State numeric quality bounds and require validation-enabled benchmark runs. |

## Checked, no change

- Dropping bit exactness removes the round 2 need to rewind global contact IDs and reconstruct the
  former graph-slot order. Contacts are assigned IDs from the world pool (`src/contact.c:189`); the
  live graph owns its current contact slots (`src/constraint_graph.c:149`).
- Island contact arrays contain touching contacts, so using them to capture touching warm starts is
  consistent with the engine's island model (`src/island.h:43`). Non-touching contacts have no
  manifold under the live invariant (`src/physics_world.c:4048`).
- The proposed body transform update follows the existing inertia and AABB update pattern
  (`src/body.c:1122`); it needs the conditional proxy behavior noted above.
- The source supports the motivation for removing the sleep exemption: island split candidates arise
  when a body is sleepy and its island has removed constraints (`src/solver.c:773`, `src/solver.c:824`).
