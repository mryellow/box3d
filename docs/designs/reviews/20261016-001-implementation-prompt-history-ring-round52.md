---
title: Review round 52 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round51.md
mode: full pass
session: 01a1436c-4e45-7262-bdbf-4703b017582d (resumed to finalize)
---

# Round 52

Target at revision 49 (1800 lines).

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
Run: foreground, `timeout 540`, exit 124 (work timeout after 64 source reads and no final
message), 2026-10-16T16:35:32+10:00 to 16:44:33+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a1436c-4e45-7262-bdbf-4703b017582d`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 16:44:40 to 16:45:23,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The implementation prompt largely follows the three binding designs and the source paths checked. Two test instructions need correction: one claims ownership counts that the fixture does not create, and the other claims a kinematic tree layout witness that its assertions do not establish.

### 2. Findings

| # | Severity | Location in target | Issue and source evidence | Concrete fix |
|---|---|---|---|---|
| 1 | Medium | §4, H4, `OwnershipCase` (around line 1320) | The case describes the merged sleeping set as handing over "five arrays" as owned blocks and the destroyed island as handing over "three." A sleeping set has no `bodyStates` allocation, and empty arrays hold no block; a lone body's island likewise need not allocate contact or joint arrays. See `src/solver_set.c`, `src/island.c`, `src/container.h`. An exact increase of five and three would fail for a correct implementation. | Count non-null handles and their byte sizes immediately before each destroy, then assert that the journal owns exactly those handles and bytes. |
| 2 | Medium | §4, H5 `HistoryTreeTest` (around lines 1460–1490); §4, H6 kinematic cases (around lines 1520–1540); §6 tree-order mapping | The prompt says the test demonstrates changed layouts in the static, kinematic and sleeping trees, but H5 requires a changed visit order only for a named static or sleeping tree. H6 explicitly has no kinematic layout witness. Thus the mapped evidence does not establish the kinematic part of the trees doc §10 test 1. | Add a deterministic kinematic case that records and asserts a different kinematic leaf visit order before replay, or narrow the evidence claim and leave that binding test requirement unresolved. |

### 3. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Finding 2 is against round
51's fix; finding 1 is a fixture fact unchanged since round 33.

1. **Adopted (M).** `b3TrySleepIsland` (`src/solver_set.c`) zero-initialises the sleeping set and
   reserves `bodySims`, `contactIndices` and `jointSims` by the island's counts; `b3Array_Reserve`
   with 0 allocates nothing, `bodyStates` is never touched, and `islandSims` is emplaced, so the
   set a joint merge destroys for a lone body holds two blocks, not five. `b3CreateIsland` creates
   its three arrays as NULL headers and `b3CreateIslandForBody` pushes one body, so the island
   `b3MergeIslands` destroys holds one block, not three. `b3MergeSolverSets` destroys the set with
   fewer bodies, `b3MergeIslands` the smaller island, and `b3Array_Destroy` on a NULL header calls
   `b3Free( NULL, 0 )`, which returns. The sensor's three are allocated at creation by
   `b3Array_CreateN` (`src/shape.c`). Fix: the case counts the non-null handles and their bytes
   from the headers immediately before each destroy and asserts those; the handover bullet says a
   NULL header is no block and no entry handle.
2. **Adopted (M), by the reviewer's second option.** The H5 bullet claimed the static, kinematic
   and sleeping trees while its witnesses are over the static and sleeping trees and round 51's
   H6 text claims none over the kinematic tree, which `b3UpdateBroadPhasePairs` rebuilds in part
   every step (`b3DynamicTree_Rebuild( ..., false )`, `src/broad_phase.c:725`), so a kinematic
   visit-order witness is not deterministic on a correct build; the first option was rounds 24
   to 51's attempts. Fix: the H5 bullet and the §6 row claim the static and sleeping witnesses
   and point to §10, where a new item records the kinematic clause of the trees doc §10 test 1
   for the owner: met by the H6 construction checks, or dropped from the trees doc.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 52: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 168 findings, 167 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> pending
- Tripwire: no mechanism added or removed; the fixes are a count rule, a narrowed claim and a
  §10 owner item. Clustering: §4 H4's `OwnershipCase` (rounds 2, 6, 33, 46, 52) and the
  kinematic tree witness (rounds 24, 31, 42, 50, 51, 52), the sixth round for the latter, a
  recurrence reported under U-S4: the H5 claim, the H6 text and the §6 row now say the same
  thing and the remaining gap is the owner's. Finding 2 is a consequence of round 51's fix, the
  third consecutive round with a finding against the previous round's fixes. Each fix is a
  sentence or a paragraph of source facts. Reviewer read of round records or changelog: none in
  the raw log. Doc length: 1800 -> 1820 lines. Above the split-check length (U-T6): no new
  decision, no split.
- Ninth timed-out round (28, 34, 43, 44, 45, 46, 49, 51, 52), each finalized on resume. Owner
  items now four: rounds 15 and 20 (design wording), 35 (the trees doc's pointer-to-const
  wording) and 52 (the kinematic clause of the trees doc §10 test 1).
