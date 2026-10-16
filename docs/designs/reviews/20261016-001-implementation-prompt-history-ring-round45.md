---
title: Review round 45 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round44.md
mode: full pass
session: 01a14323-306f-7952-83f2-7c89ea322c1b (resumed to finalize)
---

# Round 45

Target at revision 43 (1759 lines).

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 114 source reads and no final
message), 2026-10-16T15:15:41+10:00 to 15:24:41+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a14323-306f-7952-83f2-7c89ea322c1b`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 15:24:53 to 15:25:09,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The ring examples and restore sequence are consistent with the binding designs. Two reviewable defects remain: the prompt misstates the `large_world` fixture used by its Debug tests, and a hash probe cannot establish the refcount sensitivity it claims.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1; §6; §7 | H1 says its `large_world` test contains a million static bodies and needs `tickCount` 2 because each phase-0 image exceeds 500 MB. The required test builds are Debug; `shared/benchmarks.c` sets `BENCHMARK_DEBUG` in that configuration and creates a 32×32 grid. The Release million-body scene is measured for cost, but is not the `HistoryRewindTest` fixture claimed as exactness evidence. | State the Debug and Release fixture sizes separately. Correct the reason for the H1 window limit, and specify how restore exactness is checked on the full Release scene if §6 continues to claim it. |
| 2 | M | §4 H0; §6 | `HashSensitiveCase` destroys one of two hull shapes to purportedly test hull-database refcount hashing. That operation also changes a shape slot, its pool and tree state, so the hash changes even if the database refcount is omitted. | On a live hull, temporarily add and remove one database reference through the existing internal functions; assert that the hash changes and then returns to its original value. |

### 3. Checked, no change

- The stated budget-widening and eviction sequence in `HistoryRingTest` matches the ring doc §§6–7.
- The capture point and traversal count match `src/physics_world.c`.
- The serializer and recording distinctions in §2 match `src/world_snapshot.c`, `src/recording.c` and `src/replay.c`.
- The filter’s always-recreate observation matches `src/shape.c`.

### 4. Proposed edits

Correct the H1 `large_world` size and window rationale, then align §6’s exactness claim with the build that actually runs it. Add the isolated refcount probe to `HashSensitiveCase`.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `shared/benchmarks.c` defines `BENCHMARK_DEBUG` 1 whenever `NDEBUG` is not
   defined and then sets `STATIC_FLOOR_GRID` 32; the suite builds Debug (§7), so
   `HistoryRewindTest`'s `large_world` has 1,024 static bodies and round 37's window limit,
   adopted here on the Release size without checking the configuration (U-E9), rested on a false
   premise. Fix: the limit and its H5 reversal are deleted (U-F4), §2 states the two sizes, and
   the design §12 test 2 is shown on the million-body scene by one Release run of the test target
   at H5 and H6, which §7's build block adds and §6's row names.
2. **Adopted (M).** Destroying a hull shape changes the shape slot, its pool and the tree as
   well as the refcount, so the hash would differ with the database left out of the walk. Fix:
   the probe is an isolated add and remove of one database reference through the two internal
   functions, the hash differing after the add and returning after the remove; the destroy case
   is deleted in the same edit (U-F5).

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 45: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 153 findings, 152 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: round 37's `large_world` window limit is
  removed; a Release run of one test entry is added to §7's build block. Clustering: §4 H1's
  `HistoryRewindTest` (rounds 29, 36, 37, 45; finding 1 is a consequence of round 37's fix, the
  first fix of the series reversed by a later round), §4 H0's `HashSensitiveCase` (rounds 22,
  32, 33, 36, 38, 39, 45; finding 2 present since revision 1). One of two findings a consequence
  of an earlier fix. Each fix is a clause or two plus three build lines. Reviewer read of round
  records or changelog: none in the raw log. Doc length: 1759 -> 1769 lines. Above the
  split-check length (U-T6): no new decision, no split.
- Fifth timed-out round (28, 34, 43, 44, 45) and the third in a row; each finalized on resume.
  Round 37's finding 1 was adopted without reading the configuration the suite builds under:
  the correction stands here, and the round 37 record is not revised (U-R1).
