---
title: Review round 29 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round28.md
mode: full pass
---

# Round 29

Target at revision 29 (1667 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T12:46:59+10:00 to 12:53:03+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1429b-0f22-72b2-bfa7-e6bda4764384`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The source reconciliation and ring-budget examples largely check out. Three gaps need edits before an implementing agent can carry out the prompt without choosing test or restore behavior.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3a, H5 | H3a makes records writable only through guarded routes, but says `b3DeserializeIntoShell` clears `inRestore` before H1’s rewind repoints borrowed geometry and clears awake bodies’ `bodyMoveIndex`. H5 likewise leaves the step-5 clear after its marked image scatter. The hot route requires `inStep`; the restore route requires `inRestore`. | Define a restore scope that covers those post-copy writes, with compatible scopes for H4’s journal walks, or name file-local restore operations that own the guard. |
| 2 | M | §4 H1 | `ReplayOps` resolves creation ordinals through IDs obtained during replay, but replay begins at T+1. Operations on bodies, shapes, or joints created at or before T have no replayed creation from which to obtain an ID. | Specify how the ordinal-to-handle map is seeded from the restored state at T and updated by creations on the replayed timeline. |
| 3 | M | §4 H1; §6 | `HistoryRewindTest` says random churn *can* schedule each named structural sub-scene. It does not require every sub-scene to run, yet §6 assigns the test evidence for every churn case in design §12 test 2. The test could pass while missing a case. | Schedule each named case deterministically, or assert a positive execution count for each in the normal and soak runs. |

### 3. Checked, no change

- §2’s step-end capture location, serializer omissions, filter recreation behavior, pool operations, and principal record write routes match the cited source.
- The stated ring widening and window sequences match the ring doc’s rules.
- The named compile boundaries use types and diagnostics that the proposed boundary changes can enforce.
- The H5 and H6 cost cases distinguish serialized, awake-tree, and kinematic-tree image costs as claimed.

### 4. Proposed edits

Add the restore guard’s exact entry and exit points across H3a–H5. Add replay-map seeding to H1’s op-log mechanics, and make execution of every named structural fixture an asserted part of `HistoryRewindTest`.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** H3a had the mark set only by `b3DeserializeIntoShell`, the walk halves and
   the scatter, each asserting it clear on entry, with `b3World_Rewind` setting nothing. H1's
   step 3 repoints borrowed-geometry shapes after `b3DeserializeIntoShell` returns, and step 5
   nulls every awake body's `bodyMoveIndex` (the design §9 step 5); H3a hands the first to
   `b3Shape_Restore` and makes every other direct record write an accessor write, so from H3a both
   would run outside any mark and the restore writers' assertion would fire on every rewind. The
   design §5.2 states the assertion as "inside `b3World_Rewind` or inside
   `b3DeserializeIntoShell`". Fix: the mark is saved, set and put back by each function that
   writes through the restore writers, so they nest, and `b3World_Rewind` holds it around steps 2
   to 5; the repoint and the clears are `b3Shape_Restore` and `b3Body_Restore` writes from H3a.
   No new state and no branch.
2. **Adopted (M).** The op log resolved an ordinal through the id "the replay obtains"; an entity
   created at or before T is never recreated by a replay from T+1. The design §9 states that the
   same creation calls in the same order give the same ids, so the id recorded at the original
   creation is the resolution for every ordinal. Fix: the log records each creation's id, the
   replay resolves to it and `ENSURE`s each replayed creation returns it; the corrected replay's
   dropped op is a setter, since dropping a creation would shift every later id.
3. **Adopted (M).** "A named sub-scene the churn can schedule" required nothing to run, while §6
   cites the test for every churn case. Fix: the schedule starts with the whole sub-scene list in
   a seeded order, each sub-scene's runs are counted and every count is `ENSURE`d above zero per
   scene, the pattern `HistoryReplayTest` already uses for its callbacks.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 29: 3 findings (H1-M2-L0), 3 applied, 0 declined. Series: 116 findings, 116 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  pending
- Tripwire: fixes add or remove a named mechanism: none added; the restore mark's assert-clear-on-
  entry is replaced by save-and-put-back (the same bool, nesting instead of asserting), and
  `b3World_Rewind` becomes one of its holders. Clustering: §4 H3a's restore mark (rounds 12, 15,
  19, 29; finding 1 is a consequence of round 19's "`b3World_Rewind` sets nothing itself" meeting
  H3a's migration of the rewind's own writes), §4 H1's op log (rounds 9, 29; finding 2 is a gap
  present since revision 1), §4 H1's `HistoryRewindTest` (rounds 4, 11, 18, 22, 24, 29; finding 3
  is a gap present since revision 1). One of three findings a consequence of an earlier fix. Each
  fix is a clause or a short paragraph. Reviewer read of round records or changelog: none in the
  raw log. Doc length: 1667 -> 1672 lines (finding 1 +2, finding 2 +2, finding 3 +1). Above the
  split-check length (U-T6): no new decision, no split.
- Finding 1 is the first High since round 27 and the first ever against the restore mark's
  scope; it would have asserted on every rewind in the validation build from H3a to H5.
