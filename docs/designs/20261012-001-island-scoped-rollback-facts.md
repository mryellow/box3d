---
title: Island-scoped rollback - facts from our rewind and prediction work
status: Draft
date: 2026-10-12
---

# Island-scoped rollback: facts from our work

Written for the author of box3d discussion #152, who proposes saving and restoring the islands that
contain a set of seed bodies. This is a collation of what our own design work found that bears on
that approach. It makes no proposals. Everything comes from our design docs and the source they
cite; none of it has been checked by execution, and our designs are unimplemented.

Sources (our docs, by section): the client-prediction design (scoped, island-based), the
rewind-only history ring (whole-world, bit-exact) and its bit-exact predecessor, the solver
changes summary, and the tree-order preflight.

## 1. What the thread established

- Erin: islands can change every step, so restoring one body can leave a freshly formed contact
  dangling.
- Erin: dynamic-tree order determines collision pair order, and so constraint graph order.
- Erin: Box3D splits islands lazily, only when a body in the island wants to sleep, so exact island
  restore would need splitting blocked until sync points.
- Erin: an awake-only dynamic tree is being considered, to try in Box2D first.
- Erin: with separate awake and non-awake trees, an island that falls asleep and is then rolled back
  to when it was awake must be removed from the non-awake tree, in a way that preserves the
  non-awake tree's origin order. Serializing the whole awake tree preserves its order; islands that
  sleep or wake as part of the rollback are the tricky part. Section 10 covers this.

Sections 2 to 5 give our findings on each.

## 2. State that lives outside the body

Our inventory of what the step rewrites every tick for awake members:

- awake solver set and the graph colour arrays: `colors[0..23]` joint sims, convex and other
  contact arrays
- awake contact records and manifolds
- awake body, shape, joint and island records, plus the island link arrays
- moved-proxy list and sensor overlaps
- world scalars the step reads across ticks, including `splitIslandId` (chosen at the end of tick t,
  consumed in t+1), `compoundShapeCount`, and the enable flags

State touched only by structural events: sleeping, static and disabled sets, id pools, the pair
set, the graph colour bitsets, and hull refcounts.

A contact's colour and its slot in the colour arrays (`colorIndex`, `localIndex`) are written by
graph add and remove, and island membership by island link, unlink, merge and split.

## 3. Islands as the unit

- A body's island is `b3Body.islandId`. An island's bodies, contacts and joints are contiguous link
  arrays, so members can be listed without walking linked lists.
- Enabled kinematic bodies get islands and join an island through the normal touching-contact link.
- Island size varies from 1 to about 9,900 bodies across our benchmark scenes. `many_pyramids` has
  10,780 awake bodies with a largest island of 55. `large_pyramid` is one 5,050-body island, and
  `junkyard` has a 9,726-body island. A seed resting in a pile is coupled to the whole pile.
- Our scoped design recorded the islands containing registered roots at every capture. At restore
  time it used exactly the set that had been recorded, not a set recomputed from current topology.

## 4. Why island-scoped restore is not bit-exact today

Our audit found six places where one island's result depends on state outside that island:

| Dependence | Where | Effect |
|---|---|---|
| Begin and end touch are processed in contact-id order. Contact ids come from a world-global LIFO pool that unrelated contacts allocate from every tick | `physics_world.c`, `constraint_graph.c` | Decides island linking and first-fit colour assignment, and colour changes results |
| The overflow colour is solved serially in an array that other islands' removals swap-compact | `constraint_graph.c`, `solver.c` | Solve order inside an island changes with removals elsewhere |
| One island split per step, chosen by a world-wide maximum | `solver.c` | Split timing changes sleep timing, and a waking body starts from zero velocity (`solver_set.c`) |
| `anyRestitution` is one world-wide flag gating a solver pass (relevant with restitution propagation on) | `contact_solver.c`, `solver.c` | One island's restitution pass depends on the others |
| Waking recolours constraints in stored order | `solver_set.c` | Colour depends on history |
| Merge survivor and link order follow merge order | `island.c` | Island array order depends on merge history |

Our conclusion in both designs: bit-exact scoped restore is not possible with today's solver.
Our first design therefore settled for a consistent, not bit-identical, continuation.

Two related facts:

- Round 2 of the prediction design tried restoring the id pools and rejecting a restore on outside
  churn. In a scene with other moving bodies that rejected almost every restore.
- The thread's request for a colour restored together with every constraint on both bodies matches
  what we found: colour order changes results, and it is set by the global-order channels above.

## 5. Dynamic-tree order

An audit of what the step reads from the broad-phase trees found three places where the result
depends on the order a tree returns candidates:

1. **Continuous collision, solid minimum** (`b3ContinuousQueryCallback`, `solver.c`): the running
   best fraction is passed to the next candidate's time-of-impact query.
2. **Continuous collision, sensor hits**: up to 8 sensor candidates are kept, gated on the running
   solid fraction, so which 8 fill the cap depends on visit order.
3. **`b3World_Explode`** (`physics_world.c`): wake and impulse accumulation follow traversal order,
   so a multi-shape body sums floats in a layout-dependent order.

Everything else the step reads from a tree is already order-independent. Pair discovery is sorted
by shape-pair key (`broad_phase.c`), and end-of-step sensor hits are sorted and deduplicated
(`sensor.c`).

Tree layout differs between two worlds with identical state if they were built or maintained
differently (creation order, incremental rebuilds, `b3World_RebuildStaticTree`). Our designs
rebuild the trees at restore, which is why these three had to be made order-independent.

Fixing them changes results in rare cases (ulp-level differences on sweeps with several competing
candidates, a different set of reported sensor hits above 8 candidates, last-bit differences in
multi-shape explosion impulses). Recordings or golden hashes captured earlier can stop validating
in those cases.

Pair discovery cost: `b3UpdateBroadPhasePairs` gathers moved siblings by walking every sibling pair
in the whole dynamic tree, and the sensor pass scans every sensor. Both are O(world) in a resim
that is otherwise O(scope).

## 6. What our scoped design did about the problems in the thread

These are design decisions, listed because they answer the same questions.

- **Contact topology was never restored.** The live world kept its contacts, islands, graph and
  pools. The next step reconciled them against the restored poses.
- **Restore destroyed and recreated every contact incident to a scope body.** The result is cold
  narrow-phase state. A pair touching before and after reports a spurious end-touch and begin-touch,
  and gets a new contact id and generation.
- **Only convex-versus-convex contacts carried impulses through a restore.** Mesh and height-field
  contacts were recreated cold.
- **Bodies outside the scope were frozen, not simulated.** A scope body's contact with one was a
  boundary constraint, solved against the live body with zero inverse mass and zero velocity. That is
  exact for the frozen side and an approximation for the scope side.
- **Sleeping neighbours** could join the scope only if provably unchanged since at or before the
  restored tick, tracked by a per-body `validSinceTick`.
- **Bodies created in the window** had no record, and stayed in the world as non-scope bodies.
  **Bodies destroyed in the window** were skipped by a generation check.
- **Not restored:** static body transforms, shape geometry, material, filter and sensor settings,
  world settings, joint settings (only solver impulse and limit state), and sensor overlap state.
  The caller replays these.
- Worker count must stay fixed between capture and resim.

## 7. Benchmarks and memory

Measured in our performance review with a throwaway harness on the `benchmark/` scenes: Release
build, 8 workers, 60 Hz, 4 substeps, each scene stepped to half its benchmark length. The state
copy is a single-threaded gather-memcpy of the whole world into a preallocated 10-slot ring (median
of 40). The machine was a 12-thread WSL2 box, so read the absolute times as relative. The resim
column is 9 whole-world steps (about a 150 ms round trip).

| Scene | Bodies | Contacts | Largest island | Step (ms) | 9-tick resim (ms) | State per tick | 10-slot ring | Copy (ms) | Copy / step |
|---|---|---|---|---|---|---|---|---|---|
| trees100 | 51 | 1,100 | 1 | 0.52 | 4.7 | 1.0 MB | 10 MB | 0.11 | 20% |
| trees25 | 51 | 1,100 | 1 | 1.27 | 11.5 | 2.6 MB | 26 MB | 0.30 | 24% |
| spinner | 1,502 | 7,468 | 1,501 | 2.53 | 22.8 | 5.3 MB | 53 MB | 0.64 | 25% |
| sleep | 2,101 | 5,890 | 210 | 1.59 | 14.3 | 5.0 MB | 50 MB | 0.58 | 36% |
| rain | 2,620 | 4,531 | 36 | 3.59 | 32.3 | 5.2 MB | 52 MB | 0.64 | 18% |
| convex_pile | 5,121 | 28,187 | 2,652 | 5.49 | 49.4 | 12.8 MB | 129 MB | 1.76 | 32% |
| large_pyramid | 5,051 | 14,950 | 5,050 | 5.08 | 45.7 | 11.6 MB | 116 MB | 1.75 | 34% |
| washer | 8,002 | 45,022 | 6,066 | 11.8 | 106 | 22.7 MB | 227 MB | 3.25 | 28% |
| joint_grid | 10,000 | 0 | 9,900 | 8.47 | 76.2 | 17.1 MB | 171 MB | 2.59 | 31% |
| many_pyramids | 10,781 | 28,420 | 55 | 11.8 | 106 | 23.1 MB | 231 MB | 3.34 | 28% |
| junkyard | 10,586 | 140,778 | 9,726 | 25.4 | 228 | 50.9 MB | 509 MB | 6.33 | 25% |
| large_world | 1,000,049 (10 awake) | n/a | n/a | n/a | n/a | 650 MB | 6.5 GB (swapped) | not meaningful | n/a |

Notes on the table:

- "State per tick" is a whole-world figure. It includes bodies, all solver sets, contacts with
  manifolds and mesh triangle caches, joints, islands, graph colours, the pair set, id pools,
  shapes and fat AABBs, and all three trees.
- At 1 worker, step times are 3 to 4 times higher and copy / step is 7 to 12 percent.
- `large_world` stays at 360 MB per tick even without sleeping sets and the static tree, because
  the `bodies` and `shapes` arrays are dominated by static entries. The simulated state is a few KB.
- Whole-world capture is about 2 KB per awake body, and contacts and manifolds are more than half
  of it.

Estimates (not measured) from the island-scoped design:

| Quantity | Estimate |
|---|---|
| Per-tick scoped record | about 100 B per body plus about 84 B per touching convex manifold |
| Character-sized scope (tens of bodies) | a few KB per slot |
| Largest measured island (`joint_grid`, 9,900 bodies) | about 1 MB of body records plus joint records |
| Undo-journal bytes per tick (whole-world design), junkyard-scale churn | under 1 MB, about an order of magnitude below its image |

| Scene | Largest island | 9-tick resim, whole world (measured, ms) | 9-tick resim, largest island only, collide and solve (estimated) |
|---|---|---|---|
| many_pyramids | 55 | 106 | under 1 ms |
| rain | 36 | 32 | under 1 ms |
| sleep | 210 | 14 | about 1.5 ms |
| convex_pile | 2,652 | 49 | about 25 ms |
| large_pyramid | 5,050 | 46 | about 46 ms |
| junkyard | 9,726 | 228 | about 210 ms |

The island-only estimates exclude broad-phase pair discovery and the sensor pass, which stay O(world).

## 8. Island membership in the thread's terms

- Our whole-world design kept island records and link arrays inside the per-tick image for awake
  islands, and journaled changes to non-awake ones. Island create and destroy are single functions
  (`b3CreateIsland`, `b3DestroyIsland`).
- The thread's "island that lost a body converges as if warm starting had been cleared for the
  dropped contacts" has no counterpart in our designs. Ours either rebuilt contacts cold (scoped)
  or restored everything (whole-world).

## 9. Testing

- The thread's test (save at frame k, step ahead, restore, step once, hash-compare against the
  original k+1) is the same oracle our whole-world design uses.
- Our preflight adds a helper that runs a scene twice and compares per-step state hashes, with the
  second run rebuilding every world tree before each step (same proxy ids, fat AABBs and state,
  different layout). It needs a check that the layout really changed, otherwise it passes vacuously.

## 10. Split awake and non-awake trees

Status: analysis of an idea raised in the thread, checked against the source but not against a
source change. Byte figures are estimates; the churn table in 10.5 is measured.

What the source does today:

- One tree per body type (`broad_phase.h`), each holding every proxy of that type, awake or
  asleep. Sleep (`b3TrySleepIsland`) and wake (`b3WakeSolverSet`) in `solver_set.c` touch no
  proxy and no tree. Under a split each becomes one tree remove and one insert per shape of the
  island.
- The proxy key carries the tree in two bits and three body types use three codes, so a fourth
  tree fits the key format. The code also selects the fat-AABB margin (`b3UpdateShapeAABBs`,
  `shape.c`) and is checked against the body's solver set (`physics_world.c`), so the fourth code is
  a tree index, not a body type.
- Readers of tree order are the three in section 5. Pair discovery tests moved bits and overlap,
  culls through the pair set and sorts the keys (`broad_phase.c`), so the pairs it creates do not
  depend on layout. Continuous collision evaluates every candidate that is not a sensor, a bullet,
  the same body or filtered out (`b3ContinuousQueryCallback`, `solver.c`), so a bullet reaches
  sleeping bodies, and `b3World_Explode` wakes the bodies it hits. Under a split both query the
  non-awake tree, and its order is simulation-affecting exactly as the dynamic tree's is today.
- The step rebuilds the dynamic and kinematic trees (`b3UpdateTreesTask`, `broad_phase.c`) when
  the root is marked moved or the tree is not DFS-ordered. A rebuild writes every node into a
  fresh array and swaps it in (`b3DynamicTree_Rebuild`, `dynamic_tree.c`). `b3InsertLeaf` clears
  `dfsOrdered` when the new pair lands out of order; `b3RemoveLeaf` keeps the order. The static tree
  is rebuilt only by `b3World_RebuildStaticTree`.

### 10.1 Awake tree: image it

- Copy the tree into the ring slot with the rest of the hot image: `nodes` and `parents` up to
  `nodeEnd`, `proxies` up to `proxyCapacity` (the free list threads through every free slot), and
  the scalars `nodeEnd`, `pairFreeList`, `proxyCount`, `proxyCapacity`, `proxyFreeList` and
  `dfsOrdered`. The rebuild scratch arrays are not state. About 100 B per awake proxy, against the
  2 KB per awake body measured in section 7.
- Restore overwrites. Proxies that slept after T are in image T. Proxies that woke after T are not,
  and the overwrite removes them with their free-list pops, so no per-proxy removal is needed.
- Capacity is state here, unlike the replayed free lists of the rewind-only design's section 7.4:
  the imaged free list covers slots below the imaged capacity only, and `b3Free` takes the block
  size, so restore reallocates the arrays to the imaged capacity.
- The order changes in section 5 are not needed for this tree, and neither is the restore-time
  move pass or the first-step rebuild the rewind-only design pays for it.

### 10.2 Non-awake tree: cold

Which treatment it needs depends on whether the three order changes in section 5 land.

**With the order changes.** No tree's layout is simulation-affecting. The non-awake tree needs
only the proxy-identity journal the rewind-only design already has (its section 7.4): undoing a
wake's removal is an ordinary insert, the layout differs from T's, and nothing reads the
difference. The requirement in the thread, remove the island in a way that preserves the
non-awake tree's order, does not arise. The awake-tree image is then a cost saving, not a
correctness need.

**Without the order changes.** Every cold tree is read in order by continuous collision (static
always, kinematic and non-awake dynamic for bullets) and by `b3World_Explode` (non-awake
dynamic), so each must come back with its exact layout. Undoing a remove with an ordinary insert
does not do that: `b3FindBestSibling` and `b3RotateNodes` make the result depend on the path
taken, and `b3RemoveLeaf` does not undo a rotation. What does:

- Journal physically. One write accessor in `dynamic_tree.c` is the only way to write `nodes`,
  `parents`, `proxies` and the scalars, and it appends `(tree, array, index, old bytes)` before
  writing. Undo replays in reverse and writes the bytes back, so it never re-runs a heuristic and
  the tree returns with the same indices, topology and free lists. A proxy move touches the sibling,
  the new pair and every ancestor: roughly the tree height in 36 B node-plus-parent writes, 0.5 to
  1 KB for a tree of ten thousand proxies. A tick with no non-awake write costs nothing.
- The tree must never be rebuilt by the step. A rebuild is an O(tree) write, and a physical
  journal of it is the image this treatment exists to avoid. Today an insert leaves the tree
  unordered and the step rebuilds any unordered dynamic or kinematic tree, so a non-awake tree
  maintained like today's dynamic tree would be rebuilt on the step after every sleep. It has to be
  maintained like the static tree: incremental inserts with rotations, and an explicit rebuild
  only, journaled as one entry carrying the whole previous array and paid by the caller that asked.
  The kinematic tree is rebuilt by the step whenever it moved, so it is imaged (it is usually
  small) or split the same way.
- A cheaper variant logs operations rather than bytes: for a remove, the leaf index, fat AABB,
  category bits, moved bit, whether the pair was freed or the node end moved, and the old
  `dfsOrdered`; for an insert, the same plus each `b3SwapNodes` the rotation walk made, which is
  its own inverse. Ancestor AABBs are recomputed as unions, which reproduces the originals only
  while every internal AABB of the tree is the union of its children. That holds for a tree written
  only by insert, remove and move; `b3DynamicTree_EnlargeProxy` leaves ancestors loose and is
  called on the awake dynamic tree only. About 40 B per proxy move plus 12 B per rotation.
  Measure before choosing it.

### 10.3 Sleep and wake are not the only writers

The non-awake tree is also written by shape create and destroy on a sleeping body (`shape.c`),
`b3Body_SetTransform` on a sleeping body when the new box leaves the fat AABB (`body.c`),
`b3Body_Enable`, `b3Body_Disable`, `b3Body_SetType` and `b3ResetProxy` (filter, material and
geometry setters), which destroy and recreate or move the proxy, and a joint's collide-connected
change, which sets moved bits on one body's proxies (`joint.c`). All go through
`b3BroadPhase_CreateProxy`, `DestroyProxy`, `MoveProxy` and `MarkProxyMovedSerial`
(`broad_phase.c`), the points the rewind-only design's section 7.4 already journals for proxy ids.
Under the physical journal those points log node writes too.

### 10.4 What changes in the rewind-only design

- Its sections 5.4 and 7.4 shrink for the awake tree: layout imaged, no restore-time move pass, no
  first-step rebuild, and proxy create and destroy entries are not made for it. Phase 2 (derived
  trees) shrinks.
- The image grows by the awake tree, 10.1.
- Without the order changes, the journal gains the node-level entry kind of 10.2, and its section
  7.2 size estimate is remeasured with the churn in 10.5 and `SetTransform` on sleeping bodies.
- Proxy identity gets harder. A proxy changes tree and id on every sleep and wake, so
  `shape->proxyKey` and both trees' free lists must come back exactly. Free-list order feeds later
  id assignment.
- Its section 7.1 argument, "Leaving the awake set needs no entry of its own", no longer holds as
  written: sleep writes a cold structure, the non-awake tree, and that write is journaled.
- Its test 8 (sleep and wake) checks both trees, and the state hash covers both trees and their
  moved sets.
- Unchanged: ring, capture interval, discard rule and the rest of the hot and cold inventory.

### 10.5 Churn and open questions

Measured proxy moves a split would make. Each benchmark scene stepped with 8 workers, 60 Hz, 4
substeps, for its benchmark length; each step scans every non-static body through the public API
and counts the bodies whose awake state flipped since the previous step, and their shapes. One
flipped shape is one remove from one tree and one insert into the other. `large_world` is not in
the table: scanning its million bodies per step was not done.

| Scene | Steps | Bodies that slept | Bodies that woke | Ticks with a flip | Proxy moves per tick, max | mean | p99 | Worst tick at 0.5 to 1 KB per move |
|---|---|---|---|---|---|---|---|---|
| convex_pile | 500 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| joint_grid | 100 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| junkyard | 500 | 5 | 1 | 6 | 1 | 0.0 | 1 | 1 KB |
| large_pyramid | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| many_pyramids | 100 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| rain | 400 | 1,296 | 0 | 81 | 84 | 3.8 | 28 | 42 to 84 KB |
| sleep | 300 | 18,900 | 18,900 | 99 | 2,100 | 126 | 420 | 1 to 2 MB |
| spinner | 800 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| trees100 | 500 | 50 | 0 | 43 | 44 | 2.2 | 44 | 22 to 44 KB |
| washer | 1000 | 1,166 | 820 | 500 | 34 | 2.0 | 14 | 17 to 34 KB |

Shapes equal bodies in every scene but `trees100` (22 shapes per body), so a flipped body is one
move. In the `sleep` scene the ten pyramids fall asleep in the same tick, which is the worst tick,
and the step then wakes one sleeping pyramid per step, so they cycle; its mean is the figure for a
scene built to churn. The piles and pyramids never sleep in their benchmark length. The record-journal
estimate of under 1 MB per tick in section 7 is for contact and island churn in `junkyard`, which
has no sleep churn, and is an estimate, not a measurement.

Open:

- Does the split apply to the dynamic tree only? Kinematic bodies sleep through their islands like
  dynamic ones, and 10.2 needs the kinematic tree imaged or split without the order changes.
- A history-independent structure (membership determines shape) would avoid the journal, but an
  SAH-built AABB tree is not canonical. A canonical build is O(n) per change.
