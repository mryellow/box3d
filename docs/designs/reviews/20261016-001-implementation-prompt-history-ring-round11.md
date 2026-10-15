---
title: Review round 11 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round10.md
mode: full pass
---

# Round 11

Round 10 was the project's criteria review threshold; the owner chose to continue at the same
scope (B8). Target at revision 11 (1411 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:52:19+10:00 to 07:59:57+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1418d-4b5b-7b02-9db2-fbaa07d49ca0`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The implementation prompt has several blocking mechanics gaps, chiefly an H1 dependency on an H3a writer and an H4 awake-contact insertion route that its own rules forbid. Its budgeted restore measurement can also time an unavailable-tick return instead of a restore. I recommend changes before handing it to an implementing agent.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1 | H1 requires geometry pointers to be repointed through a shape restore writer, but `b3Shape_Restore` is introduced in H3a and H1 does not list `src/shape.c`. H1 cannot follow its stated mechanics as a bisectable commit. | Specify an H1 route using the then-current shape storage, or move the writer and its file into H1. |
| 2 | M | §4 H1 | `b3DestroyWorld` sets `world->locked` before stopping recording (`src/physics_world.c`). H1 then says it calls the public disable function at that point, without specifying how that call handles a locked world. The usual unlocked-world lookup asserts and returns NULL. | Specify a private cleanup function taking `b3World*`; have the public API and destructor call it through their appropriate entry paths. |
| 3 | M | §4 H3b | The proposed `b3Contact_Hot` returns only `b3Contact*`, while H3b makes `manifolds` pointer-to-const. The writable manifold access required by `src/contact_solver.c` and `src/mesh_contact.c` cannot come through that return type. | Specify a separate writable manifold view, alongside the cache view, for the hot path. |
| 4 | H | §4 H4 | H4 forbids generic pushes to the awake set’s arrays and names four exclusive push functions, but none inserts a **non-touching contact** into `awakeSet.contactIndices`. `src/contact.c`, `src/solver_set.c` and `src/physics_world.c` all use that route. | Name a dedicated awake non-touching-contact push and its journaling rule; include its create, wake and touch-transition callers. |
| 5 | M | §4 H5; §6 | `HistoryTreeTest` relies on a full rebuild to make derived-tree layouts differ, then checks only hashes and events. A full rebuild can reproduce an existing layout, so a passing run need not exercise the layout difference assigned to it in §6 and the trees doc §10. | Compare a layout signature before and after perturbation and require a difference in the relevant derived tree for each case. |
| 6 | M | §7 | Under the quarter-size budget, the effective interval can widen. Ticks 1, 5 or 9 behind the current tick may then be non-imaged, so timing `Rewind` at those offsets measures `b3_historyTickUnavailable` rather than restoration. | Measure fixed offsets on an interval-1 ring, or resolve each target with `GetRestorableTick` and report the actual rewind distance. |
| 7 | L | §2 | The stated `manifoldCount` write inventory omits its reset in `b3DestroyContact` (`src/contact.c`), a route relevant to H3b’s transfer of the block and count to pool-free handling. | Add creation and destruction to the count-write inventory. |

### 3. Checked, no change

- The ring’s interval-widening and window arithmetic matches the stated expectations in `HistoryRingTest` cases 1–5 and the ring doc §§5–7.
- The phase-0 account of serializer omissions, geometry interning, and the migration queue matches `src/world_snapshot.c`, `src/recording.c` and `src/replay.c`.
- The static, kinematic, awake and sleeping tree routes cited in §2 match the live broad-phase and solver code.
- The named CMake options, test-runner entry filtering, seventeen benchmark-table scenes, and `-t=8` benchmark behavior match the source.
- §5’s golden policy consistently treats changed hashes as a stop condition.

### 4. Proposed edits

Resolve findings 1–4 in the increment mechanics, add the layout assertion to `HistoryTreeTest`, revise the budgeted restore measurements, and correct the §2 write inventory.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** H1's step 3 said the restored shape is repointed "through its record's restore
   writer", and `b3Shape_Restore` is H3a's; H1's file list has no `src/shape.c`. In H1 the shape
   array is still a plain mutable array, so the write is a direct one. Fix: H1 says so and names
   H3a as the commit that moves the write under `b3Shape_Restore`.
2. **Adopted (M).** `b3DestroyWorld` sets `world->locked = true` before `b3StopRecordingInternal`
   (`src/physics_world.c`), and H1 had the destructor call the public disable function, whose
   `b3GetUnlockedWorldFromId` asserts on a locked world. Fix: `b3DisableHistoryInternal( b3World* )`
   in `src/world_history.c`, called by the public function after the unlocked lookup and by the
   destructor after `b3StopRecordingInternal`, the split the recording already has.
3. **Adopted (M).** `b3Contact_Hot` returned `b3Contact*` while H3b makes `manifolds` a
   `const b3Manifold*`; store impulses takes `b3Manifold* manifold = contact->manifolds +
   manifoldIndex` (`src/contact_solver.c`) and the mesh path writes the block (`src/mesh_contact.c`),
   neither of which compiles through the record. Fix: `b3Contact_HotManifolds` and
   `b3Contact_HotCache` return the writable views under the hot accessor's assertions, re-read after
   the hot path replaces the block; assigning the record's pointer and count stays a record write.
4. **Adopted (H).** The design's rule is one push per record kind onto the awake set's arrays
   (the design §7.1, Entering the awake set); H4 named four and none covered the awake set's
   `contactIndices`, which `b3CreateContact` (`src/contact.c`) and `b3WakeSolverSet`'s non-touching
   loop (`src/solver_set.c`) push onto. `b3AddNonTouchingContact` (`src/physics_world.c`) is the
   end-touch move, one of the container moves that make no entry, and `b3MergeSolverSets` pushes
   between non-awake sets. Fix: `b3SolverSet_PushAwakeContact` is the fifth exclusive push, with
   its two callers, its journaling rule (manifold block entry of count 0, asserted) and the other
   two sites classified.
5. **Adopted (M).** `HistoryTreeTest` checked hashes and events alone after a full rebuild;
   `test/test_tree_order.c` asserts its perturbation took (`WitnessDiffers` over `RecordVisitOrder`)
   because a rebuild can reproduce a layout. Fix: each case names the tree its candidates sit in and
   records a visit-order witness over it before and after the perturbation, asserting a difference.
6. **Adopted (M).** Under a quarter budget the effective interval widens (the ring doc §6;
   `HistoryRingTest` case 2 shows the sequence), so a rewind 1, 5 or 9 ticks back may return
   `b3_historyTickUnavailable`. The design §12 test 5 asks for the three offsets. Fix: the offsets
   are timed on the unbounded ring, and the quarter budget times one rewind to the tick
   `b3World_GetRestorableTick( currentTick − 9 )` resolves, with the distance reported.
7. **Adopted (L).** `b3DestroyContact` frees the block and zeroes `manifolds` and `manifoldCount`
   (`src/contact.c`); `b3CreateContact` zero-initialises the record; `b3ComputeMeshManifolds` frees
   and zeroes when no triangle manifold is accepted (`src/mesh_contact.c`). Fix: §2's inventory
   names the three.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 11: 7 findings (H2-M4-L1), 7 applied, 0 declined. Series: 53 findings, 53 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> pending
- Tripwire: fixes add or remove a named mechanism: `b3DisableHistoryInternal` (the recording's
  existing split), two writable-view companions of the contact hot accessor, and
  `b3SolverSet_PushAwakeContact` (the design's one-push-per-kind rule applied to the array the
  prompt had missed); each is the design's shape, none a new structure (AC-1), and the push closes
  a bypass the compile rule had left open (AC-2). Clustering: §4 H1 (rounds 1, 2, 4, 6, 10, 11),
  §4 H3b (rounds 3, 11) and §4 H4's awake-set bullet (round 11, first time); findings 1 is a
  consequence of round 10's fix 1 (the repoint route), findings 2 to 7 of revision 1. Each fix is
  a clause or a short paragraph. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 1411 -> 1440 lines. Above the split-check length (U-T6): no new decision, no split.
- The count rose from three to seven after the threshold round; two of the seven (1, 2) are H1
  mechanics, the section the series has revisited most. The owner may want this when deciding
  whether to continue past round 12 (U-S8; B8).
