---
title: Implementation Prompt - sorted-candidate tree-order-independent solver changes
status: Canonical
date: 2026-10-12
implements: 20261012-001
---

# Implementation Prompt: sorted-candidate tree-order-independent solver changes

This is the prompt handed to the implementing agent. It carries the test mechanism, fixtures and the
test-to-requirement mapping that the design does not. The design decides what changes and why; this
document decides how it lands and how it is shown to work.

## 1. Task

Before any code: an issue labelled `ready-for-pr` exists for this change (`CONTRIBUTING.md`; the PR
check needs a linked, approved issue, so a Discussion alone is not enough). The owner opens it and
supplies its number; do not start until it exists.

Implement `docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md`
(Canonical) in this repository, on the branch `solver-tree-order-independent`, as the three
commits its §2 names (A0, A1, A2), in that order, each bisectable. The design is binding: where this
document and the design disagree, the design wins and the disagreement is reported. Where the code
needs a behaviour the design does not state, stop and report; do not invent it.

Read, in this order, before writing code: the design (all of it), this document, `CONTRIBUTING.md`,
and the source the design names: `src/solver.c` (`b3ContinuousQueryCallback`, `b3SolveContinuous`),
`src/physics_world.h` (`b3TaskContext`), `src/physics_world.c` (worker-context creation and
destruction, `b3World_DumpMemoryStats`, `ExplosionCallback`, `b3World_Explode`), `src/qsort.h` and its
use in `src/broad_phase.c`, `src/dynamic_tree.c` (`b3DynamicTree_Query`, `b3DynamicTree_Rebuild`),
`test/main.c`, `test/test_macros.h`, `test/test_broad_phase.c`, `test/test_determinism.c`,
`shared/determinism.c`.

## 2. Reconciliation against the current tree

Checked on the branch at the time of writing. Each item is a fact the increments rely on.

- `B2_MAX_CONTINUOUS_SENSOR_HITS` is the macro's real name in `src/solver.c` (value 8). Do not rename
  it. It is defined in that file only, not in a header, so a test uses its own constant of 8.
- `b3ShapeId` carries the world index (`include/box3d/id.h`). Shape ids from two worlds compare by
  `index1` only.
- `b3DefaultExplosionDef` zero-initialises every field but `maskBits` (`src/joint.c`), so
  `impulsePerArea` is zero unless set. `ExplosionCallback` scales the impulse by the falloff only for
  a shape farther than `radius` (`src/physics_world.c`).
- A world gets the built-in scheduler only from a `b3WorldDef.workerCount` above one at creation;
  `b3World_SetWorkerCount` recreates the worker contexts but adds no scheduler (`src/physics_world.c`).
  Finalize runs over awake bodies in blocks of at least 16 and bullet CCD, over the bullets finalize
  flagged fast in that step, in blocks of at least 8 (`src/solver.c`, `src/parallel_for.c`); each
  stage runs on several workers only above its own count.
- Finalize marks a body fast, and runs CCD for it, only when its step motion exceeds its safety
  factor times its minimum extent; the flag is `b3_isFast` on the body sim, cleared and re-set by each
  step's finalize, with no public accessor (`src/solver.c`, `src/body.h`, `include/box3d/box3d.h`). A
  test reads it after a step through `b3GetBodyFullId` and `b3GetBodySim`.
- The `b3TaskContext` iteration counters are only ever raised, never reset, and `b3World_GetCounters`
  reports their maximum (`src/solver.c`, `src/physics_world.c`): a high-water mark since the worker
  contexts were last created (`b3World_SetWorkerCount` recreates them), not a per-step value.
- `b3TaskContext` (`src/physics_world.h`) holds `sensorHits` and `pairKeys` as `b3Array` fields. Both
  are reserved in world creation, destroyed in world destruction, and summed in
  `b3World_DumpMemoryStats` (`src/physics_world.c`). The new candidate array follows all three sites.
- `src/qsort.h` is a macro sort: `QSORT( count, LESS, SWAP )` with caller-defined `LESS` and `SWAP`
  macros, undefined after use. `src/broad_phase.c` sorts `pairKeys` this way; copy that shape.
- `b3DynamicTree_Query` and `b3DynamicTree_Rebuild` are public (`include/box3d/collision.h`). The
  world's trees are `world->broadPhase.trees + b3_staticBody` (and `b3_kinematicBody`,
  `b3_dynamicBody`), reachable from a test through `physics_world.h`, `broad_phase.h` and
  `b3GetWorldFromId`. `test/test_broad_phase.c` already includes these internal headers.
- `b3DynamicTree_Query` is a depth-first traversal over sibling pairs with a LIFO stack
  (`src/dynamic_tree.c`). A non-overlapping subtree is pruned, never reordered, so the order in which
  any two leaves are handed to the callback is fixed by the tree layout alone, whatever the query box.
  A witness query with a box that covers a set of candidates therefore reports exactly the relative
  order the CCD or Explode query sees for those candidates, and nothing about shapes outside that
  set. A witness is exact only when the scene puts every shape the covering box returns inside the
  query under test. This is what makes a test-side witness possible without touching `src/`.
- `b3World_Explode` builds its query box as a cube of half-extent `radius + falloff` offset to the
  explosion position. The Explode witness reproduces that box.
- `b3SolveContinuous` queries the union of the fast shape's start and end boxes, where the end box is
  at the integrated transform before any impact (`src/solver.c`). A static shape placed inside that
  union is therefore a candidate of the query whether or not the sweep is cut short by a hit. A
  witness corridor is a set of static shapes placed inside one fast body's single-step swept AABB with
  nothing else inside it; its candidate set is known by construction and the witness is exact.
- The step rebuilds the dynamic and kinematic trees before CCD (`b3UpdateBroadPhasePairs` enqueues
  it, `b3Solve` waits for it before finalize), and after CCD and the bullet stage only
  `b3DynamicTree_EnlargeProxy` touches them, enlarging bounds and marking moved nodes without
  restructuring (`src/physics_world.c`, `src/solver.c`, `src/broad_phase.c`, `src/dynamic_tree.c`).
  With sleep off, the structure of those two trees after a step is the structure CCD queried in it;
  the structure before the step is not. The static tree is not rebuilt by the step. A witness on the
  kinematic or dynamic tree is therefore recorded after the step, with the fast body's own shape
  removed from the sequence; a witness on the static tree may be recorded before it.
- A kinematic or dynamic proxy enters its tree marked moved, a static one only when pair creation
  is forced (`src/broad_phase.c`), and an internal node carries the mark of either child, on
  insertion and on build; the step's rebuild descends marked nodes and keeps unmarked subtrees
  whole, then clears the marks; a full rebuild descends everything and keeps the leaf marks
  (`src/dynamic_tree.c`). On a tree whose leaves were all inserted since the last step, both runs
  gather every leaf and build from the same set, so the layouts at the query are not controlled by
  placement. The two runs reach a kinematic or dynamic tree query in different layouts when the tree
  holds, at the perturbation, unmarked subtrees settled by an earlier step and proxies inserted since:
  run 1's step keeps the settled subtrees whole, run 2's perturbation dissolved them. A world never
  stepped keeps its insertion layout.
- The custom filter is called in CCD after the same-shape, same-body, sensor, filter and bullet
  checks, in pair discovery, and in sensor overlap (`src/solver.c`, `src/broad_phase.c`,
  `src/sensor.c`). Pair discovery overlaps tree leaf bounds, read with `b3DynamicTree_GetAABB`
  through `shape->proxyKey`: a proxy enters its tree with the shape's fat AABB (`src/shape.c`), the
  entry in `world->fatAABBs`, and a bullet's leaf is re-bounded only when it no longer contains that
  entry (`src/solver.c`), so a leaf can exceed it; `b3Shape_GetAABB` returns the shape's own box,
  which carries the speculative margin and not the fat margin. In a scene without sensors, a shape
  with `enableCustomFiltering` whose leaf bound overlaps no other shape's leaf bound at the start of
  a step receives custom-filter calls in that step from CCD only. `preSolve` is called in CCD for a solid candidate with the flag on either shape whose
  fraction lies strictly between zero and the running fraction, with no null check on the registered
  callback (`src/solver.c`): a world with the flag on a shape and no `b3World_SetPreSolveCallback`
  crashes there. Pair discovery checks the custom filter for null (`src/broad_phase.c`).
- CCD sensor hits flow from `b3ContinuousContext` to `b3TaskContext::sensorHits`, then into
  `b3Sensor::hits` (`src/solver.c`), then into the sensor's overlap set (`src/sensor.c`), and surface
  as begin events from `b3World_GetSensorEvents`. The sensor test reads them there.
- `b3WakeBody` wakes the body's whole solver set, appending its bodies to the awake set in wake order
  (`src/body.c`, `src/solver_set.c`). The awake-set order is readable from a test as the `bodyId`
  sequence of `world->solverSets.data[b3_awakeSet].bodySims`.
- `shared/determinism.c` hashes world state with `b3Hash` over `b3Body_GetTransform`, seeded with
  `B3_HASH_INIT`. The helper's hash uses the same primitive.
- `test/test_determinism.c` pins golden hashes (`RAGDOLL_HASH`, `WAVE_PILE_HASH`, `QUERY_SPAWN_HASH`,
  `MESH_DROP_HASH`, and their sleep steps), one set per precision configuration: the
  `BOX3D_DOUBLE_PRECISION` option (`CMakeLists.txt`) is off by default and selects the second set.
  The design's §1 says such goldens may stop validating. Policy in §5 below.
- No scene in `shared/benchmarks.c` creates bullets. `b3Profile` has `transforms` (where fast-body
  CCD runs inside finalize) and `bullets` stages. The `benchmark` executable prints per-run
  milliseconds and writes a `threads,ms` csv per scene, named for the scene, into its working
  directory; its `-s` per-step files hold minima accumulated over every run, thread count and scene
  so far, so they are not per-scene values, and it never writes the `bullets` stage
  (`benchmark/main.c`). The report in §7 is built from the csv totals plus a scratch harness;
  `benchmark/` and `shared/` are not changed by this PR.
- The test runner (`test/main.c`) registers entry functions through an `extern` line and
  `MAYBE_RUN_TEST`, filters by entry name only, and `RUN_SUBTEST` returns at the first failing
  subtest (`test/test_macros.h`). The new file therefore exposes one entry function per test, five
  in all, so a fail-before run isolates one test. `test/CMakeLists.txt` lists the files.
- `docs/simulation.md` holds the sentence the design's §2 A1 replaces: "Note this currently does not
  work with high speed collisions, so you may see a pause in those situations." A later passage in the
  same file already states that continuous collision issues the pre-solve callback.

## 3. Rules for the implementer

- Three commits, A0 then A1 then A2, each building and passing the whole `test` target on its own in
  both precision configurations (§7).
  Commit messages are plain descriptive sentences, no type prefix, no AI-attribution trailer.
- Tests that must fail before the change are shown to fail before the change. The new test file
  does not exist at the previous commit, so: check out that commit in a scratch `git worktree`, copy
  in the working tree's `test/test_tree_order.c`, `test/main.c` and `test/CMakeLists.txt`, build,
  and run each new test by its own entry name. The build must succeed and each test must fail on its
  divergence check, the quoted `condition false:` line naming it, not on a witness or setup check.
  Then run the same tests in the working tree, where each passes. The closure doc quotes both runs
  of each test.
- Touch only the files the design's §2 lists for each change, the one new test file, the two test
  registration files, and the `docs/simulation.md` sentence. Any other file is a deviation: stop and
  report rather than touch it, except the golden policy in §5 and the closure doc of §8.
- C17, no warnings, `clang-format` with the repo's `.clang-format` applied to the changed hunks only
  (a whole-file reformat is a deviation). Comments in human words; no Doxygen for internals.
- Do not add a validator, flag, fallback or counter the design does not name. The design's A1 stack
  buffer alternative is not implemented; it is text for the PR only if a reviewer asks.
- Every deviation, pre-existing failure and out-of-scope observation goes in the closure doc (§8),
  plainly, with no fix attempted.

## 4. Increments

### A0. Test helper and sanity test

**Files.** New `test/test_tree_order.c`; `test/CMakeLists.txt` (add the file); `test/main.c` (add
`extern int TreeOrderHelperTest( void );` and `MAYBE_RUN_TEST( TreeOrderHelperTest );` in
alphabetical position). No `src/` change. Each later increment registers its own entry functions
in `test/main.c` the same way.

**Commit message.** `Add a tree-layout perturbation test helper with a visit-order witness`

**Helper contract** (static functions in the new file; names are suggestions, the contract is not):

- `PerturbTreeLayout( b3WorldId )`: `b3DynamicTree_Rebuild( tree, true )` on each of the three trees
  of `world->broadPhase`, in static, kinematic, dynamic order. Direct call, never
  `b3World_RebuildStaticTree`. Called only between world calls, never from a callback, and never
  while a recording is active: `b3GetWorldFromId` does not check the world lock, and the direct
  rebuild writes no recording command.
- `RecordVisitOrder( b3WorldId, b3BodyType treeType, b3AABB box, uint64_t maskBits, VisitOrder* out )`:
  one `b3DynamicTree_Query` on that tree with the given mask (`B3_DEFAULT_MASK_BITS` for the CCD
  queries, the explosion definition's for Explode), whose callback appends `(int)userData` (the shape
  id) to `out` in callback order and returns true.
- `SameSequence( a, b )` and `SameSet( a, b )` over two `VisitOrder`s. A witness passes when
  `SameSet` holds and `SameSequence` does not.
- `HashWorldState( b3WorldId, const b3BodyId* bodies, int count )`: `b3Hash` from `B3_HASH_INIT`
  over, for each body in creation order: `b3Body_GetTransform`, `b3Body_GetLinearVelocity`,
  `b3Body_GetAngularVelocity`, `b3Body_IsAwake`; then `b3World_GetAwakeBodyCount`.
- `RunScenePair( const Scene* scene, bool perturbEachStep, PairResult* out )`: builds the scene twice
  in two fresh worlds with identical definitions; run 1 steps plainly; run 2 calls
  `PerturbTreeLayout` immediately before every `b3World_Step` when `perturbEachStep` is set. After
  every step it records the scene's hashes for both runs. A `Scene` is a create callback, a step
  count, a time step and sub-step count, and two optional hooks called in both runs with the world,
  the run index and the step index: one before each step (after the perturbation in run 2) and one
  after it (before the next perturbation). A test whose flow does not fit the pair runner calls the
  primitives directly.
- `CreateCorridor( b3WorldId, RandomState*, b3BodyType clusterTree, ... )`, the witness corridor: a
  cluster created in two parts in its own tree, one part with the scene and settled by the first
  step, the other, with the fast sphere, in the after-step hook of the step before the corridor
  step, so the cluster's tree holds settled subtrees and insertions at the perturbation (§2). The
  sphere's tree leaf bound overlaps no other shape's leaf bound at the corridor step's start,
  asserted from the trees before that step (§2), and its one-step motion along one axis
  sweeps a cluster of small solid boxes placed from the random state strictly inside the sphere's
  swept AABB, at least two of them on the sphere's path, with nothing else inside that AABB or its
  fat margin. A box is on the path when its time-of-impact call against the full one-step sweep
  returns a fraction strictly between zero and one. A static corridor uses a plain fast dynamic sphere and boxes on static bodies; a
  kinematic corridor a bullet and boxes on resting kinematic bodies; a dynamic corridor a bullet and
  boxes on resting dynamic bodies, one box per body. The candidate set is the cluster by
  construction (§2). Two on-path boxes make the result order-sensitive: the nearer box's
  time-of-impact call runs to the farther box's fraction when the farther is visited first and to
  the full sweep otherwise, while a box off the path returns the running fraction and changes
  nothing. A changed limit can change the fraction (design §1) but need not, so a corridor's state
  hash may or may not diverge before the change; the callback order always does. The corridor's
  witness is `RecordVisitOrder` on the cluster's tree with a box covering the corridor, in both runs,
  requiring same set, different sequence: in the before-step hook of the corridor step for a static
  corridor, in its after-step hook for a kinematic or dynamic corridor with the sphere's own shape
  removed from the sequence (§2). After the corridor step the sphere's body sim must carry
  `b3_isFast` (§2), or the CCD query the witness stands for never ran. The corridor's hash is
  `HashWorldState` over its sphere and cluster bodies. Cluster size and placement are the
  implementer's; when a witness fails to differ, the rule is changed for every corridor of that kind,
  never for one instance.

**Sanity test** `TreeOrderHelperTest` (design §2 A0 "Done when"), one worker throughout:

1. CCD witnesses. Zero gravity, no sleep. One corridor of each kind, nothing else in the world. Step
   twice with run 2 perturbed before each step; each corridor's second part is created after the
   first step and the second is the corridor step. Require each corridor's witness to differ and each sphere flagged
   fast.
2. Explode witness. Zero gravity, no sleep. Several single-shape dynamic bodies inside an explosion's
   query box, nothing else in the dynamic tree. Record the dynamic-tree visit order with the explosion
   box (`radius + falloff` cube at the position) and the definition's mask, run 1 plain, run 2 after
   `PerturbTreeLayout`. Require same set, different sequence.
3. Unchanged hash. Gravity on, sleep on. A scene with no fast body, no sensor and no explosion: a pile
   of boxes on static ground, stepped until it sleeps. `RunScenePair` with perturbation each step;
   require identical per-step hashes throughout, and in the after-step hook require `b3_isFast` clear
   on every body after every step, so the scene is the negative control it claims to be.

This test does not fail on `main`; A0 has no behaviour change, and the PR says so (design §3).

If (1) or (2) shows the same sequence, change the placement rule until it differs. Do not weaken the
check. If (3) differs, stop: the design's A0 contract does not hold and the owner decides.

### A1. CCD candidates in shape-id order

**Files.** `src/solver.c`, `src/physics_world.h`, `src/physics_world.c`, `docs/simulation.md`,
`test/test_tree_order.c`, `test/main.c` (register `ContinuousCandidateOrderTest`,
`ContinuousSensorSelectionTest` and `ContinuousCallbackOrderTest`). Nothing else.

**Commit message.** `Evaluate continuous collision candidates in ascending shape-id order`

**Code.** Design §2 A1 is the specification. Mechanics that follow from it:

- Add `b3Array( int ) continuousCandidates` to `b3TaskContext`, reserved beside `pairKeys` at world
  creation (a few dozen), destroyed beside it, and summed beside it in `b3World_DumpMemoryStats`.
- Split `b3ContinuousQueryCallback` in two. The tree callback appends `(int)userData` to the array
  and returns true; it keeps the `visitCount` increment. The body of today's callback from the
  assertion that the fast shape is not a sensor, which precedes the "skip same shape" test, to the
  stall log moves unchanged into a function taking the context and a shape id. `b3ContinuousContext`
  gains the pointer it needs to reach the array.
- In `b3SolveContinuous`, after each of the three `b3DynamicTree_Query` calls: `QSORT` the array
  ascending with `LESS`/`SWAP` macros in the `src/broad_phase.c` shape (`src/solver.c` gains
  `#include "qsort.h"`, as `src/broad_phase.c` has), call the evaluation function
  once per id in order, then `b3Array_Clear`. Three separate sort-and-evaluate passes, one per query;
  never merge the trees' candidates.
- The running fraction, sensor buffer, sensor end filter, `preSolve` gate and the iteration counters
  are untouched.

**Docs.** In `docs/simulation.md`, replace the pre-solve sentence quoted in §2 with the facts the
design states: the callback is also issued from continuous collision for bullets and fast bodies, for
a solid candidate with pre-solve events enabled on either shape whose time of impact lies strictly
between zero and the running fraction, in ascending shape id within each tree query; for a callback
that depends only on its arguments the result does not depend on broad-phase layout; a callback that
keeps state sees calls from different bodies interleave across workers, and the custom filter is also
called during pair discovery and sensor overlap in tree order. Two to four sentences in the doc's
voice. Do not touch the sensor passage.

**Tests** (all in `test/test_tree_order.c`, all must fail in the fail-before worktree of §3).

- `ContinuousCandidateOrderTest` (design §2 A1 "Seeded fuzz"). Zero gravity, no sleep, continuous
  enabled. Per seed: a random field of static boxes in a slab and fast bodies launched through it,
  bullets and plain dynamic bodies both, at speeds that mark them fast (per-step motion well above the
  shape's minimum extent); plus three corridors, static, kinematic and dynamic, in regions disjoint
  from the slab and from each other, their second parts created after the first step so that the
  second is their corridor step. Pseudo-random numbers from a local xorshift32 state seeded per
  case (the `RandomState` idiom in `test/test_broad_phase.c`), never `rand()`. `RunScenePair` with
  perturbation each step, hashing the field's fast bodies and each corridor separately. The test does
  not stop at the first divergence: per seed it checks every corridor's witness and corridor hash at
  the second step, the corridor step, and the field hash, over the field's fast bodies alone, at every
  step; corridor bodies are hashed at their corridor step only; it counts the seeds whose field hash
  diverges and, per corridor, the seeds whose corridor hash diverges; it prints every count and
  requires every witness to differ and every count to be zero. The corridor hashes are the assertions
  tied to witnessed queries, one per tree, at the one step each is witnessed; the field hash is the
  design's broad fuzz, whose queries are not witnessed: a field query's box is computed inside the
  solver (§2), so no test-side witness over it is exact, and the nonzero fail-before count below is
  its evidence of a changed order, the evidence design §2 A1 prescribes for the fuzz. Guaranteed fail-before evidence per tree kind is the callback
  test's; here it is the field's count. Run many seeds at one worker; then run a
  subset again in worlds created with a `b3WorldDef.workerCount` of four and a workload of at least
  49 awake bodies and 25 bullets, requiring `b3_isFast` on at least 25 bullets in the after-step hook
  of the first step, so finalize and bullet CCD each form at least four blocks there (§2) and the
  per-worker candidate arrays are eligible for parallel use; which worker drains which block is the
  scheduler's choice. Same assertions. Seed count, field density, body count and step count are
  the implementer's and are recorded in the closure doc. Before committing A1, run the test in the
  fail-before worktree and record every count. The corridor counts are reported as found. The field's
  count is the number the design asks the PR to record, and if it is zero raise field density or body
  count until it is not. Keep the resulting configuration.
- `ContinuousSensorSelectionTest` (design §2 A1 "more than 8 sensors"). Zero gravity, one worker.
  More sensor shapes than the CCD sensor limit of eight (`isSensor`, `enableSensorEvents`), all on one
  static body so none can report another (sensor overlap skips shapes on the sensor's own body), all
  inside the swept AABB of one bullet sphere with `enableSensorEvents` whose single step sweeps past
  them all and ends outside every sensor; no solids and no other static shape, so the covering box's
  candidate set is the CCD query's. Record the static-tree visit order over a box covering the
  sensors in both runs and require same set, different sequence. Step both once. Read
  `b3World_GetSensorEvents` begin events; require exactly eight in each run (a test-local constant
  equal to the limit in `src/solver.c`), compare sensor shape ids by `index1`, and require the eight
  to be the eight lowest sensor shape ids in both runs, which is what the design's §5 sensor
  selection gives when no solid shortens the sweep. Before the change the eight kept are the first
  eight visited, so the two runs must report different sets; a differing visit sequence alone is not
  enough, since a swap within the first eight or within the rest leaves the set unchanged. If the
  sets match before the change, change the placement rule until they differ.
- `ContinuousCallbackOrderTest` (design §2 A1 "Change" and "Costs and caveats": `preSolve` and the
  custom filter are called in ascending id within each query). Zero gravity, one worker. One corridor
  of each kind, static, kinematic and dynamic, in disjoint regions, `enablePreSolveEvents` and
  `enableCustomFiltering` set on each sphere's shape and on every cluster box, and no other shape
  with either flag. Both callbacks are registered in both worlds before any step, through
  `b3World_SetPreSolveCallback` and `b3World_SetCustomFilterCallback`, with that run's log as the
  context (§2). Argument-only callbacks that keep only calls in which one argument is a corridor
  sphere, logged under that sphere: the custom filter appends the other shape's `index1` and returns
  true; `preSolve` appends the other shape's `index1` to a second log and returns false, so the
  running fraction stays open and every on-path box reaches it. Calls between cluster boxes, which
  pair discovery makes when dynamic boxes' fat AABBs overlap, are dropped.
  Both runs step twice, the second being the corridor step, with each corridor's witness recorded
  per the corridor contract, requiring same set, different sequence, and a different sequence again
  when restricted to that corridor's on-path boxes (change the placement rule until it is). Then
  require, per corridor: the custom-filter log holds exactly the cluster's indices in ascending
  order, in both runs; the `preSolve` log holds exactly the on-path boxes' indices in ascending
  order, in both runs. The
  scene has no sensor and each sphere's leaf bound overlaps no other leaf bound at step start,
  which the corridor contract asserts, so the step's custom-filter calls involving a sphere come from CCD
  alone (§2). Fails before the change in every tree kind
  because both callbacks are then called in visit order, which each witness has shown differs for
  the whole cluster and for the on-path boxes. This is the guaranteed fail-before evidence for A1's
  three sort passes.

### A2. Explode wake and impulse order

**Files.** `src/physics_world.c`, `test/test_tree_order.c`, `test/main.c` (register
`ExplodeOrderTest`). Nothing else.

**Commit message.** `Apply explosion wakes and impulses in ascending shape-id order`

**Code.** Design §2 A2 is the specification. `ExplosionCallback` keeps the `explosionScale == 0`
skip and otherwise appends the shape id to a `b3Array( int )` held in `ExplosionContext`, created
with `b3Array_Create` before the query and destroyed before `b3World_Explode` returns. After the
query: `QSORT` ascending (`src/physics_world.c` gains `#include "qsort.h"`), then today's callback
body from the body lookup onward runs once per id in a function taking the context and a shape id.
`world->locked` stays set across query, sort and loop.

**Test** `ExplodeOrderTest` (design §2 A2 "Tests"; must fail in the fail-before worktree of §3). Zero
gravity, sleep enabled, one worker, and the world is never stepped. Every body is created with
`isAwake` false, which puts it in its own sleeping solver set at creation with its proxies in the
dynamic tree (`src/body.c`, `src/shape.c`): one dynamic body with three sphere shapes inside the
explosion radius; several single-shape dynamic bodies inside the radius and one whose closest
surface, not its centre, lies beyond `radius` and within `radius + falloff`, so the scaled branch runs
too; no other dynamic body in the world, so the explosion box's candidate set is the query's. Require
`b3World_GetAwakeBodyCount` zero in both worlds before the explosion. The explosion definition starts
from
`b3DefaultExplosionDef()`, which sets `maskBits` and nothing else, with `radius`, `falloff` and
`impulsePerArea` then set positive and the bodies placed relative to that `radius`. Run 2 then
calls `PerturbTreeLayout`; both runs record the dynamic-tree visit order with the explosion box and
the definition's `maskBits`, and the test requires same set, different sequence, and a different
sequence again when each shape id is mapped to its body through the world's shape array and only
each body's first visit is kept, so the wake order and not only the per-shape
impulse order is witnessed (the multi-shape body alone cannot satisfy this). Both runs call
`b3World_Explode` with that definition. Then require, in each run, every body in range awake with a
nonzero linear velocity. Then compute two comparisons between runs, print both results, and assert
both after printing: an identical `bodyId` sequence in the awake set's `bodySims`, and for every
body `memcmp`-equal `b3Body_GetLinearVelocity` and `b3Body_GetAngularVelocity`. If the printed
velocity comparison is equal before the change, move the three shapes so their contributions are
not symmetric; do not drop the check.

## 5. Golden hashes in `test/test_determinism.c`

Run the full suite after A1 in both precision configurations (§7). The file pins hashes, sleep
steps, a hit count and a query hash per scene, one set per configuration, and the policy applies to
each. A changed expected value of any of these kinds in a scene that performs continuous collision
with several candidates per query is the design's stated behaviour change (§1): update it in the A1
commit, name the scene, and put the old and new values and the CCD cause (which fast bodies or
bullets sweep several candidates) in the PR text and the closure doc. A changed value in a scene with
no fast body, sensor or explosion is a defect: stop and report. A change whose cause cannot be traced
to CCD is unexplained: stop and report. `test_determinism.c` is the only file this policy adds to the
A1 file list.

## 6. Test-to-requirement mapping

| Design clause | Evidence | Fails before? |
|---|---|---|
| §2 A0 Done when: visit order changes at the CCD and Explode queries | `TreeOrderHelperTest` cases 1 and 2 | No (no behaviour change) |
| §2 A0 Done when: hash unchanged without CCD, sensor or explosion | `TreeOrderHelperTest` case 3 | No |
| §2 A0 Check before merging: witness differs for the query a test relies on | Witness inside the sensor, callback and Explode tests, and inside every fuzz seed's corridor, over a candidate set that is the query's by construction; the fuzz field's queries are not witnessed and their changed order is shown by the nonzero fail-before count | n/a |
| §2 A1 Change: equal results across the two witnessed layouts per seed, in all three tree queries, at one worker and at four workers with at least four blocks available per stage | `ContinuousCandidateOrderTest`: static, kinematic and dynamic corridor hashes at the witnessed step, field hash at every step (broad fuzz) | Yes, field count recorded nonzero; corridor counts recorded |
| §2 A1 Change: the rule itself, sort then evaluate after every tree query, so the result is layout-independent in general | Inspection of `b3SolveContinuous`: three sort-and-evaluate passes, one per query, nothing evaluated inside a tree callback | n/a |
| §2 A1 Tests: same sensors with more than eight candidates | `ContinuousSensorSelectionTest` | Yes |
| §2 A1 Change and Caveats: `preSolve` and custom filter in ascending id within each query, in each of the three tree queries | `ContinuousCallbackOrderTest`, one corridor per tree kind; the `preSolve` log over the on-path boxes, whose visit order the witness shows differs | Yes, guaranteed per tree kind |
| §2 A1 Change: array reserved, destroyed, counted; reused across bodies | Inspection of the three sites and `b3Array_Clear` after each query | n/a |
| §2 A1 Change: nothing else changes | Diff inspection; `test_determinism.c` at the worker counts it runs, one to five, in both precision configurations | n/a |
| §2 A1 Caveats: docs sentence replaced | Inspection of `docs/simulation.md` | n/a |
| §2 A2 Change: deterministic wake and impulse order, with every body in range woken and pushed | `ExplodeOrderTest` | Yes |
| §2 A2 Change: `world->locked` across query and loop | Inspection | n/a |
| §3 checklist | Closure doc, item by item | n/a |

## 7. Verification and measurements

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF
cmake --build build --target test
./build/bin/test                                 # whole suite, per commit
./build/bin/test ContinuousSensorSelectionTest   # one test, by its entry name

cmake -S . -B build-dp -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF -DBOX3D_DOUBLE_PRECISION=ON
cmake --build build-dp --target test
./build-dp/bin/test                              # whole suite, double precision, per commit

cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DBOX3D_BENCHMARKS=ON -DBOX3D_SAMPLES=OFF -DBOX3D_UNIT_TESTS=OFF
cmake --build build-rel --target benchmark
./build-rel/bin/benchmark -t=8
```

The samples are off because they fetch imgui, implot and nfd and need OpenGL and X11. The benchmark
is built and run this way on `main` and on the branch, same machine, same options; it writes
`<scene>.csv` into its working directory, so each revision runs from its own directory. The
benchmark's thread cap is explicit because its default is the host's core count and its only guard
against exceeding `B3_MAX_WORKERS` (32) is an `assert` the Release build removes
(`benchmark/main.c`, `shared/utils.c`). The same file clamps `-t` to the detected core count, so the
measurement machine has at least eight cores, or the PR reports the thread range both runs share.

Measurements the PR text needs (design §2 A1 costs):

- Suite benchmark: for every scene, the csv total milliseconds per thread count, from one to the
  maximum both runs share, at most eight, `main` against the branch, stated as a no-regression check because the suite has no bullet-heavy
  scene.
- Scratch harness: an uncommitted program in the scratch directory, built against the library, that
  runs the fuzz scene and a dense scene (the sensor scene scaled to 64 candidates in one query) and
  prints per step `b3World_GetProfile` `transforms` and `bullets`, and once at the end
  `b3World_GetCounters` `distanceIterations`, `pushBackIterations`, `rootIterations` as high-water
  marks; `main` against the branch.
- Sort cost: a temporary `b3GetTicks` pair around the `QSORT` call alone in `b3SolveContinuous`,
  logging microseconds and candidate count per query in the dense scene; removed before commit.
- High-water mark: the capacity of `continuousCandidates` per worker after the fuzz and the dense
  scene, read through a temporary log line that is removed before commit.
- Fuzz seeds whose field hash diverges before the change, with the corridor's count beside it.

## 8. PR text and closure doc

PR description, in this order: one line stating results are not bit-identical to before in the §1
cases; `Closes #<issue>` with the approved issue's number (§1); the repository's
`.github/pull_request_template.md` filled in full, its samples box ticked only after running every
sample in the samples app from a build with `BOX3D_SAMPLES` on, and otherwise left unticked with a
sentence saying what was run; the three commits; the measurements of §7; any golden changed under §5 with old
and new values; the stateful-callback caveat from design §2 A1; the A0 test's reason for not failing
before.

Closure doc, written after the three commits as
`docs/designs/20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-closure.md`,
containing: suite counts per commit (tests run, passed), pre-existing failures named with output,
compiler warning status and the `clang-format` check, every deviation from this document and the
design with its reason, every out-of-scope observation, and the two outputs of each fail-before
test. It stays outside the PR: the PR is the three commits of §4 and nothing is amended or added for
it; the owner commits it to the design branch separately if at all. The implementer does not change
the status of the design doc or of this document.

## 9. Out of scope and stop conditions

- No change to `benchmark/`, `shared/`, `samples/`, public headers, or any `src/` file §4 does not
  list. A needed change there is a report, not a commit.
- Stop and report if: the perturbed plain scene's hash differs (A0 case 3); any witness cannot be made
  to differ after adjusting the fixture; the fuzz diverges in no seed before the change after raising
  density and body count; a golden changes in a scene without CCD, sensor or explosion;
  a test needs a hook in `src/` to observe something; or the design is silent on a decision the code
  forces.
- Public query order, island independence and stateful callbacks are outside the guarantee (design
  §4) and get no test.
