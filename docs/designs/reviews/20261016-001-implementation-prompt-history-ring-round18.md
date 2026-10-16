---
title: Review round 18 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round17.md
mode: full pass
---

# Round 18

Target at revision 18 (1555 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T10:54:07+10:00 to 10:59:44+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14233-b83e-7c52-a936-091609dd363f`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt has two implementation-facing defects and one incorrect source claim. The three canonical designs remain the authority; these findings concern instructions and evidence in the prompt.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H2 | H2 requires `b3Graph_SetJointSim` to be implemented in `src/constraint_graph.c`, but omits that file and its declaration header from H2’s file list. The §3 compiler-named-file exception does not cover a new function whose implementation has yet to be added. | Add `src/constraint_graph.c` and `src/constraint_graph.h` to H2’s files. |
| 2 | M | §4 H1, `HistoryRingTest` case 2 | The case lists ticks 0 through 192 as “the imaged ticks” read from the directory through step 200. Under the ring doc §7 window rule, tick 160 is evicted at step 193; at step 200 the directory retains only image 192 and its following slots. | Specify that 0, 32, …, 192 are images *observed over the run*, and give the retained directory expectation at each checkpoint. |
| 3 | L | §2 | The wake-call inventory says `b3Body_SetTargetTransform` wakes a sleeping body when the implied velocity reaches its sleep threshold. In `src/body.c`, `wake == false` returns before that velocity is calculated. | State that waking also requires `wake == true` and a positive time step. |

### 3. Checked, no change

- Verified the serializer’s stated exclusions and the phase-0 need to retain the migration queue separately against `src/world_snapshot.c`.
- Verified the filter’s always-recreate behavior and the static, kinematic, awake and sleeping proxy routes against `src/shape.c` and `src/broad_phase.c`.
- Checked the H1 interval-widening sequence and the H5/H6 cost-test rationale against the ring and trees docs.

### 4. Proposed edits

Add H2’s graph files, rewrite H1 case 2 to distinguish cumulative image captures from retained slots, and correct the §2 target-transform wake condition.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** Round 15's fix 3 put `b3Graph_SetJointSim` in `src/constraint_graph.c`
   without adding the file to H2's list; §3's compiler exception covers files the type change
   breaks, not a new function. Fix: the two graph files are listed.
2. **Adopted (M).** With `tickCount` 32 the window keeps an image at most 32 ticks behind, so
   image 160 leaves at step 193 and the directory at step 200 holds 192 alone; the case said the
   seven imaged ticks were "read from the directory" as if all were retained. Fix: the case states
   the images written over the run and the two-image and one-image spans the window retains, with
   `oldestImageTick` per span.
3. **Adopted (L).** `b3Body_SetTargetTransform` returns for a non-positive time step and, on a
   sleeping body, for `wake` false before computing the velocity (`src/body.c`). Fix: §2 states
   both conditions.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 18: 3 findings (H0-M2-L1), 3 applied, 0 declined. Series: 82 findings, 82 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §4 H2's file list (rounds
  2, 18), `HistoryRingTest` case 2 (rounds 4, 6, 18), §2's wake bullet (rounds 4, 7, 9, 13, 16,
  18). Findings 1 and 3 are consequences of rounds 15 and 13 respectively; finding 2 of revision
  1. Each fix is a clause or a short paragraph. Reviewer read of round records or changelog: none
  in the raw log. Doc length: 1555 -> 1562 lines. Above the split-check length (U-T6): no new
  decision, no split.
- No High finding for the second round running; two of three findings were loose ends of the
  series' own fixes.
