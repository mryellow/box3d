---
title: Review round 57 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round56.md
mode: full pass
session: 01a143a3-6305-7233-aeb4-de46e817bdee (resumed to finalize)
---

# Round 57

Target at revision 54 (1857 lines).

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
Run: foreground, `timeout 540`, exit 124 (work timeout after 57 source reads and no final
message), 2026-10-16T17:35:43+10:00 to 17:44:43+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a143a3-6305-7233-aeb4-de46e817bdee`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 17:44:53 to 17:45:34,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The increment order, ring transitions, source inventory and fail-before approach are broadly consistent with the three binding designs and the source checked. One event-comparison rule needs correction before the prompt is handed to an implementer.

### 2. Findings

| # | Severity | Location | Issue and source evidence | Concrete fix |
|---|---|---|---|---|
| 1 | Medium | §4, lines 478–487; §5, lines 1607–1610; §7, lines 1690–1695 | The event digest includes every named event field and is said to stand alone across processes or platforms. `b3BodyMoveEvent.userData` and `b3JointEvent.userData` are `void*` fields (`include/box3d/types.h`); their address bits are not stable across processes. Comparing those bits can reject a correct replay. | Keep field-by-field `userData` comparison for in-process replay. For cross-process digests, map each host pointer to a stable test-defined token, or limit that comparison to fixtures with specified stable `userData` values. |

### 3. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The finding checked against live source and the designs (U-V2). It is a consequence of round
56's fix, which made the digest the cross-process comparator without excluding the pointer
words.

1. **Adopted (M), by a third route.** The two event structs carry `void* userData`
   (`include/box3d/types.h`); the design excludes every `userData` word from the state hash (the
   prompt's H0, "addresses" and "every `userData` word"); the goldens pin `b3World_ComputeStateHash`
   alone; the §7 harness compares benchmark scenes, which set no `userData`
   (`shared/benchmarks.c`), between a base and a branch process. Fix: the digest leaves the
   `userData` words out, as H0's hash does, so it compares across processes and platforms with no
   token map and no fixture condition; the in-process event record compares the words themselves,
   and `HistoryReplayTest` already checks every record's `userData` word against the recording.
   The paragraph's last sentence names §5's benchmark comparison and the §7 harness as the
   digest's cross-process uses.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 57: 1 finding (H0-M1-L0), 1 applied, 0 declined. Series: 177 findings, 176 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> pending
- Tripwire: no mechanism added or removed; two words leave the digest's field list and the
  record keeps them. Clustering: the event digest paragraph (rounds 56, 57); the finding is
  against round 56's fix. The fix is four clauses. Reviewer read of round records or changelog:
  none in the raw log. Doc length: 1857 -> 1861 lines. Above the split-check length (U-T6): no
  new decision, no split.
- Thirteenth timed-out round (28, 34, 43, 44, 45, 46, 49, 51, 52, 53, 54, 56, 57), each
  finalized on resume.
