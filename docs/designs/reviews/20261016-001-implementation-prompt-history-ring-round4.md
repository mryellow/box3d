---
title: Review round 4 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round3.md
mode: full pass
---

# Round 4

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T06:58:41+10:00 to 07:02:42+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1415c-2a75-7841-820e-e7c439d75a6a`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is largely aligned with the three Canonical designs, but several implementation and verification instructions need correction. The most serious gaps are a graph bitset writer outside the stated journal route and a hash test that can reject a correct H0 implementation because recording snapshots omit state the new hash includes.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H0; §9 | `HashRoundTripCase` requires the full state hash to match after recording-player restores. The hash includes the migration queue, but §2 correctly says `src/world_snapshot.c` does not serialize it; phase 0 therefore copies it *beside* the snapshot. A tick with a parked proxy can fail this H0 gate even when the hash is correct. | Restrict this test to snapshots with an explicitly empty queue, and test queue restoration in H1 through the ring’s separate copy. Remove the H0 stop condition’s claim that any such mismatch proves a serializer defect. |
| 2 | H | §2; §4 H4 | The graph bitset writer inventory misses four direct `b3ClearBit` calls in `b3TrySleepIsland` (`src/solver_set.c`). H4 places the journaled bitset route in `src/constraint_graph.c` without directing these sleep-path writes through it; missed clears would leave the undo journal incomplete. | Add the sleep-path clears to §2 and explicitly route them through the graph’s journaled bitset mutator in H4. |
| 3 | H | §4 H3c–H4 | H3c makes `world->sensors` a by-value `b3ConstArray`, whose `count`, `capacity`, and pointer header remain writable through `b3World`. H4 describes journaled push/removeswap functions over that field but never specifies replacing the exposed field with the opaque journaled container required by the design §5.2. Direct header writes would still compile. | Specify the H4 storage conversion to an incomplete journaled array type and add a compile-boundary check for a direct sensor count write. |
| 4 | M | §4 H1 | `HistoryRewindTest` chooses random `(T, P)` pairs, then says to step from the **current** tick to `P`. A later pair can have `P` below that current tick, so the procedure cannot run as written. | Constrain each next `P` to be at least the current tick, or reset/rebuild the scene for each pair; state the sampling rule. |
| 5 | M | §4 H5; §6 | For `HistoryUndoSufficiencyTest` at interval 4, the target becomes the imaged tick at or before the scripted tick. That can move “put to sleep in tick T itself” into `(T, P]`, so this run no longer shows the case §6 assigns to it. | Place scripted target events on imaged ticks in the interval-4 run and assert the chosen `T` equals that tick. |
| 6 | M | §7 | The cost measurement calls for image and journal bytes per tick “split by the directory,” but H1’s specified directory accessor exposes only tick, total charge, and imaged flag. It cannot provide that split, including owned-block charge. | Specify scratch instrumentation or extend the read-only measurement accessor with image, journal, and owned-block byte counts. |

### 3. Checked, no change

- Verified the ring’s window, widening, minimum-window overrun, and rewind rules against the ring doc §§3–8; the stated tick ranges in `HistoryRingTest` cases 1–5 are consistent.
- Verified the step-end capture position, filter’s current recreate behavior, serializer omissions, tree write sites, and test runner filtering against the cited source.
- Verified H2’s proposed const-getter compile checks use members and getters present in the live headers.
- Verified H5 and H6’s 256 KB checks distinguish the respective serialized-image and kinematic-tree-image paths.

### 4. Proposed edits

Apply the six actions in the Findings table, then update the affected §6 evidence descriptions and §9 stop condition to match the corrected tests.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** `b3BroadPhase_DrainMigrations` (`src/broad_phase.c`) keeps a queued body whose
   marked leaf is bound for the sleeping tree, so the queue is non-empty at a step's end whenever
   a leaf is parked, as the trees doc §5 states; the serializer does not write it (§2), so a
   keyframe restore in the player empties it while the hash's section 13 reads it. Fix: the case
   records the queue count per tick and compares only at frames where it was 0, seeks to the
   nearest such frames, and requires at least 100 of 120 to qualify; §9's stop names a qualifying
   frame; the queue copy is H1's and is shown by the parked-proxy churn case.
2. **Adopted (H).** `b3TrySleepIsland` (`src/solver_set.c`) calls `b3ClearBit` on
   `color->bodySet` four times, for both bodies of a contact and of a joint it moves out of a
   colour. Fix: §2's bitset bullet names them; H4 routes them through the graph's journaled clear.
3. **Adopted (H).** With `world->sensors` a by-value `b3ConstArray`, its `count`, `capacity` and
   `data` header stay writable from any file, which the design §5.2's container rule forbids; the
   same holds for `world->constraintGraph.colors[i]`'s arrays and `bodySet` (by value on
   `b3World`) and, at H4, for `world->broadPhase.pairSet`, which `src/contact.c` writes through
   `b3AddKey` today (U-G6). Fix: H3c makes `world->sensors` and `world->constraintGraph` pointers
   to incomplete types with const element access outside their files; H4 makes `pairSet` a
   pointer-to-const to storage of `src/broad_phase.c`, as the trees are; the H3c and H4
   compile-boundary checks gain a header-write statement each.
4. **Adopted (M).** After `Rewind( T )` the current tick is T, and the next P could be drawn below
   it. Fix: each pair is drawn after the previous rewind with P in (current, current + 60] and T in
   [max( 0, P − 60 ), P].
5. **Adopted (M).** At interval 4 the rewind target is the imaged tick at or before the scripted T,
   so a "put to sleep in tick T itself" event could fall in (T, P]. Fix: the interval-4 script
   places T on a multiple of 4 and `ENSURE`s `b3World_GetRestorableTick( T ) == T` first.
6. **Adopted (M).** The slot reader of round 3 returned tick, charge and imaged flag, and §7 asked
   for the charge split. Fix: the reader returns the charge's parts (image, journal, owned-block
   bytes) and §7 names them.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 4: 6 findings (H3-M3-L0), 6 applied, 0 declined. Series: 24 findings, 24 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: two more storage headers (sensors, graph) on
  the pattern H3a to H3c already use, and the pair set made pointer-to-const on the trees' pattern;
  no new engine behaviour. Clustering: findings again in §2's inventory and §4 H3c/H4 (four rounds
  running, U-S4); finding 6 is a consequence of round 3's fix 4 (the reader it introduced was too
  narrow); finding 3 extends round 3's finding 2 (another header reachable by value). The fixes
  are clauses and one-line statements, no paragraphs; the recurring section is the module
  boundary, where each round finds one more site the design's rule reaches: the inventory is
  being completed, not a mechanism added. Reviewer read of round records or changelog: none in
  the raw log. Doc length: 1309 -> 1329 lines; the fixes add four lines (1), five (2), nine (3),
  two (4), one (5) and one (6), two lines removed by rewording. Above the split-check length
  (U-T6): one decision, no split.
