---
title: Closure - awake and non-awake dynamic trees
status: Closure
date: 2026-10-14
implements: 20261014-002
---

# Closure: awake and non-awake dynamic trees

Closure of `docs/designs/20261014-002-implementation-prompt-awake-tree-split.md` (Canonical),
which implements `docs/designs/20261013-001-preflight-awake-tree-split.md` (Canonical). This
document stays outside the four commits; nothing in them was amended for it. The status of the
design doc and of the implementation prompt is unchanged.

## 1. Branch and base

- Branch `broad-phase-awake-split`, forked from `master` at `c6c6ead`.
- Commits, in order, each building and passing the whole `test` target in both precision
  configurations on its own:
  - B0 `7a9efd9` Add a fourth broad-phase tree for sleeping dynamic proxies behind proxy code 3
  - B1 `985a983` Search the sleeping dynamic tree in pair discovery and in every membership reader
  - B2 with B3 `f54fbc0` Move dynamic proxies between the awake and sleeping trees on sleep and
    wake, queued while a tree is walked
  - B4 `0a6c286` Rebuild the sleeping dynamic tree in full when an insert takes its height past a
    threshold

With the tree-order changes landed on the base, the step's results are unchanged by the split,
and public query callbacks and tied closest-hit casts can see sleeping bodies in a different order.

## 2. Suite counts per commit

The `test` target, built as §7 of the prompt says (Debug, `BOX3D_VALIDATE` on, samples off), run
whole after each commit. Every entry registered in `test/main.c` ran and passed; there are no
pre-existing failures in either configuration.

| Commit | Float: run / passed | Double: run / passed |
|---|---|---|
| base `c6c6ead` | 36 / 36 | 36 / 36 |
| B0 `7a9efd9` | 37 / 37 | 37 / 37 |
| B1 `985a983` | 38 / 38 | 38 / 38 |
| B2 with B3 `f54fbc0` | 43 / 43 | 43 / 43 |
| B4 `0a6c286` | 44 / 44 | 44 / 44 |

`DeterminismTest` passed at every commit in both configurations: no golden in
`test/test_determinism.c` changed, so §5 of the prompt had nothing to attribute and
`test/test_determinism.c` is untouched.

## 3. Warnings and formatting

- The Debug builds of the library and the test (both configurations) compile with no warnings at
  every commit (`-Wmissing-prototypes -Wall -Wextra -pedantic`).
- The Release benchmark build emits 12 `-Wmaybe-uninitialized` warnings on the branch and the same
  12 on the base, from `xmmintrin.h` through `include/box3d/math_functions.h`; they are
  pre-existing and not touched.
- `clang-format` with the repository's `.clang-format` was run on the changed hunks of every
  changed file (line ranges from `git diff -U0`) and on the whole new file
  `test/test_awake_tree.c` before each commit; each check reported no difference.

## 4. Deviations from the prompt and the design

Each with its reason. None changes a contract the design states.

1. **Snapshot file touched in B0.** `src/world_snapshot.c` is not in the design's B0 file list;
   the prompt's §2 assigns it to B0 under the design's rule that every walk over the trees uses
   the proxy-tree count. Both tree loops run over `B3_PROXY_TREE_COUNT` and `B3_SNAP_VERSION` is
   11 (was 10), with a comment naming the fourth tree. The layout hash's `b3_bodyTypeCount` term
   stays.
2. **CCD one-pass reading.** `b3SolveContinuous` queries the awake and the sleeping tree for a
   bullet into the same candidate array and evaluates once after both, so a sweep spanning both
   sees the one sorted list the single tree gave. This is the prompt's reading of the design's B3
   ("the bullet pass queries both dynamic trees"); evaluating them as two passes would have
   ordered every awake candidate before every sleeping one.
3. **A shared helper for the target proxy code.** `b3BroadPhase_GetBodyProxyType( const b3Body* )`
   in `src/broad_phase.c`, declared in `src/broad_phase.h`, gives the code a body's type and set
   imply. It is used by the placement function, by `b3CreateShapeInternal` (`src/shape.c`) and by
   the end-of-step validation (`src/physics_world.c`) so the rule has one home. It is a plain
   function, not a validator, flag, fallback, counter or scalar. `src/broad_phase.h` also gains
   `#include "container.h"` for the `b3Array( int )` queue.
4. **Explode's entry and exit pair wraps the impulse loop too.** The prompt says the two queries
   sit inside one pair; the design says the wakes queue and drain when Explode exits, after both
   traversals return. The pair is opened before the first query and closed after
   `ApplyExplosion` ran over the sorted list, so the wakes made there queue and drain at the exit
   as the design states.
5. **Order inside `b3World_Step`'s exit.** The depth is lowered and the queue drained, then the
   end-of-step proxy-tree validation runs, then `world->locked` is cleared, then the recording's
   state hash and bounds walk follow. The prompt puts the drain "at the point where
   `world->locked` is cleared"; the two statements are adjacent and nothing runs between them.
6. **B1's interim rebuild and early return.** Until B4, `b3UpdateTreesTask` rebuilt the sleeping
   tree with `fullBuild` false and the early return of `b3UpdateBroadPhasePairs` tested
   `b3NeedsRebuild` on it, as the design's §2 says for the commits before B4. B4 removed the
   rebuild line and replaced the test with `b3HasTreeMoved` on that tree. No commit left the
   tree unbounded.
7. **`b3BroadPhase_CreateProxy`'s parameter in B0 and B1.** The rename to `markMoved` lands in
   B2 as the design says. In B0 and B1 the new test passes `false`/`true` through the old
   `forcePairCreation` parameter, whose rule marks every non-static create, so the placed
   proxies of B1's test were marked as the prompt's §4 says ("the old leaf's box, bits and mark").
8. **Height test geometry.** The tree-level case creates `B3_SLEEPING_TREE_MAX_HEIGHT + 16`
   proxies on unit boxes with centers 4 apart along x, then moves each to a cube of half extent
   `1 + 0.5 i` centered at the origin. The unrotated moves give height 79 against the threshold
   64, and the full build gives 7. The world-level case uses the same sizes through
   `b3Body_SetTransform` on sleeping bodies placed 100 apart along x. No check was weakened.

## 5. Out-of-scope observations

Plainly stated, no fix attempted.

- **The refit's slow path is reachable after a mid-step wake.** A body woken by the collide's
  begin-touch pass is placed in the awake tree at the mid-step drain, after the rebuild task was
  joined. `b3InsertLeaf` can clear `dfsOrdered` on that insert, and the refit stage then takes
  `b3RefitSubtree` (commented "This only happens in unit tests") for that step; the next step's
  pair update rebuilds the tree. Correct, slower for that one step. Today's base inserts between
  steps only, so the refit always saw an ordered tree.
- **`b3Shape_SetFilter` always destroys the proxy.** It writes `shape->filter = filter` before
  computing `destroyProxy = filter.categoryBits == shape->filter.categoryBits`, which is always
  true. `SleepingWriterTest` relies on the recreate path as the prompt describes and passes
  either way.
- **`B3_ASSERT` output is lost under redirection.** The default assert handler prints and traps;
  with stdout redirected to a file the trap drops the buffered line. The fail-before runs below
  were made with `stdbuf -o0 -e0` to capture it.
- **Release warnings** as in §3, pre-existing on the base.
- **The B1 fail-before assertion fires inside `b3DynamicTree_ValidateNoMoved`**
  (`src/dynamic_tree.c`), which `b3ValidateNoMoved` calls per tree; the prompt names the caller.

## 6. Reader inventory against the prompt's §2

Every site that addresses `broadPhase.trees`, `bp->trees` or `->trees[` under `src/` on the
branch, and what it does with the fourth tree.

| File | Site | Fourth tree |
|---|---|---|
| `src/broad_phase.c` | `b3CreateBroadPhase` | created at `max( 16, dynamicShapeCount )`; the dynamic tree at 16 (B0) |
| `src/broad_phase.c` | `b3DestroyBroadPhase` | loop over `B3_PROXY_TREE_COUNT` (B0) |
| `src/broad_phase.c` | `b3BroadPhase_CreateProxy`, `b3BroadPhase_DestroyProxy` | guards at `< B3_PROXY_TREE_COUNT` (B0); height test after an insert into code 3 (B4) |
| `src/broad_phase.c` | `b3BroadPhase_MoveProxy` | height test after a move in code 3 (B4) |
| `src/broad_phase.c` | `b3UpdateBroadPhasePairs` early return | root moved bit of the fourth tree (B1), `b3NeedsRebuild` until B4 |
| `src/broad_phase.c` | gathers and passes | marked descent for the sleeping self pass, five cross passes, self pass per dynamic tree, `ClearMoved` on static and sleeping (B1) |
| `src/broad_phase.c` | `b3UpdateTreesTask` | rebuilt with `fullBuild` false in B1 to B3, removed in B4 |
| `src/broad_phase.c` | `b3BroadPhase_TestOverlap`, `b3BroadPhase_GetShapeIndex` | index by key code, unchanged |
| `src/broad_phase.c` | placement function | removes from and inserts into either dynamic tree by key (B2) |
| `src/broad_phase.c` | `b3ValidateBroadPhase` | validates the fourth tree beside dynamic and kinematic (B0) |
| `src/broad_phase.c` | `b3ValidateNoMoved` | loop over `B3_PROXY_TREE_COUNT` (B0) |
| `src/physics_world.c` | capacity tracking in `b3World_Step` | `dynamicShapeCount` from the sum of both dynamic trees' `proxyCount` (B0) |
| `src/physics_world.c` | recording bounds walk, `b3World_GetBounds` | loop over `B3_PROXY_TREE_COUNT` (B0) |
| `src/physics_world.c` | `b3World_Draw`, `b3World_OverlapAABB`, `b3World_OverlapShape`, `b3World_CollideMover`, `b3World_CastRay`, `b3World_CastRayClosest`, `b3World_CastShape`, `b3World_CastMover` | loop over `B3_PROXY_TREE_COUNT` (B1); one entry and exit pair around all of the call's walks (B3) |
| `src/physics_world.c` | `b3World_Explode` | second query of the fourth tree into the same array before the sort (B1); one entry and exit pair around queries and impulses (B3) |
| `src/physics_world.c` | `b3GetWorldByteCount` | fourth tree's bytes and the migration queue's bytes (B0, B2) |
| `src/physics_world.c` | `b3World_GetCounters` | `sleepingTreeHeight`, `sleepingTreeRebuildCount`; `treeHeight` unchanged (B0) |
| `src/physics_world.c` | `b3World_RebuildStaticTree` | static tree, unchanged |
| `src/physics_world.c` | `b3ValidateSolverSets` | accepts code 3 for every set that is not static or disabled (B0) |
| `src/physics_world.c` | end-of-step `b3ValidateProxyTrees` | every body's shapes carry the code its type and set imply (B2) |
| `src/solver.c` | `b3SolveContinuous` | fourth tree queried for a bullet after the dynamic tree, one evaluation (B1) |
| `src/solver.c` | refit stage | kinematic and dynamic trees only; the design says the sleeping tree is not refit |
| `src/solver.c` | bullet enlarge loop | dynamic tree; a bullet is awake, the `B3_VALIDATE` on the code stays true |
| `src/sensor.c` | `b3SensorTask` | fourth tree queried after the three (B1) |
| `src/world_snapshot.c` | `b3SerTree`, `b3DesTree` loops | over `B3_PROXY_TREE_COUNT`, version 11 (B0) |
| `src/shape.c` | `b3CreateShapeProxy` | guard at `< B3_PROXY_TREE_COUNT` (B0); mark rule (B2) |
| `src/shape.c` | `b3CreateShapeInternal` | code by type and set (B2) |
| `src/shape.c` | `b3ResetProxy` | keeps the key's code, mark rule (B2) |
| `src/solver_set.c` | `b3WakeSolverSet`, `b3TrySleepIsland` | migration call beside each `setIndex` write (B2) |

`src/recording.c`'s state hash hashes body sims and states, not trees, as the prompt's §2 says;
the recording tests pass unchanged.

## 7. Fail-before evidence

Each new test, run by its entry name against the commit before the one that lands it (a scratch
worktree with the working tree's `test/test_awake_tree.c`, `test/main.c` and
`test/CMakeLists.txt` copied in), then against the working tree. Outputs quoted as printed.

### B0: `SleepingProxyTreeTest` fails before by not building

Worktree at `c6c6ead`. The build fails; the first error:

```
test/test_awake_tree.c:30:26: error: ‘B3_SLEEPING_PROXY_TYPE’ undeclared (first use in this function)
test/test_awake_tree.c:69:26: error: ‘b3Counters’ has no member named ‘sleepingTreeHeight’; did you mean ‘staticTreeHeight’?
test/test_awake_tree.c:70:25: error: ‘b3Counters’ has no member named ‘sleepingTreeRebuildCount’
```

Working tree:

```
  subtest passed: SleepingProxyTreeCase
  subtest passed: SleepingTreeCountersCase
test passed: SleepingProxyTreeTest
```

### B1: `SleepingTreePairTest` fails before in the no-moved validation of the fourth tree

Worktree at B0 `7a9efd9`. The first step's pair update neither searches the fourth tree nor
clears its bits, so the placed proxies' marks survive to `b3ValidateNoMoved`:

```
BOX3D ASSERTION: b3IsNodeMoved( nodes + i ) == false, .../src/dynamic_tree.c, line 836
```

(the process then traps). Working tree:

```
test passed: SleepingTreePairTest
```

### B2 with B3: five tests against B1 `985a983`

```
=== AwakeTreeMigrationTest
condition false: GetProxyType( world, pile[i] ) == B3_SLEEPING_PROXY_TYPE
condition false: RunMigrationScene( false, runs + 0 ) == 0
test failed: AwakeTreeMigrationTest
=== SleepingWriterTest
condition false: GetProxyType( world, dropped ) == B3_SLEEPING_PROXY_TYPE
test failed: SleepingWriterTest
=== WakeUnderRebuildTest
condition false: GetProxyType( world, sleeper ) == ( awake ? b3_dynamicBody : B3_SLEEPING_PROXY_TYPE )
condition false: RunWakeUnderRebuild( 4, runs + 0 ) == 0
test failed: WakeUnderRebuildTest
=== WakeFromCallbackTest
condition false: scene.keys[0] == B3_SLEEPING_PROXY_TYPE
  subtest failed: WakeFromQueryCase
test failed: WakeFromCallbackTest
=== SleepingExplodeTest
  sleeping explode hash 0x6821AE5D
test passed: SleepingExplodeTest
```

`SleepingExplodeTest` passes before by design; it is the regression check for B1 and B2 against
the base (§8). `SleepingTreePairTest` at B1 with its B2 form (the direct placement removed) fails
at `condition false: GetProxyType( world, sleeper1 ) == B3_SLEEPING_PROXY_TYPE`, since creation
selected the tree by body type alone until B2. Cases 2 and 3 of `WakeFromCallbackTest` are not
reached before the commit because case 1 fails first; they pass before by the prompt's own
statement and are regression checks.

Working tree:

```
  slept after 32 steps
test passed: AwakeTreeMigrationTest
test passed: SleepingWriterTest
  woke on step 8
test passed: WakeUnderRebuildTest
  subtest passed: WakeFromQueryCase
  subtest passed: DestroyFromQueryCase
  subtest passed: ReplaceFromQueryCase
test passed: WakeFromCallbackTest
  sleeping explode hash 0x6821AE5D
test passed: SleepingExplodeTest
```

### B4: `SleepingTreeHeightTest` fails before at the world-level height assertion

Worktree at B2 `f54fbc0`, built with `-DB3_SLEEPING_TREE_MAX_HEIGHT=64` in `CMAKE_C_FLAGS` for
that scratch build only, since the macro does not exist before B4:

```
  tree level: height 79 after the moves, threshold 64
  tree level: height 7 after the full build
  subtest passed: SleepingTreeHeightTreeCase
condition false: counters.sleepingTreeHeight <= B3_SLEEPING_TREE_MAX_HEIGHT
  subtest failed: SleepingTreeHeightWorldCase
test failed: SleepingTreeHeightTest
```

Working tree:

```
  tree level: height 79 after the moves, threshold 64
  tree level: height 7 after the full build
  subtest passed: SleepingTreeHeightTreeCase
  world level: 1 rebuilds, max height 64
  subtest passed: SleepingTreeHeightWorldCase
test passed: SleepingTreeHeightTest
```

## 8. `SleepingExplodeTest` against the base

A `test/test_awake_tree.c` holding only that test and its helpers, registered in the base's
`test/main.c`, built and run in a worktree at `c6c6ead`, in both configurations.

| Configuration | Base `c6c6ead` | Branch `0a6c286` |
|---|---|---|
| Float | `0x6821AE5D` | `0x6821AE5D` |
| Double | `0xD9B9D19B` | `0xD9B9D19B` |

Equal in both: the same wake set, awake-set order and velocities as the base, asleep against
awake.

## 9. Goldens

No golden changed (§2). The query-spawn pins `QUERY_SPAWN_HASH`, `QUERY_SPAWN_QUERY_HASH`,
`QUERY_SPAWN_SLEEP_STEP` and `QUERY_SPAWN_HIT_COUNT` and every other pin hold their base values
in both configurations, so the §5 attribution logging was not needed and was not run.

## 10. Public additions

- `b3Counters` (`include/box3d/types.h`) gains `int sleepingTreeHeight` and
  `int sleepingTreeRebuildCount` at its end, with `///` comments.
- `B3_SNAP_VERSION` (`src/world_snapshot.c`) is 11; an image written before B0 holds three trees
  and is refused by the version check.
- `src/broad_phase.h`: `B3_SLEEPING_PROXY_TYPE` (`(b3BodyType)3`), `B3_PROXY_TREE_COUNT` (4),
  `b3BroadPhase.sleepingTreeRebuildCount`, `traversalDepth`, `migrationQueue`;
  `b3BroadPhase_CreateProxy` takes `bool markMoved`; `b3BroadPhase_GetBodyProxyType`,
  `b3BroadPhase_MigrateBody`, `b3BroadPhase_DrainMigrations`, `b3BroadPhase_BeginTraversal`,
  `b3BroadPhase_EndTraversal`. Internal header, not public API.
- `src/dynamic_tree.h`: `B3_SLEEPING_TREE_MAX_HEIGHT` 64 beside `B3_TREE_STACK_SIZE`.
  `src/dynamic_tree.c` is unchanged.

## 11. Measurements

All on this machine (12 cores reported), Release builds as the prompt's §7 says, base in its own
worktree and directory, branch in the repository, each run with the machine otherwise idle.

### 11.0 Plain summary

The change makes no measurable difference on any standard benchmark scene. The suite table in
§11.1 shows the branch 1 to 5 percent slower everywhere, but the offset is the same in the scenes
with no sleep at all, which the split cannot touch, and an alternating re-check on one of them
puts the two sides within noise; the two fifty-minute runs drifted apart. That table neither shows
a slowdown attributable to the change nor rules out one under 5 percent. Interleaving the suite
scene by scene would settle it and was not run.

The `sleep` scene is slightly slower in the pair phase, about 0.01 ms per step, because waking
and sleeping pyramids now move proxies between trees and the pair update runs on steps where the
base did nothing. Whole-step time there is within noise.

The one clear win is a scene built to show it, the harness's grid of ten thousand sleeping boxes
with a hundred teleported every step: the pair phase drops from 0.92 to 0.23 ms per step, about
four times faster, because the base rebuilt the ten-thousand-leaf tree on every step that touched
it and the branch leaves the sleeping tree alone. No benchmark scene had that shape, a large
sleeping population beside ongoing motion, until the `sleeping_grid` scene was added after the
merge (§11.7); there the whole step is about three times faster at one thread. Every scene's
results are bit-identical on every step.

### 11.1 Suite benchmark, total milliseconds per thread count

`benchmark -t=8`, four runs per thread count, minimum kept by the driver.

| Scene | Side | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|---|
| convex_pile | base | 9296.5 | 5252.0 | 3988.2 | 3365.6 | 3037.7 | 2901.9 | 2972.2 | 2954.1 |
| convex_pile | branch | 9565.6 | 5413.0 | 3997.5 | 3426.5 | 3033.3 | 2918.6 | 3062.6 | 3064.9 |
| joint_grid | base | 3163.3 | 1789.6 | 1264.7 | 1033.3 | 1113.2 | 1107.1 | 1097.8 | 930.6 |
| joint_grid | branch | 3173.4 | 1762.7 | 1300.5 | 1043.6 | 927.8 | 868.1 | 919.3 | 951.9 |
| junkyard | base | 21783.8 | 12593.6 | 9815.7 | 8296.6 | 7511.9 | 6991.1 | 6909.3 | 6721.7 |
| junkyard | branch | 21999.8 | 12676.1 | 9911.3 | 8466.6 | 7540.2 | 7389.3 | 7205.8 | 7156.4 |
| large_pyramid | base | 2810.6 | 1608.1 | 1212.9 | 1027.2 | 959.5 | 936.5 | 946.5 | 958.8 |
| large_pyramid | branch | 2879.8 | 1658.6 | 1251.4 | 1071.1 | 995.5 | 989.2 | 1026.2 | 1009.7 |
| large_world | base | 8.1 | 19.2 | 36.7 | 69.2 | 78.3 | 84.3 | 89.5 | 98.3 |
| large_world | branch | 8.7 | 17.4 | 38.9 | 70.2 | 78.9 | 86.3 | 93.2 | 99.1 |
| many_pyramids | base | 2978.3 | 1736.3 | 1334.6 | 1173.4 | 1054.2 | 1008.1 | 1006.4 | 1009.0 |
| many_pyramids | branch | 3022.9 | 1773.4 | 1381.6 | 1188.2 | 1087.5 | 1078.0 | 1069.1 | 1038.3 |
| mesh_drop | base | 2616.0 | 1474.6 | 1130.2 | 952.8 | 832.1 | 775.6 | 785.3 | 783.7 |
| mesh_drop | branch | 2689.2 | 1578.4 | 1156.4 | 1001.7 | 900.3 | 851.1 | 860.3 | 831.3 |
| post_field | base | 5016.2 | 2790.9 | 2014.4 | 1600.4 | 1393.4 | 1258.5 | 1225.3 | 1246.8 |
| post_field | branch | 5207.8 | 2953.7 | 1990.5 | 1642.3 | 1419.3 | 1327.2 | 1290.9 | 1285.3 |
| rain | base | 3483.2 | 2039.3 | 1532.5 | 1309.7 | 1182.6 | 1144.8 | 1338.3 | 1234.2 |
| rain | branch | 3616.3 | 2140.6 | 1592.9 | 1357.9 | 1248.9 | 1217.0 | 1247.3 | 1294.3 |
| sleep | base | 1220.7 | 742.6 | 613.5 | 549.2 | 481.3 | 473.0 | 494.5 | 506.8 |
| sleep | branch | 1275.6 | 781.7 | 628.0 | 590.2 | 534.6 | 522.0 | 531.1 | 567.6 |
| spinner | base | 4584.6 | 3067.6 | 2487.7 | 2213.3 | 2203.7 | 2204.1 | 2303.9 | 2321.8 |
| spinner | branch | 4732.9 | 3046.4 | 2467.7 | 2258.8 | 2263.8 | 2270.6 | 2339.9 | 2256.8 |
| trees100 | base | 214.2 | 155.9 | 147.1 | 143.9 | 155.3 | 177.8 | 188.1 | 211.0 |
| trees100 | branch | 211.5 | 160.1 | 150.9 | 151.9 | 159.7 | 178.1 | 194.1 | 210.0 |
| trees25 | base | 707.6 | 453.4 | 371.1 | 340.3 | 344.5 | 343.0 | 362.2 | 366.8 |
| trees25 | branch | 712.0 | 457.9 | 377.2 | 355.9 | 371.0 | 350.3 | 366.9 | 381.7 |
| trees50 | base | 321.5 | 224.7 | 192.2 | 187.5 | 195.7 | 213.8 | 227.7 | 238.7 |
| trees50 | branch | 319.6 | 229.6 | 206.9 | 199.5 | 201.1 | 217.9 | 223.3 | 239.6 |
| village | base | 4199.4 | 2644.4 | 2147.3 | 1902.0 | 1786.5 | 1771.7 | 1827.6 | 1798.8 |
| village | branch | 4371.8 | 2735.7 | 2227.7 | 2019.3 | 1917.1 | 1863.9 | 1918.7 | 1979.4 |
| washer | base | 30983.5 | 18571.8 | 14881.3 | 12559.3 | 11846.0 | 11649.8 | 11695.1 | 11108.5 |
| washer | branch | 31870.6 | 18626.1 | 15158.9 | 13034.0 | 11616.2 | 11711.2 | 11521.7 | 11457.9 |

Every scene's branch total sits 1 to 5 percent above the base at most thread counts, in the scenes
with no sleep or wake (`many_pyramids`, `large_pyramid`, `joint_grid`, `spinner`) as much as in
those with it. The base suite ran first and the branch suite second, each about fifty minutes. A
focused check afterwards, `benchmark -t=1 -r=4 -b=large_pyramid` alternating base and branch three
times on the idle machine, gives the two sides within noise of each other, so the uniform offset
in the table is drift between the two long runs, not a cost of the split:

| Round | Base (ms) | Branch (ms) |
|---|---|---|
| 1 | 2770.2 | 2775.0 |
| 2 | 2748.5 | 2704.3 |
| 3 | 2684.2 | 2676.3 |

### 11.2 Profile deltas on `sleep`, `rain` and `large_world`

`b3World_GetProfile` `pairs` and `refit` per step from the scratch harness, summed over the run
and the maximum step, at one and eight workers.

| Scene | Workers | Side | pairs sum (ms) | pairs max (ms) | refit sum (ms) | refit max (ms) |
|---|---|---|---|---|---|---|
| sleep | 1 | base | 2.918 | 2.8451 | 0.194 | 0.0018 |
| sleep | 1 | branch | 5.709 | 2.2152 | 0.176 | 0.0015 |
| sleep | 8 | base | 2.315 | 2.2696 | 0.253 | 0.0020 |
| sleep | 8 | branch | 5.843 | 2.3368 | 0.191 | 0.0017 |
| rain | 1 | base | 314.275 | 2.0495 | 17.151 | 0.1521 |
| rain | 1 | branch | 319.716 | 2.6784 | 16.963 | 0.1494 |
| rain | 8 | base | 162.554 | 1.2736 | 25.308 | 0.2835 |
| rain | 8 | branch | 180.627 | 5.5636 | 25.378 | 0.2191 |
| large_world | 1 | base | 22.006 | 16.1362 | 0.302 | 0.0014 |
| large_world | 1 | branch | 21.730 | 16.8287 | 0.240 | 0.0011 |
| large_world | 8 | base | 52.586 | 20.3655 | 0.407 | 0.0022 |
| large_world | 8 | branch | 39.823 | 14.6598 | 0.331 | 0.0106 |

On `sleep` the branch's `pairs` sum is about twice the base's, 5.7 against 2.9 ms over 300 steps at
one worker (0.01 ms per step more), with a lower maximum. The reason is in §11.4: nothing in that
scene moves a proxy after the first step, so the base's pair update returns early on almost every
step, while on the branch each wake's inserts into the awake tree and each sleep's order clear make
the pair update run and the awake tree rebuild on most steps. The work it does is small (the awake
gather scans about 600 pair indices per step) and `refit` is lower. On `rain` the sums are within 2
percent at one worker and 11 percent higher at eight, where one step's `pairs` maximum is 5.6 ms
against 1.3; on `large_world` the branch is lower at both worker counts.

On `sleep` at one worker, where the tree task runs inline inside `pairs`, the values on the step
after each sleep (a step over which at least one body went from awake to asleep) and the step
after that:

| Sleep step | Side | step after: pairs / refit (ms) | second step after: pairs / refit (ms) |
|---|---|---|---|
| 29 (2100 bodies) | base | 0.0003 / 0.0001 | 0.0001 / 0.0003 |
| 60 (210 bodies) | base | 0.0003 / 0.0007 | 0.0005 / 0.0013 |
| 61 (210 bodies) | base | 0.0005 / 0.0013 | 0.0004 / 0.0006 |
| 62 (210 bodies) | base | 0.0004 / 0.0006 | 0.0002 / 0.0006 |
| 63 (210 bodies) | base | 0.0002 / 0.0006 | 0.0002 / 0.0005 |
| 64 (210 bodies) | base | 0.0002 / 0.0005 | 0.0003 / 0.0009 |
| 65 (210 bodies) | base | 0.0003 / 0.0009 | 0.0006 / 0.0017 |
| 66 (210 bodies) | base | 0.0006 / 0.0017 | 0.0002 / 0.0007 |
| 67 (210 bodies) | base | 0.0002 / 0.0007 | 0.0002 / 0.0009 |
| 68 (210 bodies) | base | 0.0002 / 0.0009 | 0.0003 / 0.0008 |
| 69 (210 bodies) | base | 0.0003 / 0.0008 | 0.0002 / 0.0006 |
| 91 (210 bodies) | base | 0.0002 / 0.0007 | 0.0003 / 0.0007 |
| (81 sleep steps in all) | base | | |
| 29 (2100 bodies) | branch | 0.0105 / 0.0002 | 0.0067 / 0.0001 |
| 60 (210 bodies) | branch | 0.0394 / 0.0004 | 0.0347 / 0.0005 |
| 61 (210 bodies) | branch | 0.0347 / 0.0005 | 0.0359 / 0.0005 |
| 62 (210 bodies) | branch | 0.0359 / 0.0005 | 0.0391 / 0.0006 |
| 63 (210 bodies) | branch | 0.0391 / 0.0006 | 0.0357 / 0.0005 |
| 64 (210 bodies) | branch | 0.0357 / 0.0005 | 0.0579 / 0.0005 |
| 65 (210 bodies) | branch | 0.0579 / 0.0005 | 0.0398 / 0.0006 |
| 66 (210 bodies) | branch | 0.0398 / 0.0006 | 0.0339 / 0.0005 |
| 67 (210 bodies) | branch | 0.0339 / 0.0005 | 0.0338 / 0.0003 |
| 68 (210 bodies) | branch | 0.0338 / 0.0003 | 0.0336 / 0.0005 |
| 69 (210 bodies) | branch | 0.0336 / 0.0005 | 0.0002 / 0.0004 |
| 91 (210 bodies) | branch | 0.0396 / 0.0006 | 0.0414 / 0.0005 |
| (81 sleep steps in all) | branch | | |

The sleep steps are the steps over which at least one body went from awake to asleep, read per
body from the live bodies; the first twelve of 81 are shown. On the base the step after a sleep
costs the same as any quiet step, since nothing moved; on the branch the step after a sleep gathers
to the awake tree's old `nodeEnd` and rebuilds it, and the second step after is as cheap again only
when no other sleep or wake happened meanwhile (step 69's second step after, 0.0002 ms, is such a
step; in this scene a wake follows nearly every sleep, so most second steps still pay).

### 11.3 Non-awake tree height and rebuilds

Maximum and final `sleepingTreeHeight`, `treeHeight` and `sleepingTreeRebuildCount` per scene at
one and eight workers, and for the harness's grid scene (ten thousand single-shape bodies created
asleep in a 100 by 100 grid, then two hundred steps each moving one hundred of them to new grid
cells with `b3Body_SetTransform`, one worker).

| Scene | Workers | sleepingTreeHeight max / final | treeHeight max / final | sleepingTreeRebuildCount max / final | awake bodies max / final |
|---|---|---|---|---|---|
| sleep | 1 | 14 / 0 | 14 / 14 | 0 / 0 | 2100 / 2100 |
| sleep | 8 | 14 / 0 | 14 / 14 | 0 / 0 | 2100 / 2100 |
| rain | 1 | 13 / 12 | 20 / 18 | 0 / 0 | 3336 / 3168 |
| rain | 8 | 13 / 12 | 20 / 18 | 0 / 0 | 3336 / 3168 |
| large_world | 1 | 8 / 8 | 7 / 5 | 0 / 0 | 11 / 10 |
| large_world | 8 | 8 / 8 | 7 / 5 | 0 / 0 | 11 / 10 |
| washer | 1 | 13 / 13 | 35 / 31 | 0 / 0 | 8001 / 7736 |
| washer | 8 | 13 / 13 | 35 / 31 | 0 / 0 | 8001 / 7736 |
| trees100 | 1 | 14 / 14 | 22 / 0 | 0 / 0 | 50 / 0 |
| trees100 | 8 | 14 / 14 | 22 / 0 | 0 / 0 | 50 / 0 |
| junkyard | 1 | 3 / 3 | 38 / 34 | 0 / 0 | 10585 / 10579 |
| junkyard | 8 | 3 / 3 | 38 / 34 | 0 / 0 | 10585 / 10579 |
| grid | 1 | 21 / 19 | 0 / 0 | 0 / 0 | 0 / 0 |

Base `treeHeight` (the single dynamic tree) for the same scenes:

| Scene | Workers | treeHeight max / final | awake bodies max / final |
|---|---|---|---|
| sleep | 1 | 13 / 13 | 2100 / 2100 |
| sleep | 8 | 13 / 13 | 2100 / 2100 |
| rain | 1 | 25 / 19 | 3336 / 3168 |
| rain | 8 | 25 / 19 | 3336 / 3168 |
| large_world | 1 | 21 / 19 | 11 / 10 |
| large_world | 8 | 21 / 19 | 11 / 10 |
| washer | 1 | 34 / 29 | 8001 / 7736 |
| washer | 8 | 34 / 29 | 8001 / 7736 |
| trees100 | 1 | 23 / 23 | 50 / 0 |
| trees100 | 8 | 23 / 23 | 50 / 0 |
| junkyard | 1 | 36 / 33 | 10585 / 10579 |
| junkyard | 8 | 36 / 33 | 10585 / 10579 |
| grid | 1 | 30 / 24 | 0 / 0 |

No scene and not the grid reached the threshold: the sleeping tree's height stayed at or below 21
and `sleepingTreeRebuildCount` stayed at 0 everywhere. The grid's two hundred steps of one hundred
unrotated moves each took the sleeping tree from height 15 at creation to 21 at most and 19 at the
end, against a single dynamic tree height of 24 on the base, whose step rebuilt it on order.

### 11.4 Pass node visits

Temporary counters in a scratch worktree's `src/broad_phase.c`, one worker, summed over the run:
once per node pair tested in `b3GatherCrossSeeds`, once per `b3VisitPair` call and once per node
visited in `b3CollideProxyAndSubtree`, per pass; beside them the awake gather's scanned pair
indices (`b3GatherMovedSiblings`) and the sleeping gather's visited nodes (the marked descent).
The counters were never committed.

| Scene | Side | awake x static | awake x kinematic | awake x sleeping | sleeping x static | sleeping x kinematic | awake self | sleeping self | gather scanned pairs | gather descended nodes |
|---|---|---|---|---|---|---|---|---|---|---|
| sleep | base | 482 | 1 | - | - | - | 22683 | - | 2099 | - |
| sleep | branch | 572 | 91 | 91 | 91 | 91 | 22683 | 0 | 181559 | 0 |
| rain | base | 957231 | 398 | - | - | - | 6885485 | - | 1107562 | - |
| rain | branch | 938888 | 398 | 49878 | 398 | 398 | 6872069 | 0 | 1035434 | 0 |
| large_world | base | 29827 | 489 | - | - | - | 12322 | - | 24251 | - |
| large_world | branch | 21107 | 489 | 3755 | 489 | 489 | 2282 | 0 | 4671 | 0 |
| washer | base | 551217 | 16778695 | - | - | - | 161096108 | - | 7999000 | - |
| washer | branch | 540063 | 16779765 | 962536 | 1000 | 1120 | 160599615 | 0 | 7906796 | 0 |
| trees100 | base | 82850 | 332 | - | - | - | 1042210 | - | 364868 | - |
| trees100 | branch | 81946 | 340 | 20799 | 340 | 340 | 1033122 | 0 | 290786 | 0 |
| junkyard | base | 996168 | 162816 | - | - | - | 95339245 | - | 5291500 | - |
| junkyard | branch | 996332 | 162460 | 34903 | 500 | 943 | 95321827 | 0 | 5290698 | 0 |

The awake self pass visits the same nodes as the base's self pass to within a fraction of a
percent, and the awake-against-static pass visits fewer in every scene but `junkyard` (equal). The
three new passes are cheap: the sleeping tree's own cross passes against the static and kinematic
trees cost one root test per step where nothing sleeping moved (`sleeping x static` equals the step
count in `sleep`, `rain`, `large_world`, `trees100`, `junkyard`, and 1000 in `washer`), and the
sleeping self pass never ran, since no scene moves a sleeping proxy. The awake-against-sleeping
pass is the one with real work, at most 6 percent of the base's static pass (`washer`). The awake
gather's scanned pair indices are lower on the branch in every scene but `sleep`, where the pair
update runs on most steps (§11.2).

### 11.5 Threshold

`B3_SLEEPING_TREE_MAX_HEIGHT` stays at 64. No measured scene reached a height near it (maximum 21, in the grid scene), and no rebuild fired, so there is no case for a change.

### 11.6 Scene hashes, base against branch

The scratch harness's per-step hash over the live bodies (transform, linear and angular velocity,
awake flag per body in index order, then the awake count) for every benchmark scene at one
worker, for the scene's benchmark length.

| Scene | Steps | Equal every step | First differing step |
|---|---|---|---|
| convex_pile | 500 | yes | - |
| joint_grid | 100 | yes | - |
| junkyard | 500 | yes | - |
| large_pyramid | 200 | yes | - |
| large_world | 500 | yes | - |
| many_pyramids | 100 | yes | - |
| mesh_drop | 170 | yes | - |
| post_field | 100 | yes | - |
| rain | 400 | yes | - |
| sleep | 300 | yes | - |
| spinner | 800 | yes | - |
| trees100 | 500 | yes | - |
| trees50 | 500 | yes | - |
| trees25 | 500 | yes | - |
| village | 600 | yes | - |
| washer | 1000 | yes | - |

Every scene equal at every step: the §9 stop condition of the prompt was not met.

### 11.7 The `sleeping_grid` benchmark scene, added after the merge

Commit `a8d5f03` on `master` adds a scene with the shape the design targets: ten thousand boxes
asleep in a grid on the ground, thirty-two spheres rolling along a lane beside it every step, four
sleeping boxes lifted by `b3Body_SetTransform` each step, and a sphere dropped onto the grid every
fifty steps to wake one box. Five hundred steps. The same commit cherry-picked onto the base
`c6c6ead` builds unchanged, since the scene uses the public API only.

Whole-step totals from `benchmark -t=8 -r=4 -b=sleeping_grid`, two rounds alternating base and
master on the idle machine (milliseconds for the run):

| Side | Round | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|---|
| base | 1 | 58.8 | 76.5 | 91.4 | 96.3 | 105.9 | 115.5 | 119.0 | 125.7 |
| master | 1 | 18.7 | 33.7 | 57.5 | 79.3 | 107.0 | 114.9 | 120.2 | 124.7 |
| base | 2 | 63.0 | 79.5 | 94.0 | 101.8 | 113.2 | 118.4 | 122.7 | 130.6 |
| master | 2 | 19.0 | 34.9 | 60.6 | 83.5 | 109.6 | 118.8 | 123.3 | 129.0 |

At one thread the step is about three times faster on master; the lead narrows through four
threads and is gone from five up, where the per-step cost of waking the worker threads dominates
a step whose awake work is thirty-odd bodies on both sides. The phases, from the scratch harness
at one worker, summed over the run: `pairs` 43.4 ms on the base against 3.8 ms on master, `refit`
5.0 against 0.1. The base rebuilds and refits the ten-thousand-leaf tree on every step that
lifted a box; master writes the sleeping tree and rebuilds nothing. The sleeping tree's height
rose from 14 to 27 over the run, under the threshold, with no rebuild. The per-step state hash
over the live bodies is equal to the base on every step, and equal between one and eight
workers.

## 12. Design §3 checklist

| Item | Evidence |
|---|---|
| Sleep then wake an island with dynamic and kinematic bodies; keys as expected; `b3ValidateBroadPhase` each step | `AwakeTreeMigrationTest` (§7), perturbed run hashes equal |
| Sleeping island moved into a static box, another sleeping island, a kinematic body; same contacts in the same sets | `SleepingTreePairTest` (§7) |
| Wake by begin touch under a pending rebuild and from a query callback; no tree write under the walk; proxy in the awake tree after the drain; same hash as the serial run | `WakeUnderRebuildTest` (4 against 1 worker, equal hashes), `WakeFromCallbackTest` case 1 (equal hashes against the plain wake); the placement's `userTreeTask == NULL` assertion in the validation build |
| Tree driven past the threshold at the tree level and the world level; height bounded after every write; moved bits survive; contacts found | `SleepingTreeHeightTest` (§7) |
| Explosion over sleeping bodies: same wake set and velocities as the base | `SleepingExplodeTest` (§8) |
| Every benchmark scene hash unchanged | §11.6 |
