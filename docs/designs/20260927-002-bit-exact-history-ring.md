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
   forward, without re-stepping; every retained tick, imaged or not, is reachable by restoring
   the newest imaged tick at or before it and replaying forward (§6).
5. **Bounded memory** with graceful degradation: a byte budget widens the capture interval
   instead of failing, except that a tick whose own required journal or image storage exceeds
   the budget is kept anyway (§8).
6. **No approximations and no new solver participant kinds.** Resimulation is `b3World_Step`.
7. **Verified by test, not by audit.** The set of journaled writes is checked by hashing cold
   state in validation builds, so a missed write site fails a test rather than a review.

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
| world scalars | one struct | `stepIndex`, `inv_h`, `inv_dt`, `compoundShapeCount` (selects a broad-phase path), `splitIslandId` (chosen end of t, consumed in t+1, `solver.c:2200`, `1717`), enable flags, gravity, thresholds, `endEventArrayIndex` |

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
| `bodies[id]` (non-awake owner, or any transition) | `b3CreateBody`, `b3DestroyBody`, `b3CreateContact`/`b3DestroyContact` (`contact.c:271-289`, `406-432`: **a static body's record and its neighbouring contacts' edge keys are written on every contact create/destroy**, and those neighbours can be sleeping contacts), `b3WakeSolverSet`, `b3TrySleepIsland`, `b3MergeSolverSets`, `b3TransferBody`, `b3MergeIslands`, `b3SplitIsland`, `b3RemoveBodyFromIsland`, `b3UpdateBodyMassData`, and the `b3Body_Set*` API family (`body.c:1601-2512`) |
| non-awake `bodySims`/`bodyStates` | `b3Body_SetTransform` and the other setters when the body is not awake (`body.c:1125-2406`), `b3Shape_ApplyWind`, explosion callback |
| `shapes[id]` fields other than `aabb`/`fatAABBs` (filter, material, `materials` array, flags, geometry) | `b3CreateShapeInternal`, `b3DestroyShapeInternal`, `b3Shape_Set*` (`shape.c:1141-1684`), unconditionally — the hot path never rewrites these fields, awake owner or not |
| `shapes[id].aabb`, `fatAABBs` (non-awake owner) | `b3ResetProxy`, and the bounds recompute inside `b3Body_Set*`/`b3Shape_Set*` when the owning body is not awake; a body's wake also checkpoints its shapes' bounds (`b3WakeSolverSet`), since the awake hot path's first rewrite of them afterward is not itself journaled |
| tree proxy `categoryBits`, keyed by shape id | proxy creation, and `b3ResetProxy` when called with `invokeContacts=true` (`b3Shape_SetFilter`); diverges from `shape->filter.categoryBits` whenever `invokeContacts=false` leaves the proxy unsynced, so it is journaled as its own field rather than derived from the shape record at restore |
| `contacts[id]` (non-awake, or create/destroy) | `b3CreateContact`, `b3DestroyContact`, wake/sleep/merge transitions (`solver_set.c:86-140`, `291-390`, `520`), `b3RefreshBodyContactIndices` (`body.c:72-94`) |
| `joints[id]` and non-awake `jointSims` | `b3CreateJoint`, `b3DestroyJointInternal`, `b3TransferJoint`, joint setters (all joint files) |
| `islands[id]` and link arrays | `b3CreateIsland`, `b3DestroyIsland`, `b3MergeIslands`, `b3SplitIsland`, link/unlink (`island.c:20-337`, `388-649`) |
| solver sets (sleeping/static/disabled) | `b3TrySleepIsland` (creates a set, `solver_set.c:195-215`), `b3WakeSolverSet` (destroys one), `b3MergeSolverSets`, `b3CreateBody` (a body created asleep gets a set) |
| id pools ×6 | `b3AllocId`, `b3FreeId` (`id_pool.c:19-45`) |
| `broadPhase.pairSet` | `b3AddKey`, `b3RemoveKey` (`table.c:137`, `163`); only membership is ever queried, so slot layout is not state |
| colour `bodySet` bitsets | `constraint_graph.c:107-140`, `181-182`, `237-268`, `312-313`; `solver_set.c:353-354`, `416-417` |
| `sensors[]` create/destroy | `shape.c:234-242`, `527-563`, `sensor.c:414-450` |
| `sensors[shapeId].overlaps2` content | `b3SensorTask`, whenever the sensor's own per-step `eventBits` bit is set (`sensor.c`) |

Swap-compaction on removal (awake rows, colour arrays, set `contactIndices`, island arrays,
`islandSims`, `sensors[]`) rewrites *another* element's `localIndex`/`islandIndex`; for bodies,
`encodedBodySimA/B` on every contact of the moved body; and for sensors, the moved sensor's
owning shape's `sensorIndex`. Those are ordinary journaled record writes; nothing special is
needed because the journal stores whole records.

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
| pool alloc/free | pool tag, id, source (free-list pop or bump index) | undo mirrors whichever source the alloc/free actually used, not always a push/pop |
| pair set add/remove | key | remove / add (self-inverse) |
| bitset set/clear | colour, body id | clear / set |
| set create (sleep) | set index, ownership handle | undo: detach arrays into the entry; redo: reattach |
| set destroy (wake) | set index, ownership handle | undo: reattach arrays; redo: detach |
| island arrays | island id, ownership handle, or (old length, popped element bytes) | truncate and restore the popped element / reattach |
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
is retained, and releases both extra references only on eviction — undo would otherwise drop the
new hull's only reference, and redo the old hull's, either of which can leave a later scrub
installing a dangling pointer. A plain record write is enough only when the write does not change
which hull entry the shape references. Journal cost for the expensive transitions is O(1) plus the record writes the engine
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
collide, solve) are not journaled; the image covers them, except a shape's `aabb`/`fatAABBs`,
whose first such write after a wake has no earlier image to fall back on and is covered instead
by the checkpoint `b3WakeSolverSet` journals for it (§5.2). A field the hot path never rewrites
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
smaller, or equal and its shape id is lower, instead of overwriting unconditionally. Cost is a
few extra TOI evaluations on the rare multi-candidate sweep. After this, tree layout is
a performance property, not a simulation property, and restore may rebuild it any way it likes:

- for every shape whose fat AABB differs between the live tree and image T (awake shapes at T,
  plus shapes with journaled records), `b3DynamicTree_MoveProxy`; for every shape whose journaled
  proxy `categoryBits` at T (§5.2) differs from the live proxy's, `b3DynamicTree_SetCategoryBits`
  to that journaled value, never derived from the restored `shape->filter`, since the two can
  have diverged; for a shape whose journaled body type at T differs from its live proxy's tree
  (`B3_PROXY_TYPE(shape->proxyKey)`), a proxy cannot move between trees, so destroy the live
  proxy and recreate it in the tree matching T, with T's journaled category bits, instead;
  proxies created or destroyed after T are handled by the journaled shape create/destroy;
- clear all moved bits, then for each shape id in image T's moved list, mark its current proxy
  moved (propagating to ancestors the same way the hot path does), resolving the id through the
  shape's own, already-restored `proxyKey` rather than a numeric proxy id, which a proxy
  recreated by the previous bullet need not still have;
- the next step's rebuild puts the tree back into DFS order as usual.

Cost O(moved · log n). Ring cost for trees: zero.

Two residual order dependences remain and are documented in §10: the order of `preSolve`
callbacks within a step, and the traversal order seen by a caller's own cast, overlap, or mover
callback.

If the CCD change is not accepted, the fallback is to image the kinematic, dynamic, and static
trees raw (≈90 B per proxy per tick): same order as the engine's own rebuild copy, but O(proxies)
ring memory for the kinematic and dynamic trees, which fails requirement 2 for
`large_world`-shaped worlds and is fine for everything in `benchmark/`. CCD also queries the
static tree (`solver.c`); since a static proxy never moves, its tree only needs re-imaging when a
static shape is created, destroyed, or `b3World_RebuildStaticTree` is called, keeping its ring
cost O(events) rather than O(static proxies) per tick.

## 8. Ring

One circular byte arena per world. Slots are variable-length regions written in tick order: a
header, the image, the journal segment. A slot directory maps tick → offset. When the writer
wraps into the oldest slot, that slot is evicted (its owned arrays freed) and the retained
window shrinks by one tick.

- `maxBytes` caps the arena **and** the arrays ownership-transfer journal entries hold, plus any
  hull-database bytes kept alive only by a journal-held reference (§7.1) — a detached sleeping
  set's or island's arrays, or a hull an entry is the last reference to, count against the same
  budget, not just the arena's own bytes, or a single large wake/sleep transition or hull
  replacement could exceed a small budget unbounded. When a capture would not fit even after evicting down to a minimum window (say 2
  ticks), the capture interval doubles, as the recording player's keyframe ring does
  (`recording_replay.c:2648-2675`). Widening the interval only reduces how many retained ticks
  carry an image; every tick in the window still carries a journal segment (§6), and the minimum
  window always carries at least one image, so if the minimum window's total required storage —
  every tick's journal segment plus that one required image — exceeds `maxBytes`, the ring
  exceeds `maxBytes` for that window rather than dropping correctness-required state. `b3World_GetHistoryInfo` reports the effective interval and
  window, and its `bytesUsed` reports true usage even when it is over budget.
- Capacity growth (a scene grows) is a realloc of the arena with slot offsets preserved; rare
  after warm-up.
- An image's size is known up front from the awake counts, so writing it never allocates inside
  the step. A journal segment's size is not known up front — it grows as the step's structural
  writes occur — so the arena reserves journal room with the same amortized realloc growth as
  the previous bullet, not a single upfront allocation.

`world->historyTick` names the last completed tick. Capture increments it, then writes slot
`historyTick`. Rewind sets it to T. Ordinary stepping after a rewind overwrites slots T+1… and
discards their old journal segments; the branch is implicit.

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
3. **Image copy.** Resize (not reallocate) the awake set arrays and colour arrays to the image
   counts and memcpy. Scatter body/shape/joint records by id. For each imaged island: resize
   (not reallocate) its three link arrays to the image counts and copy their content, then copy
   the rest of the `b3Island` record, excluding those arrays' own live data pointers. For each
   imaged contact: if the live slot has a manifold block of the right count, copy into it,
   otherwise free and allocate one; for a mesh contact, likewise resize (not reallocate) the live
   `triangleCache` to the image count and copy its content; then copy the rest of the `b3Contact`
   record, excluding the `manifolds` and `meshContact.triangleCache` pointer fields, which the
   preceding steps already set correctly. World scalars, fat AABBs.
4. **Trees** per §7.4.
5. **Scratch and events.** Clear event arrays, both end-event buffers, move events, and
   task-context bitsets, and reset every restored body's `bodyMoveIndex` to none. Events for
   step T are not re-delivered; end events that were queued between steps at P are dropped. Set
   `historyTick = T`.
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
  the replay to be exact. `preSolve` order within a step, and the traversal order seen by a cast,
  overlap, or mover callback, follow tree traversal (§7.4) and may differ after a restore; a
  callback with order-dependent side effects breaks the contract.
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
  history must keep every such buffer alive for as long as any retained tick could still
  reference the shape with that pointer installed — through the shape's destruction or the
  pointer's replacement, and until the ring evicts the tick, not merely through the buffer's own
  intended lifetime.
- **`b3World_Explode`** collects every candidate shape from its query, sorts by shape id, then
  wakes bodies and applies impulses in that order (§7.4); a replayed explode reproduces the same
  velocities and wake order as the original run, since shape ids are stable across a rewind and
  replay. Woken-constraint order for overflow constraints (a body with more than 20 constraints)
  follows this same shape-id order, not tree traversal.

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
   replayed ticks per frame plus the live tick), rendering the live world's current state after
   each frame's batch of replayed ticks. This is ordinary rendering of the live world as replay
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
tie-break, a scalar grouping, a hash, and tests.

## 12. Verification

1. **Full state hash.** Extend `b3HashWorldState` (`recording.c:1188`, transforms and
   velocities only) to `b3World_ComputeStateHash`: bodies, sims, states, shapes (filter,
   material, geometry), contact records, manifolds and impulses, joint sims, island membership
   and sleep partition, pool state, pair set membership, graph-colour `bodySet` bitsets, sensor
   overlaps, shape bounds (`fatAABBs`), each shape's tree-proxy `categoryBits`, and moved-proxy
   membership, world scalars and flags, in id order, hashing float bit patterns. This is the
   oracle for everything below and is worth exposing publicly for lockstep desync detection.
2. **Restore exactness.** For each benchmark scene, step to steady state, then for 1,000 random
   (T, P) pairs inside the window, with random API churn between (creates, destroys, setters on
   sleeping bodies, forced sleep/wake toggles, explosions): `hash(Rewind(T)) == hash recorded at
   T`. Backward and forward.
3. **Replay exactness.** After `Rewind(T)`, re-step to P replaying the recorded API calls:
   `hash == hash recorded at P` at every intermediate tick. This is the bit-exact claim.
4. **Cold-hash guard (validation builds).** At capture, hash every cold structure that the
   journal claims is unchanged since the last capture unless journaled: sleeping sets, non-awake
   records, pools, pair set, graph-colour `bodySet` bitsets, tree proxy `categoryBits`. At the
   next capture, recompute and compare after replaying the
   segment's entries against a shadow copy. A write that bypassed the journal fails here, in
   whichever test first exercises it. This is what makes §5.2's list a test rather than a
   promise.
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
  game's correction path works end to end.
- **Phase 1, hot image + cold journal.** Journal hooks, ring arena, in-place image restore,
  cold-hash guard. Trees imaged raw as the temporary fallback.
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
