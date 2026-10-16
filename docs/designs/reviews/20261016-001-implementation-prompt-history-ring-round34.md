---
title: Review round 34 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round33.md
mode: full pass
session: 01a142c4-ea76-7a40-a5f5-7845ac9f13a1 (resumed to finalize)
---

# Round 34

Target at revision 34 (1703 lines).

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 67 source reads and no final
message), 2026-10-16T13:32:43+10:00 to 13:41:43+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a142c4-ea76-7a40-a5f5-7845ac9f13a1`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 13:41:56 to 13:42:29,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the three binding designs and the live source. I found one missing read route at a compile boundary and two tests whose stated checks could pass without proving the behavior assigned to them.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H3a, H4 | H3a removes `fatAABBs` from `b3World` and specifies no read accessor for it, while H4 explicitly requires `world->fatAABBs` for static and kinematic leaf assertions. That access cannot compile after H3a. | Specify a const fat-AABB read accessor in H3a and use it in H4. |
| 2 | M | §4 H1; §6 | `HistoryCrossWorkerTest` does not specify how capture and replay actually use different workers. `b3World_SetWorkerCount` changes worker contexts but does not recreate the scheduler, which is fixed at world creation (`src/physics_world.c`, `src/scheduler.c`). A world created with one worker can report four on replay while still using its serial task callback. | Create the test world with a four-worker scheduler, switch to one for capture and back to four for replay, and verify background-worker participation. |
| 3 | M | §4 H1; §6 | `HistoryReplayTest` requires callback calls and rejections only **over the first run**. Those totals can be positive entirely before the tick-50 context change or before a rewind target, so the test can pass without exercising restoration and replay of callback-dependent host state. | Assert calls and outcome-changing rejections in the relevant post-change and replay intervals, with per-tick results compared. |

### 3. Checked, no change

- The §2 serializer, recording, proxy, pool, and step-boundary claims checked against the cited source.
- The ring’s exact interval-widening and window sequences in `HistoryRingTest` against the ring doc §§5–7.
- The H5/H6 division between imaged and derived trees against the trees doc §§5–9.
- The fail-before build boundaries, golden-hash exclusions, and benchmark command outputs.

### 4. Proposed edits

Add the fat-AABB read route to H3a/H4, make the cross-worker fixture’s scheduler setup explicit, and strengthen the callback checks in `HistoryReplayTest` and its §6 mapping.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** H3a moves `fatAABBs` into the shape array's incomplete type and named only
   the hot accessor's entry pointer (awake shapes, under the step mark); H4's leaf assertion and
   the readers `src/physics_world.c` has today (the collide pass's overlap test, the debug draw,
   `b3World_GetBounds`), the serializer's writer and the hash walk all read it outside the step's
   hot path. Fix: H3a exposes a const getter and names those readers; H4's two mentions use it.
2. **Adopted (M).** `b3CreateWorld` creates the built-in scheduler from the def's worker count
   and `b3World_SetWorkerCount` rebuilds the worker contexts only (`src/physics_world.c`), so a
   world created at 1 has no background threads at 4. Fix: the scenes are created at 4 and
   switched with `b3World_SetWorkerCount`. The reviewer's further check that background workers
   took part is not added: the threads exist by construction from the def and `src/scheduler.c`
   exposes no count to read.
3. **Adopted (M).** The counts were totals over the first run, satisfiable before tick 50. Fix:
   counted per tick, recorded with the hash, `ENSURE`d above zero on both sides of the change and
   equal to the recorded counts after every replayed step.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 34: 3 findings (H0-M3-L0), 3 applied, 0 declined. Series: 128 findings, 128 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: one const getter (`b3GetShapeFatAABB`), the
  read side of a boundary H3a already draws. Clustering: §4 H3a (rounds 12, 15, 19, 29, 34;
  finding 1 is a consequence of round 12's move of `fatAABBs` into the shape array meeting H4's
  assertion from round 19), §4 H1's `HistoryCrossWorkerTest` (rounds 6, 34; finding 2 present
  since revision 1), §4 H1's `HistoryReplayTest` (rounds 22, 24, 26, 34; finding 3 is a
  consequence of round 26's counted-calls fix). Two of three findings consequences of earlier
  fixes. Each fix is a clause or a short sentence. Reviewer read of round records or changelog:
  none in the raw log. Doc length: 1703 -> 1711 lines. Above the split-check length (U-T6): no
  new decision, no split.
- Second timed-out round of the series (28, 34), both finalized on resume with no further reads.
