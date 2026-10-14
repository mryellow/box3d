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

## 5. Provenance

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
