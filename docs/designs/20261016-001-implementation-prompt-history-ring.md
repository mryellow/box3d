---
title: Implementation Prompt - rewind-only world history (hot image, undo journal, ring)
status: Draft
date: 2026-10-16
implements: 20260929-002, 20261015-003, 20261015-004
---

# Implementation Prompt: rewind-only world history

This is the prompt handed to the implementing agent. It carries the branch, the increments, the
test mechanism, fixtures and the test-to-requirement mapping that the designs do not. The designs
decide what changes and why; this document decides how it lands and how it is shown to work.

Three Canonical documents are implemented together and are called by these names below:

- **the design**: `docs/designs/20260929-002-rewind-only-history-ring.md` (revision 60): the per-tick
  image, the undo journal, the restore, the caller contract, the verification and the public API;
- **the ring doc**: `docs/designs/20261015-003-history-ring-budget-and-eviction.md` (revision 6): slots,
  the two ends, the capture interval, the byte budget, the window, enable and disable;
- **the trees doc**: `docs/designs/20261015-004-history-ring-broad-phase-trees.md` (revision 7): which
  tree is imaged, which are derived, proxy identity, the restore pass.

A `§N` of one of them is cited as "the design §N", "the ring doc §N", "the trees doc §N". A bare `§N`
is this document's own section.

## 1. Task

Implement the design, the ring doc and the trees doc in this repository, on the branch
`world-history-ring`, forked from `master` at the commit that sets this document Canonical, as the
nine commits §4 names (H0, H1, H2, H3a, H3b, H3c, H4, H5, H6), in that order, each bisectable. They
deliver the design's phases 0, 1 and 2 (the design §13). Phase 3 is optional in the design and is
not part of this task. The designs are binding: where this document and a design disagree, the
design wins and the disagreement is reported. Where the code needs a behaviour no design states,
stop and report; do not invent it.

The base carries the awake and sleeping dynamic trees (`20261013-001`, `20261015-001`), the bulk
migration and the sorted continuous candidates, which the designs assume. `src/` and `include/` are
unchanged since 55a40d1, the commit the designs cite, as of `master` at 60ea633 where §2 was
checked. If `src/` or `include/` has moved by the time implementation starts, §2 is re-checked
against the fork point before any code is written (WORKFLOW U-I1) and the differences are recorded in
the closure doc; a difference that changes what an increment must do is a stop.

Read, in this order, before writing code: the design (all of it), the ring doc, the trees doc, this
document, `docs/designs/WORKFLOW.md` Part B, `CONTRIBUTING.md`, and the source the designs name:
`src/physics_world.h`, `src/physics_world.c` (`b3CreateWorld`, `b3DestroyWorld`, `b3World_Step`,
`b3World_Explode`, the hull database functions, the validation functions, `b3World_StartRecording`),
`src/body.h`, `src/body.c`, `src/shape.h`, `src/shape.c`, `src/contact.h`, `src/contact.c`,
`src/mesh_contact.c`, `src/joint.h`, `src/joint.c` and the eight joint files, `src/island.h`,
`src/island.c`, `src/solver_set.h`, `src/solver_set.c`, `src/constraint_graph.h`,
`src/constraint_graph.c`, `src/sensor.h`, `src/sensor.c`, `src/broad_phase.h`, `src/broad_phase.c`,
`src/dynamic_tree.h`, `src/dynamic_tree.c`, `src/id_pool.h`, `src/id_pool.c`, `src/container.h`,
`src/bitset.h`, `src/table.h`, `src/block_allocator.h`, `src/core.h`, `src/core.c`,
`src/world_snapshot.h`, `src/world_snapshot.c` (all of it), `src/recording.h`, `src/recording.c`
(`b3HashWorldState`, the geometry registry, `b3StartRecordingIntoBuffer`), `src/replay.h`,
`src/replay.c` (`b3RecPlayerCreateWorld`, `b3RecCaptureKeyframe`, `b3RecPlayerRestoreKeyframe`,
`b3RecPlayer_SeekFrame`, `b3RecPlayer_Restart`), `src/solver.c` (the stage loops, finalize, the refit
and bullet stages, the sleep pass), `src/contact_solver.c` (store impulses), `include/box3d/box3d.h`,
`include/box3d/types.h`, `include/box3d/base.h`, `test/main.c`, `test/test_macros.h`,
`test/test_recording.c`, `test/test_determinism.c`, `test/test_awake_tree.c`,
`test/test_tree_order.c`, `test/test_name_cache.c`, `shared/benchmarks.h`, `shared/benchmarks.c`,
`benchmark/main.c`.

## 2. Reconciliation against the current tree

Checked on `master` at 60ea633 (`src/` and `include/` as of 55a40d1). Each item is a fact the
increments rely on. Where a design names a mechanism and the code differs, the difference is stated
here and the increment follows the code where the design allows and reports where it does not.

**What exists and what does not.**

- None of `b3Body_Write`, `b3Shape_Write`, `b3Contact_Write`, `b3Joint_Write`, `b3Island_Write`,
  `b3Shape_WriteMaterial`, `b3CreateSolverSet`, `b3World_WriteScalars`, `b3WorldScalars`,
  `b3JournaledArray`, `b3JournaledSlotPool`, `b3JournaledPairSet`, `b3JournaledBitSet`,
  `b3HistoryDef`, `b3World_EnableHistory`, `b3World_Rewind` or `b3World_ComputeStateHash` exists.
  No identifier in `src/` or `include/` contains "journal", "history", "undo" or "rollback"
  (a comment in `src/verstable.h` and two doc comments in `include/box3d/box3d.h` aside).
- `b3World_StartRecording`, `b3World_StopRecording`, `b3CreatePlayer`, `b3RecPlayer_GetWorldId`,
  `b3RecPlayer_SeekFrame`, `b3RecPlayer_Restart`, `b3RecPlayer_SetKeyframePolicy` and the rest of the
  recording API are public (`include/box3d/box3d.h`). `b3SerializeWorld( world, buf, rec )` and
  `b3DeserializeIntoShell( data, size, world, rdr )` are internal (`src/world_snapshot.h`), called
  from `src/recording.c` and `src/replay.c` only, never from a test.

**Records and their write routes today.** Every record field is assigned directly at its setter
and structural sites; the three write routes of the design §5.2 do not exist.

- `b3Body` (`src/body.h`): `setIndex`, `localIndex`, `islandId`, `islandIndex`, the list heads and
  counts, `sleepThreshold`, `sleepTime`, `sleepVelocity`, `safetyFactor`, `mass`, `inertia`,
  `bodyMoveIndex`, `id`, `flags`, `nameId`, `type`, `generation`, `userData`. No `colorIndex` and no
  heap-owned field. `bodyMoveIndex` is set to `B3_NULL_INDEX` at creation (`src/body.c`), to the sim
  index in finalize (`src/solver.c`) and read and cleared in `b3TrySleepIsland` (`src/solver_set.c`).
- `b3Shape` (`src/shape.h`): `id`, `bodyId`, the list links, `sensorIndex`, `proxyKey`, `type`,
  `density`, `explosionScale`, `aabbMargin`, `aabb`, `localCentroid`, `materialCount`, the inline
  `material`, `b3SurfaceMaterial* materials`, `filter`, `userData`, `userShape`, `nameId`,
  `generation`, `flags`, and the geometry union whose `hull`, `heightField` and `compound` members are
  already pointer-to-const. A single-material shape keeps `materialCount` 1 and `materials` NULL; a
  multi-material or compound shape holds a `b3Alloc`'d block. `b3GetShapeMaterials` is a `static
  inline` in `src/shape.h` that takes a const shape and returns a writable pointer by casting the
  qualifier away; `b3Shape_SetFriction`, `b3Shape_SetRestitution`, `b3Shape_SetSurfaceMaterial` and
  `b3Shape_SetMeshMaterial` (`src/shape.c`) write through it. `userShape` is assigned at creation and destruction (`src/shape.c`), in the debug
  draw's query callback (`src/physics_world.c`) and by the serializer's reader (`src/world_snapshot.c`).
- `b3Contact` (`src/contact.h`): `setIndex`, `colorIndex`, `localIndex`, `islandId`, `islandIndex`,
  `contactId`, the two edges, shape ids, `childIndex`, `encodedBodySimA/B`, `flags`,
  `b3Manifold* manifolds`, `manifoldCount`, the cached rotations and pose, `friction`, a union of
  `b3ConvexContact { b3ContactCache cache }` and `b3MeshContact { b3Array( b3TriangleCache )
  triangleCache; b3AABB queryBounds }`, the tail scalars and `generation`. The flag is
  `b3_simMeshContact`. Manifold blocks come from per-count `b3BlockAllocator`s in
  `world->manifoldAllocators` behind `manifoldAllocatorMutex` through `b3AllocateManifolds` and
  `b3FreeManifolds` (`src/physics_world.h`); `manifoldCount` changes in `b3ComputeConvexManifold`,
  `b3UpdateConvexContact` (`src/contact.c`) and `b3ComputeMeshManifolds` (`src/mesh_contact.c`), and
  the triangle cache is resized in `b3RefreshCache` (`src/mesh_contact.c`). `b3DestroyContact` frees
  the manifolds and the cache itself, which the design §5.2 moves into the pool free entry.
  `b3BlockAllocator` carries an `allocationCount` field with no getter (`src/block_allocator.h`).
- `b3Joint` (`src/joint.h`): `userData`, `setIndex`, `colorIndex`, `localIndex`, the two edges,
  `jointId`, `islandId`, `islandIndex`, `drawScale`, `type`, `generation`, `collideConnected`. No heap
  field.
- `b3Island` (`src/island.h`): `setIndex`, `localIndex`, `islandId`, `constraintRemoveCount` and the
  three link arrays `bodies`, `contacts`, `joints`. `b3CreateIsland` creates the arrays and emplaces
  the `b3IslandSim`; `b3DestroyIsland` destroys them and frees the id. `b3SplitIsland`
  (`src/island.c`) detaches the base island's three arrays (nulling the headers so `b3DestroyIsland`
  will not free them), builds the new islands, destroys the base island and frees the detached
  arrays by hand. The design §7.1's island destroy entry takes those arrays instead; H3c changes the
  split so the arrays stay on the record until the destroy, and H4's entry takes them there.
- The sim getters `b3GetBodySim`, `b3GetBodyState` (`src/body.c`), `b3GetJointSim` and
  `b3GetJointSimCheckType` (`src/joint.c`) return writable pointers; `b3GetBodyState` returns NULL
  for a non-awake body. The joint setters in every joint file write through
  `b3GetJointSimCheckType` (from 6 uses in `src/parallel_joint.c` to 36 in `src/wheel_joint.c`).
  The step's stages do not use the getters: they index the awake set's and the colours' arrays by
  base pointer.

**Containers today.**

- `b3Array` (`src/container.h`) is `{ T* data; int count; int capacity; }` with `b3Array_Push`,
  `b3Array_Emplace`, `b3Array_RemoveSwap` (returns the moved element's old index or
  `B3_NULL_INDEX`), `b3Array_Clear`, `b3Array_Resize`, `b3Array_Destroy` and the rest; there is no
  const-data variant.
- `b3IdPool` (`src/id_pool.h`) is `{ b3Array( int ) freeArray; int nextIndex; }`. `b3AllocId` pops
  the last element of `freeArray` when it is non-empty and otherwise returns `nextIndex` and bumps
  it; `b3FreeId` pushes the id onto `freeArray` (its `nextIndex` decrement branch is unreachable
  under its own assertion, as its comment says). So the design §7.1's pool alloc entry records
  "popped" or "bumped" and the undo of a free is a pop of the array's last element, which the undo
  asserts is the recorded id. `b3GetIdCapacity` is `nextIndex`.
- The six pools and their sparse arrays on `b3World` (`src/physics_world.h`): `bodyIdPool`/`bodies`,
  `shapeIdPool`/`shapes` with `fatAABBs`, `contactIdPool`/`contacts`, `jointIdPool`/`joints`,
  `islandIdPool`/`islands`, `solverSetIdPool`/`solverSets`. A bump allocation pushes a zeroed
  element onto the array at the creation site (`b3CreateBody`, `b3CreateShapeInternal`,
  `b3CreateContact`, `b3CreateIsland`, `b3TrySleepIsland`, `b3CreateBody`'s sleeping set), which
  the slot pool of the design §5.2 makes one operation.
- `b3BitSet` (`src/bitset.h`) is `{ uint64_t* bits; uint32_t blockCapacity; uint32_t blockCount; }`;
  `b3SetBitGrow` grows through `b3GrowBitSet`; `b3ClearBit` and `b3GetBit` are no-ops or false beyond
  `blockCount`, as the design §7.1's bitset entry assumes. `b3RemoveContactFromGraph` and
  `b3RemoveJointFromGraph` (`src/constraint_graph.c`) clear both bodies' bits unconditionally outside
  the overflow colour, a static body's included; `b3AddContactToGraph` sets bits for dynamic bodies
  only; `b3TrySleepIsland` (`src/solver_set.c`) clears both bodies' bits itself, four `b3ClearBit`
  calls, when it moves a contact or a joint out of a colour. The entry records the old value, so
  the unconditional clear is covered.
- `b3BroadPhase.pairSet` is a `b3HashSet` (`src/table.h`), written by `b3AddKey` in
  `b3CreateContact` and `b3RemoveKey` in `b3DestroyContact` only, and by the serializer.
- `b3SolverSet` (`src/solver_set.h`) holds `bodySims`, `bodyStates`, `jointSims`, `contactIndices`,
  `islandSims` and `setIndex`. A sleeping set is allocated inline in `b3TrySleepIsland`
  (`src/solver_set.c`) and in `b3CreateBody` (`src/body.c`), and the three fixed sets in
  `b3CreateWorld`; `b3DestroySolverSet` destroys the five arrays, frees the id and leaves
  `setIndex` null. The set index enumerators are `b3_staticSet` 0, `b3_disabledSet` 1,
  `b3_awakeSet` 2, `b3_firstSleepingSet` 3.
- `b3GraphColor` (`src/constraint_graph.h`) holds `bodySet`, `jointSims`, `convexContacts`,
  `contacts` (`b3ContactSpec`) and the per-step wide constraint pointers; `B3_GRAPH_COLOR_COUNT` is 24.
- `b3Sensor` (`src/sensor.h`) is `{ hits; overlaps1; overlaps2; shapeId }`. Sensor destruction is
  written twice: `b3DestroySensor` (`src/sensor.c`, called from `b3DestroyBody`) and an inline copy in
  `b3DestroyShapeInternal` (`src/shape.c`); both push the end event, destroy the three arrays and
  removeswap. The design §5.2 makes it one function (H3c).
- The hull database is a `b3HullMap` (verstable, `src/hull.h`) stored as `void* hullDatabase`, keyed
  by `const b3HullData*` with the refcount as the map value, written only by `b3AddHullToDatabase`,
  `b3AddOwnedHullToDatabase` and `b3RemoveHullFromDatabase` (`src/physics_world.c`), as the design
  §5.2 states. `b3HullData`, `b3MeshData` and `b3HeightFieldData` carry a `uint64_t hash`;
  `b3CompoundData` carries none (`include/box3d/types.h`), which is why the design §12 test 1
  computes a compound's hash from its contents.
- The world scalars the design §5.1 images are all loose fields of `b3World` with the names the
  design uses (`splitIslandId`, `compoundShapeCount`, `enableSleep`, `enableWarmStarting`,
  `enableContinuous`, `enableSpeculative`, `enableRestitutionPropagation`, `gravity`,
  `hitEventThreshold`, `restitutionThreshold`, `restitutionIterations`, `maxLinearSpeed`,
  `contactSpeed`, `contactHertz`, `contactDampingRatio`, `contactRecycleDistance`,
  `endEventArrayIndex`, `inv_h`, `inv_dt`). `splitIslandId` is written by `b3DestroyIsland` and
  `b3SplitIslandTask` (`src/island.c`), the solver's step start and end (`src/solver.c`) and
  `b3TrySleepIsland` (`src/solver_set.c`); `compoundShapeCount` by shape creation, shape destruction
  and `b3DestroyBody` (`src/body.c`).

**Broad phase and trees.**

- `b3BroadPhase` (`src/broad_phase.h`) holds `trees[B3_PROXY_TREE_COUNT]` (4), `pairSet`,
  `b3Array( int ) migrationQueue`, `int traversalDepth`, `int migrationBatchDepth`,
  `movedSiblings` and `sleepingTreeRebuildCount`. `b3BroadPhase_BeginTraversal` and
  `b3BroadPhase_EndTraversal` exist under the design's names and are raised and lowered by
  `b3World_Step`, `b3World_Draw`, the seven public queries and casts and `b3World_Explode`
  (`src/physics_world.c`); `migrationBatchDepth` is raised only inside `b3WakeSolverSet` and
  `b3TrySleepIsland`, which call no public API, so it is zero at every API entry. The design §9's
  "no traversal open" assertion is `traversalDepth == 0`.
- `b3World_Step`'s tail, in order: finish `userTreeTask` if pending and `b3ValidateNoMoved`;
  `b3OverlapSensors`; the profile and `b3GrowStack`; the end-event flip (`endEventArrayIndex = 1 -
  endEventArrayIndex` and a clear of the two arrays at the new index); `b3BroadPhase_EndTraversal`
  (the outermost exit drains migrations); `b3ValidateProxyTrees`; `locked = false`; then, when
  recording, `b3HashWorldState`, the state-hash op and the bounds walk. The design §6's capture point
  is after `b3BroadPhase_EndTraversal`; H1 puts it after `b3ValidateProxyTrees` and before the
  unlock.
- The tree writes outside `src/broad_phase.c` are the four the trees doc §7 names,
  `b3DynamicTree_Refit` on the kinematic and dynamic trees and `b3DynamicTree_EnlargeProxy`
  (`src/solver.c`), `b3DynamicTree_Rebuild` on the static tree (`b3World_RebuildStaticTree`,
  `src/physics_world.c`, declared in `src/physics_world.h` only) and `b3DesTree`
  (`src/world_snapshot.c`), and the two `static inline` functions of `src/broad_phase.h`,
  `b3BroadPhase_MarkProxyMoved` and `b3BroadPhase_MarkProxyMovedSerial`, which write through
  `bp->trees` in every includer; H4 moves their bodies into `src/broad_phase.c`. Tests also write
  trees directly: `PerturbTreeLayout` in `test/test_awake_tree.c` and `test/test_tree_order.c`
  calls `b3DynamicTree_Rebuild` on `world->broadPhase.trees + i`, and `SleepingProxyTreeCase` in
  `test/test_awake_tree.c` calls `b3DynamicTree_ClearMoved` on two trees of a standalone
  `b3BroadPhase`; once the trees are pointer-to-const outside `src/broad_phase.c` (H4) those
  helpers call a rebuild function and a clear function of that file.
- Every leaf-box write in the static and kinematic trees passes the shape's `fatAABBs` entry:
  `b3Body_SetTransform` (`src/body.c`), `b3CreateShapeProxy` and `b3ResetProxy` (`src/shape.c`), the
  finalize mark through `b3BroadPhase_MarkProxyMoved` (`src/solver.c`). The trees doc §6 relies on
  this; H4 asserts it in the broad phase's static and kinematic leaf writers on those forward
  writes in the validation build. An undo insertion writes the entry's recorded box while the
  record's `fatAABBs` entry may still be the later value (the entries of `b3Shape_SetHull` are the
  record write, the proxy destroy and the proxy create, undone in reverse), so the undo asserts the
  recorded id and nothing of the entry; H5's restore pass runs after the records are restored and
  its validation asserts every static and kinematic leaf equals its entry.
- `b3DynamicTree_CreateProxies`, `b3DynamicTree_MarkProxyMovedSerial`, `b3DynamicTree_ClearMoved`,
  `b3DynamicTree_GatherMovedProxies` (`src/dynamic_tree.h`) and `b3DynamicTree_EnlargeProxy`
  (`include/box3d/collision.h`) exist with the signatures the trees doc assumes.
  `B3_SLEEPING_TREE_MAX_HEIGHT` is 64 (`src/dynamic_tree.h`). Proxy-pool growth in
  `b3AllocateProxy` (`src/dynamic_tree.c`) happens only on an empty free list and threads the new
  slots in ascending order, as the trees doc §7 states.
- `b3Shape_SetFilter` (`src/shape.c`) returns when the filter is unchanged, assigns
  `shape->filter = filter`, and with `invokeContacts` true calls `b3ResetProxy` with `destroyProxy`
  computed after the assignment by comparing `filter.categoryBits` with
  `shape->filter.categoryBits`, so it is always true and that path always destroys and recreates
  the proxy; with `invokeContacts` false the record is written and the proxy is left as it is. The
  design §12 test 2's two filter cases are therefore `invokeContacts` false (the proxy untouched)
  and true (the proxy recreated); the always-recreate under true is an observation for the closure
  doc (U-I4), and the behaviour is not changed by this task.
- API calls that wake a body and so, for a dynamic body, move its proxies from the sleeping tree
  into the awake tree, which the journal does not cover (the trees doc §5; a kinematic body's
  proxies stay in the kinematic tree awake or asleep, a static body never wakes, a disabled body
  has no proxy): `b3Body_SetTransform` on any non-static body (`src/body.c`),
  `b3Body_SetType`, `b3Body_SetAwake( true )`, `b3Body_Enable`, `b3DestroyBody` and `b3Body_Disable`
  for the bodies touching or jointed to the destroyed or disabled one, `b3CreateJoint` when either
  body is awake, `b3DestroyJoint` with `wakeAttached` true, `b3Shape_SetFilter` with
  `invokeContacts` true (`b3ResetProxy` recreates the leaf in the shape's own tree, which for a
  sleeping dynamic body's shape is the awake tree, and queues the body),
  and a shape created on a sleeping body (its proxy is created in the awake tree, marked, and the
  body queued, `src/shape.c`). Cold, touching no awake structure: a static shape's create with
  `invokeContactCreation` false and destroy, a sensor's create and destroy on a static body,
  `b3Shape_SetFilter` with `invokeContacts` false, the four material setters, `b3CreateJoint`
  between two sleeping bodies (a set merge when their sets differ, `src/joint.c`), `b3DestroyJoint`
  with `wakeAttached` false, `b3DestroyBody` and `b3Body_Disable` of a sleeping body with no
  contact and no joint (`b3DestroyContact` with `wakeBodies` wakes both bodies of a touching
  contact, the destroyed body's own set included, `src/contact.c`), and every `userData` setter.
  The journal tests of H4 use the cold list only.

**Serializer, recording and replay.**

- `b3SerializeWorld` writes `B3_SNAP_VERSION` 11 with `layoutHash` and the validation and
  precision flags in a `b3SnapHeader`, then the scalars, the six pools, the sets, the sparse arrays,
  `fatAABBs`, sensors, islands, the four trees (`b3SerTree`: `version`, `nodeEnd`, `pairFreeList`,
  `proxyCount`, `proxyCapacity`, `proxyFreeList`, `dfsOrdered`, `nodes` and `parents` to `nodeEnd`,
  `proxies` to `proxyCapacity`, the fields the trees doc §5 images less `version`), `pairSet`, the
  colours and the names. It does not write the migration queue, `traversalDepth`, the event arrays,
  the end-event buffers or `movedSiblings`; it writes `endEventArrayIndex` and, as part of the raw
  `b3Body` image, `bodyMoveIndex`. There is no options struct.
- `userData` is nulled in the written copy of every body, shape and joint and forced to NULL on
  restore; `userShape` is nulled in the copy, and on restore `b3DesShapes` carries a live slot's old
  handle over when the slot is live with the same generation and releases every other saved handle
  through `world->destroyDebugShape`. The design §13 phase 0's two options are therefore new: a
  writer and reader option that carries the three `userData` words, and a reader option that
  releases every live `userShape` and carries none. No `.b3rec` file is committed; the tests write
  and read their own, so the version becomes 12 in H1 without migration.
- Interned geometry: a `b3GeometryRegistry` of `b3GeometryEntry` (`src/recording.h`), filled by
  `b3InternGeometry` (deduplicated) or `b3AppendGeometry`, and read back through the
  `b3RecReader`'s `slots`; a restored hull goes through `b3AddHullToDatabase` (cloned and
  refcounted), while a restored mesh, height field or compound shape points at the slot's bytes, so
  their lifetime is the reader's. `b3RecCaptureKeyframe` (`src/replay.c`) asserts the registry did not
  grow during a keyframe capture; `b3RecSeedKeyframeRegistry` pre-seeds the slots so id equals slot
  index. A phase-0 ring therefore owns one registry and one reader of its own, interns new geometry
  at capture and appends a slot per new entry, and a phase-0 restore repoints mesh, height-field and
  compound shapes at the ring's copies (the state hash reads content hashes, so it does not see the
  pointer). H5 removes the registry from the ring and the pointers are the caller's again.
- The keyframe ring of `b3RecPlayer` doubles `keyframeInterval` while the retained keyframes plus
  the new one exceed `keyframeBudget`, dropping keyframes whose frame is not a multiple of the new
  interval; it is the precedent the ring doc §6 cites and nothing of it is reused.
- A player's world is identified only through `b3RecReader.replayWorldId`; `b3World` carries no
  mark. The design §10's assertion that `b3World_EnableHistory` refuses a player's world needs one:
  H1 adds a `bool` set by `b3RecPlayerCreateWorld` and read by that assertion alone.
  `b3World.recording` is non-NULL only while recording, which is the other half of the mutual
  exclusion.
- `b3HashWorldState` (`src/recording.c`) is FNV-1a 64 over live bodies' transforms and, for awake
  bodies, velocities. It is the recording's divergence check (`b3RecWrite_StateHash` in the step,
  `b3RecDispatch_StateHash` in replay). It is not changed: the design §12 test 1's
  `b3World_ComputeStateHash` is a new function, and a recording written on this branch carries the
  same per-frame hashes as one written before it (a version-11 recording is rejected by the
  version-12 reader, above).
- `b3DestroyWorld` (`src/physics_world.c`) stops the recording, then destroys, in order, the
  scheduler and worker contexts, the event arrays, sensors and bodies, each live shape's
  allocations, each contact's triangle cache, the hull database (asserting it is empty), the name
  cache, the sparse arrays and islands, the solver sets, the graph and broad phase, the id pools,
  the manifold allocators and the stack. The ring doc §8's disable call sits immediately after the
  recording stop, before any of those.

**Allocation.** `b3Alloc( size_t )` adds `(int)size` to the allocator's byte count and `b3Free( mem,
size )` subtracts `(int64_t)size` (`src/core.c`); `b3GetByteCount` is public. The ring doc §6 names
the `(int)` as a change this task makes (H1). `b3SetAllocator` installs user hooks that receive sizes
aligned to `B3_ALIGNMENT`. `b3GrowAlloc` takes `int` sizes and is not used for the arena.

**Compiler flags.** The box3d target compiles with `-Wmissing-prototypes -Wall -Wextra -pedantic`
on Unix (plus `-Wno-unused-value`), `/W4` with a few suppressions on MSVC, and no error flags; the
only error switch is `BOX3D_COMPILE_WARNING_AS_ERROR` (default OFF). The design §5.2's "discarded
qualifier is an error" is therefore a per-compiler addition in H2: `-Werror=discarded-qualifiers`
for GCC, `-Werror=incompatible-pointer-types-discards-qualifiers` for Clang and clang-cl, `/we4090`
for MSVC, applied to the box3d and test targets unconditionally.

**Validation build.** `B3_ENABLE_VALIDATION` is 1 when `BOX3D_VALIDATE` is defined and `NDEBUG` is
not (`include/box3d/base.h`); `BOX3D_VALIDATE` defaults ON at the top level and is a private define
of the box3d target and of the test. `b3ValidateSolverSets`, `b3ValidateContacts` and
`b3ValidateConnectivity` have real bodies under it in `src/physics_world.c`; `b3ValidateProxyTrees`
is a static function there, called at the step's end; `b3ValidateBroadPhase` (`src/broad_phase.c`)
validates the dynamic, kinematic and sleeping trees; `b3DynamicTree_Validate` is public.

**Tests, goldens and benchmarks.**

- `test/main.c` registers each entry through an `extern` line and a `MAYBE_RUN_TEST` call,
  alphabetically with two existing exceptions, and filters by exact entry name; `ENSURE` prints
  `condition false:` with the failing expression and returns at the first failure, so one entry
  function shows one fail-before (`test/test_macros.h`). A byte-count baseline is written as
  `int64_t base = b3GetByteCount(); ... ENSURE( b3GetByteCount() == base );`
  (`test/test_name_cache.c`). Tests reach internals through `b3GetWorldFromId` and the `src/`
  include directory; `HashWorldState`, `PerturbTreeLayout` and the proxy-key readers are file-static
  in `test/test_awake_tree.c` and `test/test_tree_order.c`.
- `test/test_determinism.c` pins, per precision, `RAGDOLL_HASH`, `WAVE_PILE_HASH`,
  `QUERY_SPAWN_HASH`, `QUERY_SPAWN_QUERY_HASH`, `MESH_DROP_HASH` and their sleep steps and hit
  count, under `ENSURE_GOLDEN`, which is `ENSURE` only when `B3_MAX_MANIFOLD_POINTS == 4`. Its
  `MultithreadingTest` runs worker counts 1 to 5. Policy in §5.
- `test/test_recording.c`'s `ScrubBackward` records 80 frames of four hull boxes, replays them in a
  player and asserts `b3HashWorldState` equality after seeks to `{ 80, 40, 5, 79, 0, 1 }`; it is the
  design §1's evidence that keyframe restore plus re-step reproduces transforms, with a hash that
  covers transforms and velocities only.
- `shared/benchmarks.c` provides eighteen `Create*` (`shared/benchmarks.h`; seventeen are in the
  benchmark's table, `CreateWidePyramid` is not), `Step*` for `junkyard`, `rain`,
  `large_world`, `sleep`, `sleeping_grid`, `post_field` and `village`, `Get*Capacity` for ten, and
  `Destroy*` for six; the benchmark's table (`benchmark/main.c`) gives each scene's step count. No
  benchmark scene uses height fields, sensors or `b3World_Explode`; the `Destroy*` functions free
  borrowed meshes, so a test disables history before calling one (the design §10, borrowed
  geometry). The test target links `shared`.
- `benchmark/main.c` takes `-t=`, `-w=`, `-b=`, `-r=`, `-s` and writes `<scene>.csv`; `B3_MAX_WORKERS`
  is 32 and `B3_TIME_TO_SLEEP` 0.5 (`include/box3d/constants.h`).

## 3. Rules for the implementer

- Nine commits in the order of §4, each building and passing the whole `test` target on its own in
  both precision configurations (§7), with `BOX3D_VALIDATE` on. Commit messages are plain
  descriptive sentences, no type prefix, no AI-attribution trailer.
- Tests that must fail before a commit are shown to fail before it. For every commit that adds a
  test entry or a case to one, check out the previous commit in a scratch `git worktree`, copy in
  the working tree's `test/test_history.c`, `test/main.c` and `test/CMakeLists.txt`, build, and
  run each new or extended entry by name (a new case in an existing entry is identified by its
  `condition false:` line). Where this document says a test
  fails before at runtime, the build must succeed and the test must fail on the check this document
  names, the quoted `condition false:` line or `B3_ASSERT` message naming it, not on a fixture check.
  Where it says the test fails before by not building, the build failure and its first error line
  are the evidence. Then run the same entries in the working tree, where each passes. The closure
  doc quotes both runs.
- The boundary commits H2, H3a, H3b and H3c change no behaviour. Their evidence is threefold: the
  whole suite and every golden of §5 unchanged; the `HistoryCrossWorkerTest` goldens recorded in H1
  unchanged; and a compile-boundary check, a scratch translation unit per commit in the scratch
  directory, never committed, whose statements compile against `src/` at the previous commit and
  fail to compile at the commit with the diagnostic this document names, plus, where the commit
  introduces the const getter, statements that fail at the commit alone. The closure doc quotes
  every diagnostic.
- Touch only the files each increment lists in §4, except that the boundary commits also touch
  any `src/` or `test/` file the compiler names once the types change: the designs make the compiler
  the judge of when the migration is complete (the design §5.2), so a file the compiler names is in
  scope by construction and is listed in the closure doc. Any other file is a deviation: stop and
  report rather than touch it, except the closure doc of §8.
- Names in §4 are suggestions; the contracts are not. C17, no warnings, `clang-format` with the
  repo's `.clang-format` applied to the changed hunks only (a whole-file reformat is a deviation).
  Comments in human words; Doxygen `///` on the public additions (`b3HistoryDef`, `b3HistoryInfo`,
  `b3HistoryResult` and the seven functions of the design §14), as their neighbours have.
- Do not add a validator, flag, fallback, counter, generation check or scalar the designs do not
  name, with the two exceptions §2 states and §10 records: the `bool` that marks a player's world
  and the `bool` that marks the inside of a step, each read by assertions only. An assertion of a
  construction-guaranteed invariant is welcome; it is never control flow (WORKFLOW AC-4).
- Journaling, capture and the hot accessors' assertions run only while history is enabled or inside
  a step; a world that never enables history pays one null check per journal call and nothing
  else (the design §7.3). §7's benchmark comparison shows it.
- Every deviation, pre-existing failure and out-of-scope observation goes in the closure doc (§8),
  plainly, with no fix attempted.

## 4. Increments

The new test file is `test/test_history.c`, created in H0 and extended by each later commit, one
entry function per test, registered in `test/main.c` by the commit that adds it, between
`HeightFieldTest` and `HullTest` (every entry here is named `History...`). It includes the internal
headers as `test/test_awake_tree.c` does and carries its own copies of the helpers it needs
(`HashWorldState` over named bodies; a full-rebuild perturbation that, from H4, calls the broad
phase's rebuild function rather than `b3DynamicTree_Rebuild` on the tree storage); copy, do not
move: the originals stay.

Two internal pieces of test machinery are shared by several entries and are written in H1:

- **The op log.** A test-side recorder of the API calls a scene makes per tick: each entry is a
  tagged union (body create with its def, shape create with its def and geometry, joint create,
  destroy, each setter the churn uses, `b3World_Explode`, `b3World_SetUserData`, a callback context
  change) with the tick it was made in; `ReplayOps( worldId, log, tick )` re-issues the calls of
  one tick in order. Ids are recorded as creation ordinals and resolved through the ids the replay
  obtains, since a replayed creation gives the same id (the design §9).
- **The event digest.** A per-tick hash over the step's events in reported order, each event by
  every named field of its public struct (`include/box3d/types.h`), never its padding: body move
  events (`userData`, transform, body id, `fellAsleep`), contact begin and end (shape ids, contact
  id), hit events (shape ids, contact id, point, normal, speed, the two user material ids), sensor
  begin and end (sensor and visitor shape ids), joint events (joint id, `userData`), read through
  the public getters after each step. The design §12 test 3 compares events by type, contents and
  order; the digest is that comparison.

### H0. State hash

**Files.** `include/box3d/box3d.h`, new `src/world_hash.c`, `src/CMakeLists.txt`, new
`test/test_history.c`, `test/CMakeLists.txt`, `test/main.c` (register `HistoryHashTest`).

**Commit message.** `Add b3World_ComputeStateHash, a field-wise hash of engine state`

**Code.** The design §12 test 1 and §14 are the specification. Mechanics that follow from them:

- `uint64_t b3World_ComputeStateHash( b3WorldId )` is FNV-1a 64 with the mixers `b3HashWorldState`
  uses (`B3_SNAP_FNV_INIT`, `B3_SNAP_FNV_PRIME`, `b3FnvMixPosition` for positions, so a double build
  mixes doubles), over the following, in this order, each value by its bit pattern, each count
  mixed before its elements:
  1. the simulation scalars of the design §5.1's last row (the loose fields in H0, the struct from
     H2);
  2. the six id pools in the order `bodyIdPool`, `shapeIdPool`, `contactIdPool`, `jointIdPool`,
     `islandIdPool`, `solverSetIdPool`, then the four trees' proxy pools in tree-index order, each as
     its allocation order: the free ids in the order they will be handed out (for `b3IdPool`, the
     `freeArray` from its last element to its first; for a tree, the `proxyFreeList` chain from its
     head), with a trailing run of consecutive ascending ids that ends at the last capacity slot
     dropped and replaced by the run's first id, or the capacity when there is no such run;
  3. bodies in id order over every slot: each slot's `generation`, and for a live slot (`id` equals
     the index) every field except `userData` and `bodyMoveIndex`;
  4. shapes in id order over every slot: `generation`, and for a live slot every field except
     `userData`, `userShape`, `materials` and the geometry pointer, then the `materialCount`
     materials (the inline one or the block's), the `fatAABBs` entry, and the geometry's content
     hash: a sphere's or capsule's fields, a hull's, mesh's or height field's stored `hash`, and a
     compound's hash computed over its `version`, materials, its capsule, hull, mesh and sphere
     instance arrays with their transforms in stored order, each embedded hull's and mesh's content
     hash and each mesh instance's material index array, excluding its tree;
  5. contacts in id order over every slot: `generation`, and for a live slot every field except
     `manifolds`, then the `manifoldCount` manifolds field by field (points, normal, impulses), and
     for a mesh contact the triangle cache's `count` elements and `queryBounds`, for a convex
     contact the cache;
  6. joints in id order over every slot: `generation`, and for a live slot every field except
     `userData`;
  7. islands in id order: for a live slot the record's scalars and the three link arrays in stored
     order;
  8. solver sets in index order: `setIndex`, then `bodySims` (every named field of `b3BodySim`),
     `bodyStates` (velocities, delta position, delta rotation, flags; never the padding fields),
     `jointSims` (every named field of the sim and of its type's union member), `contactIndices`,
     `islandSims`, each in stored order;
  9. graph colours in index order: `bodySet` as the ascending indices of its set bits (never block
     counts), `jointSims`, `convexContacts`, and `contacts` by each spec's contact id
     (`manifoldStart` is per-step scratch, the design §5.1);
  10. sensors in stored order: `shapeId` and `overlaps2` (visitor shape id and generation) in stored
      order; `hits` and `overlaps1` are scratch;
  11. the pair set as the sum over its keys of a per-key mix (bucket order is not state);
  12. the hull database as the sum over its entries of a mix of the hull's stored `hash` and its
      refcount;
  13. the broad phase: for each tree in index order, for each live proxy in proxy-id order the tree
      index, proxy id, category bits, shape id and leaf box read from the tree; then the tree's moved
      proxies as ascending ids (gathered by `b3DynamicTree_GatherMovedProxies`, then sorted); then the
      migration queue in stored order.
  Excluded by the design: padding, addresses, `bodyMoveIndex`, every `userData` word, `userShape`,
  `stepIndex`, `maxCapacity`, the profile and counters, the event arrays and end-event buffers, the
  names cache, task contexts and every scratch array. Pointers are never hashed, only pointee
  contents. `b3HashWorldState` is not touched.
- The function takes the world through `b3GetWorldFromId` and reads only; it is callable between
  steps on any world.

**Test** `HistoryHashTest`, four cases on fresh worlds, one worker unless stated:

1. `HashRepeatableCase`: a scene of forty hull boxes dropped onto a `b3CreateGridMesh` ground
   beside a kinematic box with a small velocity and one sensor sphere, sleep on, gravity on, stepped
   240 ticks at `workerCount` 1 and again at 4 (a third run at 1 repeats the first): the hash after
   every tick is equal across the three runs, and some bodies have slept by the end
   (`b3Body_IsAwake` false), so sleeping sets, the sleeping tree and the graph are all covered.
2. `HashSensitiveCase`: starting from one scene (a sleeping box, a static box, a dynamic hull body,
   a sensor, a joint, two shapes sharing one hull's content), each of the following, applied to a
   fresh copy of the scene, changes the hash from the scene's: `b3Body_SetTransform` on the sleeping
   body; `b3World_SetGravity`; `b3Shape_SetSurfaceMaterial` on the static shape;
   `b3Shape_SetFilter`; destroying and recreating a body with the same def (its generation differs);
   destroying one of the two hull shapes (the database's refcount differs); destroying the sensor;
   `b3Body_SetAwake( true )` on the sleeping body. Each of these leaves it unchanged: `b3Body_SetUserData`,
   `b3Shape_SetUserData`, `b3Joint_SetUserData`, `b3World_SetUserData`.
3. `HashLayoutCase`: the scene of case 1 after 120 ticks, hashed, then every tree fully rebuilt by
   the perturbation helper, hashed again: equal (a rebuild changes no leaf box, proxy id or moved
   bit, the trees doc §8).
4. `HashRoundTripCase`: the scene of case 1 recorded through `b3World_StartRecording` for 120
   ticks, the hash and `broadPhase.migrationQueue.count` recorded after each tick, and played back
   in a `b3CreatePlayer` world: at every frame whose recorded queue count is 0 (the queue is the one
   thing the hash reads that the serializer does not write, §2) the player world's hash after the
   frame equals the original's, and still does after `b3RecPlayer_SeekFrame` to the qualifying
   frames nearest 60 and 119; `ENSURE` at least 100 of the 120 frames qualify. A mismatch at a
   qualifying frame is the serializer failing to restore something the design's phase 0 relies on:
   stop and report with the first differing section of the walk above (log each section's partial
   hash in a scratch build to find it). The queue's own copy beside the image is H1's and is shown
   by `HistoryRewindTest`'s parked-proxy case.

Fails before H0 by not building: the function does not exist. The closure doc quotes the first
error.

### H1. Phase-0 ring and the public API

**Files.** `include/box3d/box3d.h`, `include/box3d/types.h`, new `src/world_history.h`, new
`src/world_history.c`, `src/physics_world.h`, `src/physics_world.c`, `src/replay.h`, `src/replay.c`,
`src/recording.c`, `src/world_snapshot.h`, `src/world_snapshot.c`, `src/core.c`, `src/CMakeLists.txt`,
`test/test_history.c`, `test/main.c` (register `HistoryRingTest`, `HistoryRewindTest`,
`HistoryReplayTest`, `HistoryCrossWorkerTest`).

**Commit message.** `Add world history: enable, rewind, info and restorable ticks over a ring of serialized images`

**Code.** The design §13 phase 0, §14, §9 steps 1, 5 and 6, §10's recording rule, and the ring doc
in full are the specification. Mechanics that follow from them:

- Public types in `include/box3d/types.h` beside `b3WorldDef`: `b3HistoryDef`, `b3HistoryInfo`,
  `b3HistoryResult` as the design §14 writes them, with its comments as `///` lines;
  `b3DefaultHistoryDef` returns `tickCount` 60, `captureInterval` 1, `maxBytes` 0. The seven
  functions in `include/box3d/box3d.h` after the recording API.
- The ring lives in `src/world_history.c` behind an incomplete `b3WorldHistory` held by
  `world->history` (NULL when disabled) with `uint64_t historyTick` on the world. It holds the arena
  (one `b3Alloc`'d block), the write position, the slot directory (per retained slot: tick, offset,
  byte length, charge, whether imaged; a growable array in tick order whose first entry is the oldest
  retained slot), the effective interval, the tick of the last imaged slot, `bytesUsed`,
  `pendingBytes`, and the phase-0 serializer state below. `src/world_history.h` declares, for the
  tests, two read-only functions over the directory: `b3History_GetSlotCount( world )` and
  `b3History_GetSlot( world, index )` returning the slot's tick, imaged flag, charge and the charge's
  parts (image bytes, journal bytes, owned-block bytes, the last two zero in phase 0) in a small
  struct; the tests and §7's harness read nothing else of the ring.
- A phase-0 slot is a header, the `b3SerializeWorld` image written into a `b3RecBuffer` with the
  ring's own `b3Recording` as the registry and the user-words option on, the migration queue's
  count and ids copied after it, and an empty journal segment. Its charge is the header, the
  directory entry and those bytes (the ring doc §6; no entry owns a block in phase 0).
- Capture (the ring doc §6 steps 1 to 4, the design §6's capture interval): in `b3World_Step` after
  `b3ValidateProxyTrees` and before `locked = false`, when `world->history` is non-NULL:
  `historyTick += 1`; decide whether this tick is imaged (at least the effective interval has passed
  since the last imaged tick); apply the window bound (the ring doc §7); size the slot (a non-imaged
  tick's slot is a header and the empty segment in phase 0, so it is a few bytes); apply the budget
  steps 2 to 4 with the `min( 2 × interval, tickCount )` widening tested as `interval ≤ tickCount −
  interval`; evict from the old end, oldest first, each imaged slot together with the non-imaged
  slots after it up to the next imaged slot, never into the minimum window; grow the arena by
  allocating the larger block, copying the retained slots in tick order from offset 0, rewriting the
  directory's offsets and freeing the old block, when the retained slots and the new one do not fit;
  write the slot. An imaged slot is sized by a first `b3SerializeWorld` run into a `b3RecBuffer`
  with `countOnly` set, which tallies the exact byte count without writing (`src/recording.h`); the
  budget and window steps use that count, the arena is grown if the slot does not fit, and a second
  run serializes into a `b3RecBuffer` over the reserved region whose `capacity` is that count, so
  `b3RecBufAppend` never grows it.
- The ring's registry and reader: a `b3Recording` whose registry the capture interns new geometry
  into (`b3InternGeometry`, deduplicated by content, so a scene that creates the same mesh twice
  adds one entry), and a `b3RecReader` with `replayWorldId` set to this world whose `slots` are
  appended for the entries added since the last capture, as `b3RecSeedKeyframeRegistry` does once;
  entries are never freed while history is enabled, since any retained image may reference them,
  and both are freed by disable. Their bytes are not in `bytesUsed`, `arenaCapacityBytes` or
  `stagingCapacityBytes`; the closure doc reports them per scene (§8), and H5 removes them.
- `b3World_EnableHistory`: takes the world through `b3GetUnlockedWorldFromId`; asserts
  `world->history == NULL`, `def->tickCount >= 1`, `1 <= def->captureInterval <= def->tickCount`,
  `broadPhase.traversalDepth == 0`, `world->recording == NULL`, the player mark clear, and that
  `contactEndEvents[endEventArrayIndex]` and `sensorEndEvents[endEventArrayIndex]` are empty (the
  ring doc §8); allocates the ring; sets `historyTick` 0; writes the slot for tick 0 as the world
  stands, imaged, through the capture path's slot writer.
- `b3World_DisableHistory`: no-op when disabled; otherwise frees the arena, the directory, the
  registry and reader and the staging state, and nulls `world->history`. `b3DestroyWorld` calls it
  immediately after `b3StopRecordingInternal`.
- `b3World_GetHistoryInfo`: all zero when disabled; else `currentTick` = `historyTick`,
  `oldestImageTick` = the oldest retained imaged slot's tick, the effective interval, `bytesUsed`,
  the arena's capacity, 0 for the staging capacity and `pendingBytes` in phase 0 (H4 fills them).
- `b3World_GetRestorableTick( worldId, tick )`: `UINT64_MAX` when disabled or when no retained imaged
  slot has a tick at or below `tick`, else the newest such tick; the rule is the same above
  `historyTick`.
- `b3World_Rewind( worldId, T )`: when disabled returns `b3_historyTickUnavailable` without touching
  the world; otherwise takes the world through `b3GetUnlockedWorldFromId`, asserts
  `traversalDepth == 0`, locks the world, and runs the design §9: step 1 returns
  `b3_historyTickUnavailable` (after unlocking) when T is not a retained imaged tick or is later
  than `historyTick`; step 2 is empty in phase 0; step 3 is `b3DeserializeIntoShell` of slot T's
  image through the ring's reader with the release-user-shapes option on, then the migration queue
  copied back into `broadPhase.migrationQueue`; step 4 is empty (the trees are in the image); step 5
  clears `bodyMoveEvents`, `sensorBeginEvents`, `contactBeginEvents`, `contactHitEvents`,
  `jointEvents` and both `sensorEndEvents` and both `contactEndEvents` buffers, sets `bodyMoveIndex`
  to `B3_NULL_INDEX` on every body of the awake set, removes the slots after T (the write position
  moves back to the end of slot T, the directory is truncated, their charges leave `bytesUsed`) and
  sets `historyTick = T`; step 6 runs `b3ValidateSolverSets`, `b3ValidateContacts`,
  `b3ValidateConnectivity`, `b3ValidateBroadPhase`, `b3DynamicTree_Validate` on the static tree and
  `b3ValidateProxyTrees` in the validation build. Unlock; return `b3_historyOk`.
- Serializer options (`src/world_snapshot.c`): `B3_SNAP_VERSION` becomes 12 with a comment naming
  the user words; a header flag `B3_SNAP_FLAG_USER_WORDS` under which the writer keeps the body,
  shape and joint `userData` words in its copies and the reader restores them (the recording passes
  it off and the reader then nulls them as today); a reader parameter `releaseUserShapes` under which
  `b3DesShapes` releases every saved handle through `destroyDebugShape` and carries none, and
  otherwise keeps today's carry-over. `b3SerializeWorld` and `b3DeserializeIntoShell` grow one
  parameter each; `src/recording.c` and `src/replay.c` pass the recording's values.
- Mutual exclusion (the design §10): `b3World_StartRecording` asserts `world->history == NULL`
  before it starts; `b3RecPlayerCreateWorld` sets the player mark on the world it creates;
  `b3RecPlayer_SeekFrame` and `b3RecPlayer_Restart` assert the player world's `history == NULL`.
- `src/core.c`: `b3Alloc` adds `(int64_t)size` to the byte count.
- Phase 0 is O(world) per imaged tick and allocates; nothing here is measured against the design
  §12 test 5, which H5 and H6 meet. §7's phase-0 measurement is informational.

**Tests.** All in `test/test_history.c`, one worker unless stated, sleep on.

- `HistoryRingTest` (the ring doc §9 test 1, every item a phase-0 ring can show). The scene is a
  pile of twelve awake hull boxes on a static ground beside eight boxes created asleep, a sensor and
  one hull shape referenced by a single body, with the op log recording the churn. The invariant
  check run after every operation below: for every tick from 0 to `historyTick + 5`,
  `b3World_GetRestorableTick` returns the newest retained imaged tick at or below it as the test
  tracks it from the directory's imaged flags, or `UINT64_MAX` when none;
  `b3World_GetHistoryInfo().bytesUsed` equals the sum of the retained slots' charges read through
  `b3History_GetSlot`; `b3World_Rewind( historyTick + 1 )` returns
  `b3_historyTickUnavailable` and leaves the hash unchanged. Cases, each on a fresh world created
  after `int64_t base = b3GetByteCount();` and each ending with `b3World_DisableHistory`,
  `b3DestroyWorld` and `ENSURE( b3GetByteCount() == base )`, except case 6, which ends with
  `b3DestroyWorld` alone (its own disable call) and the same check:
  1. unbounded budget, `tickCount` 8, interval 1: after 20 steps the retained imaged ticks are 12
     to 20 (9 slots), and `oldestImageTick` is 12;
  2. a budget: read `s`, the charge of tick 0's imaged slot after the enable, on a world of this
     scene with no churn; `maxBytes` = `s` + `s` / 2 (one image fits, two do not), `tickCount` 32,
     interval 1: the minimum window is over budget at every tick an image is due, so
     `effectiveInterval` reads 2 after step 1, 4 after step 2, 8 after step 4, 16 after step 8 and
     32 after step 16 and every later step through step 200; the imaged ticks are 0, 32, 64, 96,
     128, 160 and 192 (read from the directory), `bytesUsed > maxBytes` whenever two imaged slots
     are retained, and `oldestImageTick` follows the window bound (0 through step 32, 32 through
     step 64, and so on);
  3. a single slot over budget: `tickCount` 8, interval 1, `maxBytes` = `s` / 2: the interval widens
     as in case 2 up to 8, every step's slot is written, `bytesUsed > maxBytes` at every step,
     `b3World_GetRestorableTick( historyTick )` is never `UINT64_MAX`, and `Rewind` to it restores
     the hash recorded there;
  4. a minimum window over budget: `tickCount` 4, interval 1, `maxBytes` = `s` + `s` / 2: after
     step 4 ticks 0 and 4 are imaged and retained together, `bytesUsed > maxBytes`, and a rewind to
     either works; after step 5 tick 0 is gone;
  5. a sole image under pressure: `tickCount` 1, `captureInterval` 1, `maxBytes` = `s` / 2: tick 0's
     image is retained until the next image is written, and `Rewind( 0 )` works after one step;
  6. a rewind to each retained imaged tick in turn from the newest to the oldest, on the unbounded
     ring after 20 steps with churn: each returns `b3_historyOk` and the hash equals the hash
     recorded at that tick, and the slots after it are gone (`currentTick` equals it);
  7. a rewind to tick 10, five more steps, then a rewind to tick 13 (a re-stepped tick): hash
     equals the hash recorded at the re-stepped 13, not the original 13 (the churn differs by design
     of the test, which skips one op on the re-step);
  8. two rewinds with no step between (to 15 then to 12): both `b3_historyOk`, hashes as recorded;
  9. `Rewind( P )` after `b3Body_ApplyLinearImpulse`, `b3Body_ApplyForce` and `b3Body_ApplyTorque` on
     an awake body with no step: the hash returns to the hash recorded at P;
  10. a rewind with no step since a body create, a body destroy, `b3Body_SetTransform` on a sleeping
      body, a sensor destroy and the hull shape's destroy (the hull's last reference), each on its
      own fresh step: each `Rewind( P )` returns the hash to the one recorded at P.
  Fails before H1 by not building.
- `HistoryRewindTest` (the design §12 test 2). For each of the seventeen scenes of
  `benchmark/main.c`'s table (§2), created with its capacity
  function where one exists and `workerCount` 4: step to a steady state (the scene's benchmark step
  count capped at 200, with its `Step*` where one exists), enable history with `tickCount` 60 and
  interval 1, then for N random (T, P) pairs, seeded per scene, each drawn after the previous
  rewind with P in (current tick, current tick + 60] and T in [max( 0, P − 60 ), P], so T is
  retained at P: step from the current tick to P with the churn below applied by the seed between
  steps and recorded in
  the op log, recording the hash and the `userData` words of every body, shape and joint at every
  tick; `Rewind( T )`; `ENSURE( hash == hash[T] )` and the words equal those recorded at T; then
  step on to a fresh P. N is 1,000 when the environment variable `BOX3D_HISTORY_SOAK` is set and 25
  otherwise; the closure doc quotes the soak run. The churn draws from: body create (awake and
  asleep) and destroy; `b3Shape_SetFilter` on an awake and a sleeping dynamic body's shape, a
  kinematic body's and a static shape, with `invokeContacts` false (the proxy untouched) and
  true (the proxy recreated, §2); `b3Body_SetType` to kinematic and back on a sleeping body; `b3Body_SetType` and
  destroy of a sleeping body; `b3Body_SetAwake( false )` immediately after each rewind;
  `b3World_SetUserData` (the test sets it back itself before comparing, since it is host state,
  the design §10); setters on sleeping bodies (`SetTransform`, `SetLinearVelocity`);
  `b3World_Explode`; a distinct `userData` word per body, shape and joint, changed by setters during
  the churn; creation and destruction of the world's only compound shape; hull shape creation,
  destruction and deduplicated creation; sensor shape creation and destruction; friction,
  restitution, surface and mesh material writes on awake, sleeping and static shapes, single- and
  multi-material; and the
  structural cases the design lists (a touching contact's manifold count changing across a sleep
  and wake; a contact creation that rewrites an awake neighbour's edge followed by a step that
  changes that neighbour's manifold count; a multi-material shape destroyed and a shape with a
  different material count created in its slot, and the inline-to-block case; a convex contact
  destroyed and a mesh contact created in its slot and the reverse; a contact with no block woken,
  touching and rewound; a shape whose geometry is replaced after T (`b3Shape_SetHull`) and
  debug-drawn before and after a rewind with `b3World_Draw` under test callbacks that count
  creates and destroys; a step that grows a tree's proxy capacity followed by a rewind across it;
  a sleeping body destroyed between steps whose sleeping-tree id is taken by a body falling asleep
  in the next step, rewound across;
  steps with two different time steps followed by a rewind and `b3Joint_GetConstraintForce` before
  any step; a body put to sleep in the imaged tick while its proxy is still marked, whose pairs
  the next step must find; a shape created on a sleeping body with a rewind before and after the
  step that moves it asleep; a sleep and wake of an island of at least sixteen bodies, the bulk
  placement, with a rewind across each; a rewind whose undone wake takes the sleeping tree past
  `B3_SLEEPING_TREE_MAX_HEIGHT`; a bullet whose leaf outgrew its fat AABB entry before it slept,
  woken after T and rewound). Each structural case is a named sub-scene the churn can schedule, so
  a failure names the case. History is disabled before the scene's `Destroy*` runs. Fails before H1
  by not building.
- `HistoryReplayTest` (the design §12 test 3). On three scenes (the pile of `HistoryRingTest`; the
  `sleep` benchmark scene; a scene with a mesh ground, a compound, a sensor and two joints), with a
  `preSolve` callback that rejects a contact when the shape's `userData` word is odd and a custom
  filter that reads a context struct's field, both registered; history enabled with `tickCount` 90
  and interval 1, 90 ticks stepped
  with churn recorded in the op log and the per-tick hash and event digest recorded, the world
  `userData` and the filter context's field changed at tick 50; then for T in { 30, 60, 89, 0 }:
  `Rewind( T )`, set the world `userData` and the context field back to their values at T, replay
  the op log from T+1 to 90 with the change re-applied at tick 50, and `ENSURE` after every
  replayed step that the hash and the event digest equal the recorded ones for that tick and that
  every body, shape and joint `userData` word equals the recorded one. Then the corrected replay: a
  rewind to 60 with one op dropped, replayed to 90, differing from the record at and after the
  first tick its results differ (asserted only as "differs somewhere", the regenerated events being
  the corrected timeline's). Fails before H1 by not building.
- `HistoryCrossWorkerTest` (the design §12 test 4). The three scenes of `HistoryReplayTest` run
  under `HistoryReplayTest`'s protocol with capture at `workerCount` 1 and replay at 4, and capture
  at 4 and replay at 1: equal hashes and digests at every replayed tick. Then the goldens: a fourth
  scene, `HistoryGoldenScene` in the test file, deterministic and fixed (forty hull boxes and ten
  spheres on a wave mesh, a height field, a compound, two sensors, a chain of three joints, bodies
  created asleep and woken by a falling box, two bodies destroyed and recreated at tick 30, a
  filter change at tick 40), stepped 120 ticks with history enabled at `tickCount` 120 and interval
  4, rewound to 100
  and replayed to 120: `b3World_ComputeStateHash` at ticks 60, 100 (before the rewind), 100 (after
  it) and 120 (after replay) pinned per precision as `HISTORY_GOLDEN_HASH_60`, `_100`, `_120` under
  `ENSURE_GOLDEN`'s condition, recorded on this platform in this commit; the two values at 100 are
  asserted equal unconditionally. The cross-platform half is the owner's Windows run, recorded in a
  separate doc as `20261014-004` was; the closure doc names the Linux values. Fails before H1 by not
  building.

### H2. World scalars and const-returning sim getters

**Files.** `src/physics_world.h`, `src/physics_world.c`, `src/body.h`, `src/body.c`, `src/joint.h`,
`src/joint.c`, `src/distance_joint.c`, `src/motor_joint.c`, `src/parallel_joint.c`,
`src/prismatic_joint.c`, `src/revolute_joint.c`, `src/spherical_joint.c`, `src/weld_joint.c`,
`src/wheel_joint.c`, `src/solver_set.h`, `src/solver_set.c`, `src/shape.c`, `src/island.c`,
`src/solver.c`, `src/recording.c`, `src/world_snapshot.c`, `src/world_hash.c`, `src/CMakeLists.txt`,
`test/CMakeLists.txt`, and any `src/` or `test/` file the compiler names (§3).

**Commit message.** `Hold the simulation scalars in one struct behind a const pointer and make the sim getters const-returning`

**Code.** The design §5.2 (World scalars; Records, the sim getters) is the specification. Mechanics
that follow from it:

- `b3WorldScalars` in `src/physics_world.h` with exactly the fields of the design §5.1's last row;
  `world->scalars` is `const b3WorldScalars*`, allocated in `b3CreateWorld`, freed in
  `b3DestroyWorld`, assigned nowhere else. `b3World_WriteScalars( world, const b3WorldScalars* )` in
  `src/physics_world.c` copies the struct in and is the only converter of the pointer; the
  serializer's reader and, from H5, the image copy go through it. Every writer of one of those
  fields takes a local copy, changes it and writes it back: `b3World_SetGravity` and the other
  public setters, `b3World_EnableSleeping` and the other enable setters, the step's `inv_h`/`inv_dt`, the
  end-event flip, `splitIslandId` at its four sites and `compoundShapeCount` at its three (§2).
  Every reader reads through the pointer.
- `const b3BodySim* b3GetBodySim`, `const b3BodyState* b3GetBodyState`, `const b3JointSim*
  b3GetJointSim`, `const b3JointSim* b3GetJointSimCheckType`. Writers use `b3SetBodySim( world,
  body, const b3BodySim* )`, `b3SetBodyState( world, body, const b3BodyState* )` and
  `b3SetJointSim( world, joint, const b3JointSim* )` in `src/solver_set.c`, which locate the element
  as the getters do and copy the value in (H4 makes them the journaled `set` of the set's array and
  the awake-array write). A joint setter copies the sim, changes the field and sets it back. The
  step's stages are unchanged in this commit: they index the awake set's and the colours' arrays by
  base pointer, which H4 replaces.
- `src/CMakeLists.txt` and `test/CMakeLists.txt`: the discarded-qualifier diagnostic is an error on
  the three compiler families (§2).

**Evidence.** The suite in both precisions; every golden of §5 unchanged; the `HISTORY_GOLDEN_HASH_*`
values unchanged. Compile-boundary check: a scratch unit including `physics_world.h`, `body.h` and
`joint.h` with the statements `b3BodySim* sim = b3GetBodySim( w, b );`,
`b3GetJointSimCheckType( id, b3_revoluteJoint )->constraintHertz = 1.0f;` and `w->gravity.y = 0.0f;`
compiles at H1 and fails at H2 with the discarded-qualifier error for the first, a read-only-object
error for the second and an unknown-member error for the third.

### H3a. Body and shape write accessors

**Files.** `src/body.h`, `src/body.c`, `src/shape.h`, `src/shape.c`, new `src/body_array.h` and
`src/shape_array.h` (the storage headers, each included by its own file only),
`src/CMakeLists.txt`, `src/physics_world.h`, `src/physics_world.c`, `src/solver.c`, `src/solver_set.c`, `src/island.c`,
`src/contact.c`, `src/joint.c`, `src/sensor.c`, `src/broad_phase.c`, `src/world_snapshot.c`,
`src/world_hash.c`, `test/test_awake_tree.c`, `test/test_tree_order.c`, `test/test_broad_phase.c`,
`test/test_world.c`, `test/test_body.c`, `test/test_restitution.c`, `test/test_history.c`, and any
file the compiler names (§3).

**Commit message.** `Write body and shape records through one accessor each; the records are const outside their files`

**Code.** The design §5.2 (Records; Owned blocks; Containers, the record-array rule) and §9 step 3's
field exclusions are the specification. Mechanics that follow from them:

- The record arrays `bodies` and `shapes` with `fatAABBs` leave `b3World`'s struct: `world->bodies`
  and `world->shapes` become pointers to incomplete types (`b3BodyArray`, `b3ShapeArray`) whose
  definitions sit in the two new headers, allocated by `b3CreateWorld` through a create function of
  each file and destroyed by a destroy function. Outside its file a record array exposes
  `const b3Body* b3GetBodyRecord( world, id )` (and `b3GetBodyFullId` returning const), the count,
  and nothing writable; the push of a zeroed element at a bump allocation and the truncation to a
  length that undoes it are two functions of the file, which H4's slot pool calls.
- Three routes per record, all in the record's file (the design §5.2): the write accessor with its
  two entries, `b3Body_Write( world, const b3Body* )` (structural, journals from H4) and
  `b3Body_WriteSetter( world, const b3Body* )` (journals from H4 unless the owner is awake), which
  copy every field of the value into storage; the hot accessor `b3Body* b3Body_Hot( world, id )`,
  asserting the step mark and `setIndex == b3_awakeSet`, returning the record for finalize's
  `sleepTime`, `sleepVelocity`, flags and `bodyMoveIndex` writes; and the restore writer
  `b3Body_Restore( world, id, const b3Body* )`, asserting the rewind or deserialize mark, used by
  the serializer's reader now and by the undo and the image scatter from H4 and H5. For shapes the
  same three, with `b3Shape_Write( world, const b3Shape*, const b3AABB* fatAABB )` taking the record
  and its `fatAABBs` entry together, copying every field but `materials`, `materialCount` and
  `userShape`; the hot accessor returning the awake shape's `aabb` and `fatAABBs` entry pointers and
  never the record (the design §5.2); `b3Shape_Restore` taking the same two values and releasing a
  live `userShape` through `destroyDebugShape` before clearing it (the design §10). The serializer's
  reader with `releaseUserShapes` off keeps its carry-over around that writer: before any restore
  write it saves every live slot's handle and detaches each with `b3Shape_SetUserShape( world, id,
  NULL )`, so the restore writes release nothing; after them it puts a saved handle back through
  `b3Shape_SetUserShape` where the slot is live with the same generation and releases each
  remaining saved handle once, as today's sweep does. With the option on it saves nothing and the
  restore writes release.
- `const b3SurfaceMaterial* b3GetShapeMaterials( const b3Shape* )`; `shape->materials` is
  `const b3SurfaceMaterial*`. `b3Shape_WriteMaterial( world, id, index, const b3SurfaceMaterial* )`
  in `src/shape.c` is the one material writer: for an inline material it builds the record with the
  new material and enters `b3Shape_WriteSetter`; for a block it writes the element (journals from
  H4 whatever the owner's state). `b3Shape_SetFriction`, `b3Shape_SetRestitution`,
  `b3Shape_SetSurfaceMaterial` and `b3Shape_SetMeshMaterial` call it.
  Block allocation and release stay in `b3CreateShapeInternal` and `b3DestroyShapeAllocations`
  until H4 moves the release into the pool free entry.
- `b3Shape_SetUserShape( world, id, void* )` in `src/shape.c` is the one `userShape` writer; the
  debug draw's assignment in `src/physics_world.c` and the serializer's detach and put-back go
  through it.
- The step mark: `world->inStep`, a `bool` set at `b3World_Step`'s entry and cleared at its exit,
  read by the hot accessors' assertions only (§3).
- Every direct assignment to a body or shape field elsewhere (`src/body.c`'s setters,
  `src/solver_set.c`'s transfers and sleep and wake, `src/island.c`'s link and unlink,
  `src/contact.c`'s edge writes, `src/joint.c`'s edge writes, `src/shape.c`'s creation and setters,
  `src/sensor.c`'s `sensorIndex` fix-up, `src/broad_phase.c`'s parked and moving flags through the
  structural entry, as the trees doc §3 states) becomes a local copy, the change, and a write
  through the accessor. A function that writes a body's record and its sim makes two writes (the
  design §5.2). Tests that read internals read through the const getters.

**Evidence.** As H2. Compile-boundary check: `world->bodies.data[0].mass = 1.0f;` and
`b3GetShapeMaterials( s )[0].friction = 0.0f;` compile at H2 and fail at H3a (no member `data` on
the pointer to the incomplete type; a read-only object); `b3Body* b = b3GetBodyRecord( w, 0 );` and
`b3Shape* sh = b3GetShapeRecord( w, 0 );` fail at H3a with the discarded-qualifier error and have
no before run, the getters being new.

### H3b. Contact write accessor, manifold blocks and mesh caches

**Files.** `src/contact.h`, `src/contact.c`, new `src/contact_array.h`, `src/container.h`,
`src/CMakeLists.txt`, `src/mesh_contact.c`, `src/contact_solver.c`, `src/physics_world.h`, `src/physics_world.c`, `src/constraint_graph.c`,
`src/island.c`, `src/solver_set.c`, `src/body.c`, `src/shape.c`, `src/world_snapshot.c`,
`src/world_hash.c`, `test/test_history.c`, and any file the compiler names (§3).

**Commit message.** `Write contact records through one accessor; manifold blocks and mesh caches are bound by their own mutators`

**Code.** The design §5.2 (Records; Owned blocks) and §7.1's manifold block and cache entries are
the specification. Mechanics that follow from them:

- `world->contacts` becomes a pointer to the incomplete `b3ContactArray`; `const b3Contact*
  b3GetContactRecord( world, id )` outside `src/contact.c`. `b3Contact_Write` and
  `b3Contact_WriteSetter` copy every field except `manifolds`, `manifoldCount` and the triangle
  cache's array header (a convex contact's cache union member and a mesh contact's `queryBounds`
  are copied, the design §9 step 3); `b3Contact_Restore` likewise.
- `contact->manifolds` is `const b3Manifold*`; the triangle cache is a const-data array type
  `b3ConstArray( b3TriangleCache )` declared in `src/container.h` with `b3Array`'s layout and a
  `const T* data`. The block mutator `b3Contact_SetBlocks( world, id, manifoldCount, manifolds,
  cacheCount, cache )` in `src/contact.c` is the one writer of the pointer, the count and the cache
  header for a non-awake contact (journals from H4).
- The hot accessor `b3Contact* b3Contact_Hot( world, id )` asserts the step mark and an awake
  owner and returns the record together with writable access to its manifold block and, for a mesh
  contact, a writable `b3Array( b3TriangleCache )*` view of the cache; the narrow phase
  (`src/contact.c`, `src/mesh_contact.c`), the collide pass's flag writes (`src/physics_world.c`) and
  store impulses (`src/contact_solver.c`) use it, allocating and freeing manifold blocks in place
  through `b3AllocateManifolds` and `b3FreeManifolds` as today.
- `b3DestroyContact` no longer frees the manifold block or the cache: they stay on the record for
  the pool's free (H4's pool free entry); in this commit the pool's free function in
  `src/contact.c` frees them, so no leak is introduced between H3b and H4.
- The structural writers of a contact record (`b3CreateContact`, `b3DestroyContact`, the graph's
  add and remove, island link and unlink, the set transfers, the begin and end touch processing)
  go through the accessor.

**Evidence.** As H2. Compile-boundary check: `world->contacts.data[0].flags = 0;` compiles at H3a
and fails at H3b (no member `data` on the pointer to the incomplete type);
`b3Contact* c = b3GetContactRecord( w, 0 );` and `b3Manifold* m = b3GetContactRecord( w, 0 )->manifolds;`
fail at H3b with the discarded-qualifier error and have no before run.

### H3c. Joint, island, solver-set and sensor boundaries

**Files.** `src/joint.h`, `src/joint.c`, new `src/joint_array.h`, the eight joint files,
`src/island.h`, `src/island.c`, new `src/island_array.h`, `src/solver_set.h`, `src/solver_set.c`,
new `src/solver_set_array.h`, `src/sensor.h`, `src/sensor.c`, new `src/sensor_array.h`, new
`src/graph_storage.h`, `src/CMakeLists.txt`, `src/shape.c`, `src/body.c`,
`src/physics_world.h`, `src/physics_world.c`, `src/constraint_graph.h`, `src/constraint_graph.c`,
`src/solver.c`, `src/world_snapshot.c`, `src/world_hash.c`, `test/test_history.c`, and any file
the compiler names (§3).

**Commit message.** `Write joint and island records through one accessor each; solver sets are created and destroyed by one function each; sensors by one`

**Code.** The design §5.2 (Records; Owned blocks; Sensors; Structural transitions; Containers, the
record-array and in-record array rules) is the specification. Mechanics that follow from it:

- `world->joints`, `world->islands` and `world->solverSets` become pointers to incomplete types;
  const record access outside each file; `b3Joint_Write`, `b3Joint_WriteSetter`, `b3Joint_Restore`,
  `b3Island_Write`, `b3Island_Restore` (an island has no setter entry: no API setter writes one).
- An island's three link arrays are `b3ConstArray` fields written only by `b3Island_PushBody`,
  `b3Island_PushContact`, `b3Island_PushJoint`, the three removeswaps and the create and destroy
  in `src/island.c` (H4 makes them `b3JournaledArray` mutators by (array tag, owner id)).
  `b3SplitIsland` no longer detaches the base island's arrays: it reads them through locals that
  stay valid because nothing reallocates them, creates the new islands, and destroys the base
  island last, which frees the arrays (H4 hands them to the destroy entry instead).
- A solver set's five arrays are `b3ConstArray` fields written only by `src/solver_set.c`'s push,
  removeswap, set and clear functions and by `b3CreateSolverSet( world )` and `b3DestroySolverSet`.
  `b3CreateSolverSet` replaces the inline allocation in `b3TrySleepIsland`, in `b3CreateBody` and
  in `b3CreateWorld`; it allocates the id, pushes a zeroed set on a bump, sets `setIndex` and
  creates the arrays. The awake set's and the colours' arrays are the same `b3ConstArray` type; the
  step's stages obtain their element pointers from the hot accessor
  (`b3SolverSet_HotBodySim( world, localIndex )` and the like, inline, asserting the step mark;
  the design §5.2), one element per call, and take a const base pointer once per loop for reads.
  The SIMD-wide stages take their lanes' elements the same way.
- `world->constraintGraph` becomes a pointer to an incomplete `b3ConstraintGraph` whose definition
  sits in `src/graph_storage.h`, included by `src/constraint_graph.c` only; outside that file a
  colour is read through `const b3GraphColor* b3GetGraphColor( world, index )`, so the colours'
  `bodySet`, `jointSims`, `convexContacts` and `contacts` headers are written only by
  `src/constraint_graph.c`'s functions (H4: the push into a colour is the function that moves a
  contact or joint into the awake set, journaling its pre-wake bytes).
- Sensors: `world->sensors` becomes a pointer to an incomplete `b3SensorArray` (`src/sensor_array.h`,
  included by `src/sensor.c` only) with `const b3Sensor* b3GetSensor( world, index )` and a count
  outside the file, written only by `src/sensor.c`'s create (push) and `b3DestroySensor`
  (removeswap), which `b3DestroyShapeInternal` now calls, the inline copy
  deleted; a sensor's three arrays are `b3ConstArray` fields; the sensor pass and the continuous
  sensor-hit write take a writable view of one sensor's three arrays by index from the hot accessor
  with no awake-owner assertion (the design §5.2, Sensors); `shapeId` is unreachable for writing.
- `b3SetBodySim`, `b3SetBodyState` and `b3SetJointSim` of H2 are now the set functions of those
  arrays.

**Evidence.** As H2. Compile-boundary check: `world->joints.data[0].drawScale = 1.0f;`,
`world->solverSets.data[2].bodySims.data[0].invMass = 1.0f;`, `world->sensors.count = 0;` and
`world->constraintGraph.colors[0].contacts.count = 0;` compile at H3b and fail at H3c (no such
member on a pointer to an incomplete type, each); `b3Joint* j = b3GetJointRecord( w, 0 );`,
`int* ids = b3GetIslandRecord( w, 0 )->bodies.data;` and
`b3Sensor* sn = b3GetSensor( w, 0 );` fail at H3c with the discarded-qualifier error and have no
before run.

### H4. The journal

**Files.** New `src/journal.h`, new `src/journal.c`, `src/world_history.h`, `src/world_history.c`,
new `src/slot_pool.h` and `src/slot_pool.c` (the `b3JournaledSlotPool` over the six pools and their
arrays), `src/CMakeLists.txt`, `src/container.h`, `src/body.c`, `src/shape.c`, `src/contact.c`, `src/joint.c`,
`src/island.c`, `src/solver_set.c`, `src/constraint_graph.h`, `src/constraint_graph.c`,
`src/sensor.c`, `src/broad_phase.h`, `src/broad_phase.c`, `src/dynamic_tree.h`, `src/solver.c`,
`src/physics_world.h`, `src/physics_world.c`, `src/world_snapshot.c`, `src/id_pool.h`,
`src/bitset.h`, `src/table.h`, `test/test_awake_tree.c`, `test/test_tree_order.c`
(`PerturbTreeLayout` calls the broad phase's rebuild function), `test/test_history.c`,
`test/main.c` (register `HistoryJournalTest`), and any file the compiler names (§3).

**Commit message.** `Journal every cold write into a per-tick segment and undo the journal on rewind`

**Code.** The design §5.2 (Containers), §6 (the staging buffer; between-step mutations), §7.1
(every entry kind, the ownership rule, the rule for record writes, entering the awake set), §7.3,
§9 step 2, the ring doc §6 (charges with owned blocks, `pendingBytes`, eviction frees, disable
frees) and the trees doc §7 (proxy entries, the choke points, the tree writes in one file) are the
specification. Mechanics that follow from them:

- `src/journal.c` holds the staging buffer (growable, reused, outside the arena, its capacity
  reported as `stagingCapacityBytes`), one append function per entry kind of the design §7.1's
  table with exactly that payload, the ownership handle (a pointer, a byte size and a kind, so
  eviction and disable can free it with the right allocator: `b3Free` for arrays and material
  blocks, `b3FreeManifolds` for manifold blocks, `b3DestroyHull` for a hull), the reverse walk of a
  byte range (entries carry a trailing length so the walk reads backward), and the undo of each
  kind as the table's Undo column states, writing through the restore writers of H3 and the
  containers' own file-local undo functions. The walk asserts what the design says it asserts: the
  pair set's add and remove, the pool free's popped id, the proxy destroy undo's id.
- Every journal call is `if ( world->history == NULL ) return;` first (the design §7.3).
- The journaled containers: `b3JournaledSlotPool` (`src/slot_pool.c`), one per record kind, holding
  the `b3IdPool` and the two functions its record file installs at creation, push-zeroed and
  truncate (the record arrays' headers stay in the record file, H3a to H3c), with `alloc` (makes the
  pool alloc entry, calling push-zeroed on a bump, which for shapes pushes the `fatAABBs` entry
  too; the undo of a bump calls truncate) and `free` (moves the
  record's owned blocks into the pool free entry with the counts the design names, clearing the
  record's pointers and counts, then frees the id); `b3JournaledArray` mutators push, removeswap,
  clear and set by (array tag, owner id) over the `b3ConstArray` fields of H3c, with
  `world->sensors` offering push and removeswap only and the awake set's and the colours' instances
  offering no generic push; `b3JournaledPairSet` add and remove in `src/broad_phase.c`;
  `b3JournaledBitSet` set and clear in `src/constraint_graph.c`, which `b3TrySleepIsland`'s four
  clears call (§2); `b3BroadPhase.pairSet` becomes `b3JournaledPairSet* pairSet`, an incomplete
  type whose definition (a `b3HashSet` inside) sits in `src/broad_phase.c`, with `b3PairSet_Contains`,
  `b3PairSet_Count` and `b3PairSet_Bytes` as its readers for `src/physics_world.c`'s two uses, so
  `src/contact.c`'s add and remove become the journaled ones. The generic
  `b3IdPool`, `b3HashSet`, `b3BitSet` and `b3Array` are untouched and keep serving scratch.
- The accessors journal: the structural entry always, the setter entry unless the owner is awake
  (a body or joint by `setIndex`, a shape by its body's, a contact by its `setIndex`), the block
  mutators always (the design §7.1, material block). `b3CreateSolverSet`, `b3DestroySolverSet`,
  `b3CreateIsland` and `b3DestroyIsland` make their entries, the destroy entries taking the arrays;
  the hull database's three functions make the refcount entry, a decrement to zero moving the hull
  into the entry; `b3CreateContact` makes the mesh cache entry after its record write.
- Restore routes: every journaled container and the hull database offer, in their file, a load
  function that writes storage and makes no entry (the design §5.2's file-local loads): the slot
  pools', the arrays', the bitsets', `b3BroadPhase_LoadTree`, `b3BroadPhase_LoadPairSet` and the
  hull database's restore add and remove. `b3DeserializeIntoShell` and `b3FreeLiveSimElements`
  (`src/world_snapshot.c`) use these in place of the journaling functions from this commit, so a
  phase-0 rewind makes no entry of its own; H5's scatter uses them too.
- Entering the awake set: `b3SolverSet_PushAwakeBody( world, body, sim, state )`,
  `b3SolverSet_PushAwakeJoint`, `b3Graph_PushContact( world, contact )` and `b3Graph_PushJoint` are
  the only pushes onto the awake set's and the colours' arrays; each journals the record's pre-wake
  bytes through the structural entry before appending, the body one also every shape's record and
  `fatAABBs` entry, the contact one a manifold block entry (count 0 when there is none) and a cache
  entry for a mesh contact. The move between the awake set's arrays and a colour (begin and end
  touch) is one container operation that asserts the record is where it says and makes no entry.
- Trees (the trees doc §7): `b3BroadPhase.trees` becomes `const b3DynamicTree* trees`, pointing at
  storage `src/broad_phase.c` allocates and converts back; every tree write is a function of that
  file: `b3BroadPhase_RefitTrees( world )` and `b3BroadPhase_EnlargeProxy( world, proxyKey, aabb )`
  replace `src/solver.c`'s calls, `b3BroadPhase_RebuildTree( world, treeIndex, fullBuild )` replaces
  `b3World_RebuildStaticTree`'s rebuild and the tests' `PerturbTreeLayout` calls, and
  `b3BroadPhase_LoadTree( world, treeIndex, ... )` replaces `b3DesTree`'s in-place write and
  `b3BroadPhase_LoadPairSet( bp, ... )` replaces `b3DesHashSet`'s write through `&bp->pairSet`
  (both restores, neither journals),
  `b3BroadPhase_ClearMoved( bp, treeIndex )` is the pair update's static-tree clear and replaces
  `SleepingProxyTreeCase`'s two calls, and `b3BroadPhase_MarkProxyMoved` and
  `b3BroadPhase_MarkProxyMovedSerial` become ordinary functions of the file, declared in the
  header (§7's benchmark covers the call). The proxy
  create and destroy choke points (the single create shared by `b3BroadPhase_CreateProxy` and the
  drain's one-by-one placement, the drain's bulk `b3DynamicTree_CreateProxies` placement,
  `b3BroadPhase_DestroyProxy`) make the proxy create and destroy entries for the static and sleeping
  trees; `b3BroadPhase_MoveProxy` makes the proxy move entry for the sleeping tree. The undo insert
  is the tree's ordinary create, unmarked, asserting the popped id equals the recorded one; the undo
  of a create is the ordinary destroy. In the validation build the static and kinematic leaf writers
  assert, on forward writes and never in an undo, that the box they write equals
  `fatAABBs[shapeId]` (§2).
- The ring (`src/world_history.c`): a slot is header, image (still the phase-0 serializer image
  in this commit), queue copy and the journal segment moved in from the staging buffer at capture;
  a slot's charge adds the byte sizes of the blocks its entries own; `pendingBytes` is the staging
  buffer's used bytes plus the bytes of the blocks its entries own; eviction walks the evicted
  slots' entries once and frees the blocks they own; disable frees the blocks owned by every
  retained slot and by the staging buffer before the arena. `b3World_Rewind` step 2: undo the
  staging buffer's entries in reverse and reset it (also when T == P), then each retained segment
  from `historyTick` down to T+1 in reverse; then step 3 as in H1. In this commit the serializer
  image then overwrites what the walk restored; the walk is nonetheless exercised and its blocks
  change hands, and `b3DeserializeIntoShell`'s `b3FreeLiveSimElements` frees whatever the walk
  handed back, so no block leaks between H4 and H5.
- `b3HistoryInfo` now reports `stagingCapacityBytes` and `pendingBytes`.

**Test** `HistoryJournalTest`, exercising the walk directly through `src/world_history.h`'s
`b3History_UndoPending( world )` and `b3History_UndoSegments( world, tick )` (names are
suggestions; the functions are the two halves of the design §9 step 2 and exist for `b3World_Rewind`,
not for the test), on worlds whose every change between the recorded hash and the walk is a cold
write, so the walk alone must restore the hash:

1. `ColdChurnUndoCase`: a scene of sleeping islands (two piles created asleep), four static shapes,
   one sleeping kinematic body, two sensors on static bodies with nothing overlapping them, a
   multi-material static shape, a mesh contact between a sleeping body and a static mesh (made by
   dropping the body awake and letting it sleep before enabling history), two lone sleeping bodies
   with no contact, each in its own set, and a hull referenced once by a static shape, stepped once
   before the enable so every sleeping proxy is in the sleeping tree; history enabled, hash
   recorded; then between steps, cold operations only (§2): a static shape created with
   `invokeContactCreation` false and one destroyed; `b3Shape_SetFilter` with `invokeContacts` false
   on a sleeping body's shape and on a static shape; a joint created between the two lone sleeping
   bodies (a set merge) and destroyed with `wakeAttached` false, and a joint created between two
   bodies of one pile; one lone sleeping body destroyed and the other disabled; a sensor created
   on a static body and the other destroyed; the hull shape destroyed and a shape with the same
   hull content created; `b3Shape_SetFriction`, `b3Shape_SetRestitution`,
   `b3Shape_SetSurfaceMaterial` and `b3Shape_SetMeshMaterial` on the static shapes and on a
   sleeping body's shape. Then `b3History_UndoPending`: `ENSURE` the hash equals the recorded one, every
   sleeping body's shape `proxyKey` equals its recorded value, and `pendingBytes` is 0. Fails before
   H4 by not building.
2. `SegmentWalkCase`: the same scene with no awake body at all (the piles asleep, the kinematic
   body asleep, no sensor), stepped once at the test's time step before the enable; history
   enabled with `tickCount` 16; 12 ticks stepped with one cold op from case 1 before each step
   (static creates with `invokeContactCreation` false, so no tree holds a mark at a boundary), the
   hash and a copy of `*world->scalars` recorded per tick; then `b3History_UndoSegments( world, 6 )`,
   the tick-6 scalars copied back through `b3World_WriteScalars` (the scalars are imaged, never
   journaled, the design §5.1; `endEventArrayIndex` flips every step), and `ENSURE` the hash equals
   the hash at tick 6. The same on a fresh run of the scene with the target tick 0 (the walk starts
   at `historyTick`, which only `b3World_Rewind` moves). With no awake body `b3Solve` returns early
   and the sensor pass has nothing, so beyond the scalars no hot write occurs and the segments
   alone carry every change (§2). Fails before H4 by not building.
3. `OwnershipCase`: on case 1's scene, with `n0` the sum of `allocationCount` over
   `world->manifoldAllocators` read after the enable. Cold part: a joint between the two lone
   sleeping bodies (the merge destroys one set and one island), the multi-material shape
   destroyed, a sensor destroyed, the hull's last reference released; `ENSURE` `pendingBytes` grew
   by at least the sum of the destroyed set's five array byte counts, the destroyed island's three,
   the material block's, the sensor's three and the hull's `byteCount`, read before each destroy;
   `b3History_UndoPending`: `ENSURE` the hash equals the recorded one and `pendingBytes` is 0.
   Contact part, through `b3World_Rewind` since the destroy wakes (§2): the body with the mesh
   contact destroyed between steps (its manifold block, cache, island and set into the entries);
   `ENSURE` `pendingBytes` grew by at least those byte counts; `b3World_Rewind( historyTick )`:
   `ENSURE` the hash equals the recorded one, the manifold allocators' count equals `n0` and
   `pendingBytes` is 0. Then that destroy again followed by a step (the segment closes), `ENSURE`
   `bytesUsed` includes those bytes and `pendingBytes` is 0; then enough steps on a `tickCount` 4
   ring to evict the slot: `ENSURE` the manifold allocators' count fell by one; then disable,
   destroy the world and `ENSURE( b3GetByteCount() == base )` with `base` from before the world was
   created, which is the check that every other evicted block was freed. Fails before H4 by not
   building.
4. `ProxyIdentityCase`: a lone sleeping body destroyed between steps (its sleeping-tree proxy
   destroy entry), then `b3History_UndoPending`: `proxyKey` equals its recorded value and the hash
   is restored, section 2 of H0's walk (the sleeping tree's proxy pool) included; then a static
   shape created, another destroyed, a third created (the free list reordered), undone: hash
   restored. The id reuse across a step, a destroyed sleeping body's sleeping-tree id taken by a
   body falling asleep in the next step, is `HistoryRewindTest`'s case, since the awake tree takes
   part. Fails before H4 by not building.

`HistoryRingTest` gains, in the same commit, the ring doc §9 test 1 items that need the journal: a
rewind with no step since a sensor destruction and since a hull release now checks
`pendingBytes` before and 0 after; every rewind that returns `b3_historyOk`, the scene holding a
hull shape and case 8's two rewinds in a row included, leaves `pendingBytes` 0; and after every
operation the manifold allocators' summed
`allocationCount` equals the live contacts' block count plus the manifold handles the retained
slots and the staging buffer own, read through a third read-only function of `src/world_history.h`,
`b3History_CountOwnedBlocks( world, kind )`, added in this commit. The leak check
`ENSURE( b3GetByteCount() == base )` after the destroy stands as the check for every other block
kind.

### H5. The hot image and the in-place restore

**Files.** `src/world_history.h`, `src/world_history.c`, `src/journal.c`, `src/solver_set.c`,
`src/constraint_graph.c`, `src/body.c`, `src/shape.c`, `src/contact.c`, `src/joint.c`,
`src/island.c`, `src/sensor.c`, `src/broad_phase.h`, `src/broad_phase.c`, `src/physics_world.h`,
`src/physics_world.c`, `src/parallel_for.h` (if the gather needs a signature it lacks),
`test/test_history.c`, `test/main.c` (register `HistoryUndoSufficiencyTest`, `HistoryTreeTest`,
`HistoryCostTest`).

**Commit message.** `Capture the awake state as a hot image and restore in place from the image and the journal`

**Code.** The design §5.1, §6 steps 1 to 4, §9 steps 3 to 6, §13 phase 1, the trees doc §5 (the
awake tree's image and the migration queue), §6 (the restore list and the pass, phase 1: the
static tree), §9 (the kinematic tree imaged raw) are the specification. Mechanics that follow from
them:

- Capture at the H1 point: the sizing pass (a reduction over the awake bodies and their shapes,
  the awake contacts' manifold counts and cache sizes, the awake islands' link array lengths, the
  sensors' overlap counts, the kinematic and static trees' moved leaves by
  `b3DynamicTree_GatherMovedProxies` into a per-tick scratch array, the awake tree's and the
  kinematic tree's sizes), the slot reservation with the segment's size, then the flat copies
  (awake set arrays, the 24 colours' arrays, the scalars struct, the awake tree's and, in phase 1,
  the kinematic tree's fields the trees doc §5 names, the migration queue), then the gathers (per
  sensor `overlaps2`; per awake body its record; per awake shape its record and `fatAABBs` entry;
  the moved proxy ids; per awake contact id from the colour arrays and the awake `contactIndices`
  its record, its `manifoldCount` manifolds and its cache when `b3_simMeshContact`; per awake joint
  its record; per awake island its record and three arrays), each record prefixed by its id, then
  the segment moved in. The gathers run as a parallel-for over the awake population through the
  world's task callbacks with workers writing disjoint ranges computed by a prefix sum over the
  sizing pass, and inline when `workerCount` is 1; a fused capture inside finalize is the design's
  optimisation to measure, not this commit's.
- Restore in place (the design §9): after step 2's walk, step 3 sets each awake-set array and
  colour array to the image's count, growing capacity only when the count exceeds it, and copies;
  scatters body, shape, joint and island records by id through the restore writers (a shape write
  releasing its `userShape`); for each imaged contact resizes its block to the image's count
  through the block mutator's restore path and copies, likewise the cache and the islands' arrays;
  copies the scalars struct through `b3World_WriteScalars`, the sensors' `overlaps2` and the fat
  AABBs; step 4 per the trees doc §6: the awake tree's arrays freed and reallocated at the image's
  sizes and copied with its scalars, the queue copied back, the kinematic tree loaded the same way
  in phase 1, then the restore list (every shape id written by the undo or the scatter, appended by
  the shape restore writer) walked once: for each listed id below the shape array's length whose
  restored record holds a proxy in the static tree (the kinematic tree from H6), `b3DynamicTree_
  MoveProxy` to the restored fat AABB through the broad phase's own move; then `b3DynamicTree_
  ClearMoved` on the static tree and `b3DynamicTree_MarkProxyMovedSerial` for each id of image T's
  static moved list (non-empty only in tick 0's image); step 5 and step 6 as in H1.
- The ring's registry, reader and the serializer image leave `src/world_history.c`; the two
  serializer options stay for the recording's use of the reader through the restore writers.
- The slot's image bytes are the design §6's count; the phase-0 O(world) term is gone, and
  `bytesUsed` on a sleeping world is a few KB per tick plus the kinematic tree's raw image until H6.

**Tests.**

- `HistoryRewindTest`, `HistoryReplayTest`, `HistoryCrossWorkerTest` (with its goldens unchanged)
  and `HistoryRingTest` run unchanged and pass: that is this commit's main evidence, and they pass
  before it too, under the serializer path.
- `HistoryUndoSufficiencyTest` (the design §12 test 8): restore exactness on scripted scenes where,
  for the rewind target T and the current tick P, each of the design §7.1's cases occurs for a body
  with touching contacts, a mesh contact and a multi-material shape: asleep at T and woken in
  (T, P]; awake at T and put to sleep in (T, P]; put to sleep in tick T itself; put to sleep at or
  before T and still asleep at P; asleep at T, woken, put to sleep and woken again in (T, P]; moved
  into the awake set by `b3Body_SetType` and by `b3Body_Enable`. Each case `ENSURE`s the hash after
  `Rewind( T )` equals the recorded one, run with `tickCount` 60 and `captureInterval` 1 and 4 (with 4, the
  script places T on a multiple of 4 and `ENSURE`s `b3World_GetRestorableTick( T ) == T` before
  the rewind). Passes before H5 under the serializer path; it is the
  regression check for the journal's sufficiency once the image is partial, and the closure doc says
  so.
- `HistoryTreeTest` (the trees doc §10 test 1): restore-and-replay cases built so the static,
  kinematic and sleeping trees' layouts differ after a rewind from the original run's (the
  perturbation helper rebuilds every tree after the rewind): a bullet sweep with several competing
  candidates, some awake and some asleep, one of them a nearer hit that a `preSolve` rejects; a
  sweep with more than eight sensor candidates; an explosion hitting several shapes of one body; an
  explosion waking bodies in several sleeping sets, some connected by joints. Each: record the hash
  and event digest per tick over 30 ticks with history enabled at `tickCount` 30, rewind to 10,
  perturb, replay to 30 with the op log, and
  `ENSURE` equality at every tick. Passes before H5 (layout independence is the engine's, the design
  §3); it guards the restore pass.
- `HistoryCostTest` case 1, `SleepingDynamicCostCase`: 20,000 single-box dynamic bodies created
  asleep in a grid, far apart, in batches of 100 with a step after each batch (a shape created on
  a sleeping body enters the awake tree first, §2, and `proxyCapacity` never shrinks, so the batches
  keep the awake tree's high-water mark, which the image carries, near the sixteen awake boxes;
  `ENSURE` the awake tree's `proxyCapacity` is at most 256 before the enable), sixteen awake boxes
  falling onto a static ground elsewhere, sleep on,
  `workerCount` 4, history enabled with an unbounded budget after ten warm-up steps; for each of the
  next twenty steps the slot's charge (`bytesUsed` after minus before) is `ENSURE`d at or below 256
  KB. Fails before H5: the serialized image holds every sleeping body's record, sim and shape, over
  10 MB per tick. The closure doc quotes the failing figure.

### H6. The derived kinematic tree

**Files.** `src/broad_phase.c`, `src/world_history.c`, `test/test_history.c`.

**Commit message.** `Derive the kinematic tree at restore from fat AABBs and the moved list instead of imaging it`

**Code.** The design §13 phase 2 and the trees doc §6, §7 and §9 (phase 2) are the specification.
Mechanics that follow from them: the proxy create and destroy entries are made for the kinematic
tree as for the static and sleeping trees; the restore pass moves listed shapes whose restored
record names the kinematic tree; the kinematic tree's raw image and its load leave capture and
restore; the kinematic moved list of image T is applied as the static one is; a kinematic tree the
pass moved a proxy in is left unordered or root-marked and the first replayed step's rebuild
restores its order (the trees doc §6).

**Tests.**

- `HistoryCostTest` case 2, `SleepingKinematicCostCase`: as case 1 with 20,000 kinematic bodies
  created asleep instead of dynamic ones (a kinematic body with no velocity sleeps on its timer;
  the test steps until `b3World_GetAwakeBodyCount` is the sixteen awake boxes before enabling
  history). Per-step charge at or below 256 KB. Fails before H6: the kinematic tree is imaged raw,
  20,000 proxy slots at about 100 B each.
- `HistoryTreeTest` gains two kinematic cases: a kinematic body moved between steps by
  `b3Body_SetTransform` into contact with a sleeping pile, with the rewind across the move (the
  pass moves its proxy and the next step rebuilds the kinematic tree); and a kinematic body's
  shape filter changed after T (its proxy recreated with a possibly different id), rewound:
  `proxyKey` equals the recorded value and the replay matches.

## 5. Goldens and scene hashes

History disabled, this branch changes no simulation result: every commit is a boundary move, a
null-checked journal call, or code that runs only while history is enabled. So:

- Every value in `test/test_determinism.c` is unchanged at every commit, in both precisions. A
  change is a defect: stop and report.
- The benchmark scene hashes (§7's scratch harness, base against branch, history disabled) are
  equal every step for every scene. One unequal step is a stop.
- With history enabled at interval 1 and an unbounded budget, the same harness gives the same
  per-step hashes as with it disabled: capture reads and never writes. A difference is a stop.
- The `HISTORY_GOLDEN_HASH_*` values recorded in H1 are unchanged by H2 to H6. A change means a
  boundary commit changed state or the in-place restore differs from the serializer's: stop.

No golden policy admits a change on this branch; `test/test_determinism.c` is touched by no commit.

## 6. Test-to-requirement mapping

| Clause | Evidence | Fails before? |
|---|---|---|
| design §12 test 1, the state hash | `HistoryHashTest` cases 1 to 4; inspection of the walk against the design's list | Build failure (H0) |
| design §14, the API and its comments | Inspection of the headers; every function exercised by `HistoryRingTest` | Build failure (H1) |
| ring doc §3 to §8: slots, two ends, interval, budget steps 1 to 4, window, charge, enable and disable, destroy | `HistoryRingTest` cases 1 to 10; the invariant check after every operation; the byte-count baselines | Build failure (H1) |
| ring doc §9 test 1 | `HistoryRingTest`, extended in H4 for pending bytes and block ownership | Build failure (H1, H4) |
| ring doc §9 test 2, budget acceptance | §7 measurements per scene | n/a |
| design §13 phase 0: serializer options, queue beside the image, §9 step 5 performed by the rewind | Inspection; `HistoryRewindTest`'s debug-draw and `userData` cases; `HistoryRingTest` case 9 | Build failure (H1) |
| design §10: recording and history exclusive, player world refused, seek refused | Inspection of the three assertions; a validation-build check in `HistoryRingTest` that `b3World_StartRecording` on a history world asserts is not written (an assertion failure ends the process); the closure doc states inspection | n/a |
| design §12 test 2, restore exactness, every churn case | `HistoryRewindTest`, 25 pairs per scene in the suite, 1,000 under `BOX3D_HISTORY_SOAK` | Build failure (H1) |
| design §12 test 3, replay exactness with events, host state set back | `HistoryReplayTest` | Build failure (H1) |
| design §12 test 4, cross-worker replay and platform goldens | `HistoryCrossWorkerTest`; the owner's Windows run | Build failure (H1) |
| design §5.2 world scalars behind a const pointer, one writer | H2 compile-boundary check; inspection of the writers | n/a (boundary) |
| design §5.2 const-returning sim getters, the diagnostic an error | H2 compile-boundary check; the CMake flags | n/a (boundary) |
| design §5.2 three write routes per record; const records outside their files; owned blocks pointer-to-const; `userShape` one writer; `b3Shape_WriteMaterial` | H3a, H3b, H3c compile-boundary checks; inspection; the suite and goldens unchanged | n/a (boundary) |
| design §5.2 one function each for set and island create and destroy; one sensor destroy; split hands arrays to the destroy | Inspection of H3c and H4; `HistoryJournalTest` case 3 | n/a |
| design §7.1 every entry kind and its undo; ownership one way; the record-write rule; entering the awake set | `HistoryJournalTest` cases 1 to 4; `HistoryUndoSufficiencyTest`; `HistoryRewindTest`'s structural cases | Build failure (H4) |
| design §6 between-step mutations undone first; `Rewind( P )` | `HistoryRingTest` cases 8 to 10; `HistoryJournalTest` case 1 | Build failure (H1, H4) |
| design §7.3 one null check with history disabled | Inspection of every journal function's first statement; §7 benchmark, base against branch, history disabled, for the cost | n/a |
| design §6 capture steps 1 to 4; §9 steps 3 to 6 in place | `HistoryCostTest` case 1; the H1 tests unchanged under the new path; inspection | Yes, the 256 KB bound (H5) |
| design §12 test 8 | `HistoryUndoSufficiencyTest` | No, regression (H5) |
| trees doc §5 awake tree and queue imaged; §6 restore list, pass, moved lists, sleeping tree untouched; §7 proxy entries and identity; §8 rebuilds make no entry | `HistoryJournalTest` case 4; `HistoryRewindTest`'s tree cases; `HistoryTreeTest`; `HistoryHashTest` case 3; inspection | Case 4: build failure (H4) |
| trees doc §7 every tree write a function of `broad_phase.c`, trees const outside; design §5.2 the pair set's file | H4 compile-boundary check: `b3DynamicTree_Rebuild( world->broadPhase.trees, true )` and `b3AddKey( &world->broadPhase.pairSet, 0 );` compile at H3c and fail at H4 (discarded qualifier; incompatible pointer type, the operand being a pointer to a pointer to an incomplete type) | n/a (boundary) |
| trees doc §9 phase 2 | `HistoryCostTest` case 2; `HistoryTreeTest`'s kinematic cases | Yes, the 256 KB bound (H6) |
| trees doc §10 test 1 | `HistoryTreeTest` | No, regression |
| design §12 test 5 and the ring doc §9 test 2, cost | §7 measurements | n/a |
| design §2 requirement 2 (met from phase 2) | `HistoryCostTest` cases 1 and 2; the `large_world` figures in §7 | Yes (H5, H6) |
| WORKFLOW AC-1 to AC-4 on the code | §8's checklist, one line per structure the three designs add | n/a |

## 7. Verification and measurements

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF
cmake --build build --target test
./build/bin/test                                 # whole suite, per commit
./build/bin/test HistoryRewindTest               # one entry, by name
BOX3D_HISTORY_SOAK=1 ./build/bin/test HistoryRewindTest   # the 1,000-pair run, once, for the closure doc

cmake -S . -B build-dp -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF -DBOX3D_DOUBLE_PRECISION=ON
cmake --build build-dp --target test
./build-dp/bin/test                              # whole suite, double precision, per commit

cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DBOX3D_BENCHMARKS=ON -DBOX3D_SAMPLES=OFF -DBOX3D_UNIT_TESTS=OFF
cmake --build build-rel --target benchmark
./build-rel/bin/benchmark -t=8
```

The Debug builds carry `BOX3D_VALIDATE` (on by default at the top level), so every `B3_VALIDATE`,
the validation functions, the hot accessors' assertions, the journal's assertions and the design §9
step 6 checks are live in the suite runs. The benchmark is built and run this way on the base and
on the branch, same machine, same options, each from its own directory, with `-t=` explicit
(`B3_MAX_WORKERS` is guarded only by an `assert` a Release build removes) on a machine reporting at
least eight cores.

Measurements the closure doc needs:

- **No-regression, history disabled** (the design §7.3): for every scene, the csv total milliseconds
  per thread count from one to eight, base against branch. A scene more than 3% slower at one or
  eight threads, outside the run-to-run spread of four repeats, is a stop (§9).
- **Scene hashes** (§5): a scratch harness, uncommitted, that creates each benchmark scene through
  `shared/benchmarks.c` with its capacity function, steps it for its benchmark length at one worker
  with its step function, and prints `b3World_ComputeStateHash` per step; base against branch with
  history disabled (the base build gets the H0 function alone, cherry-picked into a scratch
  worktree), and branch against branch with history enabled at interval 1 and unbounded budget.
- **Cost** (the design §12 test 5; the ring doc §9 test 2), a scratch harness in a Release build,
  eight workers, per scene of the benchmark table (`large_world` among them) and a scene of one million
  sleeping dynamic and kinematic bodies with sixteen awake (built in the harness: a grid of boxes
  created asleep, half kinematic, in batches with a step after each as `HistoryCostTest` case 1
  does, so the awake tree's capacity stays small, awake boxes falling elsewhere): capture
  microseconds and percent
  of the step (the capture timed around the H1 insertion in a scratch build, the step timed in the
  harness around the `b3World_Step` call; `b3World_GetProfile().step` is taken before the capture
  point and is not the denominator);
  image, journal and owned-block bytes per tick (the parts `b3History_GetSlot` returns); `bytesUsed`, the
  retained window and the effective interval at `tickCount` 60 under an unbounded budget and under
  `maxBytes` of a quarter of the unbounded steady-state `bytesUsed`; restore milliseconds for
  `Rewind` 1, 5 and 9 ticks back; replay milliseconds for 9 ticks. Acceptance as the design states:
  capture under 15% of the step at eight workers on the all-awake scenes and under 1 µs per awake
  body on `large_world` and the million-body scene; restore proportional to awake plus sensors and
  journal (reported as microseconds per awake body across the scenes); replay equal to N steps
  within noise, with the first replayed step after a rewind that moved a kinematic proxy reported
  separately on the million-body scene, and a rewind whose undone wake fires the sleeping tree's
  full rebuild reported separately. Each is measured at H5 (phase 1) and again at H6 (phase 2); the
  million-body scene's phase-1 figure is expected to miss on the kinematic term and the closure doc
  says so.
- **Phase-0 figures** (informational, at H1): per scene the slot bytes per tick and the registry's
  bytes, so the closure doc can state what H5 removed.

## 8. Closure doc

Written after the nine commits as
`docs/designs/20261016-001-implementation-prompt-history-ring-closure.md`, containing: the branch
and base hashes; suite counts per commit (tests run, passed) in both configurations; pre-existing
failures named with output; compiler warning status and the `clang-format` check; every deviation
from this document and the designs with its reason, the files the compiler added to the boundary
commits (§3), the phase-0 registry bytes outside the info struct, and any §2 re-check among them;
every out-of-scope observation, `b3Shape_SetFilter`'s always-recreate under `invokeContacts` true
(§2) and `docs/faq.md`'s
"no rollback" statement (§9) among them; the compile-boundary diagnostics of H2, H3a, H3b, H3c and
H4; the two outputs of each fail-before test (H0, H1, H4 build failures; H5 and H6 bound failures);
the `HISTORY_GOLDEN_HASH_*` values and the determinism goldens unchanged; the public additions; the
snapshot version change; the §7 measurements with the acceptance verdict per scene; and a checklist
of WORKFLOW AC-1 to AC-4 against the shipped code, one line per structure the three designs add.
It stays outside the nine commits; the owner commits it to the design branch separately if at all. The implementer does not change the status of any design doc or of this
document.

## 9. Out of scope and stop conditions

- No change to `benchmark/`, `shared/`, `samples/`, `docs/` other than `docs/designs/`, `README.md`,
  `docs/faq.md` (its statement that box3d has no rollback becomes stale at H5 and is reported, not
  edited), `test/test_determinism.c`, or any file §4 does not list and the compiler does not name
  (§3). The design's phase 3, the bullet enlarge question of the trees doc §11, and the design §16's
  per-record mark are not implemented.
- Stop and report if: a determinism golden or a benchmark scene hash changes (§5); a
  `HISTORY_GOLDEN_HASH_*` value changes after H1; `HistoryHashTest` case 4 fails at a qualifying
  frame (the serializer does not restore something the hash sees); a benchmark scene regresses past §7's threshold with history
  disabled; a phase-0 restore exactness failure is traced to a serializer field the serializer
  itself does not restore verbatim (fixing the serializer is in scope only as restoring the field
  verbatim; anything else is a report); a test needs a hook in `src/` beyond the read-only
  functions §4 H1 and H4 name and what the internal headers expose; a boundary commit cannot remove a direct write without
  module-boundary work the design does not describe (the design §5.2 calls that a finding against
  the surrounding code, to report); the step's stage loops cannot take their elements through the
  hot accessor without a measurable cost (§7's threshold); or a design is silent on a decision the
  code forces.
- Public query order, island independence, stateful callbacks and `b3World_GetBounds` after a
  restore are outside the guarantee (the design §10) and get no test.

## 10. Provenance

- **Phases 0, 1 and 2 in one prompt, nine commits.** The three designs are Canonical together and
  the design's requirement 2 is met only from phase 2; a prompt per phase was considered and not
  done, since the reconciliation and the test machinery are shared and the later phases' tests are
  the earlier phases' tests run under a different restore path.
- **Phase 0 lands with the full test set of the design §12 tests 2 to 4.** The design says phase 0
  exists so those tests and the hash can be validated before any engine change; H1 therefore
  carries the heaviest tests, and H5 is shown by those tests passing unchanged plus one cost bound
  that the serializer path cannot meet.
- **Boundary commits with compile-fail evidence.** H2 and H3a to H3c change no behaviour, so a
  failing test cannot show them; the designs make the compiler the judge of the migration, so a
  translation unit that compiles before and not after is the evidence that matches the design's
  own claim. Splitting H3 by record kind keeps each commit bisectable and reviewable; one commit
  over every record was considered and not done.
- **The journal before the image.** H4 lands the journal and its walk while the serializer still
  restores the image, tested directly on cold-only churn; the reverse order (image first) would
  restore cold state wrongly across structural churn with no journal to undo it, and landing both in
  one commit was considered and not done for size.
- **Test counts.** 25 (T, P) pairs per scene in the suite and 1,000 under `BOX3D_HISTORY_SOAK`:
  the design's count is run once and quoted; the suite stays within its present duration.
- **The two marks.** A `bool` on `b3World` for a player's world and a `bool` for the inside of a
  step exist because the design names assertions that need them and no existing field tells either
  apart (§2). Each is read by assertions only; neither is control flow.
- **The 256 KB bound and the 3% threshold** are this document's numbers: the bound is an order of
  magnitude above the design §5.1's figure for sixteen awake bodies with their contacts and an order
  below the serialized image, so it separates the two paths without pinning the design's estimate;
  the threshold is the benchmark's typical run-to-run spread at eight threads.
- **Phase-0 registry bytes are reported in the closure doc, not in `b3HistoryInfo`.** The ring doc's
  fields have settled meanings; adding a field for a phase-0 artefact that H5 removes was considered
  and not done.
- **The fork is `master`**, as `20261014-002` chose and for the same reason: the tests and the
  designs' assumptions exist there; an upstream PR, if wanted, is a later cherry-pick.
