---
title: Review round 50 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round49.md
mode: full pass
---

# Round 50

Target at revision 47 (1790 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T16:09:00+10:00 to 16:17:41+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14354-0092-77c0-9291-c5d0594322cc`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt’s ring and restore mechanics largely match the three binding designs and the live source. I found two test-plan defects: one can reject a correct H6 implementation, and the other leaves a claimed journal entry untested.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H6 | The kinematic sensor case requires a different visit order at tick 10 after a full rebuild. `src/broad_phase.c` already rebuilds the kinematic tree during steps, so creating shapes in two parts around an earlier step does not guarantee a different layout at tick 10. `WitnessDiffers` can fail on a correct implementation. | Put the witness at a boundary where the kinematic layout is demonstrably incremental, or specify and verify another layout-changing fixture before requiring the assertion. |
| 2 | M | §6; §4 H4–H6 | The mapping claims evidence for every journal entry kind, but the named tests do not exercise undo of a **sleeping-tree proxy move**. An ordinary between-step `b3Body_SetTransform` wakes and migrates the body; the in-place sleeping-tree move occurs when the setter runs during an open traversal (`src/body.c`, `src/broad_phase.c`; trees doc §6). | Add a query-callback fixture that moves a sleeping body while traversal is open, then rewind and check its recorded leaf box and full state hash. |

### 3. Checked, no change

- The stated budget-widening, minimum-window and eviction sequences agree with the ring doc §§5–7.
- The phase boundaries for the awake, static, sleeping and kinematic trees agree with the design §13 and trees doc §§5–9.
- The serializer, recording and replay routes cited for phase 0 exist in the live source.
- The named compiler-boundary probes target the const or incomplete-type changes described for H2–H4.

### 4. Proposed edits

Revise the H6 kinematic witness fixture and add the sleeping-tree proxy-move case. Update §6’s evidence entry to name that case.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** The step's tree task calls `b3DynamicTree_Rebuild( tree, false )` on the
   awake and kinematic trees every tick (`src/broad_phase.c`), rebuilding moved subtrees, so the
   kinematic tree's tick-10 layout is a rebuild's and the perturbation helper's full rebuild can
   reproduce it; the staged fixture round 42 gave the H5 cases works for the static and sleeping
   trees, which no step rebuilds. The trees doc §6 states that a kinematic tree the pass moved a
   proxy in is left unordered. Fix: the four kinematic cases each move a kinematic body after
   tick 10 and `ENSURE` the pass's moved count on the kinematic tree is at least 1 after the
   rewind, the witness being the pass's own incremental move.
2. **Adopted (M).** `b3BroadPhase_MigrateBody` queues the wake while `traversalDepth` is non-zero
   and `b3BroadPhase_MoveProxy` writes the sleeping leaf in place (`src/broad_phase.c`), which the
   trees doc §6 names as the case the proxy move entry covers; no named test produced it. Fix:
   `ProxyIdentityCase` gains the in-traversal `b3Body_SetTransform` from a query callback, undone,
   with the leaf box and the hash checked; §6's row names it.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 50: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 164 findings, 163 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none (the kinematic witness reuses H5's
  moved-proxy count). Clustering: §4 H6's kinematic cases (rounds 24, 31, 42, 50; finding 1 is a
  consequence of round 42's staged-fixture wording, which covered the kinematic tree by
  reference), §4 H4's `ProxyIdentityCase` (rounds 17, 50; finding 2 present since revision 1).
  One of two findings a consequence of an earlier fix. Each fix is a sentence. Reviewer read of
  round records or changelog: none in the raw log. Doc length: 1790 -> 1798 lines. Above the
  split-check length (U-T6): no new decision, no split.
- Fifty rounds: three clean rounds (35, 41, 48), none confirmed. The last ten rounds returned
  2, 2, 3, 2, 3, 3, 0, 3, 2 findings and one High (47).
