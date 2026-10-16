---
title: Review round 67 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round66.md
mode: full pass
session: 01a1448f-10b9-7630-9daa-01e6e81a2ff6 (resumed to finalize)
---

# Round 67

Target at revision 63 (1977 lines).

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
Run: foreground, `timeout 540`, exit 124 (work timeout) after 95 source reads,
2026-10-16T21:53:08+10:00 to 22:02:08+10:00; resumed with `resume <session-id>` and the
finalize instruction (no further tool calls), exit 0, 22:02:08 to 22:02:38 (U-R8 provenance).
Header confirmed `model: gpt-6-sol`, `reasoning effort: high` on both runs, session
`01a1448f-10b9-7630-9daa-01e6e81a2ff6`. The report below is the resumed run's final message.

## Review report (Codex final message)

### 1. Summary

The increment order, ring transitions, compile boundaries, and source write routes are consistent with the three binding designs. Two test descriptions need stronger witnesses: as written, their stated checks can pass without exercising the cases they claim to cover.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1; §6 | `HistoryRewindTest` counts a run of the “undone wake past the sleeping-tree height trigger” subscene, but a run count does not prove the rebuild fired. The trigger depends on tree height (`src/broad_phase.c`), and §7 itself allows for insertions that do not reach it. | Require the subscene to check that `sleepingTreeRebuildCount` increases during the rewind; fail if its fixture does not trigger the rebuild. |
| 2 | M | §4 H5; §6 | The bullet sweep checks that `preSolve` rejected at least one contact. That does not establish the trees doc §10 case in which the **nearer** candidate was rejected; a farther rejection would satisfy the check. | Identify the rejected candidate in the callback and assert that its sweep hit is nearer than an accepted competing candidate. |

### 3. Checked, no change

- §2’s inventory of record, pool, pair-set, scalar, broad-phase, serializer, and replay routes matches the inspected live source.
- H1’s expected budget-widening and window tick sequences agree with the ring doc §§5–7.
- The H2–H4 compile-boundary examples match the proposed const and incomplete-type changes.
- The H5/H6 cost bounds distinguish the serialized image, the sleeping-dynamic hot image, and the raw kinematic-tree image.
- §5’s unchanged-golden policy and §9’s stop conditions are consistent.

### 4. Proposed edits

Add the two explicit witness assertions to the relevant §4 tests and narrow §6’s evidence claims to those verified cases.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Both are against revision-1
test text; neither follows from a previous fix.

1. **Adopted (M).** `HistoryRewindTest` counts each sub-scene's runs and `ENSURE`s the count
   above zero; the height sub-scene's claim is that the undone wake takes the sleeping tree
   past `B3_SLEEPING_TREE_MAX_HEIGHT`, which `b3RebuildSleepingTreeIfTall` (`src/broad_phase.c`)
   decides by height, and §7 itself says whether the insertions reach it "depends on the
   layout". A run count proves nothing about the rebuild. `sleepingTreeRebuildCount` is a
   `b3BroadPhase` field reported by `b3World_GetCounters` and absent from
   `src/world_snapshot.c`, so a rewind leaves it. Fix: the sub-scene's island uses the nested
   sizes `test/test_awake_tree.c`'s height cases use (one path deepened per insert) and
   `ENSURE`s the counter greater after the rewind than before.
2. **Adopted (M).** The continuous stage offers a candidate to `preSolve` only when its
   fraction is below the current best (`src/solver.c`), so a rejection count above zero shows
   a rejection of some candidate, not of the nearer one the trees doc §10 test 1 names. Fix:
   the callback rejects the designated candidate by shape id, and the bullet's position after
   the step is `ENSURE`d past that candidate and short of the accepted hit, which the time of
   impact allows only when the rejected candidate was nearer.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 67: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 200 findings, 199 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> 0/0/0 -> 0/2/0 -> 1/1/1 -> 0/2/0 -> pending
- Tripwire: no mechanism added or removed; both fixes add a witness assertion to an existing
  test case (one also fixes the fixture's sizes). Clustering: none with round 66 (H3b, §5, §2
  drew nothing); H5's `HistoryTreeTest` last drew a finding in round 55, `HistoryRewindTest`'s
  sub-scenes in round 54. No finding follows from a previous fix. Both rounds 66 and 67 found
  tests whose named check passes without the case they claim (round 63's finding 2 the same):
  three rounds of five with a witness defect, reported under U-S4 as a theme across sections
  rather than a section. Reviewer read of round records or changelog: none in the raw log. Doc
  length: 1977 -> 1984 lines. Above the split-check length (U-T6): no new decision, no split.
