---
title: Preflight - public world snapshot and restore
status: Draft
date: 2026-10-11
parent: 20260929-002-rewind-only-history-ring.md (§13 phase 0)
---

# Preflight: public world snapshot and restore

Expose the snapshot machinery that already backs the recording player's keyframes as a small public
API: take an in-memory snapshot of a world at a step boundary, restore the world to it later, bit
for bit. Delivered as a short chain of PRs, each mergeable alone and none of them mentioning
rewind, rings or journals.

## 1. What exists today

- `b3SerializeWorld` and `b3DeserializeIntoShell` (`src/world_snapshot.c`, declared in
  `src/world_snapshot.h`) write and read a raw struct image of all simulation state: world scalars,
  six id pools, solver sets, bodies, shapes, contacts with manifolds and triangle caches, joints,
  sensors, islands, all three broad-phase trees verbatim, the pair set, the constraint graph and the
  name cache. The image is guarded by a magic, a version and a layout hash over the struct sizes, and
  refuses a precision mismatch.
- The recording player already restores such an image into a **live** world in place for backward
  seeks (`src/recording_replay.c`, keyframes), so the world id stays stable. `ScrubBackward` in
  `test/test_recording.c` asserts that restore plus re-step reproduces the forward hashes.
- Hull, mesh, height-field and compound geometry is not in the image. It is interned in a geometry
  registry that belongs to the recording (`b3Recording`, written) or the reader (`b3RecReader`,
  read), and the image holds only a registry id.
- `userData` on bodies, joints and shapes is zeroed in the image and on restore. A shape's renderer
  handle (`userShape`) is carried across a restore when slot and generation match and released
  through `destroyDebugShape` otherwise.

Consequence: the engine part is done. What blocks a public API is that the two functions are typed
in terms of recording objects, geometry lifetime belongs to the recording, and nothing is
documented as a contract.

Because the image contains the trees verbatim, **restoring it needs no solver change**. The
tree-order work in `20261011-001-preflight-tree-order-independent-solver.md` is independent.

## 2. Public surface (proposed)

```c
typedef struct b3WorldSnapshot b3WorldSnapshot; // opaque

// Step boundary only. Returns NULL if the world is locked (inside a step or a callback) or recording.
B3_API b3WorldSnapshot* b3World_CreateSnapshot( b3WorldId worldId );

// Restores the world the snapshot was taken from, in place; the world id and handles to
// surviving objects stay valid. Returns false and leaves the world untouched if it is locked,
// recording, or the snapshot does not belong to this world.
B3_API bool b3World_RestoreSnapshot( b3WorldId worldId, const b3WorldSnapshot* snapshot );

B3_API size_t b3WorldSnapshot_GetSize( const b3WorldSnapshot* snapshot ); // bytes held, including geometry
B3_API void b3DestroyWorldSnapshot( b3WorldSnapshot* snapshot );
```

Names follow `b3CreateRecording` / `b3DestroyRecording` for the opaque object and `b3World_*` for the
world operations; the owner may prefer other spellings.

Contract stated in the header and docs:
- Same process and same build only. The layout hash makes it a memory image, not a save-game format.
- O(world) time and one allocation per snapshot. It copies sleeping and static state that never
  changes; that is the cost the later incremental design removes.
- Restore discards everything after the snapshot. Creating a snapshot has no effect on the world.
- Host callbacks (pre-solve, custom filter, task system) and the
  worker count are not state and are not touched.

## 3. PR sequence

```
B0 decouple serializer ── B1 snapshot API ── B2 userData policy ── B3 docs
                                          └── B4 state hash (optional)
```

### B0. Decouple the serializer from recording types

**Files.** `src/world_snapshot.h/.c`, `src/recording.c`, `src/recording_replay.c`.

**Change.** Replace the `b3Recording*` and `b3RecReader*` parameters with a small internal
geometry interface in `world_snapshot.h`: a writer side that interns a blob and returns an id
(backed by `b3GeometryRegistry`), and a reader side that resolves an id to bytes (backed by the
`b3RegistrySlot` array). `b3SerializeWorld` and `b3DeserializeIntoShell` keep their behaviour; the
recording code constructs the interface from what it already owns. The compound shape's lazily
built `slot->live` object moves behind the reader side.

**Behaviour change.** None. Pure refactor.

**Tests.** The existing `test_recording.c` suite unchanged and green (`ScrubBackward`,
`SeekWithHull`, `KeyframeHandleReuse`, `TransformedHullRoundTrip`, `GeometryMutatorReplay`).

### B1. Public snapshot API

**Files.** `include/box3d/box3d.h`, `src/world_snapshot.c`, `src/physics_world.c`, `test/`.

**Change.**
- `b3WorldSnapshot` owns the image bytes **and its own geometry table**: copies of every interned
  hull, mesh, height-field and compound blob, deduplicated by content hash with the existing
  registry. Restore therefore never depends on host geometry still being alive, and a mesh that
  the host destroyed after the snapshot comes back on restore. Cost is memory proportional to
  distinct geometry, paid once per snapshot (see decision 2).
- `b3World_CreateSnapshot` and `b3World_RestoreSnapshot` use `b3GetUnlockedWorldFromId`, refuse
  while recording (a restore would desynchronise the op stream), and record an owner token
  (world id plus generation) so a snapshot cannot be restored into another world.
- Restore through `b3DeserializeIntoShell` as the player does today; on failure (corrupt or
  incompatible image) the world must be left usable. Today it may be left half-written
  (`b3FreeLiveSimElements` runs before parsing). Either validate the whole image first or document
  and test the failure state; this is the main correctness item of the PR.

**Tests.**
1. Snapshot at step k, step N more, restore, step N: per-step hashes equal. Run on scenes with
   hulls, spheres, capsules, a mesh, a height field, a compound, joints, sensors with overlaps, a
   sleeping island, a CCD bullet.
2. Create and destroy bodies, shapes and joints between snapshot and restore; restored ids and
   generations match the original timeline, and a replayed creation sequence yields the same ids.
3. Worker count 1 and 4 give identical hashes after restore.
4. Restore refused: during a step callback, while recording, wrong world, truncated image.
5. No leaks under the allocator test harness (`test_allocator.c`) across many snapshot/restore
   cycles, including hull refcounts in the shared hull database.

**Hash used by the tests.** `b3HashWorldState` covers body transforms and velocities only. Tests
here should use a wider hash (shapes' materials, contact and manifold state, sensor overlaps, pool
state) written in `test/`, or extend `b3HashWorldState` in B4. Say which in the PR.

### B2. userData policy

**Files.** `src/world_snapshot.c`, `include/box3d/box3d.h`.

**Problem.** The recording path zeroes `userData` because a recording replays in another process.
For an in-process snapshot, zeroed `userData` makes restore useless to a game that keeps an entity
pointer there.

**Change (recommended).** An internal flag on the serializer: recording keeps zeroing; the public
snapshot stores `userData` verbatim for bodies, joints and shapes. Document that restored objects
carry the pointer they had when the snapshot was taken, so an object destroyed and recreated by
the restore comes back with a pointer the host may have freed; the host must keep targets alive or
rebind after restore. No automatic rebinding.

**Alternative.** Keep zeroing and ship an iteration helper for rebinding. Smaller diff, but every
user writes the same rebinding loop and there is no way to enumerate bodies in the public API
today. Not recommended.

**Tests.** Pointer equality after restore for surviving, destroyed-and-restored, and
created-after-snapshot bodies; `userShape` handle behaviour unchanged.

### B3. Documentation

`docs/simulation.md` (or a new `docs/snapshot.md` registered in `docs/layout.xml`): contract of
section 2, the hazard list (userData, event arrays, callbacks, cost), and one worked
snapshot / step / restore example. Update the stale statement in `docs/faq.md` that box3d has no
way to put the world back to an earlier state.

**Check in B1, document in B3.** What `b3World_Get*Events` return immediately after a restore.
The recording player already restores keyframes, so the behaviour exists; confirm it and state it.

### B4. State hash (optional)

Expose `b3World_GetStateHash` over a wider set of simulation state than `b3HashWorldState` so
users can verify a restore. Keep it separate: it fixes a hash definition upstream must then keep
stable, and the recording format already embeds `b3HashWorldState` values that must not change.
Add a second function rather than altering the first.

## 4. Review checklist for each PR

- [ ] Only the files listed for that PR changed, plus tests and docs.
- [ ] Recording suite (`test_recording.c`) passes unchanged; recordings written before the PR still
      validate (B0, B2 especially).
- [ ] New API has a header comment stating thread-safety (not callable during a step) and the
      failure return.
- [ ] No behaviour change to existing public functions.
- [ ] Allocator test shows no leak across repeated snapshot and restore.

## 5. Out of scope

- A ring of snapshots, retention policy, a rewind-by-tick API, journaling, in-place O(awake)
  capture. Those are the history-ring work (rewind-only design §§6-9, phases 1+).
- Persisting a snapshot to disk or across builds (the recording file already does that for its
  seed snapshot).
- Restoring into a different world, or into a world with a different `b3WorldDef`.
- Snapshot update-in-place to reuse the buffer. Add when a measurement asks for it.

## 6. Open decisions for the owner

1. `userData` (B2): store verbatim (recommended) or zero.
2. Geometry ownership (B1): snapshot owns copies (recommended, restore is self-contained) versus
   referencing host-owned mesh and height-field data (smaller snapshots, but a destroyed mesh
   would dangle). A middle path is a shared geometry table reused across snapshots of one world,
   which matters once many snapshots are kept; defer until the history work needs it.
3. Whether B4 ships with this chain or waits for a consumer.
