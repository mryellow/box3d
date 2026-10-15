---
title: Review round 15 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round14.md
mode: full pass
---

# Round 15

Target at revision 15 (1484 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T08:28:42+10:00 to 08:33:40+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a141ae-9424-7882-962a-fb5852986f66`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is largely aligned with the three canonical designs and the live source, but two increment boundaries cannot work as written. Three test or measurement claims also lack the checks or fixtures needed to show what §6 and §7 say they show.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3c–H4 | H3c makes solver sets, sensors and the constraint graph inaccessible outside their files, while `src/world_snapshot.c` still directly resizes and writes them. H4 introduces the serializer’s file-local load routes **one commit later**. H3c therefore cannot both build and pass the recording tests as required by §3. | Introduce the serializer load routes in H3c, before hiding the storage, and reserve H4 for making those routes journal-safe. |
| 2 | H | §4 H3a, H4 | H3a says restore writers assert `inRestore`, set by `b3World_Rewind` or `b3DeserializeIntoShell`. `HistoryJournalTest` calls `b3History_UndoPending` and `b3History_UndoSegments` directly; their record undos reach those writers without either stated guard. The validation-build test will assert. | Specify a scoped restore guard in the directly callable undo helpers, including how it composes with `Rewind`. |
| 3 | M | §4 H2–H3c | H2 places `b3SetJointSim` in `src/solver_set.c`, but an awake joint’s sim lives in a graph colour (`src/joint.c`). H3c then makes graph storage writable only in `src/constraint_graph.c` and exposes a const colour getter. The stated setter has no writable route for awake joints. | Specify a graph-local joint-sim setter and have `b3SetJointSim` dispatch to it for awake joints. |
| 4 | M | §4 H1; §6 | `HistoryRewindTest` sets `world->userData` back before comparing after rewind. That masks an implementation that wrongly restores this host value, although the design §10 says rewind leaves it alone and design §12 test 2 requires checking that property. | Assert `b3World_GetUserData` equals its pre-rewind value immediately after rewind; reset it afterward only when replay needs the value at T. |
| 5 | M | §4 H5–H6; §6 | The H5 tree-order witnesses place candidates in static and sleeping trees. H6’s kinematic cases test proxy movement and identity, but require neither a changed kinematic visit order nor replay equality across one. They do not show the kinematic part of the trees doc §10 test 1 claimed in §6. | Add kinematic candidates to a layout witness, assert their visit order changes, then compare replay hashes and events. |
| 6 | M | §7 | The million-body measurement fixture creates sleeping kinematic bodies and then specifies no post-enable kinematic mutation or wake. A rewind in that fixture cannot move a kinematic proxy or undo a wake that triggers a sleeping-tree rebuild, yet §7 asks for both timings there. | Script those transitions at known ticks, or time the rebuild using the dedicated height-trigger fixture. |

### 3. Checked, no change

- The §2 inventory of record fields, id pools, tree writes, snapshot omissions, recording state, and test runner behavior matches the cited live source in the areas checked.
- H1’s tick-window examples and interval-widening sequence match the ring doc §§5–7.
- The `b3Shape_SetFilter` always-recreate observation matches `src/shape.c`.
- H5 and H6’s 256 KB cost fixtures distinguish the serialized image, the awake image, and the temporarily imaged kinematic tree as claimed.

### 4. Proposed edits

Move serializer load mechanics into H3c; define the undo helpers’ restore guard and the awake joint-sim write route; strengthen the `userData` and kinematic-order tests; and add the two transitions required by §7’s special timings.

### 5. Unresolved

The design §12 test 5’s “under 1 µs per awake body” target needs an interpretation for ticks with zero awake bodies. `shared/benchmarks.c` can produce such ticks in `large_world`; this is a design-level question, not a finding against the prompt.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `src/world_snapshot.c` resizes and writes `world->solverSets`,
   `world->sensors` and the graph's colours directly; H3c hides all three and lists the file, but
   round 9's fix placed every file-local load in H4, so H3c had no route for its own reader and
   could not build (§3 needs each commit green). The design §5.2 says the reader writes through
   the loads "from phase 1", which the prompt's earlier boundary brings forward. Fix: each boundary
   commit introduces the loads for what it hides (record-array load-count functions in H3a to H3c,
   the set, island, colour and sensor array loads in H3c); H4 adds the pair set's, the trees' and
   the bitsets' and routes the undo through all of them.
2. **Adopted (H).** Round 14's restore mark was set by `b3World_Rewind` and the deserializer;
   `HistoryJournalTest` calls the two walk halves directly and their undo reaches the restore
   writers, so the validation build would assert in the test. Fix: the functions that write through
   the restore writers (the deserializer, the two walk halves, the scatter) set and clear the mark
   themselves, asserting it clear on entry; `b3World_Rewind` sets nothing.
3. **Adopted (M).** `b3GetJointSim` reads an awake joint's sim from a colour's `jointSims`
   (`src/joint.c`); H2's `b3SetJointSim` sits in `src/solver_set.c` and H3c makes the colours
   writable from `src/constraint_graph.c` only. Fix: the setter dispatches to a graph-local writer
   for an awake joint.
4. **Adopted (M).** The churn set the world's `userData` back before comparing, so a rewind that
   wrongly restored the word would pass; the design §10 says the word is the host's and §12 test 2
   checks it. Fix: `ENSURE` the getter returns the pre-rewind value right after each rewind, then
   set it back.
5. **Adopted (M).** H6's two kinematic cases checked proxy identity and replay, not a kinematic
   layout difference; the trees doc §10 test 1 names the kinematic tree. The perturbation helper's
   full rebuild would reproduce the restore pass's own full rebuild (the trees doc §6), so the
   witness for this tree is the original run's incremental layout at T against the pass's. Fix: a
   third case, a sensor sweep over more than eight kinematic visitors, witnessed that way, with
   replay equality.
6. **Adopted (M).** The million-body fixture had no kinematic move and no wake after the enable,
   so neither separate timing §7 asks for could occur. Fix: a scripted `b3Body_SetTransform` on a
   kinematic body three ticks before the timed rewind and a 4,096-body slab woken four ticks
   before it, the rewind undoing the wake past `B3_SLEEPING_TREE_MAX_HEIGHT`.

**Unresolved item (reviewer's).** The design §12 test 5's "under 1 µs per awake body" has no
value at a tick with no awake body, which `large_world` can produce. A design question, left to
the owner; the prompt states only how the measurement reports such a tick (absolute microseconds,
outside the ratio) and tells the closure doc to note it.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 15: 6 findings (H2-M4-L0), 6 applied, 0 declined. Series: 69 findings, 69 applied, 0
  declined. One Unresolved item recorded for the owner (a design question).
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> pending
- Tripwire: fixes add or remove a named mechanism: the file-local loads move earlier (H3a to H3c)
  rather than appear; `b3Graph_SetJointSim` is the colour's writer the design's one-writer rule
  implies; nothing else. Clustering: §4 H3a (rounds 3, 5, 8, 14, 15), §4 H3c (rounds 3, 15), §4 H4's
  restore routes (rounds 9, 15), §7's million-body fixture (rounds 9, 12, 15). Findings 1 and 2
  are consequences of round 9's fix 2 and round 14's fix 1 respectively: each placed a mechanism
  one commit or one caller too narrowly; findings 3 to 6 of revision 1. Each fix is a clause or a
  short paragraph. Reviewer read of round records or changelog: none in the raw log. Doc length:
  1484 -> 1521 lines. Above the split-check length (U-T6): no new decision, no split.
- The count rose again (2 -> 6). Two of the six were consequences of the last six rounds' own
  fixes. For the owner at the next decision point (U-S8).
