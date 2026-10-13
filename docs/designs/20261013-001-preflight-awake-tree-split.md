---
title: Preflight - awake and non-awake dynamic trees
status: Canonical
date: 2026-10-13
---

# Preflight: awake and non-awake dynamic trees

A Box3D change that splits the dynamic-body broad-phase tree into a tree of awake proxies and a
tree of sleeping proxies. It is a stand-alone engine change: on a step with no write to or moved
mark on a sleeping body's proxy, the pair gather, the rebuild and the refit become proportional
to the awake population instead of the dynamic population, from the second step after a sleep that
leaves
the awake tree non-empty (B2). The cross
passes reach the same leaves: a moved awake, kinematic or static box finds the sleeping leaves it
overlaps in the non-awake cross passes as in the single tree's passes today; the internal nodes
visited on the way depend on each tree's layout, and the non-awake tree's height is unbounded
(B4), so the PR measures them (section 4). A step with a sleeping-proxy write pays one gather over
the non-awake tree (B1), and the sensor pass still visits every sensor (`sensor.c`).

## 1. What the step does today, and what the split changes

- One tree per body type (`broad_phase.h`) holds every proxy of that type, awake or asleep. Sleep
  (`b3TrySleepIsland`) and wake (`b3WakeSolverSet`) in `solver_set.c` touch no proxy and no tree.
- The pair update gathers moved sibling pairs by walking every pair below `nodeEnd` in the
  dynamic tree (`b3GatherMovedSiblings`, `broad_phase.c`). The step's rebuild of the dynamic tree
  (`b3UpdateTreesTask`) rewrites every node into a fresh array whenever the root is marked moved
  or the tree is not DFS-ordered (`b3DynamicTree_Rebuild`, `dynamic_tree.c`). The refit
  (`b3DynamicTree_Refit`) returns when the root is unmoved and otherwise scans every pair below
  `nodeEnd`, writing only the marked internal AABBs. All three are O(dynamic proxies) while the
  collide and solve are O(awake), apart from the tree queries a bullet (`b3SolveContinuous`,
  `solver.c`) and the sensor pass make, each costing its node visits, which section 4 measures.
- Under the split, sleep removes each shape's proxy from the awake tree and inserts it in the
  non-awake tree, and wake does the reverse. The awake tree is small and churns as today; the
  non-awake tree is large and changes only on sleep, wake, and the API writes in B2. The
  kinematic and static trees are unchanged.

### Behaviour change, stated up front

With the engine changes in section 2 and the tree-order changes of the tree-order preflight
(`20261011-001`) landed, the step's and Explode's results do not depend on which tree a proxy is
in, with that preflight's exceptions: a stateful `preSolve` or custom filter still sees calls in
layout order and can make a layout-dependent decision, and public query callback order and tied
closest-hit casts see layout order (B1), as with the single tree. Without the tree-order changes
the step's and Explode's results do depend on it, in the cases that preflight
names: every bullet sweep and explosion walks the awake tree, whose layout after migrations differs
from the layout the single tree would have had, and reaches sleeping bodies through the non-awake
tree, whose layout differs likewise; a multi-candidate sweep or a multi-shape explosion can then
give a different answer whether its candidates are awake or asleep. Recordings and golden hashes can
stop validating in those cases. Whether the tree-order PR is applied before this change is set by
the Implementation Prompt, which also sets up the branch.

## 2. Changes

```
B0 fourth tree and proxy key ── B1 pair passes and readers ── B2 sleep and wake move proxies ── B3
step ordering ── B4 non-awake maintenance
```

One PR forked from `main`, four commits in order: B0, B1, B2 with B3, B4. B2 and B3 are one
commit, since a migration is unsafe without B3's guard inside the collide and Explode and the
guard has nothing to do without migrations. B4 is what bounds the non-awake tree; until its
commit the step rebuilds that tree on order as it does the dynamic tree: one line in
`b3UpdateTreesTask`, and the early return of `b3UpdateBroadPhasePairs` tests `b3NeedsRebuild`
on it as on the dynamic tree, since the exit drain's sleep inserts land after the task joined;
B4 removes both, so no commit leaves the tree unbounded.

### B0. Fourth tree and proxy key

**Files.** `broad_phase.h`, `broad_phase.c`, `shape.c`, `physics_world.c`, `types.h`.

- `b3BroadPhase.trees` gains a fourth tree. The proxy key stores its tree in two bits and the
  three body types use three codes (`B3_PROXY_TYPE`), so code 3 is the non-awake dynamic tree and
  the key format is unchanged. `b3BroadPhase_CreateProxy` and `DestroyProxy` assert the code is
  below `b3_bodyTypeCount`, and `b3CreateShapeProxy` (`shape.c`) asserts the same of the returned
  key; both check against a four-code proxy range instead, leaving the body-type range at three.
  `b3CreateBroadPhase` statically asserts three body types, and `b3DestroyBroadPhase` and the
  recording-bounds walk (`physics_world.c`) loop over `b3_bodyTypeCount`; every walk over the trees
  uses a proxy-tree count of four.
- `b3CreateBroadPhase` gives `capacity->dynamicShapeCount` to the dynamic tree, and
  `b3DynamicTree_Create` allocates and free-lists `proxies` to that capacity. The hint goes to the
  non-awake tree; the awake tree starts at the 16-slot floor and grows with what is awake.
- The tree code is not a body type. `b3UpdateShapeAABBs` (`shape.c`) selects the fat-AABB margin
  by the proxy type, and `b3ValidateSolverSets` (`physics_world.c`) checks that a static set's
  proxies are static and any other set's are kinematic or dynamic; the margin maps code 3 to the
  dynamic margin. The check that a dynamic proxy's tree matches its body's set is a world-level
  validation run at the end of the step, after B3's exit drain; it is not in
  `b3ValidateSolverSets`, which `b3WakeBody` runs before a queued migration has happened, nor in
  `b3ValidateBroadPhase`, which has no world.
- Readers that address trees by fixed index include the fourth: `b3World_GetCounters`, the step's
  capacity tracking, the memory report, `b3ValidateBroadPhase` and `b3ValidateNoMoved`
  (`physics_world.c`, `broad_phase.c`). `treeHeight` in `b3Counters` is the maximum of the dynamic
  and kinematic heights; a `sleepingTreeHeight` field (`types.h`) reports the non-awake tree's
  root height on its own, and a `sleepingTreeRebuildCount` field reports a counter in
  `b3BroadPhase` that B4's rebuild increments, diagnostic like the other counters.

### B1. Pair passes and membership readers

**File.** `broad_phase.c`, `sensor.c`, `physics_world.c`.

- Pair discovery today seeds a self pass of the dynamic tree and cross passes of it against the
  static and kinematic trees, each gated by the moved bit on either node (`b3TestPair`). A sleeping
  proxy marked moved (`b3Body_SetTransform` on a sleeping body, a joint's collide-connected change,
  a proxy create, which marks every non-static leaf) is found by those passes against every kind of
  neighbour, and a contact between two sleeping bodies is created into the disabled set
  (`contact.c`).
- The split reproduces that: the awake tree runs its self pass and cross passes against the
  non-awake, static and kinematic trees; the non-awake tree runs a self pass when its root is marked
  moved and cross passes against the static and kinematic trees when either root is. The awake
  self pass gathers its moved sibling pairs as today, by the scan to `nodeEnd`
  (`b3GatherMovedSiblings`), which the step's rebuild compacts; the non-awake self pass gathers
  them by descending from the root through marked internal nodes, as `b3DynamicTree_ClearMoved`
  does (`dynamic_tree.c`), O(marked nodes), since a remove never lowers `nodeEnd` and B4 does not
  rebuild on order, so after a large wake that tree's `nodeEnd` stays at its high-water mark
  while its population is small. Every cross
  pass is gated at the roots by `b3TestPair`, so a pass between two unmoved trees costs one test.
  After its passes, the pair update runs `b3DynamicTree_ClearMoved` on the non-awake tree, as on
  the static tree today, so a moved sleeping proxy is searched once; without it the passes would
  repeat every step and `b3ValidateNoMoved` would fail.
  The early return in `b3UpdateBroadPhasePairs`, which today tests the static root's moved bit and
  the kinematic and dynamic trees' rebuild need, also tests the non-awake root's moved bit. The
  static cross pass runs its seeds with the compound expansion and flushes before the non-static
  passes (`b3CrossPairsTask`), because compound shapes are static only; the non-awake-against-static
  pass does the same, so a moved sleeping proxy over a static compound gets its child keys. Waking
  a sleeping body on every API write that moves its proxy would remove the non-awake passes, but
  it would change what `b3Body_SetTransform` does on a sleeping body; the passes stay.
- Membership readers walk the trees by index and include the fourth: sensor overlaps (`sensor.c`),
  the world overlap and cast queries, `b3World_CollideMover` and `b3World_CastMover`, root bounds
  and debug draw (`physics_world.c`). The step's reads
  of them add no order dependence; public query callbacks and tied closest-hit casts see layout
  order, as the tree-order preflight states.

### B2. Sleep and wake move proxies

**Files.** `solver_set.c`, `shape.c`, `broad_phase.c`, `broad_phase.h`. The moved-mark parameter
and shape creation's tree selection land in this commit, after B1's passes search the non-awake
tree.

- `b3TrySleepIsland` destroys each dynamic shape's proxy in the awake tree and creates it in the
  non-awake tree; `b3WakeSolverSet` does the reverse. A kinematic body's proxies stay in the
  kinematic tree through sleep and wake, as today. Both go through `b3BroadPhase_DestroyProxy`
  and `b3BroadPhase_CreateProxy`, which are the only create and destroy points and are where the
  rewind design's proxy-identity entries sit. `b3BroadPhase_CreateProxy` takes the moved mark as a
  parameter (`b3CreateTreeProxyInternal` already does): ordinary callers pass today's rule, every
  non-static create marked, and the migration passes the old leaf's bit. The shape's `proxyKey` is
  rewritten through the shape accessor by the same function that rewrites its `setIndex`.
- After a sleep migration the awake tree's `dfsOrdered` is cleared, so the step's rebuild runs
  and compacts `nodeEnd`: a remove keeps DFS order and marks nothing, and without this the gather
  would keep scanning to the old `nodeEnd` (`b3GatherMovedSiblings`, `b3DynamicTree_Rebuild`). An
  unordered tree with an unmoved root is rebuilt as one retained subtree, copied densely, and no
  leaf's moved bit changes; marking the root instead would mark the surviving proxy itself when
  it is the root leaf. Either tree the migration empties has its node state reset (root empty,
  `nodeEnd` 2, pair free list empty, ordered), since the rebuild returns at once for an empty
  tree; its proxy slots, capacity and free-list order are retained. When the tree stays non-empty,
  the first step after the sleep still gathers to the old `nodeEnd`, because the pair update
  gathers before it enqueues the rebuild; the saving starts on the second step.
- Migration carries each leaf's moved bit in both directions, so sleep and wake change which tree
  a proxy is in and nothing else, as today's wake and sleep touch no tree. Finalize marks proxies
  moved in the same solve whose sleep pass moves the island (`solver.c`), and
  `b3Body_SetTransform` marks a proxy between steps before a `b3Body_SetAwake( false )` can sleep
  it; an unmarked create would drop the bit and the next pair update would miss the new overlaps.
  A clear bit stays clear; a woken island's contacts persisted in its sleeping set and nothing is
  re-found. Every other create in the non-awake tree (a shape created on a sleeping body,
  `b3ResetProxy`) is marked as every non-static create is today (`b3BroadPhase_CreateProxy`), so
  its pairs are found.
- Every other writer of a sleeping body's proxy keeps working unchanged, because it addresses the
  proxy through `shape->proxyKey`: shape create and destroy on a sleeping body (`shape.c`),
  `b3Body_SetTransform` when the new box leaves the fat AABB (`body.c`), `b3Body_Enable`,
  `b3Body_Disable`, `b3Body_SetType` and `b3ResetProxy` (filter, material and geometry setters).
  Shape creation (`b3CreateShapeInternal`, `shape.c`) passes `body->type` as the proxy type today
  and selects the tree from the body's type and set instead, so a shape created on a sleeping
  dynamic body gets code 3. `b3Body_Enable` transfers the body to the awake set
  before creating its proxies (`body.c`) and needs no change.

### B3. Migration timing

**Files.** `broad_phase.c`, `physics_world.c`, `solver.c`.

- Three readers walk a tree while a wake can happen inside the walk. The dynamic and kinematic
  rebuild is a task enqueued by the pair update and joined inside the solve (`solver.c`), after
  `b3Collide` has processed begin-touch events, which wake sleeping sets (`b3LinkContact`,
  `island.c`). `b3World_Explode` wakes bodies inside its traversal (`ExplosionCallback`,
  `physics_world.c`). The public overlap and cast queries and `b3World_Draw` run host callbacks
  during a traversal, while `b3DynamicTree_Query` holds the node array for the whole walk
  (`dynamic_tree.c`); a callback can wake a body through `b3Body_SetAwake`, or through
  `b3Body_SetLinearVelocity` and `b3Body_ApplyForce`, whose `b3WakeBodyWithLock` (`body.c`) sets
  and clears `world->locked` itself. Today a wake touches no tree; under B2 it migrates leaves in
  the tree being walked and can reallocate its node array.
- One rule covers all three, and it does not use `world->locked`, because the API a host calls
  from a query callback (`b3Body_SetAwake`, `b3DestroyBody`, the setters above) checks or toggles
  that lock itself. The broad phase carries a traversal depth (`b3BroadPhase`, `broad_phase.h`),
  where B4's hooks, which take the broad phase, read it. One entry and exit pair wraps every
  public call that walks trees and runs host callbacks (the overlap and cast queries,
  `b3World_CollideMover`, `b3World_CastMover`, `b3World_Draw`, Explode), around all of that call's
  tree walks, not each walk, so a callback's
  wake cannot move a proxy out of a tree the call has yet to visit; the step raises the depth for
  its own duration. The migration choke point of B2 migrates at once when the depth is
  zero and otherwise records the body id. The queue drains at the outermost exit,
  after that exit lowers the depth to zero (B4 tests each insert there),
  and, in the step, at the point where the tree task is joined, whether or not one was enqueued
  (the pair update returns early when nothing moved, and the collide can still begin a touch and
  wake a set); that point precedes the finalize stage that runs CCD, so a body woken in the
  collide is in the awake tree before a bullet looks for it; the
  sleep pass is the last thing the solve does, so its migrations drain at the step's exit, before
  `b3World_Step` returns. The
  drain reads the live body at each recorded id and places every proxy of it in the tree its
  set and type imply, doing nothing for a proxy already there: a proxy replaced meanwhile
  (`b3Body_SetType` wakes, then destroys and recreates its proxies) is found in place, a
  destroyed body's slot has no set (`b3DestroyBody` nulls `setIndex`, `body.c`) and nothing to
  place, and a slot reused by a new body has that body's proxies where its set implies already.
- `b3SolveContinuous` (`solver.c`) queries the static tree and, for bullets, the kinematic and
  dynamic trees by fixed index. The bullet pass queries both dynamic trees; sleeping bodies are
  hit today and stay hittable.
- `b3World_Explode` queries the dynamic tree by fixed index. It queries both dynamic trees inside
  one traversal entry, so its wakes queue and drain when it exits, after both traversals return;
  the impulses are applied with the wakes, over the combined list. This is the collect-then-apply
  half of the tree-order preflight's A3, needed here whether or not A3's sort lands.
- A `b3Body_SetTransform` from a query callback removes and inserts in the tree being walked
  today, unguarded (its lock check is an assertion), and is unchanged by the split.

### B4. Non-awake tree maintenance

**Files.** `broad_phase.c`, `dynamic_tree.c`, `dynamic_tree.h`.

- The non-awake tree is rebuilt by the step on a height trigger, not on order. `b3InsertLeaf`
  clears `dfsOrdered` when the new pair lands out of order and the step rebuilds any unordered
  dynamic or kinematic tree, so a non-awake tree maintained like today's dynamic tree would be
  rebuilt, O(sleeping proxies), on the step after every sleep; a partial rebuild is no cheaper,
  since it copies every retained subtree into the fresh array (`b3CopySubtree`). A tree the step
  never rebuilds has no height bound: inserts rotate on a create (`b3CreateTreeProxyInternal`) but
  not on a move (`b3DynamicTree_MoveProxyInternal`, which `b3Body_SetTransform` on a sleeping body
  reaches), and a remove (`b3RemoveLeaf`) keeps the order and rebalances nothing. So after each
  insert into the non-awake tree, `b3BroadPhase_CreateProxy` and `b3BroadPhase_MoveProxy`
  (`broad_phase.c`) test its root `height` against `B3_SLEEPING_TREE_MAX_HEIGHT`
  (`dynamic_tree.h`, beside `B3_TREE_STACK_SIZE`, initially 64) and, when it is exceeded,
  rebuild the tree in full (`b3DynamicTree_Rebuild` with `fullBuild`) on the writing thread,
  through one rebuild function in `broad_phase.c`; no world-level call is exposed. The inserts
  are a sleep migration's, a move's and a create's on a sleeping body. The test is skipped while
  B3's traversal depth is nonzero, so no rebuild runs under an open walk. The outermost exit
  lowers the depth to zero and then drains, with the tree task joined, so each drain insert is
  tested like any other; the mid-step drain inserts into the awake tree only, since a wake
  removes from the non-awake tree, so its skipped test has nothing to test. The exit runs one
  more test after its drain, whether or not anything was queued, so a `b3Body_SetTransform`
  from a query callback, the unguarded write B3 leaves as today's, has its insert tested. The
  old tree a rebuild walks is then at most one over the threshold on a layout the build bounds,
  plus one per proxy insert made from a callback inside that call (a `b3Body_SetTransform` on a
  sleeping body inserts one proxy per shape), since an unrotated insert raises the height by at
  most one; a public call's walks are single walks and the rebuild's gather pushes one net per
  level, so a call stays within the 512-entry stacks up to about 430 such inserts, and past
  that a release build's walk in that call skips branches, the failure any tree write under an
  open walk risks today, which B3 leaves as it is. Every walk that
  starts with the depth at zero, the step's and the public queries', sees a height at or below
  the threshold when the build can reach it (next paragraph). The test is O(1) on the root; the
  rebuild is O(sleeping proxies). A full build keeps every leaf's moved bit and recomputes the
  ancestors' (`b3MakeInternalNode`), so a moved sleeping proxy's pending search survives it.
- The full build bounds the height only as far as the layout lets it. `b3BuildTree` splits each
  set at the midpoint of its centers' extent on the longest axis (`b3PartitionMid`,
  `B3_TREE_HEURISTIC` 0), which isolates a single leaf when that leaf's gap exceeds the span of
  the rest, so a built height is about log2 of the proxy count plus the length of the longest
  chain of such splits, and a chain of k needs center separations in a ratio near 2^k on one
  axis, or near 2^(k/3) with the axes alternating. A layout the build leaves above the threshold
  is rebuilt again on every insert into the tree, one O(sleeping proxies) rebuild per inserted
  shape, so a sleep of a many-shape island or a `b3Body_SetTransform` on a many-shape body pays
  one per shape, until the layout changes; at ten thousand proxies that needs a chain of
  fifty leaves each placed beyond the span of all before it, separations in a geometric
  progression over a ratio past 2^17, and `sleepingTreeRebuildCount` (B0) shows it. The
  threshold is not a bound on such a layout. For the walks, the stack assert in a validation
  build is the backstop, and a release build skips a branch; the rebuild's own walks
  (`b3DynamicTree_Rebuild`'s gather and `b3BuildTree`) push after an assert with no release
  guard, as the step's rebuild of the dynamic tree and `b3World_RebuildStaticTree` do today, and
  the full build's height bounds them, past 512 only with separations over a ratio near 2^167;
  a guard there is a change to the primitive, outside this design.
- `b3DynamicTree_Refit` is not run on it; nothing in it moves between API calls.
- The depth-first walks and the pair walk use a 512-entry stack (`B3_TREE_STACK_SIZE`), assert
  when it is full in a validation build, and skip a branch in a release build; the cross-seed
  gather (`b3GatherCrossSeeds`) is breadth-first on a 128-entry circular queue and stops at 64
  seeds, which is its own bound. The single-tree walks push
  at most one entry net per level and fill past a height of 500. The cross pair walk
  (`b3CollideCrossPairs`) pushes up to three per level while both sides are internal, so its
  stack is bounded by the shallower tree and fills near a height of 170 on both sides; a leaf on
  either side is handed to `b3CollideProxyAndSubtree` (`b3VisitPair`), which walks the other
  subtree with its own stack at one net entry per level. The awake tree is rebuilt on every step
  its root moved or its order broke, so at the pair walk its height is the last build's, near
  log2 of its proxy count with the builder's caveat above, plus what one step's rotated inserts
  add, a wake's included; that is today's dynamic tree's reliance for a spawn of as many bodies
  between steps, through the same insert. The pair stack holds about three times that height;
  the single walks over the non-awake side hold up to
  its height, 64 of 512 at the threshold, and the pair stack would hold 192 of 512 with both
  trees at 64. The threshold is a trigger, and a bound only on a layout the full build brings
  under it (above). The static tree keeps its reliance on
  rotation-balanced inserts with no step rebuild, and the dynamic-against-static cross pass walks
  it with that stack today; neither changes. The non-awake root `height` is an O(1) field that
  `b3World_GetCounters` reports as `sleepingTreeHeight` (B0). The PR reports it, the node visits
  and `sleepingTreeRebuildCount` (section 4) under sleep and wake churn and under repeated
  `b3Body_SetTransform` on sleeping bodies, and the threshold is tuned on that.

## 3. Tests

Each test runs under the tree-order preflight's A0 perturbation helper where layout is involved.

- Sleep then wake an island with dynamic and kinematic bodies; assert every dynamic shape's
  `proxyKey` names the awake tree after the wake and the non-awake tree while asleep, every
  kinematic shape's the kinematic tree throughout, and that `b3ValidateBroadPhase` passes at each
  step.
- A sleeping island moved by `b3Body_SetTransform` into a static box, another sleeping island, and
  a kinematic body: assert the contacts the single tree creates today are created, in the same
  sets.
- A body woken by a begin-touch event during a step with a pending tree rebuild, and a body woken
  from an overlap-query callback: assert no tree write during the rebuild or the walk (validation
  build), the proxy in the awake tree after the drain, and the same state hash as a serial run.
- The non-awake tree driven past `B3_SLEEPING_TREE_MAX_HEIGHT`: at the tree level, proxies moved
  without rotation (`b3DynamicTree_MoveProxy`) into a nested sequence, each box enclosing the
  last, so each insert deepens one path, until the root height exceeds the threshold; at the world
  level, the same geometry through `b3Body_SetTransform` on sleeping bodies, with the boxes'
  centers evenly spaced so the build can bound the layout. The tree-level path establishes the
  tall input and asserts the full build brings it under the threshold; on the world-level path,
  whose writes go through the hooks, assert the height is at or below the threshold after every
  write, including between steps, that every moved sleeping proxy's moved bit survives the
  rebuild, and that the contacts the single tree creates today are created.
- An explosion over sleeping bodies: with the tree-order changes landed, assert the same wake set
  and velocities as `main`; without them, the same wake set.
- The scene hash of every `benchmark/` scene is unchanged with the tree-order PR landed, and
  unchanged on every scene with no bullet or explosion without it.

## 4. Cost

Measured migration churn: the proxy moves sleep and wake make, from a scratch harness that steps
each benchmark scene with 8 workers, 60 Hz, 4 substeps, for its benchmark length, scans every
dynamic body per step through the public API and counts the bodies whose awake state flipped and
their shapes (kinematic bodies sleep too, but B2 leaves their proxies where they are, and a run
counting them as well gave the same flip counts in every scene). One flipped shape is one remove and
one insert. Scanning at step boundaries sees every transition: a wake resets `sleepTime`
(`solver_set.c`), sleep needs `B3_TIME_TO_SLEEP` of it accumulated one step at a time, and the sleep
pass is the last thing a step does, so no island sleeps and wakes, or wakes and sleeps, inside one
step. `large_world` was not scanned.

| Scene | Steps | Bodies that slept | Bodies that woke | Ticks with a flip | Moves per tick, max | mean | p99 |
|---|---|---|---|---|---|---|---|
| convex_pile | 500 | 0 | 0 | 0 | 0 | 0 | 0 |
| joint_grid | 100 | 0 | 0 | 0 | 0 | 0 | 0 |
| junkyard | 500 | 5 | 1 | 6 | 1 | 0.0 | 1 |
| large_pyramid | 200 | 0 | 0 | 0 | 0 | 0 | 0 |
| many_pyramids | 100 | 0 | 0 | 0 | 0 | 0 | 0 |
| rain | 400 | 1,296 | 0 | 81 | 84 | 3.8 | 28 |
| sleep | 300 | 18,900 | 18,900 | 99 | 2,100 | 126 | 420 |
| spinner | 800 | 0 | 0 | 0 | 0 | 0 | 0 |
| trees100 | 500 | 50 | 0 | 43 | 44 | 2.2 | 44 |
| washer | 1000 | 1,166 | 820 | 500 | 34 | 2.0 | 14 |

The move counts are shapes. Flipped bodies carry one shape each except in `rain` (1.2) and
`trees100` (22). In the `sleep` scene the ten pyramids fall asleep in the same tick, which is the
worst tick, and the step then wakes one sleeping pyramid per step, so they cycle. A move is one
`b3RemoveLeaf` and one `b3InsertLeaf` with rotations, each O(height) of its tree: a remove does not
rebalance and the rotations bound nothing, so the non-awake tree's height under churn and the
cross passes' node visits are measurements the PR reports, with the `pairs` and `refit` profile
deltas on `sleep`, `rain` and `large_world`. What the split saves is the O(dynamic proxies) gather,
rebuild and refit; the cross passes reach the same leaves through a layout that differs.

## 5. What the split gives a bit-exact rewind

Its own note, `20261013-002`: the awake tree is imaged whole, and the non-awake tree is treated
as the rewind design's section 7.4 treats every tree today, or with a byte journal when the
tree-order changes are absent. Nothing in sections 1 to 4 depends on it.

## 6. Open decisions for the owner

None.

## 7. Out of scope

- The tree-order changes themselves (`20261011-001`).
- A history-independent tree (membership determines shape): an SAH-built AABB tree is not
  canonical, and a canonical build is O(n) per change.
- Island-scoped restore (`20261012-001`).

## 8. Provenance

- **Endpoint scans count migrations exactly.** The churn harness compares awake state at
  consecutive step boundaries. A within-step round trip would be missed, but none can occur: a
  wake zeroes the sleep timer, sleep needs half a second of it, and the sleep pass closes the
  step.
- **Migration carries the moved bit both ways.** Marking every woken proxy would make a wake
  search pairs that today are not searched until the body moves, a timing change outside the
  cases section 1 names. Carrying the bit keeps sleep and wake free of pair effects, as today.
- **The kinematic tree is not split.** Kinematic bodies sleep through their islands too, and a
  kinematic tree the step keeps rebuilding has to be imaged in the rewind design's non-tree-order
  branch. Splitting it would make that tree cold, at the cost of a fifth tree, a three-bit tree
  field in the proxy key and two more cross passes. The kinematic tree starts at 16 slots and is
  small in every benchmark scene, so imaging it is cheaper than that machinery.
- **Migrations queue while a traversal is open.** Joining the rebuild before contact-state
  processing and guarding public walks with the world lock were each one reader short, and the
  lock is the wrong instrument: the velocity and force setters' wake path toggles it itself, and
  `b3Body_SetAwake` and `b3DestroyBody` refuse a locked world, which hosts call from callbacks
  today. A traversal depth raised by one entry and exit pair, a queue keyed by body, and a drain
  that re-derives each proxy's tree cover the collide, Explode, every public walk and a proxy
  replaced before the drain, with no list of readers or guards.
- **The non-awake passes run.** Waking a sleeping body on every API write that moves its proxy
  would remove two passes but change what `b3Body_SetTransform` does on a sleeping body. The
  passes cost one root test each on a step where nothing sleeping moved.
- **The non-awake tree is rebuilt on a height trigger, not on order and not never.** Rebuilding
  whenever the order breaks, as the awake tree does, costs O(sleeping proxies) on the step after
  every sleep and, under the rewind note's byte journal, the tree's whole image per sleep. Never
  rebuilding in the step, as the static tree does, leaves the height unbounded under removes,
  unrotated moves and churn, with walks that skip branches silently when a 512-entry stack fills
  (past height 500 in a single-tree walk, near 170 in the cross pair walk with both sides
  deep). An O(1) height test after each insert outside a traversal, and once at every outermost
  exit,
  gives every walk, the step's and a query's between steps, a bounded tree,
  and the full rebuild is paid only when it fires. A trigger relative to the last build's height
  (fire at that height plus a margin, one more scalar of tree state) would cap the cost on a
  layout the build cannot bring under the threshold; such a layout needs a geometric progression
  of separations over a ratio past 2^17 and its cost is reported by the rebuild count, so the
  scalar and the second rule are not added.
