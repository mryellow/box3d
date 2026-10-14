# Changelog: 20261014-002 implementation prompt, awake and non-awake dynamic trees

## Revision 1: first draft

- Drafted from 20261013-001 (Canonical) after reconciling its claims against `master` at
  `c6c6ead`. Every function, field, macro and file the design names was found as the design
  states, with these additions and readings recorded in §2:
  - `src/world_snapshot.c` serializes the trees by `b3_bodyTypeCount` and is not in the design's
    B0 file list; the prompt adds it to B0 with a snapshot version increment.
  - The base's `b3SolveContinuous` keys, sorts and evaluates each tree query on its own; the prompt
    reads the design's B3 as one evaluation over both dynamic trees' candidates, since two passes
    would make a bullet's result depend on which tree a proxy is in, against the design's §1.
  - A leaf's box can exceed the fat-AABB entry (bullet enlarge path), so the migration carries the
    old leaf's box, bits and moved bit rather than recreating from the entry.
  - The base already carries the tree-order changes, so the design's §1 first branch applies and
    the prompt sets the base to `master`; the design assigns that choice to the prompt. The owner
    confirmed: the four commits on the branch are the deliverable, and an upstream PR is a later
    cherry-pick if wanted, so the prompt has no PR-text section, only the closure doc.
- Decisions the design leaves to the prompt: the branch and base; one new test file with its own
  copies of the A0 primitives; B1's test places proxies in the fourth tree directly so it fails
  before B1, and B2 removes that placement; B0's fail-before is a build failure; B4's fail-before
  run defines the threshold macro on the compiler command line; the golden policy allows only
  `QUERY_SPAWN_QUERY_HASH` to change, with its cause shown.
- The benchmark's `-s` per-step files accumulate minima across scenes and thread counts, so the
  `pairs` and `refit` deltas come from a scratch harness reading `b3World_GetProfile`.

## Revision 2: after review round 1

Round 1: 8 findings (H2-M5-L1), 8 applied, 0 declined. Findings: 2/5/1 -> pending.

- B1's file list names `src/solver.c`, which its `b3SolveContinuous` change touches.
- `SleepingTreePairTest`'s fail-before check is the `b3ValidateNoMoved` assertion on the first
  step, since B0 widens that validation to the fourth tree and nothing before B1 clears that
  tree's moved bits; §3 admits a named `B3_ASSERT` message as fail-before evidence.
- `SleepingExplodeTest`'s base-commit run copies only its entry function and helpers into an
  otherwise empty test file, since the whole file names B0's constant and counter fields.
- `WakeDuringTraversalTest` case 1 moves its third box with `b3Body_SetTransform` before every
  step and asserts the awake tree's root is marked, since finalize marks a proxy only when it
  leaves its fat AABB.
- The §5 cause proof for `QUERY_SPAWN_QUERY_HASH` logs the ray and cast values beside the overlap
  order and requires them equal, since the hash folds all three.
- §7's scene hash names the full `HashWorldState` recipe (velocities and awake count included).
- §7's node-visit counter includes the node pairs `b3GatherCrossSeeds` tests; pass counts are
  stated as five cross and two self on the branch, two cross and one self on the base.
- §6 marks `SleepingExplodeTest` as a regression comparison with the base, matching §4.
- Length 665 -> 676 lines.

## Revision 3: after review round 2

Round 2: 7 findings (H0-M5-L2), 7 applied (one in part). Findings: 2/5/1 -> 0/5/2 -> pending.

- §6's B0 row names the walks the design's B0 names; the query, draw, sensor and Explode loops
  stay in B1, where the design's B1 names them. A Provenance section (§10) records why.
- `SleepingTreeHeightTest` subtest 2 asserts the moved leaf's bit directly after each rebuild and
  creates its static neighbour with `invokeContactCreation` false, since `b3TestPair` passes a
  pair marked on either side and the default shape def marks a static proxy.
- The placement asserts `world->userTreeTask == NULL`, the design's §3 "no tree write during the
  rebuild" in the validation build; the begin-touch test names it as its evidence.
- `WakeDuringTraversalTest` becomes `WakeUnderRebuildTest` and `WakeFromCallbackTest`, since
  `ENSURE` returns at the first failure and one entry can show one fail-before.
- `WakeFromCallbackTest` gains two regression cases for the drain's live-body rule (destroy into a
  reused slot, `b3Body_SetType`), since `test_world.c` has no callback doing either.
- §5 makes a differing ray or cast value a stop that reports every candidate's fraction for the
  owner to judge against the design's §1 tie exception.
- §6 says eight loops.
- Length 676 -> 705 lines.

## Revision 4: after review round 3

Round 3: 4 findings (H0-M3-L1), 4 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> pending.

- §2's lock inventory excludes `b3Body_Enable`, which never sets `world->locked`.
- `AwakeTreeMigrationTest` expects the awake count to be the pile plus the kinematic body.
- `WakeFromCallbackTest` case 1 compares `HashWorldState` against a second world whose wake is
  made between calls, after the wake and after one step (design §3's serial-run hash).
- `SleepingTreeHeightTest` subtest 2 checks every moved body's leaf bit after each rebuild and
  asserts the post-rebuild contact on the first-moved body.
- Length 705 -> 710 lines.

## Revision 5: after review round 4

Round 4: 5 findings (H0-M4-L1), 5 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> pending.

- `SleepingTreeHeightTest` subtest 1 creates the threshold plus 16 proxies and states why the
  threshold plus two is the least that can exceed it (a leaf has height 0).
- §7's node-visit measurement also counts each self pass's gather (pair indices scanned by
  `b3GatherMovedSiblings`, nodes visited by the marked descent).
- §7's scene hash enumerates live bodies from `world->bodies` by index, since `rain` destroys and
  recreates bodies and `large_world` creates them during the run.
- §7's second-step saving is shown at one worker, where the tree task runs inline inside `pairs`,
  with the per-step awake body count marking sleep steps; the harness prints that count.
- §2 names `b3World_CastRayClosest`'s internal callback.
- Length 710 -> 718 lines.

## Revision 6: after review round 5

Round 5: 3 findings (H1-M1-L1), 3 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 ->
pending.

- §5 names the two causes the design's §1 permits, callback order and a tied closest ray hit,
  defines the evidence for each, and lets the four query-spawn pins change on either; §9 stops on
  a golden changed without a §5 cause. This replaces the owner-judged stop on a differing ray
  value: a tie moves the spawn and so changes the state hash, sleep step and hit count, which §9
  had treated as defects.
- §7 marks a sleep step by a body's awake flag going from true to false, since the sleep scene
  wakes one pyramid per step and the awake count alone can hide a sleep.
- §2 names `b3ResetProxy`'s direct destroy-and-create route.
- Length 718 -> 723 lines.

## Revision 7: after review round 6

Round 6: 5 findings (H2-M3-L0), 4 applied, 1 declined. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 ->
0/4/1 -> 1/1/1 -> 2/3/0 -> pending.

- Declined: fork from `main` per the design's §2. The design's §1 hands the branch to this
  document, the A0 helper exists only on `master`, and the owner decided the deliverable; the
  reasoning moves into §10 Provenance.
- §5 permits `QUERY_SPAWN_QUERY_HASH` alone for a callback-order difference (the spawn uses the
  overlap count) and the four pins for a tie; the order proof no longer requires a sleeping body,
  since the awake tree's own layout differs from the single tree's. §2 states both causes.
- `WakeUnderRebuildTest` moves its third box by more than its extent plus margin, since
  `b3Body_SetTransform` moves the proxy only when the AABB leaves the fat AABB.
- New B2 test `SleepingWriterTest` for shape creation on a sleeping body (fails before) and
  `b3ResetProxy` through `b3Shape_SetFilter`; §6 cites it and names the inspected body paths.
- Length 723 -> 739 lines.

## Revision 8: after review round 7

Round 7: 3 findings (H1-M1-L1), 3 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 ->
2/3/0 -> 1/1/1 -> paused (U-S4: §5 three rounds running after a cut; §4's tests every round).

- `SleepingWriterTest`'s first box falls asleep after touching, so its contact is in the sleeping
  set and the filter change wakes it; the setter changes the mask bits only, since an unchanged
  filter returns early; a reset on a sleeping body with no touching contact is asserted to keep
  code 3.
- §5's tie permits the four pins only when the tied hits' normals differ, since the spawn reads
  the fraction and normal and never the shape.
- The B2 tests intro excepts `SleepingExplodeTest` from the fail-before rule.
- Length 739 -> 744 lines.

## Revision 9: owner direction after the pause

The owner confirmed the branch forks from `master` and directed that the golden policy not be
weakened. §5 is restated as an attribution proof over the query-spawn scene's inputs (ray hit
flag, point and normal; overlap count; cast fraction; plus callback order for the query hash),
compared query by query up to the first differing spawn input, which must be a two-shape equal-
fraction tie; it permits exactly the two layout effects the design's §1 names. §10 records the
alternatives not taken. Length 744 -> 753 lines. The series resumes at the same scope.

## Revision 10: after review round 9

Round 9 (confirmation, not confirmed): 2 findings (H0-M1-L1), 2 applied. Findings: 2/5/1 ->
0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> pending.

- §5's tie clause requires the tied fraction to be the least positive fraction in both all-hits
  lists and the query's other values equal; the log carries the closest hit's flag and fraction.
- §2 states `b3Body_SetType`'s behaviour for enabled bodies and names the disabled early return.
- Length 753 -> 756 lines.

## Revision 11: after review round 10

Round 10: 2 findings (H1-M1-L0), 1 applied, 1 declined. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 ->
0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> pending.

- Declined again: fork from `main`. The owner confirmed `master` in writing; §10 already records
  it. From round 11 the review prompt states that §10's decisions are settled (U-P13).
- §5 compares the logging ray cast's all-hits list as a set of (shape id, fraction), since its
  callback order is public callback order.
- Length 756 -> 756 lines.

## Revision 12: after review round 11

Round 11: 1 finding (H0-M1-L0), 1 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1
-> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> pending.

- §5's order-only case requires an overlap permutation and the query hash recomputed from the
  branch's log by the scene's recipe to equal the new value, since the all-hits list is never
  hashed.
- Length 756 -> 758 lines.

## Revision 13: after review round 12

Round 12: 2 findings (H0-M1-L1), 2 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1
-> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> pending.

- §5's tie clause requires the two all-hits lists to be the same set of (shape id, fraction).
- §2 distinguishes the CCD layout test's sleep-off corridor worlds from its one-step sensor scene.
- Length 758 -> 760 lines.

## Revision 14: after review round 13

Round 13: 1 finding (H0-M1-L0), 1 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1
-> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> 0/1/0 -> pending.

- §5 logs point and normal for the closest hit and every all-hits entry; at a tie the point is
  equal, each closest hit matches its shape's entry, and the all-hits sets compare on all four.
- Length 760 -> 762 lines.

## Revision 15: after review round 14

Round 14: 3 findings (H0-M2-L1), 3 applied. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1
-> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> 0/1/0 -> 0/2/1 ->
pending.

- §5 no longer requires equal closest-hit points at a tie, since each shape casts in its own
  frame; each closest hit must match its shape's all-hits entry and the sets must be equal.
- `WakeFromCallbackTest` case 1 covers two sleeping boxes under one overlap so A's key is read in
  two callbacks of the same walk; §6 names the exit-drain inspection for the final interval.
- §7 requires a machine reporting at least eight cores, since the benchmark clamps the thread cap.
- Length 762 -> 766 lines.

## Canonical

Round 15 clean, round 16 (confirmation) clean. Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 ->
1/1/1 -> 2/3/0 -> 1/1/1 -> (paused) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> 0/1/0 -> 0/2/1
-> 0/0/0 -> SETTLED 0/0/0. Status set to Canonical at revision 15, 766 lines.

Rules whose cost in this series exceeded what they caught (U-R10): none. Input to Part A (B8): the
branch decision was re-raised in rounds 6, 7 and 10 until the prompt stated §10 as settled; U-P13
should be applied from round 1 of a series whose target has a Provenance section.

Design-level items for the owner, collected in the round records (rounds 3, 7, 8 and 16): the
design's §2 "forked from `main`" wording; the unbounded non-awake tree height between the B2
commit and B4 under the partial rebuild; callback-time destroy and type change under a held tree
view; a recording started from a query callback before the migration queue drains.
