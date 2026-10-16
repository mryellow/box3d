---
title: Review round 46 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round45.md
mode: full pass
session: 01a1432d-9a29-7ff1-b763-f63330f1282f (resumed to finalize)
---

# Round 46

Target at revision 44 (1769 lines).

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
message), 2026-10-16T15:27:04+10:00 to 15:36:04+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a1432d-9a29-7ff1-b763-f63330f1282f`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 15:36:16 to 15:36:36,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three binding designs, and the source checks support its main ring, tree, and serializer assumptions. I found one omitted write route and two places where the stated verification would show less than the document claims.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2; §4 H3c | `src/solver.c` writes `overflow->contacts.data[i].manifoldStart`. The writer inventory omits it, and H3c specifies a route for the collide pass’s `manifoldCount` write but not this solver setup write. H3c’s incomplete graph type will reject the direct access. | Inventory the writer and explicitly route it through the graph’s hot contact-spec accessor. |
| 2 | M | §4 H4; §6 | The mapping claims the ring doc §9 test 1’s ownership invariant for **every** heap block. The prescribed checks count manifold blocks, check aggregate `pendingBytes`, and check for leaks after destruction. They do not establish live-or-journal ownership for each sensor, material, set, island, and hull block while history is retained. | Add intermediate ownership checks by block kind using H4’s read-only ownership function, or narrow the mapping’s claim. |
| 3 | L | §7 | The “registry bytes” measurement sums geometry payload `byteCount` values only. It omits the registry entry array, reader slot array, and deduplication map allocations in `src/recording.h` and `src/replay.h`, so it understates the phase-0 memory H5 removes. | Label the figure “geometry payload bytes,” or measure and report the registry’s total allocated bytes separately. |

### 3. Checked, no change

- The interval-widening and window expectations in `HistoryRingTest` agree with the ring doc §§5–7.
- H1’s serializer options, migration-queue copy, geometry-pointer handling, and event clearing match the design §§9, 10, 13 and the cited source.
- The static, sleeping, and kinematic tree transitions across H4–H6 match the trees doc §§5–9.
- The H2–H4 compile boundaries and the H5–H6 cost-test failure mechanisms are consistent with the cited source.

### 4. Proposed edits

Add the `manifoldStart` write and its H3c route; strengthen or narrow the ownership evidence in §6; correct the §7 registry-memory label or measurement.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `src/solver.c`'s stage setup assigns `overflow->contacts.data[i].manifoldStart`
   directly; §2 named the field as per-step scratch without its writer and H3c routed only the
   collide pass's `manifoldCount` write through `b3Graph_HotContactSpec`. Fix: §2 names the
   writer; H3c routes it through the same accessor.
2. **Adopted (M).** The ring doc §9 test 1's invariant is per heap block; the H4 checks counted
   manifold blocks and aggregate `pendingBytes`. `b3History_CountOwnedBlocks( world, kind )`
   already takes a kind. Fix: `OwnershipCase`'s cold part `ENSURE`s the per-kind count after each
   destroy; §6's row cites it beside the ring test.
3. **Adopted (L).** The figure summed payload `byteCount` only, leaving out the entry array, the
   slot array and the dedup map. Fix: it is named geometry payload bytes in §7 and §8, with the
   containers stated as left out.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 46: 3 findings (H0-M2-L1), 3 applied, 0 declined. Series: 156 findings, 155 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §4 H3c's graph bullet
  (rounds 3, 16, 25, 27, 33, 46; finding 1 is a gap present since revision 1, next to the
  `manifoldCount` route round 25 added), §4 H4's `OwnershipCase` (rounds 17, 27, 33, 46; finding
  2 present since revision 1), §7's phase-0 figures (rounds 23, 44, 46; finding 3 is a
  consequence of round 44's definition). One of three findings a consequence of an earlier fix.
  Each fix is a clause. Reviewer read of round records or changelog: none in the raw log. Doc
  length: 1769 -> 1772 lines. Above the split-check length (U-T6): no new decision, no split.
- Sixth timed-out round and the fourth in a row (43 to 46); each finalized on resume with no
  further reads. No High since round 38; the last eight rounds returned 3, 4, 2, 0, 3, 3, 2, 2,
  3 findings.
