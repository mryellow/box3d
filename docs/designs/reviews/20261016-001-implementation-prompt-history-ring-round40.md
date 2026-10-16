---
title: Review round 40 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round39.md
mode: full pass
---

# Round 40

Target at revision 39 (1738 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T14:30:09+10:00 to 14:35:24+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142f9-82c7-7223-9931-75520364dafa`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three binding designs and the source mechanisms I checked. Two corrections are needed: one source inventory claim is false, and one measurement setup does not reliably exercise the budget behavior it claims to measure.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §2; §4 H2 | §2 says step stages do not use the sim getters, but `src/solver.c` calls `b3GetBodySim` in the continuous-candidate and hit-event passes. H2’s const-returning getter therefore requires changes to those read-only local pointers, despite saying the stages are unchanged. | Identify these getter reads in §2 and tell H2 to make their local pointers const. |
| 2 | M | §7 | A budget of one quarter of the unbounded 60-tick `bytesUsed` does **not** imply interval widening. Under ring doc §6, widening occurs only when the minimum retained window exceeds the budget; with roughly equal slot charges, that budget holds about 15 images while the minimum needs about two. The run can stay at interval 1 and evict old slots. | Measure eviction at the quarter budget, and use a separately chosen budget below the minimum-window charge to measure widening. Report the observed interval in each run. |

### 3. Checked, no change

- The H2–H4 compile-boundary expressions match the current mutable types and the proposed const or incomplete-type boundaries.
- The H1 ring cases’ stated tick retention and interval-doubling sequences match ring doc §§6–7.
- The H5 and H6 256 KB checks target the intended serialized-world and raw-kinematic-tree costs, respectively.
- The serializer, proxy, sensor, recording, and benchmark mechanisms cited for those checks are present in the named source.

### 4. Proposed edits

Apply the two table actions to §§2, 4 H2, and 7. Keep the measurement results for eviction and interval widening distinct.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (L).** `src/solver.c` calls `b3GetBodySim` three times: in
   `b3EvaluateContinuousCandidate` and twice in `b3Solve`'s hit-event pass, each a read; §2 said
   the stages do not use the getters and H2 said the stages are unchanged. Fix: §2 names the
   three reads; H2 makes their locals const.
2. **Adopted (M).** The ring doc §6 widens the interval (step 4) only when the minimum window,
   two images and the slots between, exceeds `maxBytes`; a quarter of the 60-tick steady-state
   `bytesUsed` holds about fifteen images at equal charges, so step 3 evicts at interval 1 and §7's
   "where the interval widens" was false for that budget. Fix: three budgets, each with the
   observed interval reported: unbounded, the quarter (eviction), and one and a half imaged
   slots' charge (widening, the budget `HistoryRingTest` case 4 uses in shape); the restore
   timings that need widening run under the third.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 40: 2 findings (H0-M1-L1), 2 applied, 0 declined. Series: 143 findings, 142 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §2's getter inventory
  (rounds 2, 40; finding 1 present since revision 1), §7's budget measurements (rounds 21, 24,
  25, 28, 31, 40; finding 2 is a consequence of round 24's quarter-budget fallback, which assumed
  widening). One of two findings a consequence of an earlier fix. Each fix is a clause or two.
  Reviewer read of round records or changelog: none in the raw log. Doc length: 1738 -> 1743
  lines. Above the split-check length (U-T6): no new decision, no split.
- Second consecutive round without a High; the finding rate since the clean round 35 is
  4, 2, 3, 4, 2.
