---

title: Bit-exact world history for box3d (hot image + cold journal ring)
status: Draft, round 1. Alternative to docs/designs/20260927-001-client-prediction-rollback.md.
  That design relaxed bit-exactness to keep capture and resimulation proportional to the predicted
  subset, and paid for it with scoped-resimulation machinery that took nine review rounds to
  converge. This design keeps bit-exactness, accepts engine changes, and gets its simplicity from
  never simulating a subset: the whole world is rewound and the whole world is re-stepped.
  Code references are against HEAD 5643cd8 and were gathered by three read-only audits of the
  step pipeline, the determinism properties, and the solver's order dependence; they have not
  been checked by execution.
date: 2026-09-27

---

# Bit-exact world history for box3d

## 1. Problem

Client prediction with server reconciliation needs to put the world back to an earlier tick T,
apply a correction, and re-step to now. `docs/faq.md:145` says box3d has no such mechanism.
Design 001 built one that is *consistent* but not bit-exact, scoped to the predicted islands,
with non-scope bodies frozen during resimulation.

This document asks a different question: what is the smallest, fastest, most robust design if
the restored world must be **bit-exact** (re-stepping from the restored state reproduces the
original timeline bit for bit, so a replay with unchanged inputs is indistinguishable from never
having rolled back), and solver changes are on the table?

The answer has three parts:

1. **Box3d already has most of it.** The recording system serializes the whole world into a raw
   struct image (`src/world_snapshot.c`), restores it in place for backward seeks
   (`src/recording_replay.c:2707`), re-steps, and checks per-step state hashes. The
   `ScrubBackward` test (`test/test_recording.c:251`) asserts that a keyframe restore plus
   re-step reproduces the forward pass's hashes, for the transforms and velocities
   `b3HashWorldState` covers. Bit-exact rewind of a whole world at that granularity is therefore
   a demonstrated property of the engine, not a research question; §12 extends the hash to the
   rest of simulation-affecting state, and that stronger claim is what this design's own tests
   establish. The FAQ statement is stale.
2. **What is missing is an in-place, O(awake) form of it, allocation-free for its image writes**,
   with a ring that can be scrubbed to any retained tick, and a public API. The serializer is
   O(world) per capture, allocates on restore, and copies static and sleeping state that never
   changes.
3. **The cost that cannot be designed away is resimulation.** Re-stepping N ticks of the whole
   world costs N steps. Sleep is the mechanism that keeps that small in real games; for stress
   scenes with 10k awake bodies it does not fit a frame, and §11 says what can be done about
   it, including the solver changes that would allow exact per-island incremental replay later.

## 2. Requirements

1. **Bit-exact.** After `Rewind(T)`, every simulation-affecting byte equals its value at the end
   of step T. Re-stepping with the same inputs reproduces the original ticks exactly, including
   ids, generations, events, island membership, and sleep timing.
2. **Per-tick cost proportional to awake state and structural churn.** Never proportional to
   the number of sleeping or static bodies. `large_world` (1M bodies, 10 awake) must cost a few
   KB per tick, not 650 MB (`docs/designs/reviews/...-perf.md:152`).
3. **Restore never rejected** for any imaged tick inside the retained window, and never leaves
   the world inconsistent. There is no "outside churn" failure mode because the whole world is
   restored. A retained-but-unimaged tick (§6) is reached by replaying forward from the nearest
   imaged tick (§9), not by a direct `Rewind` call.
4. **Scrubbable.** Any imaged tick can be restored from any current position, backward or
   forward, without re-stepping; every tick at or after the oldest surviving image is reachable
   by restoring the newest imaged tick at or before it and replaying forward (§6). A tick whose
   journal segment is still physically in the arena but predates the oldest surviving image is
   not reachable and is not part of the retained window (§8).
5. **Bounded memory** with graceful degradation: a byte budget widens the capture interval
   instead of failing, except that the minimum window is kept anyway even when its total required
   storage — every one of its ticks' journal segments plus the one image it must carry — exceeds
   the budget (§8).
6. **No approximations and no new solver participant kinds.** Resimulation is `b3World_Step`.
7. **Verified by test, not by audit.** The set of journaled writes is checked by hashing cold
   state in validation builds, so a missed write site fails a test rather than a review — except a
   proxy-reset hook, whose only effect is a field deliberately excluded from that hash (§12), and
   which therefore needs its own separate coverage check.

Non-goals: capture or resim proportional to the *predicted* subset (design 001's requirements
1–2). §11 covers what that would take on top of this design.

## 3. What the engine guarantees today

From the determinism audit (`docs/faq.md:134-147`, `docs/simulation.md:1995-2011`,
`test/test_determinism.c`):

- Same inputs reproduce the same run. No RNG, no timers in the simulation path.
- Results are independent of worker count. Per-worker results are OR-merged bitsets and the
  serial passes walk them in id order (`physics_world.c:907-923`, `938-1029`); split-candidate
  reduction is a lexicographic max with island-id tie-break (`solver.c:2200-2219`); bullet order
  is nondeterministic but only feeds a min (`solver.c:786-791`); sensor hits are sorted and
  deduplicated (`sensor.c:237-252`).
- Cross-platform determinism via `-ffp-contract=off` (`CMakeLists.txt:77-89`), width-4 SIMD
  everywhere, custom trig. So a Linux server and a Windows client can be bit-exact.
- Hash tables are never iterated, only queried. Nothing orders by pointer value. Manifold blocks
  come from a mutex-guarded allocator so their addresses vary, but nothing reads addresses.
- Id pools are LIFO free lists plus a bump index (`id_pool.c:19-45`); generations live in the
  element slots and survive free. Restoring pool state and slot contents makes a replayed
  creation sequence reproduce identical ids and generations.

One caveat: `b3_lengthUnitsPerMeter` (`core.c:42`) is process-global and feeds the slop
constants. It is not world state; it must simply not change.

## 4. Overview

Every cross-step byte of a world falls into one of three classes, and each class gets a
different treatment. The classification is the design.

| Class | What | Per-tick treatment |
|---|---|---|
| **Hot** | Index-addressed awake state rewritten every step for every awake member: awake solver set, graph colour arrays, awake contact records and manifolds, awake body/shape/joint/island records, moved-proxy list, world scalars | **Image**: flat copy into the ring slot, O(awake) |
| **Cold** | World-sized, id-addressed structures mutated only by structural events, plus sensor overlap changes (identified by the sensor task's own per-step change signal, not gated on the owning body's awake state): sleeping/static/disabled sets, records of non-awake bodies/shapes/contacts/joints/islands, id pools, pair set, graph colour bitsets, changed sensor overlaps | **Journal**: every mutation logs old and new value, O(events) |
| **Scratch** | Per-step bitsets, event arrays, arenas, prepared constraint buffers, profile | Nothing; fully rewritten before use |
| **Derived** | Broad-phase trees | Rebuilt from fat AABBs at restore (needs one small CCD change, §7.4) |

A ring slot for tick t holds the hot image at end of step t plus the journal segment for the
mutations made during step t and the API calls between step t−1 and step t.

`Rewind(T)` from current position P: apply journal segments P…T+1 in reverse (undo) or T…P+1
forward (redo), then copy image T over the hot structures, then fix the trees. Cost is
O(awake at T + journal bytes between T and P + moved proxies · log n). Nothing is rejected.

Resimulation is ordinary `b3World_Step`. Each step overwrites the ring slot for its tick, so a
later correction can rewind into the corrected timeline.

This is the Box2D-v3 architectural invariant, "sleeping sets are frozen", turned into a
snapshot policy. Everything the engine already keeps out of the hot loop stays out of the
per-tick copy.

## 5. State inventory

Sizes are single-precision (measured with a `sizeof` probe against `src/`).

### 5.1 Hot (imaged)

| Structure | Element | Written per step at |
|---|---|---|
| `solverSets[awake].bodySims` | `b3BodySim` 216 B | finalize `solver.c:723-814`, CCD `solver.c:562-620` |
| `solverSets[awake].bodyStates` | `b3BodyState` 64 B | integrate `solver.c:165-219`, finalize `741-771` |
| `solverSets[awake].contactIndices` | int | begin/end processing `physics_world.c:797-819` |
| `solverSets[awake].islandSims` | 4 B | sleep/split/merge |
| `colors[0..23].jointSims` | `b3JointSim` 444 B | prepare/solve in each joint file |
| `colors[0..23].convexContacts`, `.contacts` | 4 B / 12 B | graph add/remove, `manifoldStart` at `solver.c:1540` |
| `contacts[id]` for every awake contact id | `b3Contact` 216 B + `b3Manifold` 268 B × count + mesh `triangleCache` | collide `physics_world.c:581-795`, narrow phase `contact.c:732`, store impulses `contact_solver.c:901-913` |
| `bodies[id]` for every awake body | `b3Body` 136 B | `sleepTime` `solver.c:776/809` (results-affecting), `bodyMoveIndex`, `sleepVelocity`, flags |
| `shapes[id].aabb`, `fatAABBs[id]` for awake shapes | 24 + 24 B | `solver.c:850-858` |
| `joints[id]` for awake joints | `b3Joint` 72 B | structural only, but cheap to image with the sims |
| `islands[id]` + link arrays for awake islands | 64 B + 4/12/12 B per link | link/unlink on every begin/end touch |
| moved proxies | shape id | `B3_MOVED_NODE` bits set in step t, consumed by pair update in t+1 (`broad_phase.c:653-698`) |
| world scalars | one struct | `stepIndex`, `inv_h`, `inv_dt`, `compoundShapeCount` (selects a broad-phase path), `splitIslandId` (chosen end of t, consumed in t+1, `solver.c:2200`, `1717`), enable flags, gravity, thresholds, `endEventArrayIndex`, `userData` (caller-opaque, never read internally, imaged like any other scalar field) |

Two facts specific to box3d, unlike Box2D v3: there is no `b3ContactSim`, so the hot contact
bytes live in the id-addressed `world->contacts` array plus heap manifolds and must be
*gathered* rather than memcpy'd; and there is no move buffer, the tree nodes' moved bits are
the move buffer.

`b3_isFast` on `b3BodySim` is set in finalize and read by the next step's narrow phase
(`physics_world.c:649`); it rides in the body sim image. `b3ContactSpec.manifoldStart` and the
colour's `wideConstraints` pointers are recomputed every step and are scratch.

Estimated bytes per tick: ~470 B per awake body, ~50 B per awake shape, ~220 B per awake
non-touching contact, ~490 B per touching convex contact (more with multiple manifolds),
~520 B per awake joint. For the fully-awake stress scenes this is the same order as the
whole-world figures in the perf review (5–51 MB/tick); for `large_world` it is a few KB.

### 5.2 Cold (journaled)

Every write site below was enumerated by the inventory audit. Each site gains one journal call
that records (structure, id, old bytes, new bytes) or a semantic entry.

| Structure | Mutating functions |
|---|---|
| `bodies[id]` (non-awake owner, or any transition) | `b3CreateBody`, `b3DestroyBody`, `b3CreateContact`/`b3DestroyContact` (`contact.c:271-289`, `406-432`: **a static body's record and its neighbouring contacts' edge keys are written on every contact create/destroy**, and those neighbours can be sleeping contacts), `b3WakeSolverSet`, `b3TrySleepIsland`, `b3MergeSolverSets`, `b3TransferBody`, `b3MergeIslands`, `b3SplitIsland`, `b3RemoveBodyFromIsland`, `b3UpdateBodyMassData`, and the `b3Body_Set*`/`Enable*`/`AllowFastRotation` API family (`body.c:1601-2512`) |
| non-awake `bodySims`/`bodyStates` | `b3Body_SetTransform` and the other setters when the body is not awake (`body.c:1125-2406`), explosion callback, `b3UpdateBodyMassData` writing the owning body's `bodySim` regardless of awake state (the shape-creation family and `b3DestroyShape` when `updateBodyMass` is set, `b3Shape_SetDensity`, `b3Body_ApplyMassFromShapes`, `b3Body_SetType`, `b3Body_SetMotionLocks` on a fixed-rotation change) |
| `shapes[id]` fields other than `aabb`/`fatAABBs`/`proxyKey`/`userShape` (filter, material, `materials` array, flags, geometry) | `b3CreateShapeInternal`, `b3DestroyShapeInternal`, `b3Shape_Set*`/`Enable*` (`shape.c:1141-1684`), `b3Body_EnableHitEvents` (sets every owned shape's flags, `body.c:2524-2538`), unconditionally — the hot path never rewrites these fields, awake owner or not |
| `shapes[id].aabb`, `fatAABBs` (non-awake owner) | `b3ResetProxy`, `b3CreateShapeProxy` (initial shape creation, `b3Body_Enable`, `b3Body_SetType`'s recreate pass), and the bounds recompute inside `b3Body_Set*`/`b3Shape_Set*` when the owning body is not awake; a body's wake also checkpoints its shapes' bounds (`b3WakeSolverSet`) for undo, and a body's sleep does the same (`b3TrySleepIsland`) for redo, since neither the pre-wake nor the post-awake value is otherwise journaled |
| tree proxy `categoryBits`, keyed by shape id | every call to `b3CreateShapeProxy` (initial shape creation, `b3Body_Enable`, `b3Body_SetType`'s recreate pass) and `b3ResetProxy` when called with `invokeContacts=true` (`b3Shape_SetFilter`) — every site that creates a fresh proxy, all of which set its category bits from `shape->filter.categoryBits` at that moment; diverges from `shape->filter.categoryBits` whenever a live proxy persists through an `invokeContacts=false` `b3Shape_SetFilter` call, so it is journaled as its own field rather than derived from the shape record at restore |
| tree proxy reset (destroy and recreate in the same tree), keyed by shape id | `b3ResetProxy` when called with `destroyProxy=true` (`b3Shape_SetFilter` with `invokeContacts=true`, and other shape setters that recreate the proxy); `shape->proxyKey` itself is never journaled or restored directly (it is excluded from the generic shape-record row above), since a reset changes it to a new numeric id from the tree's live free list without necessarily changing bounds, category, or body type — §7.4 uses this entry, not a `proxyKey` value, to know when a shape's proxy needs rebuilding |
| `contacts[id]` (non-awake, or create/destroy) | `b3CreateContact`, `b3DestroyContact`, wake/sleep/merge transitions (`solver_set.c:86-140`, `291-390`, `520`), `b3RefreshBodyContactIndices` (`body.c:72-94`), `b3UpdateBodyMassData`/`b3Body_SetMassData` clearing every neighbour contact's `relativeTransformValid` flag (`body.c:895-1055`, `1856-1945`) |
| `joints[id]` and non-awake `jointSims` | `b3CreateJoint`, `b3DestroyJointInternal`, `b3TransferJoint`, joint setters (all joint files) |
| `islands[id]` and link arrays | `b3CreateIsland`, `b3DestroyIsland`, `b3MergeIslands`, `b3SplitIsland`, link/unlink (`island.c:20-337`, `388-649`) |
| solver sets (sleeping/static/disabled) | `b3TrySleepIsland` (creates a set, `solver_set.c:195-215`), `b3WakeSolverSet` (destroys one), `b3MergeSolverSets`, `b3CreateBody` (a body created asleep gets a set) |
| id pools ×6 | `b3AllocId`, `b3FreeId` (`id_pool.c:19-45`) |
| `broadPhase.pairSet` | `b3AddKey`, `b3RemoveKey` (`table.c:137`, `163`); only membership is ever queried, so slot layout is not state |
| colour `bodySet` bitsets | `constraint_graph.c:107-140`, `181-182`, `237-268`, `312-313`; `solver_set.c:353-354`, `416-417` |
| `sensors[]` create/destroy | `shape.c:234-242`, `527-563`, `sensor.c:414-450` |
| `sensors[shapeId].overlaps2` content | `b3SensorTask`, whenever the sensor's own per-step `eventBits` bit is set (`sensor.c`) |

Swap-compaction on removal needs nothing beyond the image for awake rows and colour arrays,
since the whole array is re-imaged every tick regardless of what moved. For a non-awake solver
set's dense arrays (`bodySims`, `bodyStates`, `jointSims`, `contactIndices`, `islandSims`),
island link arrays, and `sensors[]`, a swap-remove is a dense-cold-array entry (§7.1): the moved
element's own record write (already covered) restores its content, but not the array's length or
which slot it occupies, which the append/swap-remove entry restores instead. The moved element's
*other* referencing records are ordinary journaled record writes, since only their value changes,
not their own array's shape: for bodies, `encodedBodySimA/B` on every contact of the moved body;
for sensors, the moved sensor's owning shape's `sensorIndex`.

### 5.3 Scratch (nothing)

Task-context bitsets, per-step event arrays, arenas, stack, step context, prepared constraint
buffers, `movedSiblings`, `pairKeys`, sensor `hits`/`overlaps1`, profile and counters. Each is
reset before use (`physics_world.c:886`, `1055-1065`; `solver.c:1813-1815`, `1874-1877`;
`broad_phase.c:668`; `sensor.c:295`). The double-buffered end-event arrays carry *event*
state across a step boundary (end events produced by API calls between steps are reported
with the next step), not physics state; §9 says how restore treats them.

### 5.4 Derived (trees)

The static, kinematic and dynamic trees (`b3DynamicTree`, 32 B nodes + 4 B parents + 24 B
proxies) contain every proxy in the world, including sleeping bodies, so imaging them breaks
requirement 2. The rebuild (`dynamic_tree.c:1867-1992`) already copies every retained subtree
into a fresh DFS-ordered array whenever anything moved, so the engine's own per-step cost is
O(proxies); the ring must not add a copy of that size per tick. §7.4 makes the tree
reconstructible from fat AABBs plus the moved list, at the cost of one CCD change.

## 6. Capture

At the end of every `b3World_Step`, after sensors and the end-event flip:

1. Reserve a slot in the ring (§8) sized from the awake counts.
2. **Flat copies** (memcpy): awake set arrays, the 24 colours' arrays, world scalars. These are
   contiguous today.
3. **Gathers** (parallel-for over the awake population, same task system as the step):
   - per awake body: `b3Body` record, `b3BodySim`/`b3BodyState` are already in step 2;
   - per awake shape: `aabb`, `fatAABBs[id]`, and the proxy id if its moved bit is set;
   - per awake contact id (from colour arrays plus awake `contactIndices`): the `b3Contact`
     record, `manifoldCount` manifolds, and the mesh triangle cache when `b3_simMeshContact`;
   - per awake joint: `b3Joint` record;
   - per awake island: `b3Island` record and its three link arrays.
4. Close the journal segment for this tick (§7.1).

Records carry their ids, so gather order is irrelevant and workers can write disjoint ranges.
A fused variant that writes body and contact images from inside finalize and store-impulses,
while the data is in cache, is an optimisation to measure, not a design change.

**Capture interval.** `captureInterval = K` images every K-th tick; journal segments are still
recorded every tick because they are needed to walk between images. Restorable ticks are the
imaged ones; `b3World_GetRestorableTick(T)` returns the newest imaged tick ≤ T, and the caller
replays from there. K trades per-tick capture cost and ring memory (÷K) against up to K−1 extra
replayed ticks. K=1 is the default.

## 7. Journal

### 7.1 Entries

A journal segment is an append-only byte stream. Entry kinds:

| Kind | Payload | Undo / redo |
|---|---|---|
| record write | structure tag, id, old bytes, new bytes | copy old / copy new |
| pool alloc/free | pool tag, id, source (free-list pop or bump index) | undo mirrors whichever source the alloc/free actually used, not always a push/pop; a bump-index source also grows or shrinks the pool's paired sparse array in lockstep (`bodies`, `shapes` and `fatAABBs`, `contacts`, `joints`, `islands`, `solverSets`), keeping pool capacity and array count equal for `b3ValidateSolverSets` |
| pair set add/remove | key | remove / add (self-inverse) |
| bitset set/clear | colour, body id | clear / set; omitted when the operation would not change the bit's live value (e.g. clearing a static body's bit, which is never set) |
| set create | set index, ownership handle | undo: detach arrays into the entry; redo: reattach |
| set destroy | set index, ownership handle | undo: reattach arrays; redo: detach |
| dense cold arrays (island link arrays, non-awake solver-set arrays, `sensors[]`) | owning id, ownership handle; or for an append: (array, appended element's bytes); or for a swap-remove: (array, removal index, old length, removed element bytes) | reattach; or for an append: undo truncates the length by one, redo re-appends the stored bytes; or for a swap-remove: undo grows the length by one, moves the slot currently at the removal index to the new last slot, then writes the removed element's bytes into the removal index, redo re-applies the swap-remove at that index |
| manifold block | contact id, count, old manifold bytes, new manifold bytes, and (for a mesh contact) old/new triangle-cache bytes | reallocate and copy either way |

**Ownership transfer instead of copying.** When a sleeping set is destroyed by a wake, its
arrays are not freed; the entry takes ownership. Undo hands them back. The same applies to an
island destroyed by a merge or split, to a shape's `materials` array, to a mesh contact's
`triangleCache` array, and to a destroyed sensor's `hits`, `overlaps1`, and `overlaps2` arrays
(heap-owned allocations, `overlaps2` in §5.2's inventory) whenever a write would free or
reallocate one: the entry takes ownership of the old array instead of storing its bytes inline,
so undo hands back a live array rather than dereferencing a freed pointer. A shape's `hull`
pointer references a refcounted entry in the world's hull database (`b3AddHullToDatabase`/
`b3RemoveHullFromDatabase`), freed when its count reaches zero: a journaled write that installs a
new hull takes an extra reference on both the old and the new hull data, for as long as the entry
is retained — undo would otherwise drop the new hull's only reference, and redo the old hull's,
either of which can leave a later scrub installing a dangling pointer. A plain record write is
enough only when the write does not change which hull entry the shape references. Undo and redo of
a hull-changing entry, unlike a plain record write, call `b3AddHullToDatabase`/
`b3RemoveHullFromDatabase` themselves on the shape's `hull` pointer field, exactly as the original
`b3Shape_SetHull` call did — acquiring a real reference on the hull being installed and releasing
one on the hull being replaced — so the shape's own database reference always mirrors whichever
hull is its current live value, scrub after scrub, the same invariant the live engine maintains
outside any history. The entry's separate pair of extra references exists only to keep both
hulls' data from being freed by one of those real releases hitting zero while the entry is still
retained; released unconditionally, both at once, on eviction, since past that point neither undo
nor redo can reach this entry again and whichever hull the shape doesn't currently use no longer
needs protecting. Journal cost for the expensive transitions is O(1) plus the record writes the engine
already makes; nothing is copied twice. Arrays owned by evicted journal segments are freed on
eviction.

A structural write to a touching contact (create, destroy, sleep, wake, merge, split, transfer)
fires a manifold-block entry (above) for its manifold and, for a mesh contact, its triangle
cache, instead of the generic record write those fields would otherwise get.

A write to one element of a multi-material shape's heap `materials` array (`b3Shape_SetFriction`,
`SetRestitution`, `SetSurfaceMaterial`, `SetMeshMaterial`) journals that element's old and new
bytes, keyed by shape id and element index, not the shape's own record. A single-material shape's
setters write its inline `material` field instead, already covered by the shape's own record.

**Rule for record writes.** Every write made by a structural function (create, destroy, link,
unlink, merge, split, sleep, wake, transfer) is journaled unconditionally. Every other write to
a record whose owner is not in the awake set (API setters on sleeping bodies, static bodies'
contact-list heads) is journaled. Writes to awake members from the step's hot path (finalize,
collide, solve) are not journaled; the image covers them, except a shape's `aabb`/`fatAABBs`: the
first such write after a wake has no earlier image to fall back on, and the last such write
before a sleep has no later image to hand off to, so both boundaries are covered by checkpoints
(`b3WakeSolverSet`, `b3TrySleepIsland`; §5.2) instead. A field the hot path never rewrites
for any owner — a shape's filter, material, `materials` array, geometry or flags; a joint's
tuning parameters — is journaled on every write regardless of the owner's awake state, since
skipping the journal is only safe for fields the image actually contains. This rule is
sufficient in both directions because a body's record is either in image T (awake at T) or was
last written by a journaled event between T and P; §12 tests it.

### 7.2 Size

Per tick, journal bytes ≈ Σ over structural events of the records they touch: a contact
begin/end touches one contact record, one island record and a bitset bit; a create/destroy
touches a contact record, two body records and up to two neighbour contact records, plus a
pool and pair-set entry. Junkyard-scale churn (thousands of events per tick) is on the order of
1 MB/tick, an order of magnitude under its image. Wake/sleep transitions cost O(set) record
writes, which the engine pays anyway.

### 7.3 Hooks

Each site in §5.2 gains a call such as `b3JournalBody( world, body )` before the write, or a
semantic call (`b3JournalAllocId`) inside the pool/table/bitset function itself. The pool,
table, bitset and tree modules each have two to four entry points, so those hooks are
mechanical. The record sites are the audit surface; §12's cold-hash guard is what makes a
missed site a failing test.

A record-write hook captures old bytes only on that record's first touch within the currently
open segment (a second hook call for the same record in the same tick, e.g. from a mutator that
writes the same struct more than once, is a no-op against an entry that already exists for it).
New bytes for every record touched in the segment are captured once, when the segment closes
(§6 step 4), by reading each touched record's then-current live bytes — so a record written
several times in one tick still gets exactly one entry, whose old/new bytes span the tick's net
change, not each intermediate write.

### 7.4 Trees are derived, given one CCD change

The exact tree layout matters to physics in two places. First, continuous collision passes the
running best fraction into the next candidate's time-of-impact query (`solver.c:420`, `453`),
so the min depends on traversal order and the numerics of each TOI depend on the clamp. Second,
continuous collision caps recorded sensor hits at eight and fills that array in traversal order
(`solver.c:313-436`), so which candidates make the cap — not just their fractions — depends on
traversal order whenever a sweep crosses more than eight sensors. Pair discovery is already
sorted by shape-pair key (`broad_phase.c:733-748`, "makes contact order independent of tree
structure"), sensor hits are sorted after collection. The closest-hit ray cast
(`b3World_CastRayClosest`) finds the globally minimum fraction, but its callback
(`b3RayCastClosestFcn`, `physics_world.c`) currently overwrites the result on every candidate at
that fraction with no comparison, so which shape wins an exact tie depends on traversal order.
The general callback-based cast and overlap APIs (`b3World_CastRay`, `b3World_CastShape`,
`b3World_OverlapShape`, `b3World_OverlapAABB`, `b3World_CastMover`, `b3World_CollideMover`) can
prune later candidates based on the fraction their own callback returns, so their traversal
order is caller-visible.

Change: CCD evaluates every AABB candidate against the *initial* fraction and takes the min of
the results (or collects candidates and evaluates in shape-id order) to determine the final
solid fraction first; only then does it admit a sensor candidate whose own fraction is at most
that final solid fraction, not the running fraction seen during solid-candidate evaluation, and
collects every admitted sensor candidate before applying the eight-hit cap in shape-id order
rather than traversal order. `b3World_Explode` collects every candidate shape from its query,
sorts by shape id, then wakes
bodies and applies impulses in that order, instead of doing both inline during tree traversal.
`b3RayCastClosestFcn` only overwrites the result when the candidate's fraction is strictly
smaller, or equal and its shape id is lower, instead of overwriting unconditionally. This
tie-break is incomplete for a shape type whose own internal cast keeps a running best-so-far
fraction and only replaces it on a strictly smaller candidate (`b3RayCastMesh`, `mesh.c`;
`b3ShapeCastHeightField`, `height_field.c`, which every height-field ray cast also runs through):
such a shape silently reports no hit at all when its true nearest intersection exactly ties the
query's current limit, so it can never reach the callback's shape-id comparison and can never win
a tie against a shape type without that internal shrinking search — a residual order dependence on
top of the two §10 already documents, not eliminated by this change. A compound child of either
type inherits the same gap through the same functions; the tree traversal's own pruning
(`b3DynamicTree_RayCast`, `<=`, not `<`) is not where the gap is. Cost is a
few extra TOI evaluations on the rare multi-candidate sweep. After this, tree layout is
a performance property for the dynamic and kinematic trees, not a simulation property, and
restore may rebuild either any way it likes (the static tree is never rebuilt by ordinary
stepping — only explicitly, by `b3World_RebuildStaticTree` — so restore leaves its layout as
whatever the bullets below produce, not DFS order):

- **Candidate set**, for both bullets below: a shape is examined if it is awake at T; has any
  journaled entry keyed to its own shape id in the walked range (its general record, its
  `aabb`/`fatAABBs` entry, its proxy `categoryBits` entry, or a proxy-reset entry — this includes
  the shape's own creation or destruction, both unconditionally journaled); or is owned by a body
  with a journaled transfer into or out of `b3_disabledSet`, or a journaled body-type change, in
  the walked range — a body-type change recreates every owned shape's proxy (`b3Body_SetType`)
  without journaling anything at the shape's own id, the same gap disable/enable would have left
  were it not already covered by the disabled-transfer clause.
- **Presence.** For every shape in the candidate set, decide whether it should have a live proxy
  at T from state the walk and image copy have already restored: no, if the shape does not exist
  at T or its owning body's restored `setIndex` is `b3_disabledSet`; yes otherwise. Destroy the
  live proxy if one exists and should not; create one, in the tree matching T's restored body
  type, if none exists and one should — passing T's already-restored `shape->aabb`/`fatAABBs[id]`
  (§5.1, §5.2) directly as the new proxy's bounds and T's journaled proxy `categoryBits` (§5.2) as
  its category, rather than calling the ordinary live proxy-creation path, which recomputes both
  from the current transform and can leave a narrower fat AABB than T's — awake shapes coast on a
  fat AABB wider than their tight bounds until movement escapes it (`solver.c`), and a fresh
  recompute would silently shrink it. This bullet alone determines whether a shape has a proxy
  after it runs; every bullet below only ever touches a proxy this bullet left in place, never one
  it just created or destroyed.
- **Properties**, for every shape in the candidate set whose proxy was already live before this
  restore and remains live after the bullet above (a proxy that bullet just created or destroyed
  is already exactly right, including its category bits, which restore sourced from T's journaled
  `categoryBits` precisely because that can differ from what a live creation reading
  `shape->filter` right now would produce): for every such shape whose fat AABB differs between the
  live tree and image T, `b3DynamicTree_MoveProxy`; for every such shape whose journaled proxy
  `categoryBits` at T (§5.2) differs from the live proxy's, `b3DynamicTree_SetCategoryBits` to that
  journaled value, never derived from `shape->filter`, since the two can diverge while a proxy
  persists live across an `invokeContacts=false` filter change; for a shape whose journaled body
  type at T differs from its live proxy's tree (`B3_PROXY_TYPE(shape->proxyKey)`), a proxy cannot
  move between trees, so destroy the live proxy and recreate it in the tree matching T, with T's
  journaled category bits, instead; for a shape with a journaled proxy-reset entry (§5.2) in the
  walked range, destroy the live proxy and recreate it in the same tree with T's journaled
  category bits, even when its fat AABB, category, and body type all already match the live
  proxy's, since a reset changes `shape->proxyKey` without necessarily changing any of those three;
- clear all moved bits, then for each shape id in image T's moved list, mark its current proxy
  moved (propagating to ancestors the same way the hot path does), resolving the id through the
  shape's own, already-restored `proxyKey` rather than a numeric proxy id, which a proxy
  recreated by either bullet above need not still have;
- the next step's rebuild puts the dynamic and kinematic trees back into DFS order as usual; the
  static tree keeps whatever layout the bullets above left it in until the caller next calls
  `b3World_RebuildStaticTree`.

Cost O(moved · log n). Ring cost for trees: zero.

Three residual order dependences remain: two documented in §10 (the order of `preSolve`
callbacks CCD triggers during a sweep, and the traversal order seen by a caller's own cast,
overlap, or mover callback), plus the closest-ray mesh tie-break gap above.

If the CCD change is not accepted, the fallback is to image the kinematic, dynamic, and static
trees raw (≈90 B per proxy per tick): same order as the engine's own rebuild copy, but O(proxies)
ring memory for the kinematic and dynamic trees, which fails requirement 2 for
`large_world`-shaped worlds and is fine for everything in `benchmark/`. CCD also queries the
static tree (`solver.c`); its tree only needs re-imaging when a static shape is created or
destroyed, `b3World_RebuildStaticTree` is called, or a static shape's proxy moves or resets
(`b3Body_SetTransform` moves any body's shape proxies including a static body's; `b3ResetProxy`
does the same for a shape setter), keeping its ring cost O(events) rather than O(static proxies)
per tick.

## 8. Ring

One circular byte arena per world. Slots are variable-length regions written in tick order: a
header, the image, the journal segment. A slot directory maps tick → offset. When the writer
wraps into the oldest slot, that slot is evicted (its owned arrays freed) and the retained
window shrinks by one tick. Eviction is always oldest-slot-first and does not skip ahead to the
next image, so evicting an imaged slot can leave younger, image-less slots physically still in
the arena with no surviving image at or before them; §2 requirement 4 excludes those slots from
the retained window, so they are inert bytes awaiting physical overwrite, not a correctness gap —
`oldestImageTick` (§14) is the authoritative start of what's actually reachable.

- `maxBytes` caps the arena **and** the arrays ownership-transfer journal entries hold, plus any
  hull-database bytes kept alive only by a journal-held reference (§7.1) — a detached sleeping
  set's or island's arrays, or a hull an entry is the last reference to, count against the same
  budget, not just the arena's own bytes, or a single large wake/sleep transition or hull
  replacement could exceed a small budget unbounded. When a capture would not fit even after evicting down to a minimum window (say 2
  ticks), the capture interval doubles, as the recording player's keyframe ring does
  (`recording_replay.c:2648-2675`); unlike that unbounded recording, already-retained images are
  never purged for falling off the new, wider grid — this ring only ever evicts the oldest slot,
  on ordinary FIFO wraparound. Widening the interval only reduces how many retained ticks carry an
  image on the usual K-th-tick cadence going forward; a capture takes an image outside that cadence
  whenever skipping it would leave the current minimum window's oldest tick without an image at or
  before it — keeping every tick in the minimum window reachable (requirement 4), not merely
  physically retained. Every tick in the window still carries a journal segment (§6), and the
  minimum window's oldest tick always has a surviving image at or before it; that anchor image can
  predate the window itself, since a single old anchor keeps satisfying "at or before" for every
  later tick until its own slot is evicted, so every intervening tick's journal segment, from the
  anchor through the window's newest tick, must also survive to bridge the gap back to it. So if
  this total required storage — the anchor image plus every journal segment from the anchor's tick
  through the window's newest tick — exceeds `maxBytes`, the ring exceeds `maxBytes` for that
  window rather than dropping correctness-required state. `b3World_GetHistoryInfo` reports the effective interval and
  window, and its `bytesUsed` reports true usage even when it is over budget. Both `maxBytes` and
  `bytesUsed` count the arena's allocated capacity, not just the bytes its live slots currently
  occupy — amortized realloc growth (this bullet, and the journal-room growth two bullets below)
  can leave reserved capacity ahead of occupied bytes, and requirement 5's "bounded memory" promise
  is about the ring's actual footprint, not a logical slot-bytes tally that could understate it.
- Capacity growth (a scene grows) is a realloc of the arena with slot offsets preserved; rare
  after warm-up.
- An image's size is known up front from the awake counts and each awake contact's own
  `manifoldCount` and, for a mesh contact, its triangle-cache length, so writing it never
  allocates inside the step. A journal segment's size is not known up front — it grows as the step's structural
  writes occur — so the arena reserves journal room with the same amortized realloc growth as
  the previous bullet, not a single upfront allocation.

`world->historyTick` names the last completed tick. Capture increments it, then writes slot
`historyTick`. Rewind sets it to T. Ordinary stepping after a rewind overwrites slots T+1… and
discards their old journal segments; the branch is implicit.

`b3World_EnableHistory` itself captures an image at the world's current `stepIndex` (0 for a
world that has never been stepped) and sets `historyTick` to it, before any subsequent
`b3World_Step` runs — the same capture as §6, run once at enable time instead of at a step
boundary — so the enable-time state is itself an imaged, restorable tick, and its own journal
segment closes with that capture, same as any other tick's (§6 step 4). API calls made between
`b3World_EnableHistory` and the first subsequent step enter the next tick's journal segment
instead, opened as soon as the enable-time capture closes — the same rule §4 gives for any
inter-step API call, applied at the enable boundary; the first subsequent `b3World_Step` closes
that segment as usual (§9's existing open-segment handling).

`historyTick` is its own counter, seeded from `stepIndex` at enable time but incremented once per
subsequent `b3World_Step` call regardless of that call's `timeStep`: `stepIndex` (`solver.c`)
advances only inside `b3Solve`, which a `timeStep <= 0` call skips, while the broad-phase pair
update and sensor task both still run and are still captured (§6). `historyTick` and `stepIndex`
can therefore diverge after any zero-time-step call; a caller mapping its own server ticks to
history ticks must track `historyTick`'s own step-call count from enable, not assume it equals
`stepIndex`.

## 9. Restore

`b3World_Rewind( worldId, T )`, at a step boundary, world not locked:

1. **Check** T is imaged and inside the window; else `b3_historyTickUnavailable`. No other
   failure exists.
2. **Journal walk.** Any entries already recorded for calls made since step P but before step
   P+1 has run (the open segment described in §4) are reversed first, exactly like a closed
   segment's entries. Then, if T < P: for each segment t = P down to T+1, apply entries in
   reverse order using old values. If T > P: for t = P+1 up to T, apply entries forward using new
   values. Pools, pair set, bitsets, sleeping sets, islands, non-awake records and sims are now
   exactly as at the end of step T, except awake-at-T structures that the image overrides next.
3. **Image copy.** Resize the awake set arrays and colour arrays to the image counts (growing the
   underlying allocation only if the image is larger than the array's current capacity, never
   shrinking it) and memcpy. Scatter body/shape/joint records by id. For each imaged island:
   resize its three link arrays the same way to the image counts and copy their content, then
   copy the rest of the `b3Island` record, excluding those arrays' own live data pointers. For each
   imaged contact: if the live slot has a manifold block of the right count, copy into it,
   otherwise free and allocate one; for a mesh contact, likewise resize the live
   `triangleCache` to the image count and copy its content; then copy the rest of the `b3Contact`
   record, excluding the `manifolds` and `meshContact.triangleCache` pointer fields, which the
   preceding steps already set correctly. World scalars, fat AABBs.
4. **Trees** per §7.4.
5. **Scratch and events.** Clear event arrays, both end-event buffers, move events, and
   task-context bitsets, and reset every restored body's `bodyMoveIndex` to none. Events for
   step T are not re-delivered; end events that were queued between steps at P are dropped. For
   every shape with a journaled record in the walked range — the only shapes whose generation the
   walk could have changed, since a shape's generation only ever changes at creation
   (`shape->generation += 1`), itself an unconditionally journaled structural write — if its
   pre-restore live `userShape` debug-draw handle is non-`NULL` and either the shape's restored
   generation no longer matches the live one at that id (the slot held a different, or no, shape
   at T) or the walked range includes a journaled geometry-changing write for this shape
   (`b3Shape_SetSphere`/`SetCapsule`/`SetHull`/`SetMesh`, the only setters that destroy a live
   handle without changing generation), destroy that live handle (`world->destroyDebugShape`) and
   leave the restored shape's `userShape` `NULL` for lazy recreation on the next draw; otherwise
   leave the live handle exactly as it is untouched, the same handle-reuse this design borrows
   from the existing serializer's keyframe restore (`world_snapshot.c`). Set `historyTick = T`.
6. In validation builds, run `b3ValidateSolverSets`, `b3ValidateContacts`,
   `b3ValidateConnectivity`, `b3DynamicTree_Validate`, then the cold-hash check (§12).

Ids and generations of everything alive at T are restored, including free-slot generations, so
handles the caller held at T are valid again and handles created after T are invalid, with
their generation check failing exactly as after a destroy. Replaying the same creation calls
in the same order recreates them with the same ids.

## 10. Caller contract

- **Inputs are every API call.** Rewind undoes body/shape/joint creation, destruction and every
  setter made after T. The caller replays them per tick, exactly as it replays forces and
  velocities. This is the Overwatch-model contract and it is simpler than design 001's, which
  restored some mutations and not others.
- **Callback configuration is a caller input, not ring state.**
  `b3World_Set{CustomFilter,PreSolve,Friction,Restitution}Callback` change which function is
  installed; `Rewind` does not restore or undo that change. Unlike every other setter, whose
  T-time value the image or journal restores automatically, the caller must itself reinstall
  whichever callback was active at T immediately after `Rewind(T)`, then replay any later
  callback-configuration change at its correct point during resim. Independently, `preSolve`, custom filter, friction/restitution mixers, and a cast or
  overlap query's own result callback run live and must be pure functions of their inputs for
  the replay to be exact. The ordinary per-contact `preSolve` call (`contact.c`) follows the
  graph-colour array order, which the image restores exactly regardless of tree layout. Only the
  `preSolve` calls CCD triggers for its own candidates (`solver.c`), and the traversal order seen
  by a cast, overlap, or mover callback, follow tree traversal (§7.4) and may differ after a
  restore; a callback with order-dependent side effects breaks the contract for those two cases.
  `b3World_CastRayClosest`'s shape-id tie-break (§7.4) is exact except against a mesh or
  height-field shape, which can silently report no hit instead of losing the tie; a caller using
  its result as a later input inherits that same residual order dependence.
  A query result's own `nodeVisits`/`leafVisits` counts are diagnostic, not simulation-affecting,
  and may also differ after a restore-rebuilt tree; a caller must not feed them back into
  simulation input.
- **Events are regenerated** during replay exactly as originally, because event generation is
  deterministic. The engine does not flag replay; the caller knows it is replaying. Immediately
  after `Rewind(T)`, before any replay, no events are available for tick T itself — §9 clears
  the event arrays rather than restoring them — so a caller that needs T's events must have
  consumed them before rewinding.
- **Worker count** need not match between capture and replay (tested property), but the
  process-global length scale and the build must.
- **Bodies created after T vanish on rewind** and reappear with the same ids if recreated in
  the same order. Callers that want to keep a mispredicted spawn must recreate it.
- **Mesh, height-field, and baked-compound geometry buffers are caller-owned**, unlike a hull
  (which the engine refcounts in its own database, §7.1). The engine only stores the pointer
  passed at shape creation or by a later geometry setter (`b3Shape_SetMesh` and its siblings); a
  journaled record of such a write only copies the pointer, not the geometry. A caller enabling
  history must keep every such buffer alive **and unchanged** for as long as any retained tick
  could still reference the shape with that pointer installed — through the shape's destruction or
  the pointer's replacement, and until the ring evicts the tick, not merely through the buffer's
  own intended lifetime; an in-place edit of the buffer's content, even without moving or freeing
  it, changes what an earlier retained tick collides against, just as freeing it would.
- **`b3World_Explode`** collects every candidate shape from its query, sorts by shape id, then
  wakes bodies and applies impulses in that order (§7.4); a replayed explode reproduces the same
  velocities and wake order as the original run, since shape ids are stable across a rewind and
  replay. Woken-constraint order for overflow constraints (a body with more than 20 constraints)
  follows this same shape-id order, not tree traversal.
- **Body and shape names are debug data, not restored.** `b3Body_SetName`/`b3Shape_SetName`
  insert into a world-wide, append-only name cache (`name_cache.c`) keyed by a 32-bit hash of the
  string; `Rewind` restores each body's or shape's own `nameId` field like any other record field,
  but does not remove or undo insertions already made into the cache. A string added only on a
  timeline later discarded by `Rewind` remains cached; if replay on the new timeline later adds a
  different string whose hash collides with it, `b3AddName` returns the abandoned entry's id and
  `b3Body_GetName`/`b3Shape_GetName` resolve to the abandoned string, not the newly added one. The
  cache never frees an entry once added, so it grows with every distinct name ever inserted whether
  or not history is enabled; this design neither introduces nor bounds that growth, and the cache's
  bytes are not counted against `maxBytes` (§8).

## 11. Cost, and what "faster" can mean here

### 11.1 Measured baseline

From the perf review (8 workers, 60 Hz / 4 substeps, 9-tick correction):

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

### 11.2 Resimulation is N whole-world steps

That is inherent in bit-exact whole-world semantics and it is the honest cost of this design:
a 9-tick correction fits a 16.7 ms frame up to roughly 1–2k awake bodies, and does not at 10k.
Mitigations, in order of cost:

1. **Sleep.** In a game, the awake set is what is moving. The stress scenes keep 10k bodies
   awake by construction. This is the intended scoping mechanism and it costs nothing.
2. **Capture interval** does not help resim; it only trades memory for replay length.
3. **Time-sliced replay.** The caller may spread a long replay over several frames (e.g. 3
   backlogged ticks per frame, each stepped strictly in tick order before any newer tick is
   stepped), rendering the live world's current state after each frame's batch of replayed ticks.
   The world only resumes stepping genuinely live input once replay has caught the backlog up to
   the present, so what the caller renders lags real time by the backlog length divided by the
   per-frame replay rate. This is ordinary rendering of the live world as replay
   steps through it, not a read from the ring — the ring has no read-only pose-access API, so
   this works at any `captureInterval` (§6). The caller must render each old-timeline tick before
   replaying past it, since re-stepping overwrites that slot in place (§8).
4. **Incremental replay (future).** With bit-exactness, an island whose state at T and whose
   inputs over (T, now] are unchanged evolves identically to the old timeline, so its per-tick
   results could be copied from the ring instead of recomputed, and only islands touched by the
   correction re-stepped, promoting neighbours the moment a contact reaches them. That is exact
   where design 001's frozen boundary was an approximation. It is **not** possible today,
   because an island's result depends on global order in six ways found by the solver audit:
   contact ids from the global LIFO pool decide begin/end processing order and hence first-fit
   colour assignment (`physics_world.c:938-1029`, `constraint_graph.c:78-169`); the overflow
   colour is solved serially in an array that other islands' removals swap-compact
   (`constraint_graph.c:185-213`, `solver.c:1271`); one island split per step is chosen by a
   world-wide max (`solver.c:2200-2219`) and split timing changes sleep timing, which changes
   results because a waking body starts from zero velocity (`solver_set.c:61-63`); `anyRestitution`
   is one world-wide flag gating a solver pass (`contact_solver.c:291`, `solver.c:1320`),
   which matters only with restitution propagation on; wake recolours in the island's stored
   order; and merge survivor choice follows merge order. The changes that would remove them:
   process begin/end per island in shape-pair-key order; keep overflow order island-intrinsic
   (sort by key per step, or per-island lists); an island-local split trigger; per-island
   restitution flag. Each is a real solver change with its own review; none is needed for v1.

### 11.3 What is smaller than design 001

No roots, no scope, no boundary partners, no zero-mass overrides in every joint prepare, no
`validSinceTick` on every body, no pending-split slot, no side table of staged impulses, no
destroy-and-recreate of contacts, no `Begin/EndResimulation` bracket, no spurious end/begin
touch events. Restore has one failure mode (tick not retained). The engine changes are: journal
hooks at enumerated sites, one CCD order change, an explosion order change, a closest-ray
tie-break, a wind-force pointer re-fetch after wake, a scalar grouping, a hash, and tests.

## 12. Verification

1. **Full state hash.** Extend `b3HashWorldState` (`recording.c:1188`, transforms and
   velocities only) to `b3World_ComputeStateHash`: bodies (excluding `bodyMoveIndex`, which §9
   always resets to none on restore and is therefore not restore-stable), sims, states, shapes
   (filter, material — the `materialCount`-element `materials` array in full for a multi-material
   shape, not just the inline fallback — and geometry: a hull, mesh, height-field, or compound
   field hashed by its pointed-to content, never by pointer value, so a shared geometry buffer or
   material array hashes identically across processes and platforms), contact records, manifolds
   and impulses, joint records and joint
   sims (including `collideConnected`), island membership and sleep partition, each island's own
   `constraintRemoveCount` (split-candidacy state read by `solver.c` and `solver_set.c`), pool state, pair set
   membership, graph-colour `bodySet` bitsets (by logical bit value up to the body id pool's
   capacity, not raw block count), sensor
   overlaps, shape bounds (`shape->aabb` and `fatAABBs`), each shape's tree-proxy `categoryBits`,
   and moved-proxy membership, world scalars and flags — every body's, shape's, and joint's own
   `userData`, and the world's own `userData`, excluded throughout, caller-opaque and not portable
   across processes — in id order, hashing float bit patterns. This is the
   oracle for everything below and is worth exposing publicly for lockstep desync detection.
2. **Restore exactness.** For each benchmark scene, step to steady state, then for 1,000 random
   (T, P) pairs inside the window, with random API churn between (creates, destroys, setters on
   sleeping bodies, forced sleep/wake toggles, explosions): `hash(Rewind(T)) == hash recorded at
   T`. Backward and forward. Same-process only, also compare every body's, shape's, and joint's
   `userData` pointer (via `b3*_GetUserData`) and the world's `userData` (via
   `b3World_GetUserData`) against the value recorded at T, since the hash above excludes them.
3. **Replay exactness.** After `Rewind(T)`, re-step to P replaying the recorded API calls:
   `hash == hash recorded at P` at every intermediate tick, plus the same same-process `userData`
   comparison as test 2. This is the bit-exact claim.
4. **Cold-hash guard (validation builds).** At capture, hash every cold structure §5.2 lists as
   journaled, in full — sleeping sets, non-awake records, islands and their link arrays, shape
   and joint fields journaled regardless of owner awake state, non-awake shape bounds, pools,
   pair set, graph-colour `bodySet` bitsets (by logical bit value, not raw block count), tree
   proxy `categoryBits`, sensor existence, and sensor overlaps —
   not a separately maintained subset that can drift out of sync with §5.2. At the
   next capture, recompute and compare after replaying the
   segment's entries against a shadow copy. A write that bypassed the journal fails here, in
   whichever test first exercises it. This is what makes §5.2's list a test rather than a
   promise, with one exception: a tree proxy reset entry (§5.2, §7.4) can leave bounds, category,
   and body type all unchanged, changing only `shape->proxyKey`, which is deliberately excluded
   from this hash and from restored record state alike (§7.4) — so a reset whose journal hook was
   missed cannot be caught by this guard whenever it leaves every hashed field unchanged, and
   needs its own coverage check instead (comparing live reset call sites against emitted
   entries).
5. **Cross-worker replay.** Run 3 at a different worker count than capture.
6. **Cost.** Per-scene: capture µs and % of step; image and journal bytes per tick; restore ms
   for 1, 5 and 9 ticks back; replay ms. Acceptance: capture < 15% of step at 8 workers for
   the all-awake scenes and < 1 µs·awake for `large_world`; restore proportional to awake plus
   journal; replay equals N × step within noise.

## 13. Delivery in phases

- **Phase 0, semantics with existing code.** Put the new API in front of a ring of serialized
  images produced by `b3SerializeWorld` and restored by `b3DeserializeIntoShell`. This is
  O(world) and allocates, but it exists, it is tested, and it lets tests 2, 3 and 5 be written
  and the full-state hash be validated before any engine change. It also answers whether the
  game's correction path works end to end. This prototype's serializer clears body, shape, and
  joint `userData` on restore, and leaves the world's own `userData` as whatever the shell
  currently holds rather than restoring it to its value at T (`world_snapshot.c`), so a caller
  relying on those pointers inside a `preSolve`/filter/mixer callback, reading them back off a
  regenerated move or joint event (`solver.c` copies `body->userData`/`joint->userData` into the
  corresponding event), or comparing `b3World_GetUserData` against a recorded value (test 2, §12),
  must restore all four itself immediately after a Phase 0 restore; Phase 1's own per-body/shape/joint
  image and world-scalar image (§5.1) carry these fields naturally, so this limitation is
  Phase 0-only.
- **Phase 1, hot image + cold journal.** Journal hooks, ring arena, in-place image restore,
  cold-hash guard, the wind-force pointer re-fetch after wake. Trees imaged raw as the temporary
  fallback.
- **Phase 2, derived trees.** The CCD order change and tree reconstruction at restore. Removes
  the last O(proxies) term.
- **Phase 3, optional.** Contiguous hot contact storage (a per-colour contact-sim array as in
  Box2D v3) to turn the contact gather into a memcpy, if phase 1's measurements say the gather
  dominates. Incremental replay per §11.2 (4), if and when a game needs corrections in worlds
  that cannot sleep.

## 14. API sketch

```c
typedef struct b3HistoryDef
{
	int tickCount;        // retained window, in ticks (images are kept for at most this many)
	int captureInterval;  // image every K-th tick; 1 = every tick
	size_t maxBytes;      // ring arena cap; 0 = unbounded. Exceeding it widens the interval.
} b3HistoryDef;

B3_API b3HistoryDef b3DefaultHistoryDef( void );
B3_API void b3World_EnableHistory( b3WorldId worldId, const b3HistoryDef* def );
B3_API void b3World_DisableHistory( b3WorldId worldId );

typedef struct b3HistoryInfo
{
	uint64_t currentTick;     // last completed tick (historyTick)
	uint64_t oldestImageTick; // oldest restorable tick
	int effectiveInterval;    // widened under memory pressure
	size_t bytesUsed;
} b3HistoryInfo;

B3_API b3HistoryInfo b3World_GetHistoryInfo( b3WorldId worldId );

// Newest imaged tick <= tick, or UINT64_MAX if none is retained.
B3_API uint64_t b3World_GetRestorableTick( b3WorldId worldId, uint64_t tick );

typedef enum b3HistoryResult
{
	b3_historyOk,
	b3_historyTickUnavailable,
} b3HistoryResult;

// Restore the whole world to the end of `tick`, from any current position. O(awake + journal).
// Ordinary stepping afterwards overwrites history past `tick`.
B3_API b3HistoryResult b3World_Rewind( b3WorldId worldId, uint64_t tick );

// Deterministic hash of all simulation-affecting state. Same on any platform, worker count.
B3_API uint64_t b3World_ComputeStateHash( b3WorldId worldId );
```

Typical correction:

```c
uint64_t T = b3World_GetRestorableTick( w, serverTick );
if ( b3World_Rewind( w, T ) == b3_historyOk )
{
	for ( t = T + 1; t <= serverTick; ++t ) { replay API calls and inputs for t; b3World_Step( w, dt, sub ); }
	apply the server correction;
	for ( t = serverTick + 1; t <= now; ++t ) { replay inputs for t; b3World_Step( w, dt, sub ); }
}
```

There is no begin/end bracket and no scope. If the caller replays nothing and steps, the world
follows the old timeline bit for bit; that is the property everything else rests on.

## 15. Alternatives considered

- **Design 001 (scoped, consistent).** Kept as the option for worlds that cannot sleep and
  must correct inside a frame. Its complexity is the price of O(predicted) resim; this design
  does not pay it and does not get it.
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
  in §11.2 (4) are why design 001 relaxed the contract. With the solver changes listed there it
  becomes the incremental-replay layer on top of this design, not a replacement for it.

## 16. Open questions

1. Is the CCD order change (§7.4) acceptable to the solver's owner? It is one of three changes
   that alter simulation results slightly (a tighter min in rare multi-candidate sweeps for CCD;
   a shape-id impulse order for `b3World_Explode`; a shape-id tie-break for
   `b3World_CastRayClosest`), and it is what keeps trees out of the ring.
2. Should `b3World_ComputeStateHash` be public? It is the natural desync detector for lockstep
   and the test oracle here; making it public commits to its coverage.
3. Should journaling be always-on or enabled with history? Always-on costs a branch per
   structural write; enabled-only means the hooks must be cheap when disabled (a null check).
4. Does `mygame` need forward scrub (redo) at all, or only rewind? Redo doubles journal payload
   (old and new values). If not, journal old values only.
5. The recording system's keyframe ring and this ring overlap. Should the player be rebuilt on
   this ring (in-process replay with O(awake) seeks) once phase 1 lands?
