---
title: Review round 68 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round67.md
mode: full pass
session: 01a1449a-4b8a-7db0-b84f-8b5685dce1f9
---

# Round 68

Target at revision 64 (1984 lines).

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
Run: foreground, `timeout 540`, exit 0 after 52 source reads, 2026-10-16T22:05:24+10:00 to
22:12:18+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1449a-4b8a-7db0-b84f-8b5685dce1f9`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt is largely consistent with the binding designs and the current tree, but three implementation mechanics need correction. H2 risks a data race, H4 omits a required awake-island append route, and H1’s image-size guard disappears in Release builds.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H2; §5 | H2 says to replace each `splitIslandId` assignment with a copy and write of the **whole** `b3WorldScalars` struct. `b3SplitIslandTask` runs concurrently with solver tasks that read other scalars, including gravity (`src/island.c`, `src/solver.c`). A whole-struct write at the task’s current sites introduces a data race and contradicts §5’s no-change claim. | Specify how every scalar write made by the split task is deferred until the task joins, or otherwise synchronized, while retaining the design §5.2 writer boundary. |
| 2 | M | §4 H3c–H4 | H3c forbids generic pushes onto awake-set arrays, but H4’s list of the *only* awake pushes omits `islandSims`. Awake islands are appended both when created and when a sleeping set wakes (`src/island.c`, `src/solver_set.c`). The stated H4 routes leave those appends without a legal path or the design §7.1 pre-wake record entry. | Add a dedicated awake-island push route and name both callers and its journal behavior. |
| 3 | M | §4 H1 | The claimed failure at capture for an image over `INT_MAX` relies on `B3_ASSERT`. That macro is disabled in the specified Release build (`include/box3d/base.h`), leaving the `int` count-only buffer vulnerable to overflow (`src/recording.c`). | Require an always-active bounds check and defined failure at capture, or widen the serializer’s size path. |

### 3. Checked, no change

- The seven traversal-opening API paths, tree write sites, and `b3Shape_SetFilter`’s always-recreate behavior match §2 (`src/physics_world.c`, `src/solver.c`, `src/shape.c`).
- The serializer’s version, geometry interning, user-handle behavior, and recording hash match the reconciliation (`src/world_snapshot.c`, `src/recording.c`, `src/replay.c`).
- The ring budget example’s interval and retained-image sequence follows the ring doc §§5–7.
- The benchmark table has seventeen scenes; the Debug and Release `large_world` sizes and the stated test and benchmark commands match the build files.

### 4. Proposed edits

- Add safe split-task scalar sequencing to H2 and its verification.
- Add the awake-island push and pre-wake journal route to H3c–H4.
- Replace H1’s assertion-only image limit with a Release-effective check.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Findings 1 and 2 are against
revision-1 text; finding 3 is against round 58's size guard as round 63 left it.

1. **Adopted (H).** `b3SplitIslandTask` is enqueued in the solve stage's setup
   (`src/solver.c`) and joined after the solver tasks, so it runs beside stages that read
   gravity and the step scalars; it ends with `world->splitIslandId = B3_NULL_INDEX`
   (`src/island.c`), and `b3SplitIsland`'s destroy of the base island reaches
   `b3DestroyIsland`'s clear of the same field. Under H2 each of those is a copy and write of
   the whole `b3WorldScalars` struct from a worker while other workers read it. Fix: the solve
   stage's setup nulls the scalar on the main thread before any task of the stage is enqueued
   and hands the island id to the task through a local context; the task's own null and the
   post-join null are deleted. Every read of the field between the old and new null sites is
   the destroy's clear inside the task, which now finds the field already null, so no observable
   value changes (§5 holds).
2. **Adopted (M).** `b3CreateIsland` emplaces a `b3IslandSim` into its set's `islandSims`
   (`src/island.c`) and `b3WakeSolverSet`'s island loop emplaces into the awake set's
   (`src/solver_set.c`); the design §7.1 names island records among the kinds with one function
   into the awake set, and H4's list of the only awake pushes had no island entry. Fix:
   `b3SolverSet_PushAwakeIsland( world, island, sim )` with both callers named.
3. **Adopted in part (M).** `B3_ASSERT` expands to nothing unless `NDEBUG` is undefined or
   `B3_ENABLE_ASSERT` is defined (`include/box3d/base.h`), so H1's "fails at capture" held only
   in builds that compile assertions; the claim is corrected to say so. The always-active check
   and the widening are not adopted: capture has no failure result in the design
   (`b3_historyOk`, `b3_historyTickUnavailable` are the two), so a Release-effective check would
   need a result the design does not define or an abort, and the widening was declined in
   revision 56 as work on an artefact H5 removes; the largest phase-0 image §7 takes is
   `large_world`'s 650 MB (the design §11.1), a third of the limit. The reasoning is recorded in
   §10 (U-V10, U-C4).

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 68: 3 findings (H1-M2-L0), 3 applied (one in part), 0 declined. Series: 203 findings,
  202 applied, 1 declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> 0/0/0 -> 0/2/0 -> 1/1/1 -> 0/2/0 -> 1/2/0 -> pending
- Tripwire: fix 2 adds a push function the design already requires; fix 1 moves a write and
  deletes two; fix 3 corrects a claim and adds a §10 item. Clustering: H1's size guard drew
  findings in rounds 58, 63 and 68 (three of the last eleven rounds, each against the previous
  fix's wording; reported under U-S4); §4 H2 and H4's awake pushes last drew findings before
  round 50. Finding 3 is a consequence of round 63's fix wording. Two Hs in three rounds (66,
  68) after none between 61 and 65. Reviewer read of round records or changelog: none in the raw
  log. Doc length: 1984 -> 2002 lines. Above the split-check length (U-T6): no new decision, no
  split.
