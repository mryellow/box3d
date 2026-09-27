---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 4)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e063-1969-7b03-954f-2e7e6f293558
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 4)

## Review report (Codex final message)

## Summary

Round 4 makes real progress on the restore format and the distinction between capture and replay
scope. **It has not converged yet.** The proposed scope set and graph still conflict with core
ownership and validation rules, and several fixes leave replay using state from the wrong tick.
This is a source review of the design; no files were changed.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §6, §9.2 | **Round 3 #2:** A body that joined a root's current island after T has no T record. The revised design leaves it at its *now* state, then puts it in S and simulates it through T+1…now again. Touching contacts merge islands (`src/island.c:193`, `src/island.c:226`). Recomputing scope resolves membership ambiguity but does not supply the missing historical state. | Define a baseline for every body stepped in replay. Freeze bodies without a T record, require their history, or reject that replay scope before mutation. |
| 2 | High | §9.2 | **Round 3 #1:** Moving coloured contacts into G_S does not, as specified, preserve the engine's ownership invariants. The validators count contacts and joints in the main graph, require awake contacts there, and compare their totals with the global ID pools and `pairSet` (`src/physics_world.c:3864`, `src/physics_world.c:3952`, `src/physics_world.c:4029`). A shadow copied into S also duplicates a live body ID, while validation requires each set's sim to point back to its owning body and the total sim count to equal the body ID count (`src/physics_world.c:3680`, `src/physics_world.c:3858`). The existing graph removal helper hard-codes the main graph and awake set (`src/constraint_graph.c:171`). | Specify separate shadow storage and explicit graph ownership. Extend transfer, lookup, removal, and validation to account for both graphs and exactly one owner per live constraint. Validate at Begin, after each replay step, and at End. |
| 3 | High | §9.2 | **Round 3 #1/#2:** Lazy boundary creation has no working route through the stated "entry point switch." A new scope–outside pair is created in the main awake set only when an endpoint equals `b3_awakeSet`; otherwise it goes to the disabled set (`src/contact.c:172`). The normal touching transition then asserts awake ownership, links the *live* bodies' islands, and adds the contact to the main graph (`src/physics_world.c:968`, `src/island.c:193`). That can merge or wake the outside island before a shadow is installed. End's instruction to "discard every … boundary constraint" also leaves a live contact or joint needing an owner. | Define scope-aware pair creation and touching transitions **before** live island linking. At End, reconcile each surviving live boundary contact or joint into normal ownership; discard only private shadow data. |
| 4 | High | §9.2 | **Round 3 #7:** Zeroing a shadow's inverse mass does not make existing joint code use it. Revolute and prismatic preparation resolve both endpoints through the live `b3Body` table, assert an awake endpoint, and read those bodies' sims; their solver indices are chosen by `setIndex == b3_awakeSet` (`src/revolute_joint.c:266`, `src/revolute_joint.c:276`, `src/revolute_joint.c:288`, `src/prismatic_joint.c:317`, `src/prismatic_joint.c:329`). The same pattern occurs in distance joints (`src/distance_joint.c:265`). A dummy state also has zero velocity, so it cannot represent the specified copy of a moving partner (`src/body.h:183`). | Introduce endpoint resolution that explicitly selects a shadow and defines its solver state, then audit every joint type and contact path. State how a shadow remains consistent with live broad-phase state if its source body changes during replay. |
| 5 | High | §9.1 | **Round 3 #5:** `sleepStartTick ≤ T` does not prove a sleeping body's state is unchanged since T. `b3Body_SetTransform` writes a sleeping body's sim and moves its proxies without waking it (`src/body.c:1112`, `src/body.c:1122`, `src/body.c:1140`). Its old sleep stamp would still permit joining with a future pose. | Track the last relevant state mutation as well as entry to sleep, or explicitly exclude and freeze a sleeping body whose state was changed after T. |
| 6 | Medium | §8, §9.1 | **Round 3 #11:** Whole-set wake is more than a cost side effect. It transfers unrelated bodies to awake and resets their sleep timers (`src/solver_set.c:45`, `src/solver_set.c:55`, `src/solver_set.c:157`). After End, those bodies can participate in ordinary simulation even though the restore did not target them. This also complicates the proposed sleep-stamp guarantee. | Use an island-only restore wake, or document and test the changed post-replay state of every extra body woken. |
| 7 | Medium | §7.1, §8 | **Round 3 #8/#9:** Invalidating mesh `queryBounds` triggers a triangle query, but the query matches and copies old per-triangle SAT/simplex caches (`src/mesh_contact.c:75`, `src/mesh_contact.c:135`). Existing mesh manifolds also carry their old impulses into matching (`src/mesh_contact.c:1007`, `src/mesh_contact.c:1047`). Thus "full query" and "always recreated cold" do not follow from the specified invalidation. | Clear the mesh triangle cache and old mesh warm-start state safely while preserving live touching/contact graph invariants. |
| 8 | Medium | §8 | **Round 3 #3, new interaction:** The side table fixes the non-touching manifold invariant, but the proposed consumption point is only when a pair *transitions* to touching. A contact already touching after restore can remain touching; convex narrow phase then matches against its live *now* manifold (`src/contact.c:550`, `src/contact.c:586`) and never uses T's staged impulses. | Apply a validated staged record on the first post-restore narrow-phase update of any matching pair, including one already touching; define precedence over live impulses. |
| 9 | Medium | §9.2, §10 | **Round 3 #4:** `resimTick` specifies replacement of T+1…now, but the design does not specify the normal capture tick after End. `stepIndex` advances on every replay solve (`src/solver.c:1444`); using it again for normal captures would jump ahead by the replay length. | Define one persistent logical history clock for ordinary steps and replay, including its value at End and the next ordinary step. |
| 10 | Medium | §2, §9.3 | **Round 3 #6:** The O(world) stages are now disclosed accurately, but requirement 2 still promises correction cost proportional to predicted state. The source confirms a dynamic-tree-wide scan when pairs update and a scan of every sensor (`src/broad_phase.c:84`, `src/broad_phase.c:674`, `src/sensor.c:281`). | Either change requirement 2 to the stated v1 cost contract, with a total-time acceptance bound, or bring scoped pair and sensor processing into v1. |
| 11 | Low | §13 | The new tolerance is internally inconsistent: 1% of a 1 m extent **per replay tick** permits 9 cm over nine ticks, while the example requires under 1 cm after nine ticks (`docs/designs/20260927-001-client-prediction-rollback.md:532`). | Choose a final-window bound or a per-tick bound, and specify how rotation is measured. |

## Checked, no change

- **Round 3 #3's structural invariant is closed:** staging impulses outside the contact leaves
  non-touching contacts with no manifold, as required by `src/physics_world.c:4048`. Finding 8
  above concerns whether the staged value is subsequently used.
- **Round 3 #10 is closed as specified:** the design now checks shape and joint generations
  before applying records and makes joint-setting replay a caller obligation. Shape-pair keys
  alone contain no generation (`src/table.h:47`); joint IDs have their own generation
  (`src/joint.h:55`).
- **Round 3 #12 is closed:** enabled kinematics have islands and touching contacts link islands
  (`src/body.c:317`, `src/island.c:218`); proxy movement is correctly described as conditional
  (`src/body.c:1147`).
- Global contact and joint IDs need not be rewound for the stated non-bit-exact contract. Two
  live graphs would, however, need the combined ownership accounting in finding 2.

## Round 3 findings: disposition

1. **Still open** — S/G_S is more concrete, but transfer, lookup, and validation still assume one
   awake set and one graph.
2. **Still open** — current-island scope handles topology selection but steps bodies with no
   T-state baseline.
3. **Closed** — the restore no longer puts a manifold on a non-touching contact.
4. **Reopened differently** — replay slots are addressed, but the first ordinary capture after
   End lacks a defined logical tick.
5. **Still open** — a sleeping body can change state without changing its sleep-entry stamp.
6. **Still open** — the global work is disclosed, while the top-level correction-cost requirement
   remains stronger.
7. **Still open** — tested joint prepare paths read live endpoint sims rather than the proposed
   shadow.
8. **Still open** — mesh triangle caches survive the stated invalidation.
9. **Reopened differently** — omitting mesh records solves the missing-field problem, but
   existing mesh contacts are not made cold.
10. **Closed** — shape and joint generations are checked; joint settings have an explicit caller
    contract.
11. **Still open** — whole-set wake changes unrelated bodies' future sleep and simulation state.
12. **Closed** — kinematic island membership and conditional proxy movement are now described
    correctly.
13. **Closed** — a numeric criterion and validation build are specified; finding 11 above corrects
    the new criterion's arithmetic.
