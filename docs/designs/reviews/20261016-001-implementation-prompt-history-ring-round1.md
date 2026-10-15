---
title: Review round 1 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T06:34:49+10:00 to 06:38:42+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14146-53e7-79c3-a663-dc63eb443f50`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is closely aligned with the three binding designs, but several prescribed checks cannot produce the evidence they claim. The main problems are invalid compile-boundary examples, ring tests with insufficient retention or budget setup, and byte-count assertions that can reject a correct implementation.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H2–H3c | The compile-boundary units cannot compile at their “before” commits: `b3JointSim` has no `drawScale` in `src/joint.h`, and `b3GetBodyRecord`, `b3GetContactRecord`, `b3GetJointRecord` and `b3GetIslandRecord` are introduced by the commits they are meant to test. Direct assignment through a const pointer also produces a read-only-object error, not the stated discarded-qualifier diagnostic. | Use fields and access routes present on both sides of each boundary; name the expected diagnostics accurately. |
| 2 | H | §4 H1 | `HistoryRingTest` reads its byte-count baseline **after world creation but before enable**, then requires the same count after `b3DestroyWorld`. Destroying the world necessarily frees allocations already present at that baseline. The post-disable checks can also fail legitimately because stepping and churn grow world-owned capacities (`src/container.h`). | Take the destroy baseline before world creation. For disable, compare ring-owned allocations or use a state and capacity matched baseline. |
| 3 | H | §4 H1 | `HistoryReplayTest` steps 90 ticks and later calls `Rewind(0)`, but specifies no window. The stated default `tickCount` is 60, so tick 0 has been evicted under ring doc §7; `HistoryCrossWorkerTest` inherits the protocol. | Explicitly use a window of at least 90 ticks for these tests. |
| 4 | M | §4 H1 | The budget cases do not force the behavior they claim. With slots near charge `s`, `maxBytes = 3s` permits the two-image minimum window, so eviction can continue forever without interval widening. With `maxBytes = 2s`, two slots of charge `s` are not over budget. | Set budgets from measured current slot charges so widening and the two-slot overrun are guaranteed, then assert both transitions. |
| 5 | M | §4 H3b | H3b declares `b3ConstArray` in `src/container.h` but omits that file from its file list. The type cannot be introduced as instructed without touching it. | Add `src/container.h` to H3b’s files. |
| 6 | M | §3, §4 | The fail-before copying procedure applies only when the test file did not exist at the prior commit. `test/test_history.c` exists after H0, so the new H1, H4 and H5 entries, and H6’s new case, are absent from their prior commits; invoking an absent entry with the exact-name runner does not establish the stated failure. | Copy the updated test file and registration into the prior worktree for every new entry or case, with its required source changes isolated. |
| 7 | M | §6 | The §7.3 mapping cites only a benchmark for the claim that disabled history pays *one null check per journal call*. Timing cannot establish the number of checks or rule out other work. | Add inspection of each journal entry path to this mapping; retain the benchmark as cost evidence. |
| 8 | M | §7 | `b3World_GetProfile().step` is recorded before the proposed capture point in `src/physics_world.c`. Dividing capture time by that value does not measure capture as a percentage of the complete step. | Time the whole step externally, or label and evaluate the ratio explicitly as capture time relative to the pre-capture simulation profile. |
| 9 | L | §2 | `src/physics_world.c` has seven traversal-wrapped public queries and casts, rather than the stated eight. | Correct the inventory count. |

### 3. Checked, no change

- The tick-window and two-image minimum-window rules used by the prompt match ring doc §§3, 6–8.
- The awake, static, sleeping and phase-2 kinematic tree treatment matches trees doc §§5–9.
- The reported serializer omissions, `userData` and `userShape` behavior, and snapshot version match `src/world_snapshot.c`.
- The filter’s current proxy-recreation behavior, array capacity retention, and manifold allocator live-count behavior match the cited source.

### 4. Proposed edits

Correct the H2–H3c boundary units and H3b file list; specify the replay window and budgets that force the intended ring cases; replace the byte-count baselines; extend the fail-before procedure to additions in an existing test file; and correct the §6–§7 evidence descriptions and §2 count.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2). The target is an Implementation Prompt, so
test mechanism and fail-before evidence are its content (U-C10) and in scope.

1. **Adopted (H).** `drawScale` is a field of `b3Joint` (`src/joint.h`), not of `b3JointSim`, so the
   H2 statement could not compile at H1. The record getters `b3GetBodyRecord`, `b3GetShapeRecord`,
   `b3GetContactRecord`, `b3GetJointRecord` and `b3GetIslandRecord` are introduced by the commits
   whose boundary they were meant to show, so no statement through them has a before run. An
   assignment through a const pointer is a read-only-object error on every compiler; the
   discarded-qualifier diagnostic fires when the const pointer is converted to a non-const one,
   which is what the design's flag is for. Fix: each boundary check names statements that compile
   at the previous commit and fail at the commit (writes through the routes that exist before:
   `b3GetBodySim` taken into a non-const pointer, the joint sim's `constraintHertz`, the array
   fields `world->bodies.data[...]` and the like, `b3GetShapeMaterials`), with the diagnostic each
   produces, plus statements through the new getters that fail at the commit alone and are stated
   as having no before run; §3's rule admits the second kind.
2. **Adopted (H).** `HistoryRingTest` read `base` before the enable but after the world's creation,
   and one case ended with `b3DestroyWorld`, which frees what the baseline already counted; and
   between the enable and the disable the world steps and churns, growing arrays it keeps
   (`src/container.h` never shrinks capacity), so `b3GetByteCount() == base` after the disable
   alone can fail on a correct ring. Fix: every case creates its world after
   `int64_t base = b3GetByteCount();` and ends with the disable, `b3DestroyWorld` and the baseline
   check (case 6 with the destroy alone); H4's extension refers to the check after the destroy.
3. **Adopted (H).** `HistoryReplayTest` stepped 90 ticks and rewound to 0 under the default
   `tickCount` 60; the ring doc §7 evicts tick 0 at tick 61. `HistoryCrossWorkerTest`'s golden scene
   (120 ticks at interval 4, rewind to 100), `HistoryJournalTest`'s `SegmentWalkCase` (12 ticks to
   0), `HistoryTreeTest` (30 ticks to 10) and `HistoryUndoSufficiencyTest` named no window either
   (U-G6). Fix: each names its `tickCount` (90, 120, 16, 30, 60).
4. **Adopted (M).** With every tick imaged at charge `s`, the ring doc §6's minimum window is two
   images; at `maxBytes` 3 × `s` it fits, so step 3 evicts to three slots and step 4 never runs:
   case 2 could not show widening. At `maxBytes` 2 × `s` two slots of `s` are within budget, so case
   4 was not over budget. Fix: `s` is tick 0's charge read after the enable on a churn-free world;
   cases 2 and 4 use `maxBytes` = `s` + `s` / 2, where one image fits and two do not, so the
   minimum window is over budget whenever an image is due and the interval doubles at each such
   tick; case 2 states the resulting `effectiveInterval` after steps 1, 2, 4, 8 and 16 and the
   imaged ticks 0, 32, 64, ... with `oldestImageTick` following the window bound; case 3 names
   its `tickCount` and that it widens the same way; case 4 states ticks 0 and 4 retained together
   after step 4 and tick 0 gone after step 5. The invariant check reads the imaged flags from the
   directory rather than mirroring the rule.
5. **Adopted (M).** H3b declares `b3ConstArray` in `src/container.h` and its file list omitted the
   file. Fix: listed.
6. **Adopted (M).** §3's fail-before procedure was written for a test file absent at the previous
   commit, which is only H0's case; `test/test_history.c` exists from H0 on, so H1, H4, H5 and H6
   add entries or cases to an existing file. Fix: the procedure applies to every commit that adds
   an entry or a case, copying the working tree's test file and registration into the worktree; a
   new case in an existing entry is identified by its `condition false:` line.
7. **Adopted (M).** The §6 row for the design §7.3 named the benchmark alone; timing shows cost,
   not that each journal function begins with the null check. Fix: the row names inspection of
   every journal function's first statement and keeps the benchmark for the cost.
8. **Adopted (M).** `world->profile.step` is written before `b3ValidateProxyTrees` and the unlock
   (`src/physics_world.c`), so it ends before the H1 capture point. Fix: §7 times the whole step in
   the harness around the `b3World_Step` call and says the profile's step is not the denominator.
9. **Adopted (L).** `b3BroadPhase_BeginTraversal` is raised in `b3World_Step`, `b3World_Draw`,
   `b3World_Explode` and seven queries and casts (`b3World_OverlapAABB`, `b3World_OverlapShape`,
   `b3World_CollideMover`, `b3World_CastRay`, `b3World_CastRayClosest`, `b3World_CastShape`,
   `b3World_CastMover`), not eight. Fix: seven.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Gate (U-G1) run by the author before this round: the prompt adds to the three Canonical designs
  two assertion-only `bool`s on `b3World` and a phase-0 registry and reader that H5 removes; the
  journal, the accessors and the tree writes are enforced by const types and single push functions,
  never by caller lists; the restore list is derived at restore, not reconciled; the window and
  budget rules return values, never branch on a check. AC-1 to AC-4 hold.
- Round 1: 9 findings (H3-M5-L1), 9 applied, 0 declined.
- Findings: 3/5/1 (round 1) -> pending
- Tripwire: fixes add or remove a named mechanism: no; the compile-boundary checks change their
  statements, the ring test its baselines and budgets, five tests gain a window value. Clustering:
  none (first round). Reviewer read of round records or changelog: none in the raw log. Doc
  length: 1263 -> 1284 lines; the fixes add one line (1, §3) plus one, two and three in H3a, H3b
  and H3c (1), one (2), five (3), seven (4), none (5), one (6), none (7), two (8), none (9). Above
  the split-check length (U-T6): the doc decides one thing, how three designs land as one branch;
  its sections are facets of that, so no split.
