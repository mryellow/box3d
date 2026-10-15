---
title: Review round 3 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round2.md
mode: full pass
---

# Round 3

## Exact request sent to Codex

Identical to round 1 (U-P4):

```
Review the document docs/designs/20261016-001-implementation-prompt-history-ring.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for three Canonical design documents: docs/designs/20260929-002-rewind-only-history-ring.md (the design: per-tick image, undo journal, restore, caller contract, verification, public API), docs/designs/20261015-003-history-ring-budget-and-eviction.md (the ring doc: slots, ends, interval, budget, window, enable and disable) and docs/designs/20261015-004-history-ring-broad-phase-trees.md (the trees doc: which trees are imaged or derived, proxy identity, the restore pass). The designs decide what changes and why; this document decides the branch, the increments' mechanics, the test mechanism, fixtures, fail-before evidence, the test-to-requirement mapping, the golden-hash policy, the measurements and the closure doc. It is handed to an implementing agent that must not invent behaviour.

Scope: the document's own content; the three designs it implements, which are settled and binding (judge this document against them, never a design against this document); and the source it cites (src/physics_world.h, src/physics_world.c, src/body.h, src/body.c, src/shape.h, src/shape.c, src/contact.h, src/contact.c, src/mesh_contact.c, src/contact_solver.c, src/joint.h, src/joint.c, the eight joint files, src/island.h, src/island.c, src/solver_set.h, src/solver_set.c, src/constraint_graph.h, src/constraint_graph.c, src/sensor.h, src/sensor.c, src/broad_phase.h, src/broad_phase.c, src/dynamic_tree.h, src/dynamic_tree.c, src/id_pool.h, src/id_pool.c, src/container.h, src/bitset.h, src/table.h, src/block_allocator.h, src/core.h, src/core.c, src/world_snapshot.h, src/world_snapshot.c, src/recording.h, src/recording.c, src/replay.h, src/replay.c, src/solver.c, src/hull.h, include/box3d/, test/main.c, test/test_macros.h, test/test_recording.c, test/test_determinism.c, test/test_awake_tree.c, test/test_tree_order.c, test/test_name_cache.c, test/CMakeLists.txt, src/CMakeLists.txt, CMakeLists.txt, shared/benchmarks.h, shared/benchmarks.c, benchmark/main.c, CONTRIBUTING.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the document. Find real defects: a claim about the code in section 2 that is false; an increment in section 4 whose mechanics contradict a design or another increment, or that needs a behaviour neither a design nor this document states; a file an increment touches that its file list omits, or a writer or reader the section 2 inventory misses; a test in section 4 that would not fail before its commit on the check the document names, would not pass after it, or does not show the clause section 6 assigns to it; a fail-before or compile-boundary procedure that cannot be carried out as written; a golden policy in section 5 that would accept a defect or reject a correct change; a measurement in section 7 that cannot be taken as described or would not show what it is said to show; a stop condition in section 9 that contradicts an earlier section; a wrong or inconsistent number or name; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the constraint graph, the id pools, the serializer, the recording and replay, the test runner) as settled tools; judge what this document adds and the question each primitive is asked. Whether section 6 is complete is not in scope; whether each of its entries shows what it claims is. Decisions recorded in section 10 are settled unless fresh source evidence shows their reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, the designs' own decisions (a believed defect in a design goes under Unresolved, not Findings), and what the document assigns to the owner or to the closure doc. Do not review or follow any other design document except where this one or the three designs cite it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-16T06:51:43+10:00 to 06:56:23+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14155-c98c-7f42-ae3d-2c81cec6346c`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is close to the three binding designs, but four issues need edits before it can be handed to an implementer. Two omit live write routes, one makes a test reject a potentially correct state hash, and one leaves the ring tests’ inspection mechanism unclear.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, §4 H3a | `src/shape.c` also writes materials through `b3GetShapeMaterials` in `b3Shape_SetFriction` and `b3Shape_SetRestitution`. H3a names only the surface- and mesh-material setters for migration to the sole writer. | Name both setters in the inventory and H3a migration; exercise them on a cold shape in the journal tests. |
| 2 | M | §2, §4 H4 | The claim that the only tree writes outside `src/broad_phase.c` are the four listed is false. `src/broad_phase.h` has inline `b3BroadPhase_MarkProxyMoved` and `b3BroadPhase_MarkProxyMovedSerial` mutators; `test/test_awake_tree.c` also clears trees directly in a standalone broad-phase test. Making `trees` pointer-to-const requires a route for these writes. | List these routes and state how H4 moves the inline mutators and adapts the standalone test. |
| 3 | M | §4 H0 | `HashSensitiveCase` requires wake followed by `b3Body_SetAwake(false)` to change the hash relative to an initially sleeping box. That sequence can return the box to the same hashed engine state; a correct hash need not change. | Use a persistent state change, such as the wake alone, for the sensitivity assertion. |
| 4 | M | §3, §4 H1, §9 | `HistoryRingTest` must read each slot’s imaged flag and charge through `src/world_history.h`, while H1 keeps `b3WorldHistory` incomplete. The prompt specifies no read route and §9 says to stop when a test needs a source hook for unavailable internals. | Specify a read-only internal slot-inspection route and reconcile §9 with it, or make the invariant test use an independent model and public queries. |

### 3. Checked, no change

- The ring’s interval, minimum-window and sole-image examples match the ring doc §§6–8.
- The phase-0 serializer gaps and version 11 claim match `src/world_snapshot.c`.
- The H5/H6 dynamic and kinematic tree split matches the trees doc §§5–9.
- The test executable path, named-entry filtering and stated CMake options match the build and test files.
- The H5/H6 cost bounds target the intended pre-commit image terms.

### 4. Proposed edits

Apply the actions in Findings 1–4, then recheck H3a and H4 compilation and the H0 sensitivity case.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED
tokens used
140,084
### 1. Summary

The prompt is close to the three binding designs, but four issues need edits before it can be handed to an implementer. Two omit live write routes, one makes a test reject a potentially correct state hash, and one leaves the ring tests’ inspection mechanism unclear.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, §4 H3a | `src/shape.c` also writes materials through `b3GetShapeMaterials` in `b3Shape_SetFriction` and `b3Shape_SetRestitution`. H3a names only the surface- and mesh-material setters for migration to the sole writer. | Name both setters in the inventory and H3a migration; exercise them on a cold shape in the journal tests. |
| 2 | M | §2, §4 H4 | The claim that the only tree writes outside `src/broad_phase.c` are the four listed is false. `src/broad_phase.h` has inline `b3BroadPhase_MarkProxyMoved` and `b3BroadPhase_MarkProxyMovedSerial` mutators; `test/test_awake_tree.c` also clears trees directly in a standalone broad-phase test. Making `trees` pointer-to-const requires a route for these writes. | List these routes and state how H4 moves the inline mutators and adapts the standalone test. |
| 3 | M | §4 H0 | `HashSensitiveCase` requires wake followed by `b3Body_SetAwake(false)` to change the hash relative to an initially sleeping box. That sequence can return the box to the same hashed engine state; a correct hash need not change. | Use a persistent state change, such as the wake alone, for the sensitivity assertion. |
| 4 | M | §3, §4 H1, §9 | `HistoryRingTest` must read each slot’s imaged flag and charge through `src/world_history.h`, while H1 keeps `b3WorldHistory` incomplete. The prompt specifies no read route and §9 says to stop when a test needs a source hook for unavailable internals. | Specify a read-only internal slot-inspection route and reconcile §9 with it, or make the invariant test use an independent model and public queries. |

### 3. Checked, no change

- The ring’s interval, minimum-window and sole-image examples match the ring doc §§6–8.
- The phase-0 serializer gaps and version 11 claim match `src/world_snapshot.c`.
- The H5/H6 dynamic and kinematic tree split matches the trees doc §§5–9.
- The test executable path, named-entry filtering and stated CMake options match the build and test files.
- The H5/H6 cost bounds target the intended pre-commit image terms.

### 4. Proposed edits

Apply the actions in Findings 1–4, then recheck H3a and H4 compilation and the H0 sensitivity case.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (M).** `b3Shape_SetFriction` and `b3Shape_SetRestitution` (`src/shape.c`) write
   `b3GetShapeMaterials( shape )[0]` like the two material setters. Fix: §2's inventory names all
   four; H3a routes all four through `b3Shape_WriteMaterial`; `ColdChurnUndoCase` and
   `HistoryRewindTest`'s churn exercise friction and restitution writes beside the material ones.
2. **Adopted (M).** `b3BroadPhase_MarkProxyMoved` and `b3BroadPhase_MarkProxyMovedSerial` are
   `static inline` in `src/broad_phase.h` and write `bp->trees` in every translation unit that
   includes it; `SleepingProxyTreeCase` (`test/test_awake_tree.c`) calls `b3DynamicTree_ClearMoved`
   on two trees of a standalone `b3BroadPhase`. Both fail to compile once `trees` is
   pointer-to-const. Fix: §2 lists them beside the four outside writers; H4 moves the two bodies
   into `src/broad_phase.c` as ordinary functions and gives the test the file's clear function,
   which is the pair update's existing static-tree clear.
3. **Adopted (M).** A wake followed by `b3Body_SetAwake( false )` destroys the sleeping set and
   creates a new one; the set id comes back from the pool's free list, so the island, set and body
   can hash as before. Fix: the sensitivity case is the wake alone.
4. **Adopted (M).** `b3WorldHistory` is incomplete outside `src/world_history.c` and the prompt
   gave the ring test no read route to the directory while §9 made a needed hook a stop. Fix: H1
   declares two read-only functions in `src/world_history.h`, slot count and slot (tick, charge,
   imaged flag); H4 adds a third, the count of owned blocks of a kind; the tests read nothing else
   of the ring; §9's stop condition excepts them.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 3: 4 findings (H0-M4-L0), 4 applied, 0 declined. Series: 18 findings, 18 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> pending
- Tripwire: fixes add or remove a named mechanism: three read-only internal functions over the
  ring's directory and owned blocks are named, test-facing only, in place of an unspecified read
  through the header; one broad-phase clear function is named for a test. Clustering: findings in
  §2's inventory and §4 H3a/H4, sections that produced findings in rounds 1 and 2 (three rounds
  running, U-S4); none is a consequence of a previous fix, each is a site the inventory missed or
  a mechanism left implicit; the fixes are clauses, not paragraphs, so no cut is due. Reviewer read
  of round records or changelog: none in the raw log. Doc length: 1294 -> 1309 lines; the fixes
  add three lines (1), eight (2), none (3) and four (4). Above the split-check length (U-T6): one
  decision, no split.
