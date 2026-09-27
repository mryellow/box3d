---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 5)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e071-d0b9-7570-8d59-21e8db30add7
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 5)

## Review report (Codex final message)

## Summary

Round 5 fixes several specific faults, but **it has not converged**. The T-record scope rule is
sound in isolation; the proposed transfer cannot yet represent that scope when it cuts through a
current island. The second graph still conflicts with the validators the design requires, and the
shadow, sleep, and history-clock rules leave replay behavior undefined or wrong. These need a
documented revision before the prototype is a meaningful gate.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §6, §8, §9.2 | **The corrected scope can split a current island.** A body with no T-record is now correctly excluded, but Begin still says to move "each scope island." Destroying a contact only unlinks it and marks the island for a later split; a joint may also keep the bodies connected. Island validation (`island.c:690`) requires every body and constraint in an island to have its island's set index. Moving only recorded bodies violates that invariant; moving the whole island reintroduces round 4 #1. Island-only wake has the same problem if a sleeping island contains recorded and unrecorded bodies. | Define how mixed islands are partitioned or represented at Begin and restored at End, including joints and pending splits. Keep unrecorded bodies out of S. |
| 2 | High | §9.2 | **Separate shadow storage fixes duplicate body IDs, but graph ownership and validation remain broken.** The existing validator requires `bodyStates.count == 0` for every set other than awake, counts graph constraints only in `world->constraintGraph`, and expects awake ownership there (`physics_world.c:3670`, `physics_world.c:3864`). Contact validation (`physics_world.c:4026`) treats S as a sleeping set, where it permits touching contacts in `contactIndices` but has no case for G_S. The required Begin/step validation cannot pass as specified. | Define S as a distinct set kind and extend all three validators to count both graphs, accept S's states and contact types, and prove one owner per live constraint. |
| 3 | High | §9.2 | **The shadow audit stops too early.** Updating joint and contact *prepare* to resolve a shadow does not supply its captured velocity to later solver phases. Revolute warm start (`revolute_joint.c:343`) uses a zero-velocity dummy state for a nonindexed endpoint; contact preparation (`contact_solver.c:152`) likewise samples zero velocity for a null index. Those paths change relative velocity, restitution, and impulses. Joint graph creation and removal also hard-code the main graph (`constraint_graph.c:277`); §9.2 names only contact removal for generalization. | Specify endpoint state access through prepare, warm start, solve, and event paths; generalize both contact and joint graph helpers. |
| 4 | High | §9.2 | **The new-pair interception closes the stated contact race, but boundary lifecycle is incomplete.** End describes reconciling a joint through "standard pair-creation/touching-transition," which are contact paths. Joint creation instead assigns graph ownership and calls `b3LinkJoint`, which can wake and merge live islands (`joint.c:300`, `island.c:282`). The design does not define how an existing boundary joint and its island link survive Begin and return at End without doing that work during replay. | Give contacts and joints separate Begin/End ownership and island-link procedures, including constraints created or removed during replay. |
| 5 | High | §9.1 | **`validSinceTick` does not yet prove the proposed sleeping join is safe.** §6 says scope is *exactly* the T-recorded set, while §9.1 allows an unrecorded sleeping body to join. The stamp must also advance when a body enters sleep after T, following ordinary integration; the text explicitly specifies API mutation updates but does not define that transition or how a stamp behaves when `historyTick` is rewound. `SetTransform` (`body.c:1122`) confirms the original mutation problem; sleep transfer (`solver_set.c:216`) shows where a later sleep transition occurs. | Either make all unrecorded bodies boundaries, or define the join exception, sleep-entry stamp, replay-time stamp semantics, and its effect on scope and budget accounting. |
| 6 | Medium | §9.2, §10 | **The single counter resolves the post-End clock question but has a slot offset.** The text says capture writes `historyTick % historyLength` and then increments; setting it to T at Begin therefore writes the first replayed state, T+1, into slot T. It also describes `BeginResimulation(T)`, while the API has no T argument. The solver's separate `stepIndex` is real (`solver.c:1443`). | Define whether `historyTick` names the last completed or next capture tick, give the exact increment/write order for ordinary and replay steps, and align the API with how Begin obtains T. |
| 7 | Medium | §8 step 4 | **Destroy and recreate is genuinely cold, but it changes observable contact identity and events.** Destroy emits an end-touch event and frees the contact ID (`contact.c:340`); creation allocates a contact with a new generation and begins it as non-touching (`contact.c:189`, `contact.c:292`). A still-touching pair can therefore report an artificial end and begin across restore. | State whether restore emits these events and invalidates contact handles; if continuity is intended, specify how to suppress or reconcile the artificial transition. Recheck shape generations when consuming staged impulses if shapes can be replaced during replay. |
| 8 | Low | §13 | The arithmetic is now a single end-of-window position bound, but orientation is only *measured*; no acceptable angular divergence is stated. | Add a final-window angular bound and the orientation metric for the acceptance test. |

The **Remaining risk** paragraph accurately calls the architecture novel, but understates the work
still needed: findings 1–5 are defined invariant and lifecycle conflicts, not merely uncertainty a
prototype might reveal. Its proposed single scene is insufficient as a de-risking gate. After
those rules are revised, the prototype should include a multi-root scene with a current island
containing both recorded and unrecorded bodies, several live and newly formed boundaries, touching
and non-touching contacts, a boundary joint, a sleeping set with multiple islands, and Begin/step/
End validation. One joint type can establish the mechanism, but one quiet benchmark scene cannot
exercise the ownership and interception cases.

## Checked, no change

- Round 4's closed non-touching-manifold invariant remains closed: recreated contacts start
  without a manifold, and impulses stay in the side table (`contact.c:296`, `physics_world.c:4048`).
- Destroy and recreate does not itself defeat the round 4 shape and joint generation checks.
  Consumption-time shape reuse is the narrower new risk in finding 7.
- The kinematic-island and conditional proxy-movement conclusions remain supported
  (`island.c:218`, `body.c:1147`).
- Requirement 2 now expressly excludes broad-phase and sensor work; that wording closes round 4
  #10.

## Round 4 findings: disposition

1. **Still open** — T-record selection fixes the missing baseline, but transferring that exact
   set can split a live island.
2. **Still open** — `shadowSims` fixes duplicate body IDs; S, G_S, and validators still disagree
   about constraint ownership.
3. **Still open** — interception before contact linking is specified; boundary joint lifecycle
   and End reconciliation are not.
4. **Still open** — per-type prepare changes are acknowledged, but warm start and solve still need
   shadow velocity and state.
5. **Still open** — API mutation tracking improves the test; sleep-entry and rewound-clock
   semantics are missing.
6. **Still open** — island-only wake avoids whole-set wake only when the target island can be
   moved without unrecorded members.
7. **Closed** — destroying contacts removes the old mesh triangle caches and impulses
   (`contact.c:434`).
8. **Closed** — the staged record is consulted on the first update regardless of a touch
   transition; recreation removes the already-touching case.
9. **Reopened differently** — one clock defines continuation after End, but its stated write
   order offsets replay slots.
10. **Closed** — requirement 2 now states the world-size cost exceptions.
11. **Still open** — the position tolerance is consistent; orientation still lacks a pass/fail
    bound.
