---
title: Implementation Prompt - awake and non-awake dynamic trees
status: Canonical
date: 2026-10-14
implements: 20261013-001
---

# Implementation Prompt: awake and non-awake dynamic trees

This is the prompt handed to the implementing agent. It carries the branch, the test mechanism,
fixtures and the test-to-requirement mapping that the design does not. The design decides what
changes and why; this document decides how it lands and how it is shown to work.

## 1. Task

Implement `docs/designs/20261013-001-preflight-awake-tree-split.md` (Canonical) in this repository,
on the branch `broad-phase-awake-split`, forked from `master` at `c6c6ead`, as the four commits its
§2 names (B0, B1, B2 with B3, B4), in that order, each bisectable. The design is binding: where this
document and the design disagree, the design wins and the disagreement is reported. Where the code
needs a behaviour the design does not state, stop and report; do not invent it.

The base carries the tree-order changes (`20261011-001` as implemented by `20261012-002`, plus the
sweep-order and lazy-keying commits on `src/solver.c`), so the design's §1 applies in its first
branch: with those changes landed, the step's and Explode's results do not depend on which tree a
proxy is in, and the exceptions are a stateful `preSolve` or custom filter, public query callback
order, and tied closest-hit casts. The design's §2 says the PR forks from `main`; the design's §1
assigns the base to this document, and the base is `master` because the design's §3 tests run
under the A0 perturbation helper, which exists only there (`test/test_tree_order.c`). The four
commits on this branch are the deliverable; an upstream PR, if one is ever wanted, is assembled
from them by cherry-pick outside this document.

Read, in this order, before writing code: the design (all of it), this document, `CONTRIBUTING.md`,
and the source the design names: `src/broad_phase.h`, `src/broad_phase.c` (all of it),
`src/dynamic_tree.h`, `src/dynamic_tree.c` (`b3DynamicTree_Create`, `b3InsertLeaf`,
`b3RemoveLeaf`, `b3CreateTreeProxyInternal`, `b3DynamicTree_MoveProxyInternal`,
`b3DynamicTree_EnlargeProxy`, `b3DynamicTree_Rebuild`, `b3BuildTree`, `b3PartitionMid`,
`b3DynamicTree_ClearMoved`, `b3DynamicTree_Refit`, `b3DynamicTree_GetHeight`), `src/shape.c`
(`b3UpdateShapeAABBs`, `b3CreateShapeInternal`, `b3CreateShapeProxy`, `b3DestroyShapeProxy`,
`b3ResetProxy`), `src/solver_set.c` (`b3WakeSolverSet`, `b3TrySleepIsland`), `src/body.c`
(`b3CreateBody`, `b3DestroyBody`, `b3WakeBody`, `b3WakeBodyWithLock`, `b3Body_SetTransform`,
`b3Body_SetType`, `b3Body_SetAwake`, `b3Body_Disable`, `b3Body_Enable`), `src/physics_world.c`
(`b3World_Step`, `b3Collide`, `b3World_GetBounds`, `b3World_Draw`, the overlap, cast and mover
queries, `b3World_Explode`, `b3World_GetCounters`, `b3GetWorldByteCount`, `b3ValidateSolverSets`),
`src/solver.c` (`b3SolveContinuous`, `b3Solve` from the tree-task join to the sleep pass),
`src/sensor.c` (`b3SensorTask`), `src/island.c` (`b3LinkContact`), `src/contact.c`
(`b3CreateContact`), `src/world_snapshot.c` (the broad-phase serialize and deserialize loops),
`include/box3d/types.h` (`b3Counters`, `b3Capacity`), `include/box3d/collision.h`
(`b3DynamicTree`, `b3TreeNode`), `test/main.c`, `test/test_macros.h`, `test/test_dynamic_tree.c`,
`test/test_tree_order.c`, `test/test_determinism.c`, `shared/determinism.c`, `shared/benchmarks.c`
(`CreateSleep`, `StepSleep`, `CreateRain`, `StepRain`, `CreateLargeWorld`, `StepLargeWorld`),
`benchmark/main.c`.

## 2. Reconciliation against the current tree

Checked on `master` at `c6c6ead` at the time of writing. Each item is a fact the increments rely
on. Where the design names a mechanism and the code differs, the difference is stated here and the
increment follows the code.

**Proxy key and tree array.**

- `B3_PROXY_TYPE( KEY )` masks the low two bits and casts to `b3BodyType`; `B3_PROXY_ID` shifts by
  two; `B3_PROXY_KEY( ID, TYPE )` packs them (`src/broad_phase.h`). The enumerator
  `b3_bodyTypeCount` has the value 3 (`include/box3d/types.h`), so the design's code 3 is that
  value cast to `b3BodyType`. The format holds four codes with no change.
- `b3BroadPhase.trees` is declared `[b3_bodyTypeCount]`. `b3CreateBroadPhase` has the
  `_Static_assert( b3_bodyTypeCount == 3 )`, creates the static tree at
  `max( 16, staticShapeCount )`, the kinematic tree at 16 and the dynamic tree at
  `max( 16, dynamicShapeCount )`; `b3DynamicTree_Create` floors its argument at 16, allocates
  `proxies` to that capacity, threads the free list through every slot, and starts with
  `nodeEnd` 2 and `dfsOrdered` true (`src/dynamic_tree.c`).
- `b3BroadPhase_CreateProxy` and `b3BroadPhase_DestroyProxy` assert `proxyType < b3_bodyTypeCount`;
  `b3CreateShapeProxy` (`src/shape.c`) asserts the same of the returned key. These are the three
  guards the design's B0 widens to four codes. `b3BroadPhase_CreateProxy` takes
  `forcePairCreation` and computes the mark as `proxyType != b3_staticBody || forcePairCreation`.
  Its callers are `b3CreateShapeProxy` and `b3ResetProxy` (`src/shape.c`), both of which pass a
  `forcePairCreation` they own. `b3BroadPhase_MoveProxy` always marks; its callers are
  `b3Body_SetTransform` (`src/body.c`) and `b3ResetProxy`. `b3ResetProxy`, when asked to replace
  the proxy, calls `b3BroadPhase_DestroyProxy` and `b3BroadPhase_CreateProxy` directly; every
  other destroy goes through `b3DestroyShapeProxy` (`src/shape.c`), called from shape destruction,
  `b3DestroyBody`, `b3Body_SetType` and `b3Body_Disable` (`src/body.c`).
- `b3UpdateShapeAABBs` (`src/shape.c`) selects the margin by `proxyType == b3_staticBody`, so
  code 3 already takes the dynamic margin with no change to that function. `b3ResetProxy` passes
  `B3_PROXY_TYPE( shape->proxyKey )` back into `b3UpdateShapeAABBs` and `b3BroadPhase_CreateProxy`,
  so it keeps a sleeping proxy in the non-awake tree once the guards accept code 3.
- `b3ValidateSolverSets` (`src/physics_world.c`) asserts a static set's proxies are static, a
  disabled set's are null, and every other set's are kinematic or dynamic. It is compiled only
  under `B3_ENABLE_VALIDATION`, which `include/box3d/base.h` defines as `BOX3D_VALIDATE` and not
  `NDEBUG`: a Debug build of the library and the test. It is called from `b3WakeBody` immediately
  after `b3WakeSolverSet`, before any queued migration, so under B0 it accepts code 3 in any
  non-static, non-disabled set, whether or not that set is awake.

**Readers that address trees by index.** These are the walks the design's B0 and B1 extend to the
fourth tree; the list is from a search of `broadPhase.trees`, `bp->trees` and `->trees[` over
`src/`.

- `src/physics_world.c`: the step's capacity tracking (`c->dynamicShapeCount` from the dynamic
  tree's `proxyCount`); the recording's world-bounds walk at the end of `b3World_Step` and
  `b3World_GetBounds`, both over `b3_bodyTypeCount` reading root bounds; `b3World_Draw`,
  `b3World_OverlapAABB`, `b3World_OverlapShape`, `b3World_CollideMover`, `b3World_CastRay`,
  `b3World_CastRayClosest`, `b3World_CastShape` and `b3World_CastMover`, each a loop over
  `b3_bodyTypeCount` running one query per tree with a host callback (`b3World_CastRayClosest`
  with its own internal callback); `b3World_Explode`, one query
  of the dynamic tree; `b3GetWorldByteCount`, three named tree byte counts; `b3World_GetCounters`,
  `staticTreeHeight` from the static tree and `treeHeight` as the maximum of the dynamic and
  kinematic heights; `b3ValidateSolverSets` as above.
- `src/broad_phase.c`: `b3DestroyBroadPhase` and `b3ValidateNoMoved` loop over `b3_bodyTypeCount`;
  `b3ValidateBroadPhase` validates the dynamic and kinematic trees; `b3UpdateTreesTask` rebuilds
  the dynamic and kinematic trees with `fullBuild` false; `b3UpdateBroadPhasePairs` tests the static
  root's moved bit and `b3NeedsRebuild` on the kinematic and dynamic trees for its early return,
  gathers the dynamic tree's moved sibling pairs by the scan to `nodeEnd`
  (`b3GatherMovedSiblings`), gathers cross seeds of the dynamic tree against the static and
  kinematic trees (`b3GatherCrossSeeds`, at most `B3_CROSS_SEED_COUNT` each, into one array of
  twice that), runs `b3CrossPairsTask` over the static seeds with `checkCompounds` and a flush, then
  the kinematic seeds, runs `b3SelfPairsTask` over the sibling pairs, and clears the static tree's
  moved bits. `b3CrossPairsTask` and `b3SelfPairsTask` read the three node arrays by fixed index.
- `src/solver.c`: `b3SolveContinuous` takes the static, kinematic and dynamic trees by fixed index;
  the bullet enlarge loop after the bullet stage validates `B3_PROXY_TYPE( proxyKey ) ==
  b3_dynamicBody` for a bullet's shapes, which stays true, since a bullet is awake; the refit stage
  refits the kinematic and dynamic trees and calls `b3ValidateBroadPhase`.
- `src/sensor.c`: `b3SensorTask` queries `trees + 0`, `trees + 1` and `trees + 2` by literal index.
- `src/world_snapshot.c`: the world image serializes and deserializes `b3_bodyTypeCount` trees in
  order (`b3SerTree`, `b3DesTree`) under `B3_SNAP_VERSION` 10, and its layout hash mixes
  `b3_bodyTypeCount`. The design's B0 file list does not name this file; its rule that every walk
  over the trees uses a proxy-tree count of four covers it, so B0 touches it (§4). An image written
  before B0 holds three trees, so the version is incremented in B0; the layout hash's body-type
  term stays, since the body-type count is unchanged.
- `src/recording.c`'s `b3HashWorldState` hashes body sims and states, not trees, so a recording's
  state hashes do not see which tree a proxy is in.

**Tree primitives the design relies on.** All in `src/dynamic_tree.c` unless stated.

- `b3InsertLeaf` clears `dfsOrdered` when the new pair or its parent lands out of order, and
  rotates on the way up only when `shouldRotate`; `b3CreateTreeProxyInternal` passes true,
  `b3DynamicTree_MoveProxyInternal` passes false. `b3RemoveLeaf` moves the sibling up into the
  parent's slot, frees the pair, refits the ancestors and rotates nothing; a root leaf's removal
  writes an empty root and leaves `nodeEnd` and the pair free list as they were.
- `b3DynamicTree_Rebuild` returns 0 for a tree with no proxies before writing anything; with
  `fullBuild` false it also returns 0 when the root is unmoved and the tree is ordered; its gather
  keeps every leaf's moved bit on a full build and clears it otherwise, descends marked internal
  nodes (every internal node on a full build), asserts its stack below `B3_TREE_STACK_SIZE` with no
  release guard, and `b3BuildTree` splits by `b3PartitionMid` under `B3_TREE_HEURISTIC` 0 and
  recomputes internal nodes with `b3MakeInternalNode`, which carries the children's moved bits up.
  The build writes `nodeEnd` 2 then bumps, sets `pairFreeList` empty and `dfsOrdered` true, and
  swaps the node arrays.
- `b3DynamicTree_ClearMoved` returns when the root is unmoved and otherwise descends through
  marked internal nodes only. `b3DynamicTree_GetHeight` returns the root node's `height` field.
  `b3DynamicTree_Refit` returns when the root is unmoved. `B3_TREE_STACK_SIZE` is 512 in
  `src/dynamic_tree.h`, which is where the design puts `B3_SLEEPING_TREE_MAX_HEIGHT`.
- A leaf's AABB can exceed the shape's entry in `world->fatAABBs`: the bullet enlarge loop in
  `b3Solve` enlarges a leaf only when it no longer contains the entry, so a leaf enlarged in an
  earlier step keeps its larger box. Pair discovery overlaps leaf boxes. A migration that recreated
  the leaf from the fat-AABB entry could therefore shrink it and change the pairs found, which the
  design's B2 forbids ("which tree a proxy is in and nothing else"). The migration reads the old
  leaf's box with `b3DynamicTree_GetAABB`, its category bits with `b3DynamicTree_GetCategoryBits`,
  its moved bit from the node at `tree->proxies[proxyId].node` with `b3IsNodeMoved`
  (`src/dynamic_tree.h`), and creates the new leaf from those three and the shape id. `b3TreeNode`,
  `b3TreeProxy` and the `b3DynamicTree` fields are public (`include/box3d/collision.h`), so this
  needs no new tree function.
- `b3DynamicTree_GatherMovedProxies` exists in `src/dynamic_tree.h` and gathers leaves, not
  sibling pairs; the non-awake self pass of B1 needs sibling pairs, so it gets its own gather in
  `src/broad_phase.c`, written like `b3DynamicTree_ClearMoved`'s descent.

**Sleep, wake and the step.**

- Solver set indices are `b3_staticSet` 0, `b3_disabledSet` 1, `b3_awakeSet` 2,
  `b3_firstSleepingSet` 3 (`src/physics_world.h`). A dynamic body's target tree is code 3 when
  `body->setIndex >= b3_firstSleepingSet` and code 2 when it is `b3_awakeSet`; a kinematic body's
  is the kinematic tree in either; a static body's the static tree; a disabled body has no proxies.
- `b3WakeSolverSet` writes `body->setIndex = b3_awakeSet` per body and `b3TrySleepIsland` writes
  `body->setIndex = sleepSetId` per body (`src/solver_set.c`); these are the only two writers that
  move a body between the awake set and a sleeping set, and the design's B2 migration call sits
  beside each write. `b3WakeSolverSet` is called from `b3WakeBody` (`src/body.c`), `b3LinkContact` and
  `b3LinkJoint` (`src/island.c`), joint creation (`src/joint.c`) and `b3World_EnableSleeping`
  (`src/physics_world.c`), so the migration call inside it covers every wake; `b3TrySleepIsland`
  is called from the sleep pass at the end of `b3Solve` and from `b3Body_SetAwake( false )`.
- `b3CreateBody` with `isAwake` false (and `enableSleep` true) puts the body in a fresh sleeping
  set; `b3CreateShapeInternal` then creates the proxy with `body->type` as the proxy type, which
  the design's B2 changes to select by type and set. `b3Body_Enable` transfers the body to the
  awake or static set before creating its proxies. `b3Body_SetType` on an enabled body destroys
  contacts, wakes the body, then destroys and recreates every proxy with the new type; on a disabled
  body it records the type and returns, there being no proxies. `b3DestroyBody` destroys the
  proxies and writes `body->setIndex = B3_NULL_INDEX`.
- `b3Body_SetAwake`, `b3Body_SetType`, `b3Body_Disable`, `b3Body_Enable` and `b3DestroyBody` take
  the world through `b3GetUnlockedWorld`, which refuses a locked world; all but `b3Body_Enable`
  set `world->locked` themselves. `b3WakeBodyWithLock` asserts the world unlocked, sets the lock, wakes, clears it; it
  is reached from the velocity, force, impulse and torque setters. `b3Body_SetTransform` asserts
  the world unlocked with `B3_ASSERT` and otherwise proceeds. The public overlap, cast and mover
  queries and `b3World_Draw` never set `world->locked`; `b3World_Explode` and `b3World_Step` set
  it for their duration. This is the design's B3 reason for a traversal depth that is not the lock.
- The step (`b3World_Step`, `src/physics_world.c`): capacity tracking; `b3UpdateBroadPhasePairs`;
  `b3Collide`, whose contact-state pass calls `b3LinkContact` for each contact that began
  touching, which wakes a sleeping set; `b3Solve`; then, when `b3Solve` left the tree task
  unjoined, the step joins it and calls `b3ValidateNoMoved`; `b3OverlapSensors`; the lock is
  cleared; the recording's state hash and world-bounds walk follow. Inside `b3Solve`
  (`src/solver.c`): the constraint solve, then the tree task is joined if present,
  `b3ValidateNoMoved` runs, finalize runs (fast-body CCD inside it), hit events, the refit stage,
  the bullet stage, and the sleep pass is last. `b3Solve` returns before all of that when the awake
  set has no bodies; with no awake body the awake contact set is empty (`b3CreateContact`,
  `src/contact.c`, puts a contact in the awake set only when a body is awake), so `b3Collide` wakes
  nothing on that path and the step's exit drain of B3 finds the queue empty.
- The mid-step drain of B3 sits after the join and after that `b3ValidateNoMoved`, before finalize:
  a woken proxy's carried bit is clear by then (the pair update that found its pair cleared the
  non-awake tree's bits), but placing the drain after the validation keeps the validation's
  statement exact.
- `b3SolveContinuous` collects each tree query's candidates into the worker's array, then keys,
  sorts and evaluates that query on its own (`b3EvaluateContinuousCandidates`), in the order
  static, kinematic, dynamic; a candidate can be a sleeping body, and the evaluation reads its sim
  through `b3GetBodySim`, which handles any set. Under the split, a bullet's dynamic query becomes
  two tree queries. Evaluating them as two sorted passes would order every awake candidate before
  every sleeping one, so a sweep spanning both would depend on which tree a proxy is in, against
  the design's §1. Both dynamic trees are therefore queried into the same array and evaluated in one
  pass, the one sorted list the single tree gives today. This is the implementer's reading of the
  design's B3 ("the bullet pass queries both dynamic trees"); it is reported in the closure doc
  as such.
- `b3World_Explode` on this base collects candidates into an array inside the query callback,
  sorts them by shape id and applies wakes and impulses in that order (`ApplyExplosion`), with
  `world->locked` set across all of it. The design's B3 collect-then-apply half is already there;
  the split adds the second query into the same array before the sort.
- `b3_isFast`, `b3GetBodyFullId` and `b3GetBodySim` are reachable from a test through the internal
  headers; `test/test_tree_order.c` reads a shape's internals as
  `b3Array_Get( world->shapes, shapeId.index1 - 1 )` and a body's as `b3GetBodyFullId`.

**Tests, goldens, benchmark.**

- `test/test_tree_order.c` holds the A0 helper as file-static functions: `PerturbTreeLayout`
  rebuilds the three trees by index with `b3DynamicTree_Rebuild( tree, true )`; `RecordVisitOrder`
  takes a `b3BodyType` tree index; `HashWorldState`, `RunScenePair`, `SameSet`, `SameSequence`,
  `WitnessDiffers`. `ExplodeOrderTest` there creates every body with `isAwake` false and witnesses
  the dynamic tree, so after B2 those proxies are in the non-awake tree and its witness would see
  nothing: B2 changes that test's witness tree. `ExplodeWitnessCase` creates awake bodies and is
  unchanged. `UnchangedHashCase` lets a pile sleep and is unchanged. `test/test_ccd_layout.c`'s
  corridor worlds run with sleep off and its sensor scene steps a default world once, so nothing
  there sleeps; it is unchanged.
- `test/test_dynamic_tree.c` is the idiom for tree-level tests: `b3DynamicTree_Create( 16 )`,
  `b3DynamicTree_CreateProxy`, `b3DynamicTree_MoveProxy`, `b3DynamicTree_Rebuild`,
  `b3DynamicTree_Validate`, `b3DynamicTree_Destroy`, all public (`include/box3d/collision.h`).
- The test runner (`test/main.c`) registers entry functions through an `extern` line and
  `MAYBE_RUN_TEST` in alphabetical order, filters by entry name, and `ENSURE` prints
  `condition false:` with the failing expression (`test/test_macros.h`). `test/CMakeLists.txt`
  lists the files.
- `test/test_determinism.c` pins, per precision configuration, `RAGDOLL_HASH`, `WAVE_PILE_HASH`,
  `QUERY_SPAWN_HASH`, `QUERY_SPAWN_QUERY_HASH`, `MESH_DROP_HASH` and their sleep steps. The query
  spawn scene (`shared/determinism.c`) folds into `queryHash` each overlap callback's shape id in
  callback order, the closest ray hit's fraction and normal, and the shape cast's closest
  fraction; the spawn itself uses the overlap count, the ray hit and the cast fraction, none of
  which depends on callback order except a tie. Under the split, `b3World_OverlapAABB` hands back
  the awake tree's leaves before the non-awake tree's, and the awake tree's own layout differs
  from the single tree's, so an overlap's callback order can change and with it
  `QUERY_SPAWN_QUERY_HASH`, which the design's B1 states (public query callbacks see layout order). The state hashes and sleep steps
  do not depend on it. Policy in §5.
- `benchmark/main.c` takes `-t=` (thread cap), `-w=` (one worker count), `-b=` (one scene by name
  or index), `-r=` (repeats) and `-s`, and writes `<scene>.csv` with the total milliseconds per
  thread count. Its `-s` per-step files hold minima accumulated over every run, thread count and
  scene so far (one `profiles` array, initialised once), so they are not per-scene values and the
  `pairs` and `refit` deltas of §7 come from a scratch harness reading `b3World_GetProfile`. It
  prints body, shape, contact, joint and stack counters only. The scenes are `convex_pile`, `joint_grid`, `junkyard`,
  `large_pyramid`, `large_world`, `many_pyramids`, `mesh_drop`, `post_field`, `rain`, `sleep`,
  `spinner`, `trees100`, `trees50`, `trees25`, `village`, `washer`. `sleep` lets ten pyramids
  sleep and wakes one per step through a filter joint; `post_field` moves bodies with
  `b3Body_SetTransform`. `B3_TIME_TO_SLEEP` is 0.5 (`include/box3d/constants.h`).
- `b3Counters` (`include/box3d/types.h`) is public with `///` comments on its later fields; the
  samples read `treeHeight` and `staticTreeHeight` and are off in the §7 builds.
- `CONTRIBUTING.md`: C17, no warnings, `clang-format`, comments in human words, Doxygen comments
  for public APIs only.

## 3. Rules for the implementer

- Four commits, B0 then B1 then B2 with B3 then B4, each building and passing the whole `test`
  target on its own in both precision configurations (§7). Commit messages are plain descriptive
  sentences, no type prefix, no AI-attribution trailer.
- Tests that must fail before the change are shown to fail before the change. The new test file
  does not exist at the base, so: check out the previous commit in a scratch `git worktree`, copy
  in the working tree's `test/test_awake_tree.c`, `test/main.c` and `test/CMakeLists.txt`, build,
  and run each new test by its own entry name. Where this document says a test fails before its
  commit at runtime, the build must succeed and the test must fail on the check this document
  names, the quoted `condition false:` line or `B3_ASSERT` message naming it, not on a fixture
  check. Where it says the test fails before by not building, the build failure and its first
  error line are the evidence.
  Then run the same tests in the working tree, where each passes. The closure doc quotes both runs
  of each test.
- Touch only the files each increment lists in §4. Any other file is a deviation: stop and report
  rather than touch it, except the golden policy in §5 and the closure doc of §8.
- Names in §4 are suggestions; the contracts are not. C17, no warnings, `clang-format` with the
  repo's `.clang-format` applied to the changed hunks only (a whole-file reformat is a deviation).
  Comments in human words; Doxygen `///` only on the two new `b3Counters` fields, as their
  siblings have.
- Do not add a validator, flag, fallback, counter or scalar the design does not name. The
  threshold-relative trigger the design's §8 declines is not implemented. No world-level rebuild
  function is exposed (design B4).
- Every deviation, pre-existing failure and out-of-scope observation goes in the closure doc (§8),
  plainly, with no fix attempted.

## 4. Increments

The new test file is `test/test_awake_tree.c`, created in B0 and extended by each later commit,
one entry function per test, registered in `test/main.c` by the commit that adds it. It includes
the internal headers as `test/test_tree_order.c` does. It carries its own copies of the A0
primitives it needs (a perturbation that fully rebuilds every tree of the world by direct
`b3DynamicTree_Rebuild`, called only between world calls; a state hash over named bodies as
`HashWorldState`; a visit-order witness as `RecordVisitOrder` taking a tree index), since those in
`test/test_tree_order.c` are file-static. Copy, do not move: the originals stay.

### B0. Fourth tree and proxy key

**Files.** `src/broad_phase.h`, `src/broad_phase.c`, `src/shape.c`, `src/physics_world.c`,
`src/world_snapshot.c`, `include/box3d/types.h`, `test/test_tree_order.c` (`PerturbTreeLayout`
loops over four trees), new `test/test_awake_tree.c`, `test/CMakeLists.txt`, `test/main.c`
(register `SleepingProxyTreeTest`).

**Commit message.** `Add a fourth broad-phase tree for sleeping dynamic proxies behind proxy code 3`

**Code.** Design §2 B0 is the specification. Mechanics that follow from it:

- `src/broad_phase.h` names the sleeping proxy code and the proxy-tree count, for example
  `B3_SLEEPING_PROXY_TYPE` as `(b3BodyType)3` and `B3_PROXY_TREE_COUNT` 4; `trees` becomes
  `[B3_PROXY_TREE_COUNT]`. The `_Static_assert` on three body types stays. The three guards of §2
  check `< B3_PROXY_TREE_COUNT`. `b3DestroyBroadPhase` and `b3ValidateNoMoved` loop over the
  proxy-tree count; `b3ValidateBroadPhase` validates the fourth tree beside the dynamic and
  kinematic ones.
- `b3CreateBroadPhase` gives `capacity->dynamicShapeCount` to the fourth tree and 16 to the
  dynamic tree. The step's capacity tracking records the sum of the two dynamic trees'
  `proxyCount` as `dynamicShapeCount`, since that hint sizes the non-awake tree of the next world.
- `b3GetWorldByteCount` adds the fourth tree's bytes beside the three. `b3World_GetCounters`
  keeps `treeHeight` as the maximum of the dynamic and kinematic heights and sets the two new
  fields: `sleepingTreeHeight` from `b3DynamicTree_GetHeight` on the fourth tree, and
  `sleepingTreeRebuildCount` from a counter in `b3BroadPhase`, zeroed in `b3CreateBroadPhase`,
  which B4 increments. The two fields go at the end of `b3Counters` with `///` comments.
- `b3World_GetBounds` and the recording's world-bounds walk loop over the proxy-tree count. The
  query loops of `b3World_Draw`, the overlap, cast and mover queries, the sensor pass and Explode
  are B1's.
- `b3ValidateSolverSets` accepts code 3 beside kinematic and dynamic for every set that is not
  static or disabled. The end-of-step check that a dynamic proxy's tree matches its body's set is
  B2's, since nothing migrates before it.
- `src/world_snapshot.c`: both tree loops run over the proxy-tree count; `B3_SNAP_VERSION` becomes
  11 with a comment naming the fourth tree.
- `PerturbTreeLayout` in `test/test_tree_order.c` rebuilds the four trees in index order.

**Test** `SleepingProxyTreeTest`, broad-phase level, no world stepping beyond a sanity step:

1. `b3CreateBroadPhase` with a capacity of 100 dynamic shapes: the fourth tree's `proxyCapacity`
   is 100 and the dynamic tree's 16. Create one proxy with code 2 and one with code 3 on
   overlapping boxes; the keys decode to their codes and ids; `b3BroadPhase_GetShapeIndex` returns
   each shape index; `b3BroadPhase_TestOverlap` is true across the two codes;
   `b3ValidateBroadPhase` and `b3ValidateNoMoved` run after `b3DynamicTree_ClearMoved` on both
   trees; destroy both, `b3DestroyBroadPhase`.
2. A fresh world's `b3World_GetCounters` reports `sleepingTreeHeight` 0 and
   `sleepingTreeRebuildCount` 0; a dynamic body created with `isAwake` false and one shape, then
   one step, leaves `b3ValidateBroadPhase` passing and both counters at 0. The tree the shape is
   in is not asserted here; it changes in B2.

Fails before B0 by not building: the fourth tree, the code constant and the two counter fields do
not exist. The closure doc quotes the first error.

### B1. Pair passes and membership readers

**Files.** `src/broad_phase.c`, `src/sensor.c`, `src/physics_world.c`, `src/solver.c`,
`test/test_awake_tree.c`, `test/main.c` (register `SleepingTreePairTest`).

**Commit message.** `Search the sleeping dynamic tree in pair discovery and in every membership reader`

**Code.** Design §2 B1 is the specification. Mechanics that follow from it:

- `b3UpdateBroadPhasePairs`: the early return also tests the fourth tree's root moved bit (and,
  until B4, `b3NeedsRebuild` on it, as the design's §2 states for the commits before B4). The
  gathers become: the awake tree's moved sibling pairs by the scan to `nodeEnd` as today; the
  non-awake tree's moved sibling pairs by a new descent from its root through marked internal
  nodes, pushing each pair index whose siblings carry a mark, with a stack of
  `B3_TREE_STACK_SIZE` as `b3DynamicTree_ClearMoved` has, O(marked nodes), skipped when the root is
  unmoved; cross seeds for five passes, awake against static, awake against kinematic, awake against
  non-awake, non-awake against static, non-awake against kinematic, each by `b3GatherCrossSeeds`
  into one array of five times `B3_CROSS_SEED_COUNT`, each gated at the roots by `b3TestPair`
  inside the gather as today.
- The self task takes the tree and its sibling list from its context instead of fixed fields, and
  runs once per dynamic tree with a nonzero list. The cross task takes a list of pass descriptors
  (node array A, node array B, seed range, `checkCompounds`) with contiguous ranges in the order
  above; every pass whose B side is the static tree runs with `checkCompounds` set to the world's
  compound flag and flushes before the next pass, as the static pass does today.
- After the passes, `b3DynamicTree_ClearMoved` on the static tree as today and on the fourth tree.
  Until B4, `b3UpdateTreesTask` rebuilds the fourth tree after the dynamic and kinematic trees
  with `fullBuild` false; B4 removes that line.
- `b3SensorTask` queries the fourth tree after the three. The loops of `b3World_Draw`,
  `b3World_OverlapAABB`, `b3World_OverlapShape`, `b3World_CollideMover`, `b3World_CastRay`,
  `b3World_CastRayClosest`, `b3World_CastShape` and `b3World_CastMover` run over the proxy-tree
  count. `b3World_Explode` runs its query on the fourth tree after the dynamic tree, into the same
  array, before the sort. `b3SolveContinuous` queries the fourth tree for a bullet after the
  dynamic tree, into the same candidate array, and evaluates once after both (§2).

**Test** `SleepingTreePairTest` (design §3, second bullet). One worker, gravity on, sleep on.
Three sleeping islands created asleep (`isAwake` false) well apart, each a single box body, plus
a static box, a kinematic box at rest and awake with no velocity, and a fourth sleeping box;
nothing touching at creation. Step once (every dynamic body is asleep, so only the kinematic body is solved). Then move the
first island's box with `b3Body_SetTransform` into overlap with the static box, the second into the fourth sleeping box, and the third into the
kinematic box. Step once. Assert, through the bodies' contact lists and `b3Contact.setIndex` via the internal
headers: one contact between the first box and the static box
in the disabled set; one between the second and fourth boxes in the disabled set; one between the
third box and the kinematic box in the awake set, with the third box now awake (begin touch
against an awake kinematic body wakes it) and the first, second and fourth still asleep. Assert
`b3ValidateBroadPhase` after each step.

In this commit the test places the four sleeping boxes' proxies in the fourth tree itself before
the first step, between world calls: for each, `b3BroadPhase_DestroyProxy` on the old key and
`b3BroadPhase_CreateProxy` with code 3, the old leaf's box, bits and mark (§2), writing the new
key into `shape->proxyKey`. B2 deletes that placement in the same edit that lands sleep
migration, so the test then exercises the engine's own path. Fails before B1 on the first step,
in `b3ValidateNoMoved` on the fourth tree: B0's pair update neither searches that tree nor
clears its moved bits, so the placed proxies' marks survive the step.

### B2 with B3. Sleep and wake move proxies; migration timing

**Files.** `src/solver_set.c`, `src/shape.c`, `src/broad_phase.c`, `src/broad_phase.h`,
`src/physics_world.c`, `src/solver.c`, `test/test_tree_order.c` (`ExplodeOrderTest` witnesses the
fourth tree), `test/test_awake_tree.c`, `test/main.c` (register `AwakeTreeMigrationTest`,
`SleepingWriterTest`, `WakeUnderRebuildTest`, `WakeFromCallbackTest`, `SleepingExplodeTest`).

**Commit message.** `Move dynamic proxies between the awake and sleeping trees on sleep and wake, queued while a tree is walked`

**Code.** Design §2 B2 and B3 are the specification. Mechanics that follow from them:

- `b3BroadPhase_CreateProxy` takes `bool markMoved` in place of `forcePairCreation`;
  `b3CreateShapeProxy` and `b3ResetProxy` pass `proxyType != b3_staticBody || forcePairCreation`.
- `b3BroadPhase` gains the traversal depth (an `int`), the migration queue (a `b3Array( int )` of
  body ids, created in `b3CreateBroadPhase`, destroyed in `b3DestroyBroadPhase`, counted in
  `b3GetWorldByteCount`), and the one placement function, in `src/broad_phase.c`, taking the world
  and a body: it derives the target code from the body's type and set (§2), and for each shape
  whose key names another tree destroys the old leaf and creates the new one from the old leaf's
  box, bits, mark and the shape id, writing `shape->proxyKey`. A body whose set is null or
  disabled, or whose proxies are already where its set implies, is left alone. When the tree the
  remove emptied is a dynamic tree, its node state is reset (`nodeEnd` 2, `pairFreeList` empty,
  the empty node beside the root rewritten, `dfsOrdered` true; proxy slots, capacity and free
  list untouched); when a remove leaves the awake tree non-empty, its `dfsOrdered` is cleared.
  The placement asserts `world->userTreeTask == NULL` (`B3_ASSERT`): that pointer is set at the
  tree task's enqueue and cleared at its join, so the assertion is the design's "no tree write
  during the rebuild" in the validation build.
- The choke point, in `src/broad_phase.c`, takes the world and a body id: with the depth at zero it
  calls the placement at once; otherwise it pushes the id. `b3WakeSolverSet` calls it per body
  beside its `setIndex` write, and `b3TrySleepIsland` likewise. No other caller.
- The drain, in `src/broad_phase.c`, runs the placement for every queued id against the live body
  slot and clears the queue; duplicates are harmless by the "already there" rule. Its callers: the
  outermost exit of every public walk (below), after that exit has lowered the depth to zero; and
  `b3Solve`, after the tree-task join and its `b3ValidateNoMoved`, before finalize (§2), with the
  depth still raised, so a sleeping body woken there lands in the awake tree before finalize's CCD
  and before the sleep pass. In B4 the drain tests the height after each insert when the depth is
  zero (B4).
- Entry and exit: `b3World_Step` raises the depth at its start and lowers it at its exit, the
  point where `world->locked` is cleared, then drains; the recording's bounds walk follows the
  drain. `b3World_Draw`, `b3World_OverlapAABB`, `b3World_OverlapShape`, `b3World_CollideMover`,
  `b3World_CastRay`, `b3World_CastRayClosest`, `b3World_CastShape`, `b3World_CastMover` and
  `b3World_Explode` each raise the depth before their first tree walk and lower it after their
  last, then drain; the pair sits around all of the call's walks, not each. The depth is a count,
  so a query issued from inside another query's callback nests and drains only at the outermost
  exit.
- `b3CreateShapeInternal` selects the proxy type from the body's type and set (§2): code 3 for a
  dynamic body in a sleeping set.
- The end-of-step validation (design B0): a function in `src/physics_world.c` under
  `B3_ENABLE_VALIDATION`, called at the step's exit after the drain, that walks every solver set
  and asserts each body's shapes carry the code its type and set imply (§2). It is an assertion,
  never control flow.
- `b3SolveContinuous` already queries both dynamic trees from B1; nothing changes here.
  `b3World_Explode`'s two queries sit inside its one entry and exit pair.
- `ExplodeOrderTest` in `test/test_tree_order.c`: its `RecordVisitOrder` calls name the fourth tree
  (its bodies are created asleep), and its `ENSURE` on the witness count is unchanged. No other
  change to that file.

**Tests** (all in `test/test_awake_tree.c`; each fails before this commit at the check named,
except `SleepingExplodeTest`, a comparison with the base).
`SleepingTreePairTest` loses its direct placement of B1 in this commit, in the same edit, and
otherwise stands.

- `AwakeTreeMigrationTest` (design §3, first bullet). One worker, gravity on, sleep on. A pile of
  dynamic boxes resting on a static ground beside a kinematic box created with zero velocity, the
  pile touching the kinematic box so they share contacts. Step until every body reports
  `b3Body_IsAwake` false (bounded by a step cap; the kinematic body sleeps on its own timer).
  Assert, through `shape->proxyKey`: every dynamic shape names code 3 and the kinematic shape the
  kinematic tree. Wake the pile with `b3Body_SetAwake( true )` on one box and the kinematic box
  with the same call; assert every dynamic shape names the dynamic tree and the kinematic shape
  the kinematic tree, and `b3World_GetAwakeBodyCount` equals the pile's count plus one (the
  kinematic body; the static ground is never in the awake set). Assert
  `b3ValidateBroadPhase` after every step and after the wake. Run the whole sequence twice, the
  second time with the perturbation before every step, and assert equal per-step hashes over all
  bodies (the A0 negative control extended across a sleep and a wake). Fails before at the first
  code-3 assertion.
- `SleepingWriterTest` (design B2, the writers it leaves unchanged). One worker, gravity on, sleep
  on. A box dropped awake onto a static ground and stepped until it reports asleep (its touching
  ground contact is then in the sleeping set), and a second box created asleep apart from
  everything. Create a second box shape on the second body with the default def: assert its key
  names code 3 and the body still reports asleep (shape creation wakes nothing). Call
  `b3Shape_SetFilter` on the second body's shape with the mask bits changed, the category bits
  unchanged and `invokeContacts` true, which destroys and recreates the proxy (`b3ResetProxy`) with
  no touching contact to wake anything: assert the new key names code 3 and the body reports
  asleep. Make the same call on the first body's shape, which destroys its touching contact with a
  wake: assert the shape names the dynamic tree and the body reports awake. `b3ValidateBroadPhase`
  after each. Fails before at the first code-3 assertion, since creation selects the tree by body
  type alone until this commit.
- `WakeUnderRebuildTest` (design §3, third bullet, the begin-touch half). A dynamic box created
  asleep on a static ground; an awake box dropped onto it from above so it lands and begins
  touching within a few steps; a third awake box elsewhere, moved with `b3Body_SetTransform`
  before every step by more than its extent plus margin along one axis, so the new AABB leaves
  the fat AABB and the proxy is marked (`src/body.c` moves the proxy only then) and with it the
  awake tree's root, so the pair update enqueues the rebuild every step; assert before each step that the awake tree's root carries the
  moved bit. World created with `workerCount` 4 (the built-in scheduler) and the same scene at
  `workerCount` 1. Step both until the sleeping box reports awake, then a few more. Assert, after
  every step in both: the sleeping box's shape names code 3 while the body reports asleep and the
  dynamic tree once it reports awake, `b3ValidateBroadPhase` passes, and the per-step hash over
  the three boxes is equal between the two worker counts. The placement's assertion on the
  outstanding tree task (above) is what shows no migration ran under the rebuild; the test runs in
  the validation build. Fails before at the code-3 assertion on the first step.
- `WakeFromCallbackTest` (design §3, third bullet, the callback half), three cases, each on a
  fresh world.
  1. Wake from a query callback. Two dynamic boxes A and B created asleep, apart;
     `b3World_OverlapAABB` with a box covering both. Whichever shape is reported first, the callback
     calls `b3Body_SetAwake( true )` on A, then reads A's shape key through the world and records
     it; the second callback records A's key again; after the query returns, read it once more.
     Assert both keys recorded inside the walk name code 3 (the migration is queued while the walk
     is open, including between callbacks) and the key after the return names the dynamic tree, A
     is awake, and `b3ValidateBroadPhase` passes. The interval from the last callback to the
     outermost exit is covered by inspection of the exit drain. Then step once. A second world runs the same scene with the
     wake made by `b3Body_SetAwake( true )` between calls, no query, then one step; assert equal
     `HashWorldState` after the wake and after the step. Fails before at the inside-callback
     assertion.
  2. Destroy from the callback after the wake. As case 1, but the callback calls `b3DestroyBody`
     on the body after the wake and then creates a new awake dynamic body with one shape. Assert
     the new body's id index equals the destroyed one's (the pool pops the freed slot), and after
     the query returns the new body's shape names the dynamic tree, `b3ValidateBroadPhase` passes,
     and one step passes the end-of-step check.
  3. Replace from the callback after the wake. As case 1, but the callback calls
     `b3Body_SetType( b3_kinematicBody )` after the wake. Assert after the return that the shape
     names the kinematic tree and `b3ValidateBroadPhase` passes.
  Cases 2 and 3 pass before this commit; they are regression checks for the drain's live-body rule.
- `SleepingExplodeTest` (design §3, fifth bullet). Zero gravity, sleep on, one worker, never
  stepped. The `ExplodeOrderTest` scene (one body with three shapes, several single-shape bodies
  inside the radius, one in the falloff) built twice: run 1 with every body created asleep, run 2
  with every body created awake. `b3World_Explode` with the same definition in both. Assert in
  both runs every body awake with a nonzero linear velocity; then assert `memcmp`-equal linear
  and angular velocities per body between the runs, and in run 1 the awake set's `bodyId`
  sequence ascending by each body's lowest shape id (what the base's sort gives). Print a hash over run 1's
  awake-set order and both runs' velocities. The test's entry function and the helpers it calls
  use only the public API and the awake set's body list, so the implementer also runs it at the
  base commit: in a worktree at `c6c6ead`, a `test/test_awake_tree.c` holding only that function
  and those helpers, registered in `test/main.c`, built and run by entry name. That printed hash
  is recorded beside the working tree's in the closure doc; they must be equal,
  which is the design's "same wake set and velocities as `main`". Fails before this commit on no
  assertion; it is the regression check for this commit and B1, and the closure doc says so.

### B4. Non-awake tree maintenance

**Files.** `src/broad_phase.c`, `src/dynamic_tree.h`, `test/test_awake_tree.c`, `test/main.c`
(register `SleepingTreeHeightTest`).

**Commit message.** `Rebuild the sleeping dynamic tree in full when an insert takes its height past a threshold`

**Code.** Design §2 B4 is the specification. Mechanics that follow from it:

- `B3_SLEEPING_TREE_MAX_HEIGHT` 64 in `src/dynamic_tree.h` beside `B3_TREE_STACK_SIZE`. `src/dynamic_tree.c`
  is not changed.
- One function in `src/broad_phase.c` tests the fourth tree's root height against the threshold and,
  when it is exceeded, calls `b3DynamicTree_Rebuild( tree, true )` and increments the
  `b3BroadPhase` rebuild counter. It is called after every insert into the fourth tree in
  `b3BroadPhase_CreateProxy` and `b3BroadPhase_MoveProxy`, skipped while the depth is nonzero;
  by the drain after each insert it makes with the depth at zero; and once more at every outermost
  exit after its drain, whether or not anything was queued.
- `b3UpdateTreesTask` no longer rebuilds the fourth tree and the early return of
  `b3UpdateBroadPhasePairs` no longer tests `b3NeedsRebuild` on it (its root moved bit stays in the
  test). `b3DynamicTree_Refit` is not run on it.

**Test** `SleepingTreeHeightTest` (design §3, fourth bullet), two subtests. The test includes
`dynamic_tree.h` and reads the threshold from `B3_SLEEPING_TREE_MAX_HEIGHT`.

1. Tree level. `b3DynamicTree_Create( 16 )`; create `B3_SLEEPING_TREE_MAX_HEIGHT + 16` proxies
   (a path of n leaves has height n - 1, so the threshold plus two is the least that can exceed
   it) on small disjoint boxes with centers evenly spaced along one axis; then, in order, `b3DynamicTree_MoveProxy`
   each to a box that encloses every box moved before it, so each unrotated insert deepens one
   path. Assert `b3DynamicTree_GetHeight` exceeds the threshold (if it does not, change the
   geometry until it does; do not weaken the check), then `b3DynamicTree_Rebuild( tree, true )`,
   `b3DynamicTree_Validate`, and assert the height is at or below the threshold.
2. World level. One worker, zero gravity, sleep on. The same count of dynamic bodies created
   asleep with box shapes of the nested sizes, placed apart; then, in order, `b3Body_SetTransform`
   each to the nested arrangement so each write moves its proxy with no rotation. After every
   write assert `b3World_GetCounters().sleepingTreeHeight` is at or below the threshold and
   `b3ValidateBroadPhase` passes, and, when the write raised `sleepingTreeRebuildCount`, that every
   body moved so far still has its leaf's moved bit (`b3IsNodeMoved` on the node at
   `tree->proxies[proxyId].node`); assert `sleepingTreeRebuildCount` is nonzero at the end. Then
   place a static box overlapping the first-moved body, created with `invokeContactCreation` false
   so its own proxy is unmarked and the pair can be found only through the sleeping proxies' bits,
   and step once; assert a contact between the static box and the first-moved body exists in the
   disabled set. Assert
   the height bound again after the step. Contacts among the nested sleeping boxes land in the
   disabled set and are not asserted.

Fails before B4 at the world-level height assertion, since nothing rebuilds the tree. The macro
does not exist before B4, so the fail-before build defines it on the compiler command line for
that scratch build only (`-DB3_SLEEPING_TREE_MAX_HEIGHT=64` in `CMAKE_C_FLAGS`), which the closure
doc states.

## 5. Golden hashes in `test/test_determinism.c`

Run the full suite after each commit in both precision configurations (§7). The base carries the
tree-order changes, so the design's §3 requires every benchmark scene hash unchanged, and this
file's state hashes and sleep steps with it. The design's §1 permits two layout effects and no
other: the order of public query callbacks, and which of two shapes hit at one equal fraction a
closest cast returns. The query-spawn scene (`shared/determinism.c`) reads from its queries, per
spawn, the ray's hit flag, point and normal, the overlap count and the shape cast's closest
fraction, and folds into `QUERY_SPAWN_QUERY_HASH` the overlap callback order as well. So a change
to any query-spawn pin is attributed, or it is a defect:

- Log, in a scratch build only, per query on the base and the branch: the overlap callback's shape
  ids in order; every ray hit's shape id, fraction, point and normal from a logging
  `b3World_CastRay`; the closest hit's flag, shape id, fraction, point and normal; the shape cast's
  closest fraction.
- Compare the two logs query by query up to the first query at which a spawn input differs. Before
  it, every logged value is equal except that an overlap's callback order, and the all-hits
  list's order, may each be a permutation of the same set (both are public callback order). At it, if there is one, the two closest hits are different shapes at one equal fraction,
  that fraction is the least positive fraction in both all-hits lists, each side's closest hit
  matches its shape's all-hits entry (point and normal come from the shape's own cast, so the
  tied shapes' points may differ), the two all-hits lists are the same set of (shape id, fraction,
  point, normal), and every other logged value of that query (overlap set, cast fraction) is
  equal. Nothing after it is compared: the worlds
  have diverged.
- No spawn input differs anywhere, at least one overlap permutation was logged, and the query hash
  recomputed from the branch's log by the scene's recipe (overlap ids in callback order, the ray's
  fraction and normal, the cast fraction) equals the new value: `QUERY_SPAWN_QUERY_HASH` alone may
  change. The first differing spawn input is such a tie: `QUERY_SPAWN_QUERY_HASH`,
  `QUERY_SPAWN_HASH`, `QUERY_SPAWN_SLEEP_STEP` and `QUERY_SPAWN_HIT_COUNT` may change. Update the
  permitted pins for that configuration in the B2 commit, name the step, and put the old and new
  values and both logs in the closure doc.
- Anything else, in any scene, is a defect: stop and report.

`test_determinism.c` is the only file this policy adds to the B2 file list.

## 6. Test-to-requirement mapping

| Design clause | Evidence | Fails before? |
|---|---|---|
| B0 fourth tree, key format unchanged, guards at four codes, capacity to the non-awake tree | `SleepingProxyTreeTest` case 1; inspection of the three guards and `b3CreateBroadPhase` | Build failure |
| B0 counters report the fourth tree | `SleepingProxyTreeTest` case 2; inspection of `b3World_GetCounters`, `b3GetWorldByteCount`, capacity tracking | Build failure |
| B0 the walks the design's B0 names (destroy, validation, bounds, snapshot, counters, byte count, capacity) use the proxy-tree count | Inspection against the §2 reader inventory, file by file, listed in the closure doc; `test_recording.c` and the snapshot round trip in the suite | n/a |
| B1 moved sleeping proxy finds static, sleeping and kinematic neighbours; contacts in the same sets | `SleepingTreePairTest` | Yes, `b3ValidateNoMoved` assertion |
| B1 non-awake self pass gathers by marked descent, cross passes gated at the roots, static passes flush with compounds | Inspection of the gathers and the pass descriptors; the §7 node-visit measurement | n/a |
| B1 membership readers include the fourth tree | Inspection of the eight loops, the sensor pass, CCD and Explode; `SleepingExplodeTest` for Explode; `WakeFromCallbackTest` case 1 for the overlap query | Overlap: yes; Explode: regression comparison with the base |
| B2 sleep and wake migrate, kinematic proxies stay, keys rewritten | `AwakeTreeMigrationTest` | Yes, code-3 assertion |
| B2 migration carries box, bits and moved bit; emptied tree reset; `dfsOrdered` cleared after a sleep | Inspection of the placement function; `AwakeTreeMigrationTest`'s perturbed-run hash equality; `SleepingTreeHeightTest` subtest 2's leaf-bit assertions and its contact against an unmarked static box | n/a |
| B2 every other writer of a sleeping proxy keeps working | `SleepingTreePairTest` (`b3Body_SetTransform`), `SleepingTreeHeightTest` subtest 2, `SleepingWriterTest` (creation, `b3ResetProxy`); `b3Body_Disable` and `b3Body_SetType` by inspection of their destroy-and-recreate paths in `src/body.c` | Creation: yes |
| B3 wakes inside the collide and inside a public walk migrate at the drain, not during the walk | `WakeUnderRebuildTest`; `WakeFromCallbackTest` case 1 (two callbacks in one walk); inspection of the outermost-exit drain for the interval after the last callback; the placement's tree-task assertion in the validation build | Yes |
| B3 drain re-derives from the live body (replaced, destroyed, reused slots) | Inspection of the placement function; `WakeFromCallbackTest` cases 2 and 3 | No, regression |
| B3 bullet CCD queries both dynamic trees in one sorted pass; Explode both inside one entry | Inspection; `SleepingExplodeTest`; benchmark scene hashes equal to the base (§7) | Explode: regression comparison with the base |
| §1 results do not depend on which tree a proxy is in | `SleepingExplodeTest` asleep against awake; `AwakeTreeMigrationTest` perturbed-run equality; benchmark scene hashes and `DeterminismTest` goldens (§5) | n/a |
| B4 height trigger, full rebuild, counter, moved bits survive, no step rebuild, no refit | `SleepingTreeHeightTest`; inspection of `b3UpdateTreesTask` and the refit stage | Yes, height assertion |
| B4 measured heights, node visits and rebuild counts; threshold tuned on them | §7 measurements in the closure doc | n/a |
| §3 sixth bullet: every benchmark scene hash unchanged | §7 scene-hash harness, base against branch | n/a |
| §3 checklist | Closure doc, item by item | n/a |

## 7. Verification and measurements

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF
cmake --build build --target test
./build/bin/test                                 # whole suite, per commit
./build/bin/test SleepingTreeHeightTest          # one test, by its entry name

cmake -S . -B build-dp -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF -DBOX3D_DOUBLE_PRECISION=ON
cmake --build build-dp --target test
./build-dp/bin/test                              # whole suite, double precision, per commit

cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DBOX3D_BENCHMARKS=ON -DBOX3D_SAMPLES=OFF -DBOX3D_UNIT_TESTS=OFF
cmake --build build-rel --target benchmark
./build-rel/bin/benchmark -t=8
```

The Debug builds carry `BOX3D_VALIDATE` (on by default at top level), so every `B3_VALIDATE`,
`b3ValidateBroadPhase`, `b3ValidateNoMoved`, `b3ValidateSolverSets` and the end-of-step
proxy-tree check are live in the suite runs. The benchmark is built and run this way on the base
and on the branch, same machine, same options, each from its own directory, since it writes its
csv and `.dat` files into the working directory. The thread cap is explicit because the default is
the host's core count and the only guard against `B3_MAX_WORKERS` is an `assert` a Release build
removes; `-t=` is clamped to the core count, so the machine must report at least eight cores and
each csv must hold rows for thread counts one to eight before comparison.

Measurements the closure doc needs (design §1, B1, B4 and §4):

- Suite benchmark: for every scene, the csv total milliseconds per thread count from one to eight,
  base against branch, as a no-regression check.
- Profile deltas: from the scratch harness below, `b3World_GetProfile` `pairs` and `refit` per
  step on `sleep`, `rain` and `large_world`, base against branch, at one and eight workers,
  summarised as the sum over the run and the maximum step. On `sleep`, additionally the two
  values on the step after each sleep and the step after that, at one worker, where the tree task
  runs inline inside `pairs` (`b3DefaultAddTaskFcn`), with a sleep step marked by at least one
  body's `b3Body_IsAwake` going from true to false over the step, read per body from the live
  bodies of `world->bodies` (the scene wakes one pyramid per step, so the awake count alone can
  hide a sleep), to show the saving starts on the second step (B2).
- Non-awake tree height and rebuilds: a scratch harness, uncommitted, in the scratch directory,
  built against the library with the `shared` library, that creates each named scene through
  `shared/benchmarks.c` with its capacity and step functions, steps `sleep`, `rain`,
  `large_world`, `washer`, `trees100` and `junkyard` for their benchmark lengths at one and eight
  workers, and prints per step `b3World_GetProfile` `pairs` and `refit`, `b3World_GetAwakeBodyCount`
  and `b3World_GetCounters` `sleepingTreeHeight`, `treeHeight` and `sleepingTreeRebuildCount`, and
  a second scene of the
  harness's own: ten thousand single-shape dynamic bodies created asleep in a
  grid, then two hundred steps each of which moves one hundred of them with `b3Body_SetTransform`
  to new grid cells, printing the same three counters per step. Report the maximum and final
  values per scene.
- Pass node visits: a temporary counter per pass, incremented once per node pair tested in
  `b3GatherCrossSeeds`, once per `b3VisitPair` call and once per node visited in
  `b3CollideProxyAndSubtree`, summed per step and logged, on the branch, for the five cross
  passes and the two self passes, over the same scenes; on the base, the same counters over its
  two cross passes and one self pass. Beside them, per step, the gather cost of each self pass:
  pair indices scanned by `b3GatherMovedSiblings` and nodes visited by the non-awake marked
  descent.
  Report the per-scene sums. Removed before commit.
- Threshold: `B3_SLEEPING_TREE_MAX_HEIGHT` starts at 64. A change needs the measured heights and
  rebuild counts beside it in the closure doc; without such a case it stays.
- Scene hashes: a scratch harness, uncommitted, that creates each benchmark scene through
  `shared/benchmarks.c` with its capacity function, steps it for its benchmark length at one
  worker with its step function, and prints `HashWorldState`'s hash per step over the live bodies
  of `world->bodies` in index order through the internal headers, skipping slots whose `setIndex`
  is `B3_NULL_INDEX` (`rain` destroys and recreates bodies, `large_world` creates them during the
  run), with the recipe's transform, linear and angular velocity and awake flag per body, then the
  awake count, base against branch. Every scene equal, every step. One
  unequal step is a stop condition (§9).

## 8. Closure doc

Closure doc, written after the four commits as
`docs/designs/20261014-002-implementation-prompt-awake-tree-split-closure.md`, containing: the
branch and base hashes; one line stating that with the tree-order changes landed the step's
results are unchanged and public query callbacks and tied closest-hit casts can see sleeping
bodies in a different order; suite counts per commit (tests run, passed) in both configurations;
pre-existing failures named with output; compiler warning status and the `clang-format` check;
every deviation from this document and the design with its reason, the CCD one-pass reading of §2
and the snapshot file among them; every out-of-scope observation; the file-by-file reader
inventory checked against §2; the two outputs of each fail-before test; the
`SleepingExplodeTest` hash equality with the base; any golden changed under §5 with old and new
values; the `b3Counters` additions and the snapshot version change; the §7 measurements. It stays
outside the four commits: nothing in them is amended or added for it; the owner commits it to the
design branch separately if at all. The
implementer does not change the status of the design doc or of this document.

## 9. Out of scope and stop conditions

- No change to `benchmark/`, `shared/`, `samples/`, `docs/` other than `docs/designs/`, any
  `src/dynamic_tree.c` walk (the release stack guard is the design's open item, not this branch's),
  or any `src/` file §4 does not list. A needed change there is a report, not a commit.
- Stop and report if: a benchmark scene hash differs between base and branch at any step; a golden
  changes without a §5 cause; the
  tree-level height cannot be driven past the threshold or the full build cannot bring it under;
  `AwakeTreeMigrationTest`'s perturbed run hashes differently; `WakeUnderRebuildTest`'s two
  worker counts hash differently; the `SleepingExplodeTest` hash differs from the base's; a test
  needs a hook in `src/` to observe something; the owner contests the one-pass CCD reading of §2;
  or the design is silent on a decision the code forces.
- Public query order, island independence and stateful callbacks are outside the guarantee (design
  §1) and get no test.

## 10. Provenance

- The public query, draw, sensor and Explode loops change in B1, where the design's B1 names them
  as membership readers; the design's B0 covers the walks its own text names. Moving those loops
  into B0 was considered and not done: B0 would then hand callers leaves from a tree no pass
  searches yet, and the design assigns the readers to the commit whose passes fill that tree.
- The branch forks from `master`, not `main`, and the four commits are the deliverable with no
  PR: the design's §1 hands the branch to this document, the design's §3 tests need the A0
  perturbation helper that exists only on `master`, and the owner decided that an upstream PR, if
  wanted, is a later cherry-pick. Forking from `main` was considered and not done: the tests could
  not run there.
- The golden policy of §5 permits exactly the two layout effects the design's §1 names, proved by
  attribution over the scene's query inputs. Two alternatives were considered and not taken: a
  stop on any state-pin change for the owner to judge, which would block the implementer on a case
  the design settles; and a permission keyed to the pins rather than to the scene's inputs, which
  could accept an unrelated defect.
