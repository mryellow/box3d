---
title: Review round 12 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round11.md
mode: full pass
---

# Round 12

Target at revision 12 (1440 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T08:05:49+10:00 to 08:10:12+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14199-a252-7940-a829-dcb15408d0d3`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The increment order broadly follows the three Canonical designs, and the source checks support the prompt’s main serializer and tree assumptions. Four defects need edits before handoff: the H4 journal test leaves the ring inconsistent, its required hooks conflict with §9, and two verification claims are not supported by the proposed measurements.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4, `SegmentWalkCase` | `b3History_UndoSegments(world, 6)` reverses closed segments without removing those slots or changing `historyTick`. The live world is then at tick 6 while the ring still says tick 12; another rewind can undo the segments again, and disable can free blocks already handed back to the world. This conflicts with the ring doc §4 and the design §7.1 ownership rule. | Make the direct-walk test operate on a disposable copy, or specify an atomic test route that also truncates slots, updates charges and tick, and leaves ownership consistent. |
| 2 | M | §9 versus §4 H4 | §9 stops if a test needs a source hook beyond the named *read-only* functions. `HistoryJournalTest` expressly requires the mutating `b3History_UndoPending` and `b3History_UndoSegments` hooks, so the stop rule rejects its own H4 test mechanism. | Explicitly allow the H4 undo test mechanism in §9, alongside the read-only inspection functions. |
| 3 | M | §5 | Equal scene hashes can miss changed events: H0 excludes event arrays from `b3World_ComputeStateHash`, while the §5 base-to-branch check compares only hashes and existing determinism goldens. An event-order or event-content regression could pass that policy and the branch’s replay-against-itself tests. | Compare a field-wise event digest between base and branch for the benchmark runs, in addition to the state hash. |
| 4 | M | §7 | The proposed “microseconds per awake body” figure cannot demonstrate restore cost proportional to **awake bodies plus sensors and journal work**. The listed benchmark scenes have no sensors, and the million-body fixture adds none; the ratio also omits journal size. | Add a sensor-bearing restore fixture and report absolute restore time alongside awake count, sensor overlap count, and journal entries or bytes. |

### 3. Checked, no change

- The ring’s interval widening and window examples agree with the ring doc §§5–7.
- The phase-0 omissions identified for the serializer agree with `src/world_snapshot.c`; the separate migration-queue copy is necessary.
- The proxy identity and derived-tree mechanics in H4–H6 agree with the trees doc §§5–9 and the cited broad-phase source.
- The event digest’s listed fields match `include/box3d/types.h`.
- The stated 17 benchmark table entries and 18 available scene creators match `benchmark/main.c` and `shared/benchmarks.h`.

### 4. Proposed edits

Resolve findings 1–2 together so H4 has a safe, explicitly permitted journal test mechanism. Add the base-to-branch event comparison to §5 and the sensor and journal measurements to §7.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** H4 had `b3History_UndoSegments` undo the closed segments and nothing else;
   `SegmentWalkCase` then ends by disabling history, and disable "frees the blocks owned by every
   retained slot", which the walk has just handed back to the world (the design §7.1's ownership
   rule: an entry owns a block until the undo returns it). The design §9 step 5 removes the slots
   after T; placed after the walk it is the same removal. Fix: the walk discards each slot as it
   undoes its segment (directory entry, charge, write position, `historyTick`), in `b3World_Rewind`
   and in the test alike, so no slot claims a block the world holds; the test checks `currentTick`,
   the newest slot and `bytesUsed` after the walk. `b3History_UndoPending` already reset the staging
   buffer.
2. **Adopted (M).** §9 stopped on "a hook in `src/` beyond the read-only functions §4 H1 and H4
   name"; the two walk halves mutate. They are the rewind's own halves, not test-only code. Fix:
   §9 names them beside the read-only functions.
3. **Adopted (M).** H0's hash excludes the event arrays (§4 H0); §5's base-against-branch check
   compared hashes and `test/test_determinism.c`'s values, neither of which sees event order or
   content. Fix: the scratch harness prints the field-wise event digest beside the hash, built from
   the public event getters so the base build (H0 cherry-picked) has it, and §5's two harness
   bullets compare both.
4. **Adopted (M).** `shared/benchmarks.c` creates no sensor shape, and the million-body scene had
   none, so a microseconds-per-awake-body figure could not show the sensor term; the ratio also hid
   the journal term. Fix: absolute restore microseconds beside awake count, summed sensor overlap
   count and undone segment bytes, per scene, and the million-body scene gains 64 static sensors
   overlapping the awake boxes.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 12: 4 findings (H1-M3-L0), 4 applied, 0 declined. Series: 57 findings, 57 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none; the walk's slot discard is the design's
  step 5 removal moved to where ownership changes hands, the digest is a comparison the harness
  already had the function for, the sensors are fixture. Clustering: §4 H4's test block (rounds
  8, 12), §5 (round 12, first time), §7 (rounds 5, 9, 11, 12), §9 (rounds 6, 12). Finding 2 is a
  consequence of round 6's hook rule meeting round 8's walk functions; the others of revision 1.
  Each fix is a clause or a short paragraph. Reviewer read of round records or changelog: none in
  the raw log. Doc length: 1440 -> 1460 lines. Above the split-check length (U-T6): no new
  decision, no split.
