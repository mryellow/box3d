---
title: Sleeping tree without marks - b3Body_SetTransform wakes
status: Implemented
date: 2026-10-15
supersedes-in-part: 20261013-001
---

# Sleeping tree without marks: `b3Body_SetTransform` wakes

Reverses the owner decision of `20261013-001-preflight-awake-tree-split.md` revision 12 (round 11,
U-S5): the non-awake tree ran a self pass and two cross passes for its own moved proxies so that
`b3Body_SetTransform` on a sleeping body could leave it asleep. Now `b3Body_SetTransform` wakes a
sleeping body, the sleeping tree never holds a marked leaf, and the step runs no self pass over it.
The breaking change is recorded for users in `MIGRATION.md`.

## 1. Rule

A dynamic body's proxy is in the awake tree while the body is awake and in the sleeping tree while
it sleeps, with one exception: **a marked leaf of a sleeping body waits in the awake tree** until
the next pair update has searched it. The sleeping tree is never searched for its own marks, so it
never holds one. The step's cross passes of the sleeping tree against the static and kinematic
trees, and of the awake tree against it, stay: they find a moved static, kinematic or awake proxy
over a sleeping one, gated by the moved side, and cost one root test when nothing moved.

`b3PlaceBodyProxies` (`broad_phase.c`) derives each proxy's tree from the body's type, set and the
leaf's moved bit. A sleeping body with a marked leaf is placed in the awake tree, flagged
`b3_proxyParked` (`body.h`, body only) and kept in the migration queue; the queue now persists
across the step boundary. The step's pair update searches the awake tree as it does for any moved
proxy, the rebuild clears the marks, and the mid-step drain after the tree join (`solver.c`) places
the leaf in the sleeping tree, unmarked. The contacts are created in the same step and the same
sets as the sleeping passes created them, one step after the write, as before.

The drain tests the sleeping tree's height once after its placements, whether or not any ran, so
the drain's own inserts, made under the step's traversal, and an insert made from a query callback
are bounded as the per-insert test bounds the others. The exit's separate test is folded into it.

## 2. Writers

| Write on a sleeping dynamic body | Before | Now |
|---|---|---|
| `b3Body_SetTransform` | proxy moved in the sleeping tree, marked; found by the sleeping passes | body woken (kinematic too); proxy moved in the awake tree |
| `b3Body_SetAwake( false )` after a write that marked the leaf | leaf carried into the sleeping tree, marked; found by the sleeping passes | leaf parked in the awake tree; placed asleep by the next step |
| sleep pass after an enlarge in the same step | same | same as above |
| shape created on a sleeping body (`b3CreateShape`, a body created with `isAwake = false`) | created in the sleeping tree, marked | created in the awake tree, marked, body queued once the shape is in its list |
| `b3ResetProxy` (filter, material, geometry setters) | recreated or moved in the sleeping tree, marked | recreated in the awake tree, or moved then migrated there; body queued |
| `b3Joint_SetCollideConnected( true )` | leaves marked in the sleeping tree | leaves marked, then migrated to the awake tree; body queued |

Nothing else changes for the host: every write but `b3Body_SetTransform` leaves the body asleep
and its new pairs are found by the next step. A parked leaf is visible to every tree reader as
before, since the public queries, the sensor pass and the explosion walk every tree.

A parked leaf returning to the sleeping tree is inserted without rotation, as the single tree
moved a sleeping proxy; every other insert rotates as before, so a sleep builds the tree it built
before. A rotation walks the whole path of a large, cold tree and cost several times the insert in
the grid scene below; the height test bounds the tree without it.

## 3. Validation and tests

- `b3ValidateProxyTrees` (`physics_world.c`) accepts a sleeping body's proxy in the awake tree
  only when its leaf is marked. `b3ValidateNoMoved` still checks every tree after the join, so a
  parked leaf must have been searched and cleared by then.
- `SleepingTreePairTest` (`test_awake_tree.c`): bodies created asleep are parked and settle after
  one step; `b3Body_SetTransform` wakes and the woken boxes find their pairs in the awake set;
  the same moves followed by `b3Body_SetAwake( false )` park the leaves and the step creates the
  contacts the sleeping passes created, in the same sets; a kinematic box moved onto a sleeping one
  and a static box created over one are found from the moved side.
- `SleepingWriterTest`: shape creation and both filter paths on a sleeping body park the leaf and
  settle after a step, with the body asleep throughout.
- `SleepingTreeHeightTest` world case: eighty nested boxes moved to the origin and put back to
  sleep one per step; the drain's inserts drive the sleeping tree past the threshold and the
  rebuild fires; every contact with an unmarked static box is found through the parked leaf.
- The callback and explode tests settle their sleeping bodies with one step where they witness
  the sleeping tree.

## 4. Cost

Measured on this machine with `benchmark -t=8 -r=4 -b=sleeping_grid`, three rounds alternating
the base (`master` at `4b0df2a`) and this change, best of four per thread count, milliseconds for
the five-hundred-step run. The scene now puts each lifted box back to sleep at once, so the
workload is the base's: four sleeping boxes moved per step and left asleep.

| Threads | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| base | 17.6 | 32.1 | 57.7 | 77.1 | 105.3 | 112.1 | 116.6 | 123.5 |
| this change | 19.1 | 39.3 | 67.7 | 93.6 | 118.2 | 125.3 | 129.0 | 137.3 |
| change | +9% | +22% | +17% | +21% | +12% | +12% | +11% | +11% |

The `sleep`, `rain` and `large_world` scenes, which sleep and wake islands but move no sleeping
body by the API, are unchanged within run-to-run noise at one and eight workers (two rounds of two
repeats each side).

Where the time goes, from the step profile at one worker: the base moved each lifted proxy inside
the sleeping tree, one unrotated insert; this change wakes the box, moves the proxy to the awake
tree, puts the box back to sleep, and the next step's drain moves the proxy back. The extra is the
wake and sleep of the box's solver set outside the step and the second remove and insert inside
it. At eight workers the parked leaves mark their paths in the awake tree, so the pair update has
more sibling pairs and cross seeds, each a task of its own. The first step of a world created
asleep pays once for moving every parked leaf to the sleeping tree: about 15 ms for the ten
thousand boxes of this scene, where the base paid nothing in the step.

The sleeping self pass this removes cost the base one root test per step when no sleeping proxy
had moved, and a gather over the marked nodes otherwise. The win the preflight's alternative
expected, two passes removed, does not appear in the step time: the passes stay as cross passes,
gated by the other side, and the per-step saving is the self pass's root test.

## 5. Migration cost: no rotation, bulk inserts

Measured after the change above, the `sleep` scene (ten pyramids of 210 bodies that sleep and are
woken one per step) spent 42 ms of a 340 ms eight-worker run moving 37,800 proxies between the
trees, 34 ms of it in inserts. The inserts within one tree cannot be spread across threads, since
each one mutates the structure the next one descends, so the serial work is made smaller instead:

- **An insert into the awake tree never rotates.** A migration's remove leaves that tree
  unordered, so the next step rebuilds it in full from its leaves on the tree task and any rotation
  made by the insert is discarded. A sleep's insert into the sleeping tree still rotates, since
  nothing rebuilds that tree on a schedule. Measured 1.5% to 3.4% on `sleep` across the thread
  counts; inserts fell from 34 ms to 26 ms.
- **A drain that inserts sixteen or more proxies into one tree builds them as one subtree.**
  `b3DynamicTree_CreateProxies` (`dynamic_tree.c`) builds the batch with the rebuild's partition
  into the live node array and joins the subtree beside the old root by one new node, O(n log n)
  in the batch and independent of the tree, with no sibling search per leaf. Sleep and wake open a
  migration batch around their body loops (`b3BroadPhase_BeginMigrationBatch`, `solver_set.c`), so
  a whole set is placed by one drain; the drain gathers every move, removes every leaf, then
  inserts per destination tree (`b3PlaceBodies`, `broad_phase.c`). Each join raises the root by
  one. The sleeping tree's height trigger bounds that as it bounds every other insert; the awake
  tree's step rebuild keeps unmarked subtrees whole, so joins made while nothing below them moves
  would stack up, and a full build at the same threshold bounds it there, keeping every mark.
  `BulkMigrationTest` drives both trees past the threshold and checks the parked leaves of a
  forty-box island return through the bulk path with their marks consumed.

Measured against master (`4d46738`, which has neither change), four rounds of
`benchmark -t=8 -r=4 -b=sleep` with the side order alternated each round, best of four per thread
count, ms per 300-step run:

| Threads | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| master | 941 | 566 | 435 | 374 | 340 | 329 | 331 | 334 |
| both changes | 943 | 542 | 408 | 343 | 308 | 295 | 298 | 306 |
| change | +0.2% | -4.3% | -6.2% | -8.3% | -9.6% | -10.3% | -9.9% | -8.4% |

The migration total on `sleep` at eight workers fell from 42 ms to 11 ms, every one of the 37,800
proxies going through the bulk path. `sleeping_grid`, whose lifts move four proxies per step and
wake little, and `rain`, whose islands are small, are unchanged within the run-to-run noise. This
recovers most of what the tree split gave up to `main` on `sleep` at high thread counts (§4 of
`20261014-003-benchmark-main-vs-master-linux.md` has the earlier comparison).

### Pair passes inline when the work is small

At eight workers on `sleeping_grid` the pair update did 1.2 ms of traversal over a run and cost
42 ms, against 25 ms on `main`: every cross seed is a task of its own, and the split gathers five
passes and the parked leaves double the seeds, so a step whose pair work is a few microseconds
woke worker threads for a handful of tasks. `b3UpdateBroadPhasePairs` now runs the cross passes
and the self pass on the calling thread when the seed count plus the moved sibling pair count is
at or below `B3_INLINE_PAIR_WORK` (64). The pair keys are sorted before contacts are created, so
the contacts are the same either way.

Measured against `main` (`8128d74` with the scene cherry-picked), three rotated rounds of
`benchmark -t=8 -r=4`, best of four, ms per run:

| Threads | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| main | 57.5 | 75.3 | 91.0 | 96.0 | 105.9 | 111.9 | 117.0 | 122.0 |
| branch | 19.0 | 39.8 | 51.4 | 64.4 | 83.9 | 89.9 | 96.0 | 100.7 |
| change | -67% | -47% | -44% | -33% | -21% | -20% | -18% | -17% |

Every round agreed within a point. `sleep` and `rain` have pair work above the threshold on most
steps and are unchanged within the drift. A threshold of 16 leaves this scene's forty-odd units
parallel and loses the gain; 256 measures the same as 64 on both scenes, so the lower value stays.

## 6. Provenance

- **Why the parked leaf and not a wake on every write.** Waking on shape creation would defeat
  `isAwake = false`; waking on a filter change on a body no contact touches is a behaviour change
  no host asked for. The parked leaf keeps every write but `b3Body_SetTransform` as it was.
- **Why not leave marks in the sleeping tree and search them at wake.** A marked sleeping leaf
  that is never searched misses an overlap with a resting awake body until one of them moves,
  and the cross passes' two-sided gate would descend to it every step until then.
- **Why not defer the sleep instead.** A body whose proxy is enlarged every step, as one with a
  zero margin is, would never sleep; and `b3Body_SetAwake( false )` must sleep the body at once.
- **Why the parked return insert does not rotate.** Measured: the rotating insert into the
  ten-thousand-leaf sleeping tree cost about 8 µs each, four per step in the grid scene, against
  about 1 µs unrotated; the base's move did not rotate either.
- **Why the bulk path joins beside the root rather than rebuilding.** A full rebuild of the
  destination is O(n log n) in the whole tree, ten thousand leaves for the sleeping tree, where
  the batch is two hundred; the partial rebuild copies every retained subtree and is no cheaper.
  The join is O(1) after the batch's own build, and the height triggers pay for a full build only
  when the joins have stacked up.
- **Why not parallel inserts.** Each insert reads the node array, the parent array and the free
  lists the previous insert changed; a lock per insert serialises them again, and a tree built
  for concurrent inserts is another data structure.
