---
title: Review round 51 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round50.md
mode: full pass
session: 01a1435f-8b33-7580-bace-c450d12a1002 (resumed to finalize)
---

# Round 51

Target at revision 48 (1798 lines).

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 53 source reads and no final
message), 2026-10-16T16:21:36+10:00 to 16:30:36+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a1435f-8b33-7580-bace-c450d12a1002`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 16:30:54 to 16:31:13,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt tracks the three binding designs in the areas checked, but two prescribed tests conflict with the restore mechanics. One asks a journal-only helper to restore imaged tree state; the other expects a moved bit that the restore pass deliberately clears.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4, `ProxyIdentityCase` | The callback calls `b3Body_SetTransform` on a sleeping body. `src/body.c` wakes it, and the query’s traversal exit in `src/broad_phase.c` drains its proxy into the awake tree. The awake tree has no proxy journal entries; the trees doc §5 restores it from an image. `b3History_UndoPending` alone therefore cannot restore the recorded full hash. | Use `b3World_Rewind(historyTick)` for this API case. Keep a separate cold-only case for direct journal undo. |
| 2 | M | §4 H6, kinematic `HistoryTreeTest` | Each case requires `b3DynamicTree_CountMovedProxies >= 1` after rewind as proof that the pass moved a proxy. The trees doc §6 instead requires the pass to clear moved bits and reapply tick T’s saved moved list. `b3DynamicTree_MoveProxy` itself inserts unmarked (`src/dynamic_tree.c`). With an empty tick-10 list, a correct restore reports zero. | Compare the moved set with tick 10’s saved set; check the restored leaf box, proxy identity and replay separately. |

### 3. Checked, no change

- The §2 step-tail and traversal ordering matches `src/physics_world.c` and `src/broad_phase.c`.
- The serializer omissions named in §2 match `src/world_snapshot.c`; the prompt supplies the phase-0 queue and event cleanup they require.
- The ring budget and window sequences in §4 H1 agree with the ring doc §§5–7.
- The H2–H4 compile-boundary approach matches the current mutable routes and the proposed const and incomplete-type boundaries.

### 4. Proposed edits

Revise the H4 callback subcase to use full rewind, and replace the H6 moved-count assertion with checks of the saved moved set and restored proxy state. Update the corresponding evidence claims in §6.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Both findings are against
round 50's fixes.

1. **Adopted (H).** `b3BroadPhase_MigrateBody` queues the body while the traversal is open and
   the drain at the outermost exit migrates its leaf into the awake tree (`src/broad_phase.c`),
   which the trees doc §5 images and the journal does not cover, so `b3History_UndoPending`
   alone cannot restore the hash; `HistoryJournalTest` case 3 already routes its waking part
   through `b3World_Rewind`. Fix: the subcase rewinds to `historyTick`, the undo re-inserting the
   sleeping leaf from the migration's destroy entry before the move entry moves it back.
2. **Adopted (M).** H5's step 4 clears the moved bits and re-marks image T's saved list, and the
   pass's move leaves `dfsOrdered` false only "when it breaks the order" (the trees doc §6,
   `src/dynamic_tree.c`), so neither a moved count nor the order flag is a construction-level
   witness, and round 50's count would read zero on a correct restore with an empty tick-10 list.
   Fix: the kinematic cases claim no layout witness; they check the moved set against image T's
   list, every restored leaf's box against its fat AABB entry and the named proxy ids, then the
   replay; the layout-independence witness stays with the H5 cases over the trees no step
   rebuilds.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 51: 2 findings (H1-M1-L0), 2 applied, 0 declined. Series: 166 findings, 165 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: round 50's moved-count witness is removed.
  Clustering: §4 H4's `ProxyIdentityCase` (rounds 17, 50, 51) and §4 H6's kinematic cases
  (rounds 24, 31, 42, 50, 51); both findings are consequences of round 50's fixes, the second
  time in the series a round's fixes both drew the next round's findings (round 36 after 35's
  clean pass was the first with three of four). U-S4: the H6 kinematic bullet has produced a
  finding in five rounds; this revision cuts its witness claim to what the pass constructs, and a
  recurrence is reported. Each fix is a sentence. Reviewer read of round records or changelog:
  none in the raw log. Doc length: 1798 -> 1800 lines. Above the split-check length (U-T6): no
  new decision, no split.
- Seventh timed-out round (28, 34, 43, 44, 45, 46, 49, 51), each finalized on resume. The owner
  asked for a pause after this round's fold-in; no round 52 is sent.
