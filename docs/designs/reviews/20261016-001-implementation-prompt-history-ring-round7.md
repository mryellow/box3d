---
title: Review round 7 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round6.md
mode: full pass
---

# Round 7

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:20:08+10:00 to 07:25:34+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1416f-cf74-7971-84f2-b2b977fedfba`. The stream emitted the final message twice; the last copy
follows (the reviewer wrote absolute file links this round; they are left as emitted).

## Review report (Codex final message)

1. Summary

The prompt largely follows the three binding designs, but two verification recipes could report success or failure incorrectly. I also found an omitted scalar writer in its source inventory.

2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H5; §7 | `SleepingDynamicCostCase` creates 20,000 sleeping dynamic shapes before stepping. [src/shape.c](/home/user/box3d/src/shape.c) initially puts their marked proxies in the awake tree; [src/dynamic_tree.c](/home/user/box3d/src/dynamic_tree.c) never shrinks its proxy capacity. H5 must image that capacity, so proxy slots alone exceed the test’s 256 KB limit after warm-up. The million-body measurement has the same fixture problem. | Create sleeping dynamic bodies in batches, stepping between batches, and check awake-tree capacity before enabling history. |
| 2 | M | §4 H1; §6 | The event digest omits public event contents: contact IDs on begin, end and hit events; hit material IDs; and body-move `userData` ([include/box3d/types.h](/home/user/box3d/include/box3d/types.h)). Equality of this digest does not establish the full event-content equality assigned to it by design §12 test 3. | Digest every named public event field, in reported order, without hashing padding. |
| 3 | L | §2; §4 H2 | The `splitIslandId` writer inventory names `b3DestroyIsland` and the solver, but misses `b3SplitIslandTask` in [src/island.c](/home/user/box3d/src/island.c) and the sleep path in [src/solver_set.c](/home/user/box3d/src/solver_set.c). H2’s specific writer list repeats the omission. | Name both routes and require their conversion to `b3World_WriteScalars`. |

3. Checked, no change

- The ring’s window, widening, over-budget admission and rewind-discard rules match the ring doc §§3–8.
- The serializer omissions and phase-0 additions match [src/world_snapshot.c](/home/user/box3d/src/world_snapshot.c) and design §13.
- The proxy high-water behavior, tree restore division and proxy journal routes match the trees doc §§5–7.
- The named test filter, validation options and benchmark repeat behavior match the cited test and build sources.

4. Proposed edits

Revise the H5 and million-body fixtures to bound awake-tree high-water capacity; complete the event digest’s field list; add the two `splitIslandId` writers to §§2 and 4.

5. Unresolved

None.

6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** A shape created on a sleeping body is created in the awake tree and queued
   (`src/shape.c`, §2's own list), `b3DynamicTree`'s `proxyCapacity` never shrinks
   (`src/dynamic_tree.c`), and the trees doc §5 images the awake tree's proxies to that capacity.
   Creating 20,000 sleeping bodies at once leaves a 20,000-slot awake tree in every image, about
   2 MB, so the 256 KB bound would fail after H5 too. Fix: the fixture creates in batches of 100
   with a step after each and `ENSURE`s the awake tree's capacity is at most 256 before the enable;
   §7's million-body harness builds the same way.
2. **Adopted (M).** `b3ContactBeginTouchEvent` and `b3ContactEndTouchEvent` carry `contactId`,
   `b3ContactHitEvent` carries `contactId` and two user material ids, `b3BodyMoveEvent` carries
   `userData` and `b3JointEvent` carries `userData` (`include/box3d/types.h`); the digest named
   fewer fields. Fix: the digest hashes every named field of each public event struct, listed.
3. **Adopted (L).** `world->splitIslandId` is also written in `b3SplitIslandTask` (`src/island.c`)
   and `b3TrySleepIsland` (`src/solver_set.c`), beside `b3DestroyIsland` and the solver's step start
   and end (`src/solver.c`). Fix: §2 names the four sites; H2 refers to them.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 7: 3 findings (H1-M1-L1), 3 applied, 0 declined. Series: 35 findings, 35 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no; one fixture is built in batches, one digest
  lists its fields, one inventory names two more sites. Clustering: none of the three findings is
  in §4 H4 or the journal fixtures, which produced findings in rounds 2, 4, 5 and 6; §2's inventory
  produced one again (a writer list), the smallest of the round; no finding is a consequence of a
  previous fix. Reviewer read of round records or changelog: none in the raw log. Doc length:
  1361 -> 1370 lines; the fixes add six lines (1), two (2) and one (3). Above the split-check
  length (U-T6): one decision, no split.
