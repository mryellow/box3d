---

title: Rewind-only bit-exact world history for box3d (hot image + undo journal ring)
status: Draft, not reviewed (no review round has been run against this document). Derived from
  docs/designs/20260927-002-bit-exact-history-ring.md, whose review series converged at round 43.
  That convergence does not carry over. This document removes forward scrub, which changes the
  journal payloads, the ownership of heap blocks, the ring's discard rule and the sleep-side
  journal entries, and none of those changes has been reviewed. The document is self-contained
  and does not rely on the earlier one being read.
  Code references are against source as of 5643cd8, the last commit touching `src/` and `include/`.
  They were gathered by three read-only audits of the step pipeline, the determinism properties,
  and the solver's order dependence, and the wake and sleep paths in `solver_set.c` were re-read
  for this document; none has been checked by execution.
date: 2026-09-29

---

# Rewind-only bit-exact world history for box3d

## 1. Problem

Client prediction with server reconciliation needs to put the world back to an earlier tick T,
apply a correction, and re-step to now. `docs/faq.md` says box3d has no such mechanism.

One way to build it is to restore and resimulate only the predicted islands, with every other
body frozen during resimulation (`docs/designs/20260927-001-client-prediction-rollback.md`). That
keeps capture and resimulation proportional to the predicted subset, and pays for it twice: the
result is *consistent* but not bit-exact, and the scoped-resimulation machinery is large.

This document asks a different question: what is the smallest, fastest, most robust design if
the restored world must be **bit-exact** (re-stepping from the restored state reproduces the
original timeline bit for bit, so a replay with unchanged inputs is indistinguishable from never
having rolled back), solver changes are on the table, and the only movement through history the
caller needs is **backward**?

The answer has four parts:

1. **Box3d already has most of it.** The recording system serializes the whole world into a raw
   struct image (`src/world_snapshot.c`), restores it in place for backward seeks
   (`src/recording_replay.c`), re-steps, and checks per-step state hashes. The
   `ScrubBackward` test (`test/test_recording.c`) asserts that a keyframe restore plus
   re-step reproduces the forward pass's hashes. Bit-exact rewind of a whole world along the
   dimensions that hash covers — body transforms and velocities — is therefore a demonstrated
   property of the engine, not a research question; the FAQ statement is stale for those
   dimensions. The hash does not yet cover everything requirement 1 claims (shape materials,
   contact/manifold state, sensor overlaps, tree proxy identity, pool and pair-set membership),
   which is why §12's hash is extended, not reused as-is.
2. **What is missing is an in-place, O(awake) form of it**, with a ring that can be rewound to
   any retained tick, and a public API. The serializer is O(world) per capture, allocates on
   restore proportional to the *whole world*, and copies static and sleeping state that never
   changes. Capture's allocation in the new design is amortized to zero once the ring arena and
   its journal staging buffer have warmed up to the scene (§8); restore may still realloc a
   bounded amount — for the awake state, a manifold block whose count changed or a resized awake
   array (§9), and for each journal entry walked, the material or manifold block it replaces —
   which is a different thing from the serializer's O(world) restore allocation.
3. **The correction only ever goes back.** The caller rewinds to T, applies the correction and
   re-steps. It never returns to a later tick without stepping, because the later ticks belong to
   the timeline the correction replaces. So the journal is an undo log: each entry records what is
   needed to reverse its mutation and nothing else, and `Rewind(T)` discards every tick after T.
   That removes the redo half of every journal entry, makes heap-block ownership one-directional,
   and removes any need to detect that a retained tick has gone stale (§7.1, §8).
4. **The cost that cannot be designed away is resimulation.** Re-stepping N ticks of the whole
   world costs N steps. Sleep is the mechanism that keeps that small in real games; for stress
   scenes with 10k awake bodies it does not fit a frame, and §11 says what can be done about
   it, including the solver changes that would allow exact per-island incremental replay later.

## 2. Requirements

1. **Bit-exact**, with two named exceptions that v1 does not fix: §10's `preSolve`
   and custom filter callback order after a layout-changing restore, for a caller whose callback has
   order-dependent side effects, and §5.3's name string for two names whose hashes collide. After
   `Rewind(T)`, every simulation-affecting byte equals its value at the end of step T (tree layout is not
   simulation-affecting once §7.4's order changes land). Re-stepping
   with the same inputs reproduces the original ticks exactly, including ids, generations, events,
   island membership, and sleep timing. This is a claim about re-stepping, not about querying the
   world immediately after `Rewind(T)` alone: event arrays are not simulation-affecting (nothing
   reads them back into physics) and are not restored to their end-of-T contents by `Rewind`
   itself; events of the steps after T are reproduced by re-stepping, and step T's own events are
   not available after `Rewind(T)` (§9's step 5, §10).
2. **Per-tick cost proportional to awake state, structural churn, and sensor count.** Never
   proportional to the number of sleeping or static bodies. `large_world` (1M bodies, 10 awake)
   must cost a few KB per tick, not 650 MB
   (`docs/designs/reviews/20260927-001-client-prediction-rollback-perf.md`). Sensor
   overlap state is the one exception to "proportional to awake": the engine re-evaluates every
   sensor's overlaps every step regardless of the owning shape's awake state (`sensor.c`), so
   capturing it is O(sensor count + total overlap count) — `sensors[i].overlaps2` holds one entry
   per currently-overlapping visitor shape (§5.1), so a sensor with many overlaps costs
   proportionally more, not a flat per-sensor amount. `large_world`-shaped scenes with few
   sensors and few overlaps are unaffected; a scene with many sensors, or sensors with many
   overlaps, on sleeping or static bodies pays for all of it every tick.
3. **Restore never rejected** for any restorable tick — an imaged tick inside the retained window
   and at or before the current tick (§6; every such tick when the capture interval is 1) — and
   never leaves the world inconsistent. There is no "outside churn" failure mode because the whole
   world is restored.
4. **Rewindable.** Any restorable tick can be restored from the current position without
   re-stepping. Rewind is one-way: it discards every tick after T, and the only way forward from T
   is `b3World_Step`.
5. **Budgeted memory** with graceful degradation: a byte budget widens the capture interval
   instead of failing. The budget is a target under normal churn, not a hard ceiling, and any
   overrun is limited to the retained minimum window's own slots: a single
   tick whose own image and journal segment exceed the budget is still admitted (§8) rather than
   rejected, since rejecting it would violate requirement 3. `maxBytes` bounds retained history;
   arena and staging capacity are reported separately (§8, §14).
6. **No approximations and no new solver participant kinds.** Resimulation is `b3World_Step`.
7. **Verified by construction, not by audit.** Every cold structure in §5.2 is written only
   through its write accessor or journaled container type, and the journal call lives inside that
   accessor or container. A caller cannot reach the underlying field any other way, so completeness
   is a property of the module boundary, not a promise a review or a runtime check stands in for.

Non-goals:

- **Forward scrub, and peek-and-return.** Restoring a tick later than the current one, or
  rewinding to inspect the world at T and then returning to the present without re-stepping
  (server-side lag compensation done by rewinding the physics world, a debugger timeline that jumps
  both ways). Both need a journal that can be replayed forward; §15 says what that costs and why
  it is left out. A caller that needs the past poses of bodies for a hit check records them itself.
- **Capture or resimulation proportional to the *predicted* subset.** §11 covers what that would
  take on top of this design.

## 3. What the engine guarantees today

From the determinism audit (`docs/faq.md`, `docs/simulation.md`,
`test/test_determinism.c`):

- Same inputs reproduce the same run. No RNG; timer readings feed only the profile, never
  simulation results.
- Results are independent of worker count. Per-worker results are OR-merged bitsets and the
  serial passes walk them in id order (`physics_world.c`); split-candidate
  reduction is a lexicographic max with island-id tie-break (`solver.c`); bullet order
  is nondeterministic but only feeds a min (`solver.c`); sensor hits are sorted and
  deduplicated (`sensor.c`).
- Cross-platform determinism on 64-bit platforms, as documented in `docs/faq.md` and
  `docs/simulation.md`, via `-ffp-contract=off` (`CMakeLists.txt`), width-4 SIMD everywhere,
  custom trig. So a 64-bit Linux server and a 64-bit Windows client can be bit-exact.
- Hash tables are never iterated by anything that affects simulation state, only queried. Nothing
  orders by pointer value. Manifold blocks
  come from a mutex-guarded allocator so their addresses vary, but nothing reads addresses.
- Id pools are LIFO free lists plus a bump index (`id_pool.c`); generations live in the
  element slots and survive free. Restoring pool state and slot contents makes a replayed
  creation sequence reproduce identical ids and generations.

One caveat: `b3_lengthUnitsPerMeter` (`core.c`) is process-global and feeds the slop
constants. It is not world state; it must simply not change.

## 4. Overview

Every cross-step byte of a world falls into one of four classes, and each class gets a
different treatment. The classification is the design.

| Class | What | Per-tick treatment |
|---|---|---|
| **Hot** | Index-addressed awake state rewritten every step for every awake member: awake solver set, graph colour arrays, awake contact records and manifolds, awake body/shape/joint/island records, moved-proxy list, sensor overlaps, world scalars | **Image**: flat copy into the ring slot, O(awake) |
| **Cold** | World-sized, id-addressed structures mutated only by structural events: sleeping/static/disabled sets, records of non-awake bodies/shapes/contacts/joints/islands, id pools (including each tree's own proxy-id pool, §7.4), the `world->sensors` array, the shared hull database's refcounts, pair set, graph colour bitsets | **Undo journal**: every mutation logs what reversing it needs, O(events) |
| **Scratch** | Per-step bitsets, event arrays, arenas, prepared constraint buffers, profile | Nothing; fully rewritten before use |
| **Derived** | Broad-phase tree layout (which node holds which proxy) | Rebuilt from fat AABBs at restore (needs the three order changes of §7.4); proxy identity itself is cold, not derived |

A ring slot for tick t holds the hot image at end of step t plus the journal segment for the
mutations made during step t and the API calls between step t−1 and step t.

`Rewind(T)` from the current tick P: undo journal segments P…T+1 in reverse, copy image T over
the hot structures, fix the trees, and discard the slots after T. Cost is
O(awake at T + sensor count and overlaps + journal bytes between T and P + (restored shape records + moved proxies) · log n).
Nothing is rejected.

Resimulation is ordinary `b3World_Step`. Each replayed step captures a new slot for its tick, so a
later correction can rewind into the corrected timeline.

The current tick is always the newest retained tick. There is no position inside the ring other
than its head, no slot that belongs to a timeline other than the current one, and therefore
nothing to invalidate when the caller writes to the world after a rewind.

This is the Box2D-v3 architectural invariant, "sleeping sets are frozen", turned into a
snapshot policy. Everything the engine already keeps out of the hot loop stays out of the
per-tick copy.

## 5. State inventory

Sizes are single-precision (measured with a `sizeof` probe against `src/`).

### 5.1 Hot (imaged)

| Structure | Element | Written per step at |
|---|---|---|
| `solverSets[awake].bodySims` | `b3BodySim` 216 B | finalize `solver.c`, CCD `solver.c` |
| `solverSets[awake].bodyStates` | `b3BodyState` 64 B | integrate and finalize `solver.c` |
| `solverSets[awake].contactIndices` | int | begin/end processing `physics_world.c` |
| `solverSets[awake].islandSims` | 4 B | sleep/split/merge |
| `colors[0..23].jointSims` | `b3JointSim` 444 B | prepare/solve in each joint file |
| `colors[0..23].convexContacts`, `.contacts` | 4 B / 12 B | graph add/remove, `manifoldStart` at `solver.c` |
| `contacts[id]` for every awake contact id | `b3Contact` 216 B + `b3Manifold` 268 B × count + mesh `triangleCache` | collide `physics_world.c`, narrow phase `contact.c`, store impulses `contact_solver.c` |
| `bodies[id]` for every awake body | `b3Body` 136 B | `sleepTime` `solver.c` (results-affecting), `bodyMoveIndex`, `sleepVelocity`, flags |
| `shapes[id]` (whole record) for every shape owned by an awake body | `b3Shape` 208 B | `aabb`, `fatAABBs[id]` at `solver.c`; `density`/`filter`/`material`/geometry/`flags` only via API setters, never per-step |
| `joints[id]` for awake joints | `b3Joint` 72 B | structural only, but cheap to image with the sims |
| `islands[id]` + link arrays for awake islands | 64 B + 4/12/12 B per link | link/unlink on every begin/end touch |
| moved proxies | proxy key (tree and id) | `B3_MOVED_NODE` bits set in step t, consumed by pair update in t+1 (`broad_phase.c`); sleeping does not clear them, so the list is read from the trees, not from awake shapes |
| `sensors[i].overlaps2` | 8 B per overlap | `sensor.c` |
| world scalars | one struct | `splitIslandId` (chosen end of t, consumed in t+1, `solver.c`), `compoundShapeCount` (gates compound pair discovery, `broad_phase.c`), the enable flags (`enableSleep`, `enableWarmStarting`, `enableContinuous`, `enableSpeculative`, `enableRestitutionPropagation`), `gravity`, `hitEventThreshold`, `restitutionThreshold`, `restitutionIterations`, `maxLinearSpeed`, `contactSpeed`, `contactHertz`, `contactDampingRatio`, `contactRecycleDistance`, `endEventArrayIndex`, `inv_h` and `inv_dt` (set at the start of each step, read between steps by the joint force and torque getters and the contact-force debug draw) |

Two facts specific to box3d, unlike Box2D v3: there is no `b3ContactSim`, so the hot contact
bytes live in the id-addressed `world->contacts` array plus heap manifolds and must be
*gathered* rather than memcpy'd; and there is no move buffer, the tree nodes' moved bits are
the move buffer.

`b3_isFast` on `b3BodySim` is set in finalize and read by the next step's narrow phase
(`physics_world.c`); it rides in the body sim image. `b3ContactSpec.manifoldStart` and the
colour's `wideConstraints` pointers are recomputed every step and are scratch.

Bodies image their whole `b3Body` record even though only a few fields are hot-path-written,
because the image is already gathered per record and the rest of the struct is small; shapes
image their whole record for the same reason, not just `aabb`/`fatAABBs`. This matters: a
shape's `density`, `filter`, inline `material`, geometry union and `flags` are written only by
API setters, never by the step's hot path, so §7.1's "hot path covers it, no journal needed"
exemption does not apply to them — and unlike a body's non-hot fields, which are covered because
the body record is already imaged whole, a shape's non-hot fields would be uncovered by *both*
image and journal if only `aabb`/`fatAABBs` were imaged. Imaging the whole record removes the gap
without adding a wake-state-conditional journal path for shape setters.

Estimated bytes per tick: ~470 B per awake body, ~208 B per awake shape (whole record, up from
the ~50 B of `aabb`/`fatAABBs` alone), ~220 B per awake non-touching
contact, ~490 B per touching convex contact (more with multiple manifolds), ~520 B per awake
joint. For the fully-awake stress scenes this is the same order as the whole-world figures in
the perf review (5–51 MB/tick); for `large_world` it is a few KB.

### 5.2 Cold (journaled)

Each cold structure below is written one of two ways: a record through its structure's write
accessor, or a container through its journaled type. Types enforce both, so a write that bypasses
them does not compile. The same rule holds for every kind of structure, applied below to records,
the blocks they own, sensors, world scalars and world-owned containers: outside its defining file
a structure is reachable read-only, and every writable route is a function in that file.

**Records.** The record types are `bodies[id]` with the non-awake `bodySims`/`bodyStates`,
`shapes[id]` with `fatAABBs`, `contacts[id]`, `joints[id]` with the non-awake `jointSims`, and
`islands[id]`. Outside its container's file a record array exposes const-qualified element access
only, so a field assignment through anything but the three functions below is a compile error.

The first is the structure's write accessor (`b3Body_Write`, `b3Shape_Write`, `b3Contact_Write`,
`b3Joint_Write`, `b3Island_Write`), replacing the direct field assignment used at every setter
site today: it takes the completed new value, the record together with its sim and state for a
body or joint, makes the journal call itself with the value it is about to overwrite, and copies
the new value in, so no caller has a journal call; it never returns a pointer to the record. It has
two entry points: the structural one, which every structural function calls, always journals, and
the setter one, which API setters call, journals unless the record's owner is awake.
`b3Body_Write` covers a non-awake body's `bodySims`/`bodyStates` the way `b3Joint_Write` covers
non-awake `jointSims`, since the same setters write the record and its sim (`b3Body_SetTransform`,
`b3Body_SetMassData`, and the rest of `body.c`'s non-awake-owner setters listed below). API
setters between steps (`b3Body_SetLinearVelocity`, the explosion callback) reach an awake owner's
record through its setter entry, which writes it in place without a journal entry (§7.1).

The second is the hot accessor, which the step's own stages use: it asserts the world is inside a
step and that the record's owner is awake, and returns a writable pointer to one element, either a
record of an awake owner (a contact's flags, a body's `sleepTime`, a shape's `aabb` and its
`fatAABBs` entry) or an element of an awake set's or graph colour's arrays. It never returns a
container's count or storage, a non-awake set, or a record of a non-awake owner. The pointer is used for the one write it was
obtained for and not kept; transitions out of the awake set run in the step's serial passes, between
the parallel stages that make hot writes, so no stage holds one across a transition. It restricts who
owns the record, not which field is written: a write to an awake owner's record is never
journaled, whatever the field, because the image of the target tick covers it, and a body or
contact that enters the awake set has its pre-wake bytes journaled by the transition (§7.1).
A structural function writes a record through its write accessor, even inside a step; a structural
write it makes through the hot accessor to a record whose owner is already awake is covered anyway,
by image T or by the pre-wake entries of the function that moved the record into the awake set
(§7.1). A stage takes a
const base pointer from an array once per loop for reads, so the pointer costs one load per stage,
not one per element; no accessor returns a writable base pointer of a sparse record array, whose
entries include cold records.

The third is restore's writer, used by the journal walk's undo and by the image scatter: it takes
recorded bytes, asserts the world is inside `b3World_Rewind`, and does not journal.

A shape's `userShape` renderer handle is host-owned: one function in `shape.c` writes it, for
debug-draw creation between steps and for the release of §10, and it does not journal; a record
write never carries it (§9 step 3).

**Owned blocks.** A record field that points to a heap block the record owns (a contact's
`manifolds` and `triangleCache`, a shape's `materials`, an island's `bodies`, `contacts` and
`joints`) is const-qualified in its pointee, so a const record does not expose a writable block. A
plain pointer (`manifolds`, `materials`) is declared pointer-to-const. An array-valued field
(`triangleCache`, an island's arrays, a sensor's arrays) is declared with a const-data array type
of the same layout as `b3Array`, since a const `b3Array` still holds a writable `data` pointer.
Each block has a write function in its defining file, the only place that converts it back:
`b3Shape_WriteMaterial` for `materials`, with `b3GetShapeMaterials`, which returns a writable
pointer from a const shape today, replaced by a const-returning accessor; the manifold block and
triangle cache entries of §7.1; and the array push and removeswap of an island's arrays. A
record's own write copies the pointer, not what it points to. For an awake contact the hot
accessor also returns writable access to its manifold block and mesh triangle cache, since the
collide and impulse-store passes write them and the narrow phase allocates and frees the block in
place, and it returns no other owned block.

**Sensors.** `world->sensors` is a journaled array of `b3Sensor` elements, so a sensor is created
and destroyed by a push and a removeswap of a completed value, and `shapeId` is read-only outside
that array. Its type offers only push and removeswap: an element owns three heap arrays, and the
container's clear and set entries have no rule for handing them back. Its `hits`, `overlaps1` and `overlaps2` arrays are owned blocks by the rule above. The
sensor pass and the continuous sensor-hit write change the arrays of every sensor, whichever state
its owner is in (the pass swaps and clears each sensor's arrays every step, including a sensor on
a static, sleeping or disabled body), so the hot accessor gives them a view of one sensor's three
arrays by sensor index, with no awake-owner assertion, and not the sensor element, so `shapeId`
and the other fields are unreachable. The `overlaps1`/`overlaps2` swap and the `hits` writes make
no journal entry, since `overlaps2` is imaged and the other two are scratch. On a removeswap the
moved sensor's owning shape's `sensorIndex` update is an ordinary `b3Shape_Write` (table below).

**World scalars.** The simulation scalars (the world fields §5.1's last row images, every one of
them) are one struct type, `b3WorldScalars`, which the world holds through a pointer-to-const, so
any file reads them and none assigns one. The only writers are `b3World_WriteScalars` and
restore's copy of the imaged struct (§9), both in the one file that converts the pointer back.
`b3World_WriteScalars` takes the completed struct and makes no journal entry, since the struct is
imaged. Setters of a simulation scalar (`b3World_SetGravity`) and structural code that writes one
(`compoundShapeCount` in `b3CreateShapeInternal`, `b3DestroyShapeInternal` and `b3DestroyBody`)
reach it that way. Callback registration and their contexts, `userData`, `workerCount` and the
task-system callbacks are host configuration (§10), not members of the struct: they are plain
world fields that are not journaled, and restore leaves them alone (§9).

Structural transitions — island and solver-set create and destroy — are each performed by
exactly one function (`b3CreateIsland`/`b3DestroyIsland`, `b3DestroySolverSet`, and a
`b3CreateSolverSet` that replaces the inline set allocation in `b3TrySleepIsland` and
`b3CreateBody`), and that function makes the journal entry itself; every caller, `b3DestroyBody`
included, reaches it. Merge, split, sleep and wake are compositions of those functions, the record
accessors and the containers below, and carry no journal calls of their own.

**Containers.** World-owned pools, sets, bitsets and arrays are journaled by the container that
holds them, not by the caller. The world's fields of these kinds have journaled types:
`b3JournaledSlotPool` (an id pool together with the sparse arrays it grows in lockstep — `shapes`
and `fatAABBs` for the shape pool, one array for each other pool — so a bump-alloc pushes every
array and its undo truncates every array in one operation), `b3JournaledPairSet`,
`b3JournaledBitSet` and `b3JournaledArray`. Their only mutators take the world's journal, and take
completed values: `b3JournaledArray` offers push(value), removeswap, clear and set(index, value),
and no pointer-returning `b3Array_Emplace` and no writable `count`, so every element is complete
when it enters the array, except in an awake set's arrays and the graph colours' arrays, which are
the same container type but are imaged and make no entry. They are distinct types from the generic
`b3IdPool`, `b3HashSet`, `b3BitSet` and `b3Array` primitives, which stay unjournaled and keep
serving state this design does not track (`b3BitSet` is also used for per-step scratch bitsets in
`sensor.c`, `solver.c` and `contact_solver.c`, which must not be journaled), so a caller that
reaches a world-owned container through a generic primitive passes the wrong type and does not
compile. Each journaled container type, and each record array's type inside a slot pool, is an
incomplete struct type whose definition, storage fields included, sits in a header included only
by that container's own file, and the world holds each by a pointer to that type, allocated at
world creation (a by-value field needs the complete type, which would expose the storage to every
file that includes the world struct). That file holds every function that produces a writable
element pointer: the container's mutators and the undo of their journal entries, the hot
accessor's element function, restore's writer, and, for a record array, the write function that
the structure's write accessor enters. So an element, count or pointer assignment anywhere else
does not compile, and there is no per-call-site journal call to miss. The one-time migration
redirects each current direct-write caller to its accessor or container type, and the
const-qualified and incomplete types then remove direct-write access from every other translation
unit. Where a structure's storage cannot be moved out of reach of direct assignment without
further module-boundary work — joint setters, for instance, are spread across every joint type's
own file — that is a finding against the surrounding code to fix there, not a reason to enumerate
call sites and check afterward that none were missed.

| Structure | Current direct-write callers, to migrate |
|---|---|
| `bodies[id]` and non-awake `bodySims`/`bodyStates` | `b3CreateBody`, `b3DestroyBody`, `b3CreateContact`/`b3DestroyContact` (`contact.c`: **a static body's record and its neighbouring contacts' edge keys are written on every contact create/destroy**, and those neighbours can be sleeping contacts), `b3WakeSolverSet`, `b3TrySleepIsland`, `b3MergeSolverSets`, `b3TransferBody`, `b3MergeIslands`, `b3SplitIsland`, `b3RemoveBodyFromIsland`, `b3UpdateBodyMassData`, every mutating `b3Body_*` API in `body.c` (the `Set*`, `Apply*` force, torque and impulse, `Enable*`/`Disable` and mass families, including `b3Body_SetTransform` and the other setters when the body is not awake), `b3Shape_ApplyWind`, explosion callback |
| `shapes[id]`, `fatAABBs` (non-awake owner) | `b3CreateShapeInternal`, `b3DestroyShapeInternal`, `b3ResetProxy`, `b3Shape_Set*` (`shape.c`); outside `shape.c`, `b3DestroyBody` (frees its shapes' ids), `b3Body_SetTransform` (shape and fat AABB writes), `b3Body_EnableHitEvents` (shape flags) (`body.c`), and sensor destruction's swap-moved shape `sensorIndex` write (`sensor.c`) |
| `contacts[id]` (non-awake, or create/destroy) | `b3CreateContact`, `b3DestroyContact`, wake/sleep/merge transitions (`solver_set.c`), `b3RefreshBodyContactIndices` (`body.c`) |
| `joints[id]` and non-awake `jointSims` | `b3CreateJoint`, `b3DestroyJointInternal`, `b3TransferJoint`, joint setters (all joint files) |
| `islands[id]` and link arrays | `b3CreateIsland`, `b3DestroyIsland`, `b3MergeIslands`, `b3SplitIsland`, link/unlink (`island.c`) |
| solver sets: create/destroy | `b3CreateSolverSet` (new), `b3DestroySolverSet`; each makes its own journal entry |
| solver sets: `bodySims`/`bodyStates`/`jointSims`/`contactIndices`/`islandSims` array contents (non-awake owner) | every push and removeswap goes through `b3JournaledArray` |
| id pools ×6 | `b3JournaledSlotPool` alloc and free |
| `broadPhase.pairSet` | `b3JournaledPairSet` add and remove; only membership is ever queried, so slot layout is not state |
| colour `bodySet` bitsets | `b3JournaledBitSet` set and clear |
| `world->sensors` push/removeswap | `b3JournaledArray` — dense array, swap-compacted on removal like the solver-set arrays; the moved sensor's owning shape's `sensorIndex` field update is an ordinary `b3Shape_Write` record write, the array's own length change is the array push/removeswap entry |

Swap-compaction on removal (awake rows, colour arrays, set `contactIndices`, island arrays,
`islandSims`) rewrites *another* element's `localIndex`/`islandIndex` and, for bodies,
`encodedBodySimA/B` on every contact of the moved body; that rewrite is an ordinary journaled
record write, nothing special, because the journal stores whole records. The compaction itself —
the array shrinking by one, and the removed element's own bytes being gone once the mover is
copied over them — is the array push/removeswap entry (§7.1), not a record write: no record's id
changes, so no record-write entry would otherwise capture it.

A shape's `materials` array and a contact's manifold blocks and mesh triangle cache are
heap-owned pointees, not inline record bytes; `shapes[id]`'s and `contacts[id]`'s own record
writes capture the pointer, not what it points to. §7.1's material block and manifold block
entries cover the pointees; the same ownership-transfer rule as sleeping sets and islands
applies to a destroyed contact's manifold and mesh cache.

A sensor's `hits`, `overlaps1` and `overlaps2` arrays are heap-owned pointees of its `world->sensors`
element. Sensor destruction, written today in both `shape.c` and `sensor.c` and freeing the three
arrays before the removeswap, becomes one function that moves them into the removeswap entry; the
entry owns them until it is undone or evicted, the same ownership-transfer rule as a destroyed
contact's manifold.

A hull shape's geometry is a *shared* pointee, not an owned one: `world->hullDatabase`
(`physics_world.c`) is a reference-counted, content-deduplicated map from hull content to
`b3HullData*`, and `b3RemoveHullFromDatabase` frees the hull when its count reaches zero. A
`shapes[id]` record write restores `shape->hull` as a raw pointer; if that hull's last other
reference was also removed after T, the pointer it restores has already been freed, live. The hull
database is mutated only by `b3AddHullToDatabase`, `b3AddOwnedHullToDatabase` and
`b3RemoveHullFromDatabase`; they are its journaled boundary, and the map is not reachable outside
`physics_world.c`. §7.1's hull refcount entry journals the increment/decrement, and reuses the
ownership-transfer pattern: a decrement to zero does not free the hull, the journal entry takes
ownership of it until it is undone or evicted, the same as a destroyed contact's manifold.

### 5.3 Scratch (nothing)

Task-context bitsets, per-step event arrays, arenas, stack, step context, prepared constraint
buffers, `movedSiblings`, `pairKeys`, sensor `hits`/`overlaps1`, profile and counters (`stepIndex`,
which only the step increments and only the serializer reads, and `maxCapacity`, a high-water
statistic behind a public getter, are neither restored nor hashed). Each is
reset before use (`physics_world.c`; `solver.c`;
`broad_phase.c`; `sensor.c`). The double-buffered end-event arrays carry *event*
state across a step boundary (end events produced by API calls between steps are reported
with the next step), not physics state; §9 says how restore treats them.

`world->names` (the name cache behind every `nameId`) is not read by the step. It is
append-only, deduplicated by content hash, and untouched by `Rewind`, so it grows only with distinct
names the caller supplies, a replay that supplies the same names adds none, and its bytes are
outside the ring's budget. A `nameId` is restored with its record, and a
callback that reads a name through the getters reads a value the caller set by an API call, replayed
like any other (§10). A `nameId` is a 32-bit hash of the name's content (`name_cache.c`), so it is a
deterministic function of the name and the state hash covers it (§12);
two names that collide share an id and read back as the first name added to the cache, whichever
timeline added it, so a name getter's string for a colliding name is outside the bit-exact
contract; the `nameId` itself is exact.

### 5.4 Derived (tree layout)

The static, kinematic and dynamic trees (`b3DynamicTree`, 32 B nodes + 4 B parents + 24 B
proxies) contain every proxy in the world, including sleeping bodies, so imaging them breaks
requirement 2. The step's rebuild (`broad_phase.c`, `dynamic_tree.c`) covers the kinematic and
dynamic trees only: it copies every retained subtree into a fresh DFS-ordered array whenever the
tree's root is marked moved or the tree is not DFS-ordered, so the engine's own per-step cost for
those two trees is O(proxies) while they are moving; the static tree is rebuilt only by an explicit
`b3World_RebuildStaticTree` call. The ring must not add a copy of that size per tick. §7.4 makes
tree layout reconstructible from fat AABBs plus the moved list, at the cost of the three order
changes. Which proxy id each shape holds is not derived the same way; §7.4 journals it alongside
the id pools it is structurally identical to.

## 6. Capture

Journal entries are written throughout the step, as each journaled mutator runs (§7.3) — not
only at step end — so they need somewhere to go before the ring even knows this tick's total
size. Each world keeps one small per-tick staging buffer for this (growable, reused tick to
tick, not part of the ring arena); a journal call appends to it directly. At the end of every
`b3World_Step`, after sensors and the end-event flip:

1. The staging buffer's size is now final. Reserve a slot in the ring (§8) sized from the image's
   exact byte count, which is zero on a tick that is not imaged, plus the staging buffer's byte
   count. The image count is a parallel reduction
   over the awake bodies (shapes per body), contacts (manifold counts and mesh cache sizes),
   islands (link array lengths) and sensors (overlap counts), plus the moved leaves counted by the
   enumeration below.
2. **Flat copies** (memcpy, on an imaged tick): awake set arrays, the 24 colours' arrays, world
   scalars. These are contiguous today.
3. **Gathers** (on an imaged tick; parallel-for over the awake population, same task system as the
   step):
   - per sensor: `overlaps2`, a separate heap array per sensor, counted in the sizing pass and copied
     to its own offset;
   - per awake body: `b3Body` record, `b3BodySim`/`b3BodyState` are already in step 2;
   - per awake shape: the whole `b3Shape` record and `fatAABBs[id]` (§5.1);
   - moved proxies: each of the three trees' moved leaves as proxy keys, enumerated by descending
     moved nodes only, as `b3DynamicTree_ClearMoved` does, so a proxy still marked moved whose body
     the step put to sleep is included;
   - per awake contact id (from colour arrays plus awake `contactIndices`): the `b3Contact`
     record, `manifoldCount` manifolds, and the mesh triangle cache when `b3_simMeshContact`;
   - per awake joint: `b3Joint` record;
   - per awake island: `b3Island` record and its three link arrays.
4. Close the journal segment for this tick (§7.1) by moving the staging buffer's bytes into the
   reserved slot and resetting the buffer for the next tick.

Records carry their ids, so gather order is irrelevant and workers can write disjoint ranges.
A fused variant that writes body and contact images from inside finalize and store-impulses,
while the data is in cache, is an optimisation to measure, not a design change.

**Capture interval.** `captureInterval = K` images every K-th tick; journal segments are still
recorded every tick because they are needed to walk back to an image. Restorable ticks are the
imaged ones; `b3World_GetRestorableTick(T)` returns the newest imaged tick ≤ T, and the caller
replays from there. K divides image bytes and image-capture cost by K; journal bytes are
per-tick regardless of K, so total ring memory falls by less than K, by an amount that depends
on the scene's image-to-journal ratio. K trades that against up to K−1 extra replayed ticks.
K=1 is the default.

**Between-step mutations and repeated rewinds.** API calls made after `historyTick`'s step
closed but before the next `b3World_Step` runs are journaled into the *next* tick's segment when
it closes — this is what §4 means by "the API calls between step t−1 and step t". `Rewind` may
itself be called in that same window, with no step in between (two corrections applied back to
back). It treats whatever live entries exist since `historyTick` as an unclosed pending segment:
it undoes them first, the same way it undoes any closed segment, before undoing the segments back
to the requested tick. Once undone, an entry owns no heap block (§7.1), so discarding the pending
entries is resetting the staging buffer, and an undone call never enters the next segment. A step
boundary is required for capture (§8's slot reservation), not for `Rewind` to see and undo
mutations made since the last one.

## 7. Journal

### 7.1 Entries

A journal segment is an append-only byte stream, read only backward. Each entry records what
undoing its mutation needs, and nothing about the value the mutation wrote. Entry kinds:

| Kind | Payload | Undo |
|---|---|---|
| record write | structure tag, id, old bytes | copy old |
| pool alloc | pool tag, id, whether `b3AllocId` popped the free list or bumped `nextIndex` (`id_pool.c`), plus the prior `nextIndex` if it bumped | free the owned blocks the record holds at that moment (a shape's `materials`, a contact's manifold block) and clear each freed pointer and its count; then pop-alloc undo = push the id back; bump-alloc undo = restore the prior `nextIndex` **and truncate the slot pool's record arrays** (`world->bodies`/`shapes` with `fatAABBs`/`contacts`/`joints`/`islands`/`solverSets`) **to that length**, since the slot pool pushes the arrays in the same operation that bumps |
| pool free | pool tag, id, and ownership handles with their byte sizes (§8) for the blocks the freed record owned: a shape's `materials` block, a contact's manifold block, and a mesh contact's triangle cache array header (each empty when there was none), with the shape's `materialCount` (1 for an inline material) and the contact's `manifoldCount` whether or not a block existed | pop the id and hand the blocks back to the record, setting the counts; eviction frees the blocks the entry holds |
| pair set add/remove | key, which of the two | remove / add (a contact's create adds a key that is absent and its destroy removes one that is present, and the entry asserts it) |
| bitset set/clear | colour, body id, the bit's old value | write the old value (a clear of a bit that was already clear, as `b3RemoveContactFromGraph` and the joint removal make for a static body, leaves the bit clear) |
| island create | island id | destroy the island's `bodies`, `contacts` and `joints` arrays with `b3Array_Destroy`, leaving each empty (`b3DestroyWorld` destroys the arrays of every island slot, free ones included) |
| island destroy | island id, ownership handles and byte sizes for the island's `bodies`, `contacts` and `joints` arrays; a destroy is made only by `b3DestroyIsland`, and the split path hands its detached base arrays to that entry instead of freeing them itself | reattach the arrays to the restored record; eviction frees the arrays the entry holds |
| set create (sleep) | set index | destroy the set's `bodySims`, `bodyStates`, `jointSims`, `contactIndices` and `islandSims` arrays, leaving each empty and setting the set's `setIndex` to `B3_NULL_INDEX`, as `b3DestroySolverSet` leaves a free set |
| set destroy (wake) | set index, ownership handle and byte size for each of those five arrays | reattach the arrays and set the set's `setIndex` back to the set index; eviction frees them |
| array push | array tag (a solver set's `bodySims`/`bodyStates`/`jointSims`/`contactIndices`/`islandSims`, an island's link arrays, or `world->sensors`), owner id (set index or island id), old length | truncate to the old length; for `world->sensors`, first free the `hits`, `overlaps1` and `overlaps2` arrays the pushed element holds |
| array removeswap | array tag, owner id, old length, index, and the full old bytes of the removed element (that slot's content is gone once the last element is copied over it, not captured by any record write); for `world->sensors` also the ownership handles and byte sizes of the removed sensor's `hits`, `overlaps1` and `overlaps2` arrays | grow to the old length, copy the element now at the index (the mover) back to the last slot, then write the removed element's bytes at the index (no mover copy when the index was the last slot); for `world->sensors`, hand the arrays back to the element |
| array clear / set | clear: array tag, owner id, old length and all old bytes; set: array tag, owner id, index and old bytes | restore the length and bytes / write the old bytes |
| material block | shape id, and for an element write (`b3Shape_SetSurfaceMaterial`, `b3Shape_SetMeshMaterial`, and every other material write, all made through one `b3Shape_WriteMaterial` accessor) the index and old element bytes, for a resize or replacement the old `materials` array bytes; either way for a write that leaves a block on both sides (inline single-material shapes have no block and are covered by the shape's own record write; journaled whatever the owner's wake state, because an image carries no pointees). A block created or destroyed with its shape is not copied: a destroyed one rides in that shape's pool free entry as an ownership handle, and a created one is freed by the pool alloc's undo | reallocate and copy old |
| manifold block, mesh triangle cache | contact id, which of the two, count, old bytes, for a write that leaves a block on both sides (the wake transition's pre-wake bytes, below, are entries of this kind, made for a contact that has no block too, with count 0 and no bytes; its undo frees whatever block the contact holds live); a block or cache destroyed with its contact rides in that contact's pool free entry as an ownership handle; and, for a mesh contact, one cache entry made by `b3CreateContact` after its record write, whose undo frees the live cache and clears the array header, so a reused slot never holds a live cache in its union storage when an older record's bytes are copied over it | reallocate and copy old |
| proxy create | tree, proxy id (§7.4) | destroy the proxy id |
| proxy destroy | tree, proxy id, fat AABB, category bits, shape id (§7.4) | re-insert the recorded proxy at the recorded id |
| hull refcount | shape id, hull pointer, increment or decrement; a decrement that takes the count to zero moves the hull out of the database into the entry, with its byte size | increment: decrement, and free the hull if the count reaches zero; decrement: increment, moving a hull the entry holds back into the database. An entry frees a hull it holds when evicted |

**Ownership moves one way.** When a sleeping set is destroyed by a wake, its
arrays are not freed; the entry takes ownership. Undo hands them back. The same applies to an
island destroyed by a merge or split, a destroyed contact's manifold block and mesh triangle
cache, a destroyed shape's `materials` block, a destroyed sensor's arrays, and a hull whose last
reference is released. Journal cost for the expensive transitions is O(1) plus
the record writes the engine already makes; nothing is copied twice.

Only entries that destroy something own a block, and a block leaves its entry in exactly one of
two ways: undo hands it back to the world, or eviction frees it. An entry that creates something
owns nothing, because undoing it frees the live block: once the creation is undone, no retained
tick and no future operation can refer to what was created. So after a segment has been undone
none of its entries owns a block, and discarding the segment frees no heap memory and needs no
walk. A slot's charge against the budget (§8) is fixed when the slot is captured and never
changes while the slot is retained.

A record write's undo copies every field except the fields §9 step 3 excludes, so a stale
block pointer, block count or renderer handle in a recorded record never overwrites a live one;
block pointers and counts are bound only by block entries and by the ownership transfers above.

**Rule for record writes.** Every write made by a structural function (create, destroy, link,
unlink, merge, split, sleep, wake, transfer) is journaled unconditionally. Every other write to
a record whose owner is not in the awake set (API setters on sleeping bodies, static bodies'
contact-list heads) is journaled. Writes to awake members from the step's hot path (finalize,
collide, solve) and API writes to an awake owner between steps are not journaled.

The rule is sufficient for a rewind from P to any imaged T ≤ P. Take any record. If its owner is
awake at T, image T holds it and the image scatter, which runs after the journal walk, writes it.
If its owner is not awake at T, consider the writes made to it in (T, P]. Each was either
journaled, or made while the owner was awake. An unjournaled write made while awake must follow a
transition into the awake set that also falls in (T, P], and that transition journals the
record's bytes as they stood before it (below). Undoing the entries in reverse order therefore
ends with the oldest recorded value, which is the record's value at T. §12 tests it.

The rule covers the record itself, not an array a
record's owning set holds it in — `b3TransferBody`/`b3TransferJoint` (`solver_set.c`) and the
sleep/wake/merge transitions already in §5.2's table also push or removeswap the moved record
into a solver set's arrays, which is the array push/removeswap entry above, not a second record
write.

**Entering the awake set.** Body, contact, joint and island records are written structurally by
every transition into the awake set (`setIndex`, `localIndex`, `colorIndex`), so their pre-wake
bytes are journaled by the rule above, and a woken body's and joint's sims stay in the destroyed
sleeping set's arrays, which the set destroy entry owns. Each kind of record has one function that
moves it into the awake set (factored out of the wake, merge and transfer paths, as `b3CreateSolverSet`
is), and that function journals the record's pre-wake bytes itself before it writes any field of it,
so a later write to the record through the hot accessor needs no entry of its own. Shape records
(`aabb`, `fatAABBs[id]`), contact manifold blocks and mesh triangle caches are different: they are
written in place by the step's hot path once their owner is awake, and none of a wake's own record
writes covers them. The body function and the contact function therefore also journal, through
`b3Shape_Write` and a manifold block entry, the pre-wake bytes of every shape, manifold block and
mesh cache they bring with them, and a contact that brings none still makes its entry (count 0).

**Leaving the awake set needs no entry of its own.** A body or contact that the step puts to
sleep in tick t carries the values the hot path wrote earlier in that step, and image t no longer
holds it. Those bytes never need to be recorded, in any of the cases a rewind can meet:

- T < t and the owner was awake at T: image T holds the record, its manifolds and its cache.
- T < t and the owner was not awake at T: it entered the awake set in (T, t], and that
  transition's pre-wake entries restore it.
- T ≥ t and the owner has not woken since: the live bytes are the bytes at T; nothing wrote them.
- T ≥ t and the owner has woken since: the later wake's pre-wake entries recorded exactly the
  bytes the sleep left behind.

The sleep path's structural writes (the body, contact, joint and island records, the pushes into
the new sleeping set, the colour bitset clears in `b3TrySleepIsland`) are journaled by their
accessors and containers like any others.

### 7.2 Size

Per tick, journal bytes ≈ Σ over structural events of the old value of each record they touch: a
contact begin/end touches one contact record, one island record and a bitset bit; a create/destroy
touches a contact record, two body records and up to two neighbour contact records, plus a
pool and pair-set entry. Junkyard-scale churn (thousands of events per tick) is estimated at under
1 MB/tick, an order of magnitude under its image. Wake/sleep transitions cost O(set) record
writes, which the engine pays anyway; a push into a new sleeping set records a length, not the
element.

### 7.3 Hooks

Each record-field structure's write accessor (§5.2) makes its own journal call, such as
`b3JournalBody( world, body )` inside `b3Body_Write`, before assigning the new value. Id-pool,
pair-set, bitset and array mutations are journaled by their container types (§5.2), and the
island and solver-set create and destroy entries by the one function that performs each, so
the structural functions carry no journal calls of their own. The one-time work is migrating
§5.2's record-field callers to their accessor and changing each world-owned container field to
its journaled type; the compiler then rejects any mutator that still reaches one directly.

No accessor or container does anything on behalf of the ring beyond appending its entry. A write
after a rewind is an ordinary write, because the ring holds nothing later than the current tick
for it to invalidate (§8).

### 7.4 Trees are derived, given three order changes

Two different things depend on the trees: layout (which internal nodes hold which proxies, node
rotations) and proxy identity (which integer id a given shape's proxy has in its tree, stored in
`shape->proxyKey`). Layout is a performance property once this section's three order changes
land; proxy identity is simulation-affecting today regardless of that change, because
`shape->proxyKey` is part of the journaled/imaged `shapes[id]` record, and a live tree that
assigned a shape a different proxy id than it had at T would disagree with that record.

**Layout.** Continuous collision passes the running best fraction into the next candidate's
time-of-impact query (`solver.c`), so the min depends on traversal order and the
numerics of each TOI depend on the clamp. Pair discovery is already sorted by shape-pair key
(`broad_phase.c`, "makes contact order independent of tree structure"; a compound's per-child keys
join the same list, and its own child tree is borrowed geometry that restore never touches),
ray/shape casts take a min. Change: CCD evaluates every AABB candidate for the *solid* min against
the initial fraction and takes the min of the accepted results, those a `preSolve` callback does
not reject (or collects candidates and evaluates in shape-id order, keeping the first accepted).
Cost is a few extra TOI evaluations on the rare multi-candidate sweep. After this and the two
order changes below, layout is a performance property, and restore may rebuild it any way it
likes:

- every restore-time write of a shape record, whether a journal undo write or the image
  scatter, goes through one function that appends the shape id to a restore list and touches no
  tree;
- after the journal walk and the image scatter, every proxy alive at T exists, and one
  pass calls `b3DynamicTree_MoveProxy` with the restored fat AABB on the proxy of each listed shape
  whose restored record holds one (`proxyKey` not `B3_NULL_INDEX`; disabled shapes and free slots hold
  none), and a shape listed more than once is moved once per listing, which is idempotent;
- clear each tree's moved bits (`b3DynamicTree_ClearMoved`, which descends only moved nodes),
  then mark each proxy of image T's moved list moved together with its ancestors
  (`b3DynamicTree_MarkProxyMovedSerial`), since `ClearMoved`, the rebuild and pair update all descend by
  the internal nodes' moved bits;
- the next step's rebuild puts the tree back into DFS order as usual.

Every write of a shape's `fatAABBs` entry is followed, in the same step or API call, by a move or
enlarge of its proxy to that value (a bullet's in the serial pass after the sweeps,
`b3DynamicTree_EnlargeProxy` in `solver.c`, which replaces the leaf's box with it), so at a step
boundary each live leaf's AABB equals its `fatAABBs` entry and moving a proxy to its restored fat
AABB reproduces the leaf.

Cost O((restored shape records + moved proxies) · log n). Ring cost for tree layout: zero.

**Proxy identity.** Each tree allocates proxy ids from its own LIFO free list
(`tree->proxyFreeList`, `dynamic_tree.c`), independent of the six id pools §5.2 already
journals. The choke point is `b3BroadPhase_CreateProxy`/`DestroyProxy` (`broad_phase.c`), which
wrap the per-tree allocation and are already the only callers of
`b3DynamicTree_CreateProxy`/`DestroyProxy` for the world's three broad-phase trees (a compound
shape's own child tree, built in `compound.c`, is borrowed geometry, §10, not world state) — a
seventh id-pool-shaped entry, shared by all three trees since the entry records which tree. The
journal call lives inside those two functions, so every caller is covered, not only
`b3CreateShapeInternal`/`b3DestroyShapeInternal`, but also `b3ResetProxy` (`shape.c`), which
destroys and recreates a shape's proxy — possibly *in a different tree* — on a filter or
body-type change; §5.2's table already lists it as a `shapes[id]` write-accessor caller for the
same reason. Undoing a proxy's destruction is a real tree insert, not a bare id reservation, and
in every caller the `shapes[id]` write follows the proxy operation (`proxyKey` is assigned from
the create call's return value), so a destroy entry carries its own payload: the tree, the proxy
id, the fat AABB, the category bits and the shape id. A create entry carries the tree and the id.

Entries are undone in strict reverse journal order, so when an entry is undone the free list is in
the state the recorded operation left it in. A destroy pushed its id onto the head of the free
list, so undoing it with the tree's ordinary create pops that id; the undo asserts the id matches.
A create popped its id from the head, so undoing it with the tree's ordinary destroy pushes the id
back where it came from. Undoing a shape's re-create then destroy through the journal therefore
leaves it with the same proxy id, in the same tree, that it had at T, and `shape->proxyKey` never
needs a post-restore correction. Proxy capacity is not state: growth appends fresh ids in
ascending order only when the free list is exhausted, so a tree left with extra capacity by a
discarded timeline hands out the same id sequence as the original. This is why proxy identity is
not in §5.4's derived class: it is cold and journaled, like the pools it structurally matches.

**Sensor hits during CCD are a separate order dependence.** A CCD sweep collects up
to `B2_MAX_CONTINUOUS_SENSOR_HITS` (8) sensor candidates, each gated on `output.fraction <=
continuousContext->fraction` against the *running* solid fraction as candidates are visited
(`solver.c`) — not the initial fraction the layout fix above uses for the solid min. Which
candidates pass that gate, and which 8 fill the cap first, both depend on tree traversal order,
so the layout change does not make the reported sensor hit set order-independent. The fix
mirrors the pattern already used for the end-of-step sensor pass (§3: "sensor hits are sorted and
deduplicated"): the first traversal finds the solid min and evaluates no sensor candidate; a second
traversal, after the final solid fraction is known, evaluates each sensor candidate and keeps the eight
smallest (sensor shape id, visitor shape id) pairs among those whose fraction is strictly less than the final solid
fraction, as the engine does today, in a fixed eight-slot buffer, so storage stays bounded and the kept set,
the eight smallest of a set, does not depend on visit order, rather than
gating and capping during traversal. Every candidate's time-of-impact query, sensor or solid, is
evaluated against the initial fraction, not the running one, so a sensor candidate's own fraction
does not depend on which candidates were visited before it.

**`b3World_Explode` is a further, separate order dependence.** Its callback wakes sets and
accumulates an impulse into each hit body's velocity state (`physics_world.c`) once per
qualifying shape, in tree traversal order. Wake order fixes the colour and overflow-colour order of
the woken constraints, and a body with more than one shape in the explosion radius accumulates its
impulses in traversal order, where floating-point addition is not associative, so a different
layout produces a different bit pattern even before any solving happens. Fixing this means the
callback only collects the hit shapes; after the query they are sorted by shape id, and sets are
woken and impulses accumulated in that order.

Until the restore-time tree pass covers the kinematic and dynamic trees (phase 2), they are
imaged raw (≈90 B per proxy per tick): same order as the engine's own rebuild copy, but
O(proxies) ring memory, which fails requirement 2 for `large_world`-shaped worlds and is fine for
everything in `benchmark/`. The static tree is world-sized and never imaged, so the order changes
above are required in every phase. Proxy identity journaling is needed either way: the static tree
is never imaged, and the other two trees need it once the raw image is gone.

## 8. Ring

One circular byte arena per world. Slots are variable-length regions written in tick order: a
header, the image, the journal segment. A slot directory maps tick → offset. The ring changes at
its two ends only:

- **The new end.** Capture appends the slot for the next tick. `Rewind(T)` removes every slot
  after T and moves the write position back to the end of slot T.
- **The old end.** When the writer wraps into the oldest slot, that slot is evicted (the blocks
  its entries own are freed), together with the non-imaged slots after it up to the next imaged
  slot, which no rewind can reach, and the retained window shrinks by those ticks. Segments are only
  walked from the current tick back to an imaged tick T, so the segments of ticks at or before
  the oldest retained imaged tick are never read; `oldestImageTick` is that imaged tick, and
  every segment after it is retained.

`world->historyTick` names the last completed tick, which is always the newest slot in the ring.
Capture increments it, then writes slot `historyTick`. Rewind sets it to T.

**Rewind discards, by construction.** The slots after T describe a timeline the caller is
replacing, and nothing in this design can return to them, so `Rewind` removes them as part of the
same call, after undoing their segments. Removing them is moving the write position: their images
are flat bytes, and their entries own no block once undone (§7.1). Every retained tick is
therefore always on the current timeline, without any write path having to notice that a rewind
happened and without any slot carrying a mark of which timeline it belongs to.

- `maxBytes` is the target for retained bytes. When a capture would not fit even after evicting
  down to a minimum window (say two imaged ticks, with the journal segments between them;
  `tickCount` is a separate limit that can leave fewer, and it never widens the interval), the
  capture interval doubles, as the recording player's keyframe ring
  does (`recording_replay.c`), and never past `tickCount`, beyond which a wider interval retains
  no fewer image bytes. `b3World_GetHistoryInfo` reports the effective
  interval and window. Doubling the interval trades how *often* an image is taken; it does not
  shrink any single tick's own image or journal segment, so it cannot help when one tick's own
  slot — awake state that size, or a burst of structural churn — exceeds `maxBytes` outright.
  That tick's slot is admitted anyway, and so is a minimum window whose slots each fit but do not fit
  together (journal bytes are not reduced by widening), growing the arena past `maxBytes` for as
  long as those slots are retained; `b3World_GetHistoryInfo`'s `bytesUsed` reports the overrun
  rather than hiding it. `maxBytes` is therefore a target the ring holds to under normal churn,
  not a hard ceiling.
- A slot's charge against `maxBytes`, and its share of `bytesUsed`, is its header, image and
  journal bytes plus the byte size of every heap block its journal entries own (a destroyed
  sleeping set's, island's or contact's arrays, a destroyed shape's `materials` block, a destroyed
  sensor's arrays, a hull); each owning entry records that size. The charge is computed once, at
  capture, and a slot is evicted whole, so the oldest retained slot's own journal segment, which
  no rewind reads, stays retained and charged until then and a charge never changes while its slot
  is retained; blocks owned by entries still in the staging buffer (destroys made since the last step)
  are charged when that segment closes, and are not in `bytesUsed` before then. Evicting the slot frees the blocks and releases the charge; removing the slot in a
  rewind releases the charge, the blocks having gone back to the world.
- Capacity growth (a scene grows) is a realloc of the arena with slot offsets preserved; rare
  after warm-up.
- Once the arena has warmed up, the ring slot is carved from existing capacity and nothing is
  allocated for it: its size is only known at step end
  (image bytes from the §6 count, journal bytes from the staging buffer's final size, §6),
  and reservation happens then. The per-tick journal staging buffer (§6) can itself grow during
  the step on an unusually large burst of structural churn; it is reused tick to tick and grows
  rarely once warmed up to the scene's typical churn, the same amortized sense as the arena's own
  capacity growth above, but it is not claimed to be allocation-free the way the ring slot is. Its
  capacity is outside `maxBytes`, `bytesUsed` counts retained history and not arena capacity, and
  both capacities are reported separately (`b3HistoryInfo`).

`b3World_EnableHistory` is called at a step boundary with no end events queued by API calls since
the last step (it asserts this; tick 0's image carries no event state, and `Rewind(0)` clears both
end-event buffers). It sets `historyTick` to 0 and images the world as it stands as the slot for
tick 0, with an empty journal segment; `Rewind(0)` restores that world, and the ticks after it count
steps from there. `tickCount` bounds the window independently of `maxBytes`: after each capture,
slots are evicted from the oldest end while the ticks from the oldest retained imaged tick to
`historyTick` exceed `tickCount` and a later imaged tick remains, so an unbounded byte budget
still retains only `tickCount` ticks plus the interval to the next image.

`b3World_DisableHistory` frees every slot, the blocks their entries own, the blocks owned by
entries still in the staging buffer, the arena and the staging buffer. `b3DestroyWorld` does the
same first, before it tears down any structure those blocks were allocated from (the manifold block
allocators, the hull database).

## 9. Restore

`b3World_Rewind( worldId, T )`, at a step boundary, world not locked. P is `historyTick` on entry.

1. **Check** T is imaged, inside the window, and not later than P; else
   `b3_historyTickUnavailable`. No other failure exists.
2. **Journal walk.** First undo the live entries since P and reset the staging buffer (§6),
   including when T == P. Then for each segment t = P down to T+1, apply its entries in reverse
   order. Pools, pair set, bitsets, sleeping sets, islands, non-awake records and sims are now
   exactly as at the end of step T, except awake-at-T structures that the image overrides next.
3. **Image copy.** Set the awake set arrays and colour arrays to the image
   counts, growing capacity only when an image count exceeds it, and memcpy. Scatter
   body/shape/joint/island records by id. For each imaged contact:
   resize its manifold block to the image's count, copy the manifolds, and resize and copy its mesh
   triangle cache when `b3_simMeshContact`; likewise resize and copy each imaged island's `bodies`,
   `contacts` and `joints` arrays. Records are copied field by field excluding the fields bound
   elsewhere: a contact's `manifolds` and `manifoldCount` and, for a mesh contact, `triangleCache` (a
   convex contact's cache, which shares its union storage, and a mesh contact's `queryBounds` are
   copied), a shape's `materials` and `materialCount` and an island's three arrays keep their live
   blocks and counts (a shape's material block is restored by the journal), and a shape's `userShape`, a host-owned renderer handle, is never
   copied; the shape write releases and clears it (§10).
   The world scalar struct (§5.2); sensor overlaps, fat AABBs.
4. **Trees** per §7.4.
5. **Scratch, events and ring.** Clear event arrays and both end-event buffers, and set `bodyMoveIndex` to null on every awake body, since it indexes the cleared
   body-move event array. The task-context bitsets are left alone: they are sized by world-wide id
   capacity, and the stage that uses each clears it first, so a clear here would add an O(world)
   term. Events for step T are not re-delivered; end events that were queued
   between steps at P are dropped. Remove the slots after T (§8) and set `historyTick = T`.
6. In validation builds, run `b3ValidateSolverSets`, `b3ValidateContacts`,
   `b3ValidateConnectivity`, `b3DynamicTree_Validate`.

Ids and generations of everything alive at T are restored, including free-slot generations, so
handles the caller held at T are valid again and handles created after T are invalid. Replaying
the same creation calls in the same order recreates them with the same ids.

When P is imaged, `Rewind(P)` with no tick to go back over is meaningful: it undoes every API call
made since the last step and restores image P over the awake state those calls wrote without a
journal entry. When P is not imaged, those calls are undone by rewinding to the newest imaged tick
before P.

## 10. Caller contract

- **Rewind is one-way.** `Rewind(T)` discards every tick after T. The ticks between T and the
  present exist again only when the caller steps through them. A caller that wants to look at the
  past without giving up the present records what it needs to look at (poses for a lag-compensated
  hit check, for instance) as the ticks happen.
- **Inputs are every API call.** Rewind undoes body/shape/joint creation, destruction and every
  setter made after T. The caller replays them per tick, exactly as it replays forces and
  velocities, except the world-level host configuration listed under *Callbacks are not state*
  below. No mutation is restored by some other route or left in place: the contract has no list
  of exceptions to remember.
- **Callbacks are not state.** `preSolve`, custom filter, friction/restitution mixers run live;
  the caller's callbacks must be pure functions of their inputs for the replay to be exact. The
  *function pointers themselves* (`world->customFilterFcn` and similar) and `world->userData` are
  also not restored by `Rewind` — the world uses whatever is currently registered, not whatever
  was registered at T. A caller that swaps a callback mid-timeline is changing the rules the
  replay runs under, the same as changing engine code between capture and replay; it is not a case
  bit-exactness covers. Registering a callback or setting `world->userData` writes no simulation
  state and makes no journal entry.
  `preSolve` order within a step, and custom filter order within a CCD sweep, a sensor query or
  pair discovery, follow tree traversal (§7.4) and may differ after a restore; a callback with
  order-dependent side effects breaks the contract. Host state a callback reads
  (`world->userData`'s value, what any `userData` points to, the callback's own context) belongs
  to the caller, which sets each such value to what it was at T before the first replayed step and
  replays its changes per tick like any input, and
  `b3World_ComputeStateHash` covers engine state only, so a lockstep caller combines it with its own
  host-state hash.
- **Query order is not state.** Overlap, ray-cast and shape-cast callbacks receive shapes in tree
  traversal order, which is unspecified (`docs/simulation.md`) and may differ after a restore
  (§7.4). A caller replays the API calls it recorded, not the queries that produced them; a caller
  that issues API calls from a query callback records those calls as inputs.
- **Events are regenerated** for every step replayed after T exactly as originally, because event
  generation is deterministic; step T's own events are not available after `Rewind(T)`. The engine
  does not flag replay; the caller knows it is replaying.
- **Recording.** `b3World_StartRecording` and `b3World_EnableHistory` are mutually exclusive: the
  recording's operation list has no rewind, so playback of a recording that continued after a
  rewind would follow the discarded timeline. Each asserts the other is not active.
- **Worker count** need not match between capture and replay (tested property), but the
  process-global length scale and the build must.
- **Host handles.** `userShape` is never restored. Every restore-time write of a shape record
  (journal undo, image scatter) releases the live `userShape` through `destroyDebugShape` and
  clears it, as an ordinary destroy does, so a handle built for geometry the write replaces never
  survives; the debug draw creates a new one on demand. `userData` is restored to its value at T,
  and what it points to is the caller's, valid for as long as a retained tick can restore the
  owner.
- **Borrowed geometry.** Mesh, height-field and compound data are borrowed by the shape, not
  owned by the engine, and must stay valid *and unmodified* until no retained tick can restore a
  shape that references them; the caller frees them only after the window has moved past the last
  such tick. Geometry referenced only by shapes created after T can be freed once `Rewind(T)`
  returns, since no retained tick holds those shapes.
- **Bodies created after T vanish on rewind** and reappear with the same ids if recreated in
  the same order. Callers that want to keep a mispredicted spawn must recreate it.
- **`b3World_Explode`** collects its hit shapes during the tree query, then wakes sets and applies
  impulses in shape-id order (§7.4), so a replayed explode does not depend on tree traversal.

## 11. Cost, and what "faster" can mean here

### 11.1 Measured baseline

Serializer measurements from the perf review
(`docs/designs/reviews/20260927-001-client-prediction-rollback-perf.md`;
8 workers, 60 Hz / 4 substeps, 9-tick correction). They are the baseline this design is compared
with, not evidence for its own cost; its cost claims are gated by §12 test 5:

| scene | awake | step | 9-tick resim | whole-world copy/tick (old serializer) |
|---|---|---|---|---|
| trees100 | 51 | 0.52 ms | 4.7 ms | 1.0 MB, 20% of step |
| rain | 2,620 | 3.6 ms | 32 ms | 5.2 MB, 18% |
| large_pyramid | 5,051 | 5.1 ms | 46 ms | 11.6 MB, 34% |
| many_pyramids | 10,781 | 11.8 ms | 106 ms | 23 MB, 28% |
| junkyard | 10,586 | 25 ms | 228 ms | 51 MB, 25% |
| large_world | 10 (of 1M) | small | small | 650 MB (fails) |

The image in this design is bounded above by those copy figures for the all-awake scenes
(it drops the world-sized arrays, sleeping sets, static tree and pair-set table they included)
and collapses to kilobytes for `large_world`. The copy share of a step should fall further
because the gather is parallel where the review's memcpy was single-threaded.

Dropping forward scrub does not move these numbers much. The image dominates the ring in every
scene with meaningful awake state, and it is the same image either way; the journal, which is
what shrinks, is an order of magnitude smaller (§7.2). The gain is in what has to be built and
proven, §11.3.

### 11.2 Resimulation is N whole-world steps

That is inherent in bit-exact whole-world semantics and it is the honest cost of this design:
a 9-tick correction fits a 16.7 ms frame up to roughly 1–2k awake bodies, and does not at 10k.
Mitigations, in order of cost:

1. **Sleep.** In a game, the awake set is what is moving. The stress scenes keep 10k bodies
   awake by construction. This is the intended scoping mechanism and it costs nothing.
2. **Capture interval** does not help resim; it only trades memory for replay length.
3. **Time-sliced replay.** The caller may spread a long replay over several frames (e.g. 3
   replayed ticks per frame; ticks that arrive while the world is behind join the replay backlog
   and are stepped in order, since the world cannot step the current tick without its inputs for the
   ticks in between), rendering from the caller's own recorded poses of
   the old timeline for the ticks not yet caught up. The ring cannot serve them: the rewind
   removed the old timeline's slots.
4. **Incremental replay (future).** With bit-exactness, an island whose state at T and whose
   inputs over (T, now] are unchanged evolves identically to the old timeline, so its per-tick
   results could be copied from the old timeline's images instead of recomputed, and only islands
   touched by the correction re-stepped, promoting neighbours the moment a contact reaches them.
   That would be exact, where freezing the bodies outside a predicted scope is an approximation.
   It needs three things this design does not provide: an image every tick (`captureInterval` 1);
   the old timeline's images kept until the replay has passed their ticks, which means `Rewind`
   would hand the removed slots' images to the replay instead of dropping them (their journal
   segments are still undone and dropped); and the events of the copied ticks retained or
   reproduced, since the ring stores none. It is also **not** possible with today's solver,
   because an island's result depends on global order in six ways found by the solver audit:
   contact ids from the global LIFO pool decide begin/end processing order and hence first-fit
   colour assignment (`physics_world.c`, `constraint_graph.c`); the overflow
   colour is solved serially in an array that other islands' removals swap-compact
   (`constraint_graph.c`, `solver.c`); one island split per step is chosen by a
   world-wide max (`solver.c`) and split timing changes sleep timing, which changes
   results because a waking body starts from zero velocity (`solver_set.c`); `anyRestitution`
   is one world-wide flag gating a solver pass (`contact_solver.c`, `solver.c`),
   which matters only with restitution propagation on; wake recolours in the island's stored
   order; and merge survivor choice follows merge order. The changes that would remove them:
   process begin/end per island in shape-pair-key order; keep overflow order island-intrinsic
   (sort by key per step, or per-island lists); an island-local split trigger; per-island
   restitution flag; wake recolouring in shape-pair-key order rather than stored order; merge survivor
   and link order by the lowest body id among the merged islands. Each is a real solver change
   with its own review; none is needed for v1.

### 11.3 What this design does not have

Compared with restoring and resimulating only a predicted scope: no roots, no scope, no boundary
partners, no zero-mass overrides in every joint prepare, no per-body validity stamp, no
pending-split slot, no side table of staged impulses, no destroy-and-recreate of contacts, no
begin/end-resimulation bracket, no spurious end/begin touch events.

Compared with a journal that can also be replayed forward: no new-value half of any entry, no
element bytes in a push, no redo of any entry kind, no entry that holds a block on behalf of
something created, no charge that moves between a slot and the world, no journal entries for
leaving the awake set, no position inside the ring, no slots left behind a rewind, and no hook in
the write paths to discard them.

Restore has one failure mode (tick not retained). The engine changes are:
per-structure write accessors, a handful of journal entry kinds beyond plain record writes
(§7.1), the CCD/sensor/explode order changes (§7.4), a scalar grouping, a hash, and tests.

## 12. Verification

1. **Full state hash.** Extend `b3HashWorldState` (`recording.c`, transforms and
   velocities only) to `b3World_ComputeStateHash`: bodies, sims, states, shape records and their
   material arrays, hull database membership and refcounts (each entry's stored `hash` and refcount
   hashed together and the entries' results summed, since the database is content-deduplicated and
   its map's bucket order is not state), contact records, manifolds and
   impulses, mesh triangle caches, joint sims, island membership and sleep partition, pool state
   (all id pools, including each tree's proxy pool, §7.4, which is hashed as its allocation order:
   free-list ids head to tail with any trailing run of consecutive ascending ids that ends at the last capacity slot
   dropped, then the first id the allocator would hand out after them, so capacity grown by a
   discarded timeline is not hashed), the sensor array and each sensor's `overlaps2`, pair set
   membership, colour bitsets, fat AABBs, each live proxy's tree, id, category bits, shape id and
   leaf AABB read from the tree (equal to its fat AABB, §7.4), each tree's moved-proxy set, and
   world scalars, hashing float bit patterns; sparse id-addressed records in id order, with the
   generation of every slot of each sparse record array that carries one (bodies, shapes,
   contacts, joints), free slots included, ordered arrays (solver-set
   arrays, colour arrays, island link arrays) in stored order. The hash is field-wise over
   semantic values, never over raw struct bytes: pointers
   are replaced by the pointee's content (manifold and material array contents, and for borrowed
   geometry a content hash: a hull's, mesh's or height field's own stored `hash` field, which is
   computed over its whole blob with explicit padding; a compound's is computed over its version,
   materials, and its capsule, hull, mesh and sphere instance arrays with their transforms, in stored
   order, together with the content hash of each hull and mesh embedded in its blob and each mesh
   instance's material index array, excluding its `tree`, whose pointers and layout are derived),
   and padding, addresses, `bodyMoveIndex` and every `userData` word (a host value) are excluded.
   It is the oracle for engine state in everything below, not for host state, and is worth
   exposing publicly for lockstep desync detection.
2. **Restore exactness.** For each benchmark scene, step to steady state, then for 1,000 random
   (T, P) pairs inside the window with T ≤ P, with random API churn between (creates, destroys,
   setters on sleeping bodies, explosions): `hash(Rewind(T)) == hash recorded at T`. Each case
   steps from the restored T back up to a fresh P before the next rewind. The churn includes a
   shape filter change that does not touch the proxy (`invokeContacts` false), a filter change
   with `invokeContacts` set, which destroys and recreates the proxy in its tree, a body-type
   change that recreates it in a different tree, `SetAwake(false)` immediately after each rewind,
   a `b3World_SetUserData` call whose value must survive each rewind, a distinct `userData` word
   on every body, shape and joint (changed by setters during the churn) compared against the value
   recorded at T, creation and destruction of the world's only compound shape, hull shape
   creation, destruction and deduplicated creation, sensor shape creation and destruction, surface
   and mesh material writes on awake, sleeping and static shapes, a touching contact's manifold
   count changing across the sleep and wake of its bodies, a contact creation that rewrites an
   awake neighbouring contact's edge followed by a step that changes that neighbour's manifold
   count, a multi-material shape destroyed and a shape with a different material count created in
   its slot, an inline-material shape destroyed and a multi-material shape created in its slot, a
   convex contact destroyed and a mesh contact created in its slot and the reverse, a contact with
   no block woken, touching and then rewound, a shape whose geometry is replaced
   after T and debug-drawn before and after a rewind, a step that grows a tree's
   proxy capacity followed by a rewind across it, steps taken with different time steps followed
   by a rewind and a joint force getter read before any step, and a body put to sleep in the
   imaged tick while its proxy is
   still marked moved, whose pairs the next step's pair update must still find.
3. **Replay exactness.** After `Rewind(T)`, re-step to P replaying the recorded API calls:
   the hash equals the hash recorded at that tick at every tick from T+1 to P, and each replayed
   step's events (types, contents and order) equal those the original run produced for that tick.
   This is the bit-exact claim. Run it with `preSolve` and custom filter callbacks whose results
   depend on `userData`, with a world `userData` and a callback context changed partway through the
   replayed interval.
4. **Cross-worker and cross-platform replay.** Run 3, hashes and events, at a different worker
   count than capture, and compare `b3World_ComputeStateHash` at fixed ticks of fixed scenes
   against golden values recorded on another platform of the determinism guarantee
   (`test_determinism.c`'s approach); the scenes
   include geometry shapes, pool churn, proxy churn and sleeping islands.
5. **Cost.** Per-scene: capture µs and % of step; image and journal bytes per tick; restore ms
   for 1, 5 and 9 ticks back; replay ms. Measured in a release build, without §9 step 6's
   validation-build checks. Acceptance: capture < 15% of step at 8 workers for
   the all-awake scenes and < 1 µs·awake for `large_world` and for a scene of a million sleeping
   dynamic and kinematic bodies with few awake; restore proportional to awake plus
   sensors and journal; replay equals N × step within noise.
6. **Ring transitions.** Interval widening under `maxBytes`, eviction, a single slot over budget,
   a minimum window over budget, and removal on rewind: a rewind to each retained imaged tick; a
   rewind followed by steps and a second rewind into the re-stepped ticks; two rewinds with no
   step between them; `Rewind(P)` after a force, torque or impulse call on an awake body; and a
   rewind with no step since a create or destroy, a setter on a sleeping body, and a heap-owning
   change (sensor destruction, hull release). After each, the restorable ticks are exactly the
   retained imaged ticks at or before `historyTick`, `bytesUsed` is the sum of the retained slots'
   charges, a request for a tick after `historyTick` returns `b3_historyTickUnavailable`, every
   heap block is either live in the world or owned by an entry of a retained slot, and the block
   allocators' live allocation counts (not their retained capacity) return to baseline after
   `DisableHistory` and after `b3DestroyWorld` with history enabled.
7. **Tree-order independence.** Restore-and-replay cases built to differ in tree layout after a
   rewind from the original run's: a CCD sweep with several competing solid hits, one of them a
   nearer hit that `preSolve` rejects, a sensor and a solid hit at exactly equal fractions, a CCD
   sweep with more than eight sensor candidates, including a multishape fast body whose shapes hit
   the same sensor, an explosion hitting several shapes of one body, and an explosion waking
   bodies in several sleeping sets, some connected by constraints. The full state hash
   and the events of each replayed step match the original run.
8. **Undo sufficiency across sleep and wake.** Restore exactness (test 2) on scenes scripted so
   that, for the rewind target T and the current tick P, each of §7.1's cases occurs for a body
   with touching contacts, a mesh contact and a multi-material shape: asleep at T and woken in
   (T, P]; awake at T and put to sleep in (T, P]; put to sleep in tick T itself; put to sleep at
   or before T and still asleep at P; asleep at T, then woken, put to sleep and woken again in
   (T, P]; and a body moved into the awake set by a body-type change and by `b3Body_Enable`, not a
   wake. Run with `captureInterval` 1 and 4.

## 13. Delivery in phases

- **Phase 0, semantics with existing code.** Put the new API in front of a ring of serialized
  images produced by `b3SerializeWorld` and restored by `b3DeserializeIntoShell`; a rewind
  restores image T and drops the later ones. This is
  O(world) and allocates, but it exists, it is tested, and it lets tests 2, 3 and 4 be written,
  without their `userData` clauses and test 2's geometry-replacement debug-draw clause, which apply
  from phase 1, and the full-state hash be validated before any engine change. It also answers
  whether the game's correction path works end to end. Two things this needs beyond the ring
  plumbing itself: interned geometry (hull/mesh/height-field/compound shapes) goes through a
  recording registry (`world_snapshot.c`) that the ring's images must share and keep alive for the
  retained window; and the reader zeroes host
  `userData` on bodies/joints/shapes rather than restoring it (`world_snapshot.c`), so a host
  whose callbacks depend on `userData` needs to rebind it after every phase-0 restore; and it
  carries a live shape's renderer handle across a restore whenever the slot and generation match,
  even if the geometry was replaced. Neither applies once phase 1's in-place restore replaces the
  serializer.
- **Phase 1, hot image + undo journal.** Per-structure write accessors (§5.2) with the existing
  callers migrated to them and the direct-write access removed from every other translation unit (the
  build fails on any remaining direct write; the phase is not complete until it does), ring arena,
  in-place image restore, and the §7.4 order changes (CCD
  solid min, CCD sensor hits, explode), because the static tree is never imaged and its layout
  after an undone proxy operation is not the original. Kinematic and dynamic trees
  imaged raw as the temporary fallback, which already includes their proxy free lists and moved
  bits: the journal walk undoes proxy operations against the live trees, then the image replaces
  each of those two trees wholesale, so no tree pass runs on them; proxy-identity
  journaling (§7.4) is in from this phase for all three trees, and the restore-time tree pass (§7.4)
  runs on the static tree.
- **Phase 2, derived trees.** Tree layout reconstruction at restore (§7.4) for the kinematic and
  dynamic trees, replacing their raw images. Removes the last O(proxies) term. Phase 1 does not meet
  requirement 2 for a world with many sleeping kinematic or dynamic proxies; requirement 2 is met
  from phase 2, and §12 test 5's cost target on the sleeping dynamic and kinematic scene and test 7
  with layout-changing dynamic and kinematic cases gate it (`large_world`'s million proxies are
  static, so it cannot).
- **Phase 3, optional.** Contiguous hot contact storage (a per-colour contact-sim array as in
  Box2D v3) to turn the contact gather into a memcpy, if phase 1's measurements say the gather
  dominates. Incremental replay per §11.2 (4), if and when a game needs corrections in worlds
  that cannot sleep.

## 14. API sketch

```c
typedef struct b3HistoryDef
{
	int tickCount;        // at least 1 (asserted); retained window, in ticks: the oldest retained image is at most this many ticks before the current tick (a wider capture interval can retain up to one interval more, §8)
	int captureInterval;  // at least 1 (asserted); image every K-th tick; 1 = every tick
	size_t maxBytes;      // target for retained bytes (arena and staging capacity are reported separately, and can exceed it); 0 = unbounded. Exceeding it widens the interval; a slot that cannot fit is still admitted (§8).
} b3HistoryDef;

B3_API b3HistoryDef b3DefaultHistoryDef( void );
B3_API void b3World_EnableHistory( b3WorldId worldId, const b3HistoryDef* def );
B3_API void b3World_DisableHistory( b3WorldId worldId );

typedef struct b3HistoryInfo
{
	uint64_t currentTick;     // last completed tick (historyTick); always the newest retained tick
	uint64_t oldestImageTick; // oldest restorable tick
	int effectiveInterval;    // widened under memory pressure
	size_t bytesUsed;         // retained history, including blocks owned by the journal entries of closed slots
	size_t arenaCapacityBytes;   // ring arena capacity
	size_t stagingCapacityBytes; // journal staging buffer capacity
} b3HistoryInfo;

B3_API b3HistoryInfo b3World_GetHistoryInfo( b3WorldId worldId );

// Newest imaged tick <= tick, or UINT64_MAX if none is retained.
B3_API uint64_t b3World_GetRestorableTick( b3WorldId worldId, uint64_t tick );

typedef enum b3HistoryResult
{
	b3_historyOk,
	b3_historyTickUnavailable, // not imaged, outside the window, or later than the current tick
} b3HistoryResult;

// Restore the whole world to the end of `tick`, which is at or before the current tick.
// O(awake + sensors + journal). Discards every tick after `tick`: the way forward is b3World_Step.
B3_API b3HistoryResult b3World_Rewind( b3WorldId worldId, uint64_t tick );

// Deterministic hash of all simulation-affecting state. Same across the platforms the engine's determinism guarantee covers, and any worker count.
B3_API uint64_t b3World_ComputeStateHash( b3WorldId worldId );
```

Typical correction:

```c
uint64_t T = b3World_GetRestorableTick( w, serverTick );
if ( b3World_Rewind( w, T ) == b3_historyOk )
{
	if ( T == serverTick ) { apply the server correction; }
	for ( t = T + 1; t <= now; ++t )
	{
		replay API calls and inputs for t;
		b3World_Step( w, dt, sub );
		if ( t == serverTick ) { apply the server correction; }
	}
}
```

A call made after step t and before step t+1 is an input to tick t+1, so a rewind to t, or to an
earlier imaged tick when t is not imaged, undoes it and the replay applies it again.

There is no begin/end bracket and no scope. If the caller replays nothing and steps, the world
follows the old timeline bit for bit; that is the property everything else rests on.

## 15. Alternatives considered

- **A journal that also replays forward (rewind plus forward scrub).** Every entry stores the new
  value beside the old one, `Rewind` takes a tick on either side of the current position, and the
  slots after T survive a rewind so the caller can return to them
  (`docs/designs/20260927-002-bit-exact-history-ring.md`). It buys peek-and-return and a timeline
  that can be scrubbed both ways. It costs four things. Record-write payloads double and pushes
  carry their elements. Ownership of heap blocks becomes two-way: an entry that created something
  must hold its blocks while the creation is undone, so blocks and the byte charge that follows
  them move between entries and the world in both directions. Leaving the awake set needs its own
  journal entries, for the bytes a forward walk across the transition has to write. And because
  slots after T outlive the rewind, they must be discarded by the first write that makes them
  stale, which puts a hook in every write path, including the ones that make no journal entry.
  The correction flow uses none of what it buys. Left out; §16 question 4 is what would bring it
  back.
- **Rewind-only, but keep the slots after T until the next write.** Saves nothing the caller can
  use, since nothing can return to them, and brings back the write-path hook.
- **Scoped, consistent rollback.** Restore and resimulate only the predicted islands, with the
  rest of the world frozen (`docs/designs/20260927-001-client-prediction-rollback.md`). Kept as
  the option for worlds that cannot sleep and must correct inside a frame. Its complexity is the
  price of O(predicted) resim; this design does not pay it and does not get it.
- **Reuse the serializer as-is for the ring.** Correct and already tested (phase 0), but
  O(world) per tick with allocation: 650 MB/tick on `large_world`. It is the prototype, not the
  product.
- **OS page write-watching / copy-on-write.** Snapshot only dirtied pages of a world arena
  (`GetWriteWatch`, soft-dirty bits, `userfaultfd`, `mprotect` faults). Zero engine audit and
  automatically bit-exact, but platform-specific, page-granular, and either fault-heavy (a
  fault per newly dirtied page per tick, ~10 ms/tick at 10k bodies) or forward-delta-only
  (needs periodic full keyframes). Rejected for a portable engine.
- **`fork()` snapshots.** Not on Windows; unsafe with worker threads.
- **A lagging shadow world at the confirmed tick,** copied over the live world on correction.
  Doubles step cost every tick to save a copy that costs 20–30% of a step. Rejected.
- **Undo-journal everything, image nothing.** The awake arrays are fully rewritten every tick,
  so a journal of them is an image with worse locality.
- **Bit-exact scoped rollback with today's solver.** Impossible; the six global-order channels
  in §11.2 (4) are why a scoped design has to relax the contract. With the solver changes listed
  there it becomes the incremental-replay layer on top of this design, not a replacement for it.

## 16. Open questions

1. Are the §7.4 order changes acceptable to the solver's owner? Three alter simulation results
   slightly: the CCD solid-fraction min (a tighter min in rare multi-candidate sweeps, and what
   keeps tree layout out of the ring), the CCD sensor-hit collection becoming two-pass, and
   `b3World_Explode`'s wake and impulse accumulation becoming shape-id-ordered.
2. Should `b3World_ComputeStateHash` be public? It is the natural desync detector for lockstep
   and the test oracle here; making it public commits to its coverage.
3. Should journaling be always-on or enabled with history? Always-on costs a branch per
   structural write; enabled-only means the hooks must be cheap when disabled (a null check).
4. Does any caller need to return to the present without re-stepping? A server that does lag
   compensation by rewinding the physics world, or a debugging timeline, would. If one does, the
   forward-replaying journal of §15 is the design for it, and the choice should be made before
   phase 1, because it decides the payload of every journal entry and the ownership rules.
5. The recording system's keyframe ring and this ring overlap. Should the player be rebuilt on
   this ring once phase 1 lands? Its backward seeks would become O(awake) rewinds; its forward
   seeks already re-step from the operation list, which a one-way ring does not change.
6. Within one tick's segment, only the first recorded old value of a record matters to undo.
   Skipping later record-write entries for the same record in the same tick would shrink segments
   under repeated setter calls, but needs a per-record mark of "already journaled this tick". Is
   that worth measuring after phase 1, or is the repeated-write case too rare to matter?
