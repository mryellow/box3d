---
title: Review round 59 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round58.md
mode: full pass
session: 01a143c7-ef85-75b3-b65a-d9db34db09b9 (second attempt)
---

# Round 59

Target at revision 56 (1893 lines).

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
Run: two attempts, same model and effort (U-P4). Attempt 1, 2026-10-16T18:10:08+10:00 to
18:11:14, exit 1 after 6 source reads: "Selected model is at capacity" (session
`01a143c2-e912-7fc3-8462-c8041e847090`; log kept as `round59.attempt1.raw.log`; no result, no
finding). After a four-minute pause, attempt 2, foreground, `timeout 540`, exit 0 after 92 source
reads, 18:15:37 to 18:24:05. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`,
session `01a143c7-ef85-75b3-b65a-d9db34db09b9`. The stream emitted the final message twice; the
last copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt tracks the three binding designs closely, but two H1 mechanics need correction before handoff. The corrected-replay reference test cannot compare IDs as written, and the phase-0 geometry registry can retain bytes indefinitely across repeated rewinds.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1, `HistoryReplayTest` | The corrected replay compares the original world's event fields with those from a fresh reference world and reuses the op log's original IDs. Simultaneously live worlds receive different `world0` values; entity IDs and event IDs contain that value (`src/physics_world.c`, `include/box3d/id.h`, `include/box3d/types.h`). The reference operations may target the original world, and exact event-field comparisons will fail even for identical simulations. | Specify a per-world ID map for replayed operations and definitions. Normalize only the world component when comparing events across the two worlds; retain exact field comparisons for rewind and replay in the same world. |
| 2 | M | §4 H1; §10 | Geometry entries are released only when `lastImagedTick` falls below the oldest retained image, while rewind deliberately leaves stamps from discarded ticks unchanged. Repeatedly creating unique geometry through tick 60 and rewinding to tick 0 keeps the oldest image at 0, so those discarded entries and reader copies accumulate without bound. This contradicts H1's stated lifetime and §10's bounded-retention reason. | Release entries no retained image references when rewind removes slots, or track references from retained images. Add a repeated rewind-and-branch geometry-churn case; the current case advances far enough after one rewind to miss this failure. |

### 3. Checked, no change

- Checked the record, container, serializer, broad-phase and step-tail claims in §2 against the cited live source; the write routes and capture point used by the increments are consistent.
- Checked the interval, minimum-window and eviction sequences in `HistoryRingTest` against the ring doc; the stated tick sequences are consistent.
- Checked the H2–H4 compile boundaries against the current exposed types and the proposed const or incomplete types.
- Checked the H5/H6 tree split and restore pass against the trees doc: the awake tree is imaged, while static, sleeping and eventually kinematic proxy identity is journaled.

### 4. Proposed edits

Define cross-world ID remapping and event comparison in H1's test machinery. Revise H1 geometry cleanup to account for discarded slots on rewind, then extend `HistoryRingTest` with repeated branching that keeps tick 0 retained.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Both are against this block's
own fixes: finding 1 against round 56's reference run, finding 2 against round 58's release rule.

1. **Adopted (H).** `b3BodyId` and `b3ShapeId` carry `world0` (`include/box3d/id.h:44`), the
   events carry those ids, and `b3CreateWorld` gives a second live world the next free index
   (`src/physics_world.c:223`), so the op log's recorded ids address the original world and a
   field-wise event compare across the two worlds fails on `world0`. The reference cannot run
   before the original (it replays the op log the original run records). Fix: ordinals resolve
   through a per-world table the replaying world's creations fill, checked against the recorded
   id exactly in the test's world and in `index1` and `generation` in the reference world, whose
   calls take ids from its table; the cross-world event compare leaves `world0` out, the
   same-world compare keeps every field; the reference is built after the original run and kept
   alive beside it; the state hash reads records, which hold no world index.
2. **Adopted (M).** With tick 0 retained, entries interned on dropped branches kept stamps above
   the oldest image forever. Fix: the stamps go; the registry collects the ids the serializer
   interns during an imaged capture (a per-entry `seenTick` deduplicates), the slot stores the list
   in its charge, each listed entry's `refCount` rises at write and falls when the slot is evicted
   or dropped by a rewind, and zero releases it; case 11 checks the figure falls at the drop and
   runs twenty five-step branches each rewound to tick 0 with the figure back to tick 0's after
   each. The tombstoned entry structs remain, a few dozen bytes per distinct geometry, stated.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 59: 2 findings (H1-M1-L0), 2 applied, 0 declined. Series: 181 findings, 180 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  pending
- Tripwire: fix 2 replaces round 58's stamp with a reference list (a mechanism exchanged in one
  round, U-S4 reported); fix 1 adds the per-world id table to the op log. Clustering:
  `HistoryReplayTest`'s corrected replay (rounds 56, 59) and H1's registry bullet (rounds 58,
  59); both findings against this block's fixes, the fourth such round since 50. Each fix is a
  paragraph. Reviewer read of round records or changelog: none in the raw log. Doc length:
  1893 -> 1907 lines. Above the split-check length (U-T6): no new decision, no split.
- Second round with a failed attempt (model at capacity); finished inside the timeout on the
  retry.
