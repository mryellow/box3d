---
title: Review round 43 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round42.md
mode: full pass
session: 01a1430f-4585-7af1-8ab5-1aeeddc9fe37 (resumed to finalize)
---

# Round 43

Target at revision 41 (1749 lines).

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 62 source reads and no final
message), 2026-10-16T14:53:56+10:00 to 15:02:56+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a1430f-4585-7af1-8ab5-1aeeddc9fe37`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 15:03:08 to 15:03:21,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three binding designs, and the source checks support its main ring, serializer, and tree inventory. Three points need correction before an implementing agent can use its hash and verification instructions without making an unstated choice or accepting incomplete evidence.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1 | `HistoryReplayTest` and `HistoryCrossWorkerTest` require callbacks to count calls and rejections in a shared context, but specify no thread-safe counting mechanism. The callbacks can run on parallel workers (`src/physics_world.c`, `src/broad_phase.c`, `src/solver.c`); `include/box3d/types.h` requires them to be thread-safe. Ordinary counter increments would race and make the comparisons unreliable. | Specify atomic counters or per-worker counters reduced after each step. Keep callback decisions independent of the counters. |
| 2 | M | §4 H0 | The hash walk says to hash each joint sim’s type-specific union member, but `b3_filterJoint` has no member in the `b3JointSim` union (`src/joint.h`, `src/joint.c`). The instruction leaves that valid joint type without a defined hash case. | State that filter joints hash the common sim fields and no union fields; include one in the hash fixture. |
| 3 | M | §§4 H1/H4, 6 | The mapping claims `HistoryRingTest` shows the ring doc §9 test 1, including manifold allocator live counts returning to baseline after `DisableHistory`. The specified tests check counts after rewind and bytes after world destruction, but do not check allocator counts after disable while the world remains alive. Destruction can conceal an incorrect ownership transfer. | Add a post-disable live-world check that allocator counts equal the remaining live contacts’ manifold blocks. |

### 3. Checked, no change

- Verified the prompt’s serializer, recording, replay, step-tail, id-pool, and broad-phase claims against the cited source.
- Checked the ring’s stated widening and window sequences against the ring doc §§5–7; the exact tick sequences in `HistoryRingTest` are consistent.
- Checked the H2–H4 compile-boundary examples against the current declarations and storage types.

### 4. Proposed edits

Make the callback counting method explicit, add the filter-joint hash case, and add the post-disable allocator-count assertion to `HistoryRingTest` and its §6 evidence entry.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `include/box3d/types.h` requires the custom filter and pre-solve callbacks
   to be thread-safe, and the scenes run at `workerCount` 4; plain increments in the shared
   context would race. `src/platform.h` provides `b3AtomicInt` with `b3AtomicFetchAddInt`. Fix:
   the counters are atomic and are read into the record after each step. The callback
   decisions never read the counters, as before.
2. **Adopted (M).** `b3JointSim`'s union (`src/joint.h`) has eight members for nine joint types;
   `b3_filterJoint` creates through `b3CreateJoint` with no member. Fix: the hash rule says a
   filter joint has none, and `HashSensitiveCase`'s scene holds one.
3. **Adopted (M).** The ring doc §9 test 1 requires the block allocators' live counts to return
   to baseline after `DisableHistory` and after `b3DestroyWorld`; the H4 extension checked the
   count after every operation and the byte count after the destroy, not the count after a
   disable on a live world. Fix: that check is added, between the last operation and the
   destroy.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 43: 3 findings (H0-M3-L0), 3 applied, 0 declined. Series: 149 findings, 148 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none (atomic counters are test fixture).
  Clustering: §4 H1's `HistoryReplayTest` callbacks (rounds 22, 24, 26, 34, 43; finding 1 is a
  consequence of round 26's counted-calls fix, which placed the counts in the shared context),
  §4 H0's hash walk (rounds 1, 5, 9, 13, 22, 32, 39, 43; finding 2 present since revision 1), §4
  H4's `HistoryRingTest` extension (rounds 17, 27, 30, 43; finding 3 present since revision 1).
  One of three findings a consequence of an earlier fix. Each fix is a clause or two. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 1749 -> 1755 lines.
  Above the split-check length (U-T6): no new decision, no split.
- Third timed-out round of the series (28, 34, 43), each finalized on resume with no further
  reads. No High since round 38.
