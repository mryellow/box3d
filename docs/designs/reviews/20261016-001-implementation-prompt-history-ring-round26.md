---
title: Review round 26 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round25.md
mode: full pass
---

# Round 26

Target at revision 26 (1642 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T12:10:10+10:00 to 12:16:59+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14279-5772-7490-845f-9579566eadf4`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely matches the binding designs and the live source. One verification fixture tests ordinary sensor overlaps where the trees design requires a continuous sweep with more than eight sensor candidates.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H5/H6; §6 | `HistoryTreeTest` uses `src/sensor.c`’s overlap pass and asserts more than eight overlaps. The trees doc §10 calls for a **sweep** with more than eight sensor candidates. That is a different path in `src/solver.c`, where continuous sensor hits have an eight-entry limit. The proposed test could pass without exercising that limit or its tree-order behavior. | Add a fast-body continuous sweep through at least nine enabled sensor candidates, witness a different tree visit order after rewind, and compare replay hashes and events. Keep the overlap cases as separate coverage. |

### 3. Checked, no change

- Verified the ring’s widening, minimum-window, eviction, and restorable-tick examples against the ring doc §§3–8.
- Verified the serializer’s tree fields, omitted migration queue, geometry registry, and `userData` handling against the live source.
- Verified the stated filter-recreation behavior and the main proxy-write routes.
- Checked the compile-boundary sequence and fail-before procedure for the named increments; found no further defect.

### 4. Proposed edits

Revise `HistoryTreeTest` in §4 and its §6 mapping so the trees doc §10 sweep claim points to a continuous-sweep case. Describe the existing overlap cases as overlap-pass coverage.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** The trees doc §10 test 1 names "a sweep with more than eight sensor
   candidates": the continuous stage in `src/solver.c` keeps at most
   `B2_MAX_CONTINUOUS_SENSOR_HITS` (eight) sensor hits per fast shape and skips a sensor unless
   both shapes carry the flag. Rounds 13 and 22 had turned the case into the overlap pass of
   `src/sensor.c`, a different path with no such limit. Fix: the case is a bullet crossing at
   least nine static sensors in one step with exactly eight begin events asserted; the overlap
   case stays as extra coverage; the kinematic case follows suit.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 26: 1 finding (H0-M1-L0), 1 applied, 0 declined. Series: 107 findings, 107 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: the tree test's sensor case
  (rounds 11, 13, 15, 16, 22, 26: the finding is a consequence of round 13's fix 2, which read the
  trees doc's "sweep" as the overlap pass). Each fix is a clause or a short paragraph. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 1642 -> 1644 lines. Above the
  split-check length (U-T6): no new decision, no split.
- The smallest round since round 14 and the first single-finding round of the series.
