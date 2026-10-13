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

The analysis of the thread's split-tree idea is its own preflight, `20261013-001`. In short: with
the tree-order changes of `20261011-001` landed, no tree's layout affects the simulation, so a
rolled-back island is removed from the non-awake tree by an ordinary remove and only proxy
identity has to come back; without them, the non-awake tree needs an exact-layout journal, which is
bounded only if the step never rebuilds that tree. The awake tree is imaged whole either way.
