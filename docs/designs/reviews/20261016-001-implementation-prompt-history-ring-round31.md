---
title: Review round 31 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round30.md
mode: full pass
---

# Round 31

Target at revision 31 (1676 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T13:07:06+10:00 to 13:12:55+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142ad-781a-7ee1-b7e4-e6cf23fe9515`. The stream emitted the final message twice; the last copy
follows (the reviewer used bold list numbering in place of headings this round).

## Review report (Codex final message)

1. **Summary**  
The prompt largely matches the three binding designs and the checked source paths. One phase-specific measurement claim is incorrect and would mislabel the H5 replay timing.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §7 | The million-body timing script says a rewind across a kinematic proxy move makes the first replayed step rebuild the kinematic tree at both H5 and H6. At H5, §4 restores that tree from its raw image; the restore pass does not move its proxy. The rebuild caused by the restore pass applies only at H6 (trees doc §9). | Report H5’s first replayed step as the raw-image baseline. Attribute the restore-pass rebuild timing to H6, and verify that its moved-proxy condition occurred. |

3. **Checked, no change**

- The ring test’s stated interval-widening sequence follows the ring doc’s budget rule.
- The source supports §2’s `b3Shape_SetFilter` observation: the category comparison occurs after assignment.
- The H2–H4 compile-boundary examples target fields or writable pointers that those increments hide.
- The test runner accepts an exact test-entry name, and the benchmark command runs thread counts one through eight when eight cores are available.

4. **Proposed edits**  
Revise §7’s kinematic timing description and its H5/H6 reporting instructions as stated in finding 1.

5. **Unresolved**  
None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** H5's step 4 loads the kinematic tree from its raw image "the same way" as the
   awake tree; the restore pass covers it only at H6, where the pass moves listed kinematic
   proxies and the first replayed step's rebuild restores the order (the trees doc §6, §9). §7's
   million-body script said the rewind moves the proxy and the first step rebuilds, unqualified,
   and the reporting line asked for that step at both H5 and H6 as the same figure. Fix: the
   script and the reporting line say H5 is the raw-image baseline and H6 the restore-pass
   rebuild, the harness reading in a scratch build that the pass moved the proxy and the step
   rebuilt the tree, the pattern the sleeping-tree rebuild timing already uses.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 31: 1 finding (H0-M1-L0), 1 applied, 0 declined. Series: 120 findings, 120 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §7's million-body script
  (rounds 21, 24, 25, 28, 31); the finding is a consequence of round 24's addition of the
  scripted kinematic transition, written before H6's phase split was read back against it. The
  fix is two clauses. Reviewer read of round records or changelog: none in the raw log. Doc
  length: 1676 -> 1680 lines. Above the split-check length (U-T6): no new decision, no split.
- Second single-finding round of the series (26, 31); no High in three of the last four rounds.
