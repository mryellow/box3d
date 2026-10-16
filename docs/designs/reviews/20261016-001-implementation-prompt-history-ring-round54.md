---
title: Review round 54 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round53.md
mode: full pass
session: 01a14385-6a30-7241-be83-bab6898d33d3 (resumed to finalize)
---

# Round 54

Target at revision 51 (1835 lines).

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
Run: foreground, `timeout 540`, exit 124 (work timeout after 68 source reads and no final
message), 2026-10-16T17:02:59+10:00 to 17:11:59+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a14385-6a30-7241-be83-bab6898d33d3`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 17:12:11 to 17:12:46,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The increment sequence and ring budget cases are broadly consistent with the binding designs and the live source. Section 2 has two inaccurate API classifications that could misdirect the implementing agent's journal and test work.

### 2. Findings

| # | Severity | Location | Issue and source evidence | Concrete fix |
|---|---|---|---|---|
| 1 | Medium | §2, around lines 254–282 | The awake-tree mutation inventory omits `b3Joint_SetCollideConnected(true)`. In `src/joint.c`, it marks a proxy moved and calls `b3BroadPhase_MigrateBody` for a sleeping dynamic body, placing that body's proxy in the awake tree. | Add this call and its condition to the inventory; include it in the rewind/replay churn that exercises between-step migration. |
| 2 | Low | §2, around lines 278–282 | The "cold, touching no awake structure" list states `b3DestroyJoint` with `wakeAttached=false` without qualifying the joint's state. `src/joint.c` still calls `b3RemoveJointFromGraph` when that joint is in the awake set, changing graph state. The H4 fixture uses sleeping joints, so its specific operation remains cold. | Qualify the list entry as applying to a joint in a non-awake set, and keep that condition explicit in the H4 fixture. |

### 3. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Neither is a consequence of a
previous round's fix; both are inventory omissions in §2's wake and cold lists.

1. **Adopted (M).** `b3Joint_SetCollideConnected` (`src/joint.c`) returns on no change; with
   `true` it picks the body with fewer shapes (B on a tie), calls
   `b3BroadPhase_MarkProxyMovedSerial` on each of its proxies and, for a dynamic body outside the
   disabled set, `b3BroadPhase_MigrateBody`, which moves a sleeping body's leaves into the awake
   tree, the trees doc §5's uncovered structure; with `false` it calls
   `b3DestroyContactsBetweenBodies`, whose `wakeBodies` is a literal `false`. Fix: the call and
   its condition join the wake list with the `false` branch noted, and `HistoryRewindTest`'s
   structural sub-scene list gains a joint between two sleeping bodies made `collideConnected`
   after T, rewound across.
2. **Adopted (L).** `b3DestroyJointInternal` (`src/joint.c:806`) calls `b3RemoveJointFromGraph`
   when the joint's set is the awake set and removeswaps the set's `jointSims` otherwise, before
   the `wakeBodies` branch. Fix: the cold list's entry reads "of a joint in a sleeping set" with
   the graph removal named, and the H4 fixture states the joint is in the merged sleeping set.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 54: 2 findings (H0-M1-L1), 2 applied, 0 declined. Series: 171 findings, 170 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> pending
- Tripwire: the fixes add one named sub-scene to `HistoryRewindTest`'s structural list and
  qualify two inventory entries; nothing removed. Clustering: §2's wake and cold lists (round 5
  last named the cold list's wording; the lists have been edited in many rounds since). Not a consequence of the previous round's fixes. Each fix is a clause. Reviewer read
  of round records or changelog: none in the raw log. Doc length: 1835 -> 1843 lines. Above
  the split-check length (U-T6): no new decision, no split.
- Eleventh timed-out round (28, 34, 43, 44, 45, 46, 49, 51, 52, 53, 54), each finalized on
  resume; the last four in a row.
