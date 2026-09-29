---

title: Box2D's snapshot and rollback, compared with box3d's serializer and the rollback designs
status: Analysis, not a design. Findings come from reading source, not from building or running
  either engine. Box2D was read at 956ce4e (2026-09-24, `main`), upstream Box3D at 9f998c8
  (2026-09-27), and this fork at 5643cd8, the last commit touching `src/` and `include/`. Four
  read-only searches gathered the findings; the claims this document leans on hardest (the dates
  the public API landed, the event-buffer clear, the id generation behaviour, the interning path)
  were re-checked by hand. Anything still inferred is marked as such. No size or timing in this
  document is a measurement.
date: 2026-09-29

---

# Box2D's snapshot and rollback, and what it means for box3d

Citations prefixed `box2d:` are paths in https://github.com/erincatto/box2d. Unprefixed paths are
in this repository. The three designs are referred to by name:

| Name used here | Document |
|---|---|
| scoped rollback | `docs/designs/20260927-001-client-prediction-rollback.md` |
| two-way ring | `docs/designs/20260927-002-bit-exact-history-ring.md` |
| rewind-only ring | `docs/designs/20260929-002-rewind-only-history-ring.md` |

## 1. Summary

1. **Box2D has a public, whole-world snapshot and in-place restore.** It is on `main` only, in no
   tagged release, and is about four months old. The author documents it as "useful for rollback,
   undo, and deterministic save games" and the FAQ now says rollback determinism is achievable
   with it.
2. **It is the same serializer box3d already has.** Box2D's `snapshot.c` and this repository's
   `world_snapshot.c` share function names, section order and header layout. Box2D added a thin
   public wrapper, a deeper state hash, a rollback sample, and a few fixes. Box3d has none of
   those yet.
3. **The rewind-only ring's phase 0 is, in substance, a port of Box2D's public API.** Both put a
   public call in front of `b3SerializeWorld` and an in-place `b3DeserializeIntoShell`. Upstream
   intends to ship the same thing for box3d.
4. **Geometry is what holds box3d back.** Box2D shapes carry their geometry inline, so an image is
   self-contained. Box3d hulls, meshes, height fields and compounds sit behind pointers, and the
   serializer can only write them by interning into a registry that belongs to a recording. That
   is the most likely meaning of the author's "cache large collision shapes" (§5).
5. **Box2D's approach does not address what the ring designs exist for.** It is O(world) per
   capture, has no partial or incremental mode, and allocates on restore. The author says so. The
   ring designs' case for an O(awake) image plus a journal is unchanged.
6. **Box2D's tests stop short of the correction flow.** Nothing in its test suite rewinds a world
   that has run ahead, changes an input, re-steps, and compares against a reference run. The
   rewind-only ring's verification plan already covers this and should keep doing so.

## 2. What Box2D ships

### 2.1 API

All in `box2d:include/box2d/box2d.h`, implemented in `box2d:src/snapshot.c`.

```c
int       b2World_GetSnapshot( b2WorldId worldId, uint8_t* image, int capacity );
bool      b2World_Restore( b2WorldId worldId, const uint8_t* image, int size );
b2WorldId b2CreateWorldFromSnapshot( const uint8_t* image, int size, int workerCount );
uint64_t  b2World_GetStateHash( b2WorldId worldId );
```

- **The caller owns the buffer.** Passing a null `image` returns the size needed. There is no
  ring, window or tick in the API; the caller keeps whatever images it wants.
- **Restore is in place.** The world keeps its slot, generation, callbacks, task wiring and world
  user data.
- **Both calls require a step boundary** and refuse a locked world.

### 2.2 History

| Date | Commit | What |
|---|---|---|
| 2026-06-01 | 9e3b576 | Recording and replay |
| 2026-06-07 | 436365a | Snapshot serializer, public `b2World_Snapshot`, `b2World_Restore`, `b2CreateWorldFromSnapshot`, `test_snapshot.c`, docs |
| 2026-08-19 | d710ba7 | `b2World_GetStateHash`, the deep hash, event buffers cleared on restore |
| 2026-08-20 | 617d32a | Rollback sample |
| 2026-09-13 | 773ed92 | `b2World_Snapshot` renamed to `b2World_GetSnapshot`; `world_snapshot.c` renamed to `snapshot.c` |

The latest release tag is v3.1.1 (2025-06-03), which predates all of it. The author's blog post
(https://box2d.org/posts/2026/06/replay/) still uses the old name `b2World_Snapshot`. The
published website FAQ is still the v3.1.0 text and still says "Box2D does not have rollback
determinism"; `box2d:docs/questions.md` on `main` says the opposite.

### 2.3 What an image holds

Read from `b2SerializeWorld` in `box2d:src/snapshot.c`.

| Captured | How |
|---|---|
| World scalars and enable flags | field by field |
| Seven id pools, including free-list order | field by field |
| Every solver set: static, awake, disabled, each sleeping set | raw arrays |
| Bodies, shapes, joints, including free slots | per element, with `userData` nulled |
| Contacts, fat AABBs | raw arrays |
| Manifolds, warm-start impulses, simplex cache | inline in the contact sims, so raw |
| Chains, sensors and their overlaps, islands | field by field |
| All three broad-phase trees | raw nodes and proxies, not rebuilt |
| Pair set | raw at full capacity, because probe order depends on it |
| Constraint graph colours | bitset plus raw arrays |

| Not captured | After an in-place restore |
|---|---|
| Body, shape and joint `userData` | reads back as null, even for objects that existed throughout |
| World `userData`, `preSolve`, custom filter, friction and restitution callbacks, task callbacks, worker count | left as they are on the live world |
| Event arrays | cleared |
| Arenas, profile, debug state | untouched |

### 2.4 Cost model

- **Capture is O(world).** It walks sleeping sets, the static tree, free slots and the pair set at
  full capacity on every call. There is no dirty tracking and no partial mode.
- **Capture copies twice.** It serializes into an internal growable buffer, then copies that into
  the caller's buffer.
- **Restore allocates.** The large record arrays reuse capacity. Bitsets, the pair set, all three
  trees, and the chain, sensor and island arrays are freed and allocated again on every restore.
- **Single-threaded, uncompressed.**
- **No measurements exist.** Nothing in Box2D's benchmarks, docs or commit messages gives a size or
  a time. The one figure found is 2,441 bytes for an empty world, from a third-party bug report.

The author's blog is direct about this: "The performance of these operations may be slow for large
worlds and the image size can be large", and "It is not possible to snapshot a portion of the
world". That quote was retrieved through a summarizing fetch and may not be word for word.

### 2.5 Image validity

An image is tied to the build that wrote it. The header carries a magic number, a format version
(12, bumped by nearly every simulation change since June), a hash of struct sizes, and flags for
double precision and validation builds. A header mismatch is rejected before the world is touched.
A corrupt payload found after restore has begun "leaves the world unusable, so the caller must
destroy it": restore is not atomic. Two fuzzing reports against the loader were filed and closed
in September 2026 (erincatto/box2d issues 1109 and 1110).

None of this matters to an in-memory ring in one process. It matters to save games and to anyone
tempted to send an image over a network.

### 2.6 Ids across a restore

Generations live in the records and are restored with them. Ids held at the snapshot instant work
again. An id created after the snapshot fails validation immediately after the restore, which is
what `box2d:test/test_snapshot.c` asserts.

The header's claim that such ids fail "rather than aliasing a different object" holds only until
the slot is reused. Creation increments the slot's generation and destruction preserves it
(`box2d:src/body.c`), so after a restore the next object created in that slot gets the same
generation the discarded id had. For a replay that repeats the same creations this is the desired
result: the same ids come back. For a replay that diverges, a stale id can resolve to a different
object. No Box2D test covers the reuse case. The same property holds in every design here, since
all of them restore generations.

## 3. What Box2D claims, and what it tests

### 3.1 Claims

| Source | Statement |
|---|---|
| `box2d:include/box2d/box2d.h` | "Useful for rollback, undo, and deterministic save games." |
| `box2d:docs/questions.md` | "Some developers want rollback determinism. You can achieve this using the [snapshot] system." |
| `box2d:docs/recording.md` | "The same machinery is available directly, without recording, as a public API for save states and rollback" |
| `box2d:docs/recording.md` | "snapshots are worker-count independent" |
| `box2d:include/box2d/box2d.h`, on the state hash | "Use this to detect desyncs (for example rollback resimulation) instead of comparing serialized world bytes, which carry non-canonical padding." |
| Blog | "Rollback simulation is only deterministic if all your game code is deterministic." |

No document says in so many words that an in-place restore followed by stepping is bit-identical
to the original run. The claim is carried by the tests and the sample.

### 3.2 Tests that restore and re-step

| Test | What it establishes |
|---|---|
| `SnapshotTest` phase 9, `box2d:test/test_snapshot.c` | A world snapshotted mid-motion and loaded into a *new* world reproduces the original's next 90 steps, by the bodies-only hash. |
| `SnapshotTest` phases 3, 4, 7 | Worlds built from the same image stay in lockstep for 120 steps at 1 and 4 workers. The scene is asleep when these start. |
| `SnapshotTest` phase 5 | In-place restore of a world that ran ahead returns the deep hash to its snapshot value. It does not step afterwards. |
| `SnapshotTest` phase 11 | Restore clears end events queued by a between-step mutator. |
| `RecordingKeyframeTest`, `RecordingScrubTest`, `RecordingQueryScrubTest`, `box2d:test/test_recording.c` | The replay player restores keyframes in place into a world that has run ahead, re-steps, and matches a forward-only reference by deep hash, up to 320 frames. The third also checks broad-phase traversal order. |
| `ReStepRaceTest` | 16 rounds of in-place restore plus 50 steps on a large scene at 2 workers give 16 identical deep hashes. |
| Rollback sample, `box2d:samples/sample_determinism.cpp` | Restores any of a run's snapshots and checks the re-run reaches the same sleep step and transform hash. Manual, not run in CI. |

### 3.3 What is not tested

- Rewind, **change an input**, re-step, and compare with a reference run that had that input from
  the start. Every re-step above replays unchanged inputs.
- Creating or destroying bodies, shapes or joints **between the restore and the re-step**.
- A restore across the destruction of an object, followed by re-stepping.
- Restore with `preSolve` or custom filter callbacks installed. Recording refuses to run with
  them; plain snapshot restore is untested with them.
- Restore-then-step across platforms. The cross-platform determinism test does not use snapshots.

### 3.4 The state hash

`b2World_GetStateHash` covers body transforms and velocities, body set and local indices, contact
indices and manifold impulses, accumulated impulses for eight joint types, and id pool counts. It
does not cover shapes, trees, islands, sensors, sleep timers, contact anchors, the simplex cache,
free-list order, or joint state other than impulses. It is a strong check, and a partial one.

## 4. Where box3d stands against Box2D

Upstream Box3D has no public snapshot API. Its `docs/recording.md` says the machinery "is currently
internal" and its `docs/faq.md` still says there is no rollback determinism. Upstream differs from
this fork by one commit (SAT samples), which touches none of the snapshot or recording files.

| | Box2D | Box3d (this fork and upstream) |
|---|---|---|
| Serializer | `b2SerializeWorld`, `b2DeserializeIntoShell` | `b3SerializeWorld`, `b3DeserializeIntoShell`, same structure |
| Public snapshot and restore | yes | no |
| Image is self-contained | yes | no, geometry is a registry id (§5) |
| Serializer needs a recording | no | yes, `b3Recording` to write and `b3RecReader` to read |
| State hash | bodies-only, plus the public deep hash | bodies-only (`b3HashWorldState`) |
| Events cleared on restore | yes | no |
| Locked-world guard on restore | yes | no |
| Size query | yes | the buffer supports counting, nothing calls it |
| Dedicated snapshot tests | `test_snapshot.c` | none; `test_recording.c` covers restore through the player |
| Per-contact heap | none, manifolds are inline | manifold block and mesh triangle cache per contact, allocated again on restore |
| Player backward seek | keyframe ring, restore, re-step | the same |

Box3d's serializer also carries things Box2D's has no need for: the name cache, multi-material
arrays, and carrying a shape's renderer handle across a restore when slot and generation match.

## 5. "Cache large collision shapes"

### 5.1 The statement

From https://github.com/erincatto/box3d/discussions/99, a thread titled "Rollback determinism"
opened on 2026-07-21 asking whether the FAQ's statement would ever change. The author's only reply,
on 2026-07-25:

> Box2D has a snapshot and rollback mechanism that is deterministic. Box3D almost has it. I need
> to do some extra work for 3D to cache large collision shapes. Otherwise the memory usage will be
> huge.

The thread has three posts. He gave no mechanism, no timeline and no follow-up.

### 5.2 Why 3D differs

`b3Shape` holds its geometry in a union (`src/shape.h`). Only spheres and capsules are inline.

| Shape | Held as | Owner | Sharing | Size |
|---|---|---|---|---|
| Sphere, capsule | inline | | | tens of bytes |
| Hull | pointer | engine | deduplicated by content and reference counted in the world's hull database (`src/physics_world.c`) | 144-byte header, at most 128 vertices, faces and edges: a few KB |
| Mesh | pointer plus per-shape scale | caller | borrowed; one blob can serve many shapes | unbounded |
| Height field | pointer | caller | borrowed | unbounded |
| Compound | pointer | caller | borrowed; child hulls and meshes are copied into the blob | unbounded |

Estimated from the blob layouts in `src/mesh.c` and `src/height_field.c`, not measured: a mesh
costs roughly 30 to 40 bytes per triangle, so about 35 MB for a million triangles, and a 1024 by
1024 height field is about 5 MB.

In Box2D a polygon is a fixed-size value inside the shape record, so a raw copy of the shape array
is the geometry. Writing box3d geometry into every image the same way would cost the blob size
times the number of images kept. A million-triangle mesh in a 60-image ring is about 2 GB.

### 5.3 What box3d does today

The serializer never writes a blob into an image. For each hull, mesh, height-field or compound
shape it calls `b3RecInternHull` and its siblings (`src/recording.c`), which place the blob in the
recording's registry and return an id. The image holds the id.

| Property | Behaviour |
|---|---|
| Key | 64-bit content hash, confirmed by byte count and `memcmp` |
| Stored | once per recording |
| Released | never individually; only when the recording is destroyed or restarted |
| Restore, hull | added to the world's hull database, which clones or bumps a count |
| Restore, mesh and height field | the shape points at the registry's bytes, no copy |
| Restore, compound | one live copy per registry slot, shared across restores |
| Without a registry | the read fails for any hull, mesh, height field or compound |

Two costs follow from the code as written:

- **Interning cost scales with shape references, not unique blobs.** Each call allocates, copies
  and hashes the whole blob before looking it up, and frees the copy on a hit. Ten thousand shapes
  sharing one mesh means ten thousand copies and hashes of that mesh per capture. Each blob
  already stores its own content hash (`include/box3d/types.h`), which the interning path does not
  use.
- **The player holds geometry twice.** It loads the file's registry, then copies every slot into a
  second registry used for keyframes (`src/recording_replay.c`).

### 5.4 Reading of the statement

The evidence supports one reading best, and cannot rule out others. This subsection is
interpretation.

**Most likely.** A store of geometry blobs that lives outside any image and outside any recording,
keyed by content, shared by every snapshot, and holding each blob for as long as a snapshot refers
to it. That is the recording registry made standalone and given a lifetime rule. It is what a
`b3World_GetSnapshot` would need in order to have somewhere to put geometry, and it is what "almost
has it" suggests: the serializer exists, the place to hang its geometry outside a recording does
not.

**Not ruled out.**

1. The same store, but engine-owned like the hull database, so the engine takes over the lifetime
   of meshes, height fields and compounds from the caller.
2. Removing the per-reference copy and hash from the interning path, with no new store.
3. Something specific to compounds, which already need a fixed-up live copy on restore.
4. Something other than geometry, such as the per-contact mesh triangle cache. The FAQ's "Box3D
   caches a lot of internal state" is a different use of the word and was not investigated.

## 6. Comparison with the designs

Box2D's approach applied to box3d is what the rewind-only ring calls phase 0, so the first column
also describes phase 0.

| | Box2D-style snapshot (phase 0) | Rewind-only ring (phases 1 and 2) | Two-way ring | Scoped rollback |
|---|---|---|---|---|
| **Restores** | whole world | whole world | whole world | predicted islands only |
| **Re-steps** | whole world | whole world | whole world | scope, rest frozen |
| **Capture cost** | O(world), every tick | O(awake) image plus O(events) journal | same image; journal entries also carry new values, so segments are larger | O(scope) |
| **`large_world`, 10 awake of 1M** | ~650 MB per capture (this fork's perf review) | a few KB per tick (estimate) | a few KB per tick (estimate) | kilobytes (estimate) |
| **Restore cost** | O(world), allocates | O(awake + sensors + journal since T) | same terms, in either direction | O(scope), recreates scope contacts |
| **Exactness** | bit-exact by test, for the fields the hash covers | bit-exact, against an extended full-state hash, with two named exceptions (callback order after a layout-changing restore; colliding name hashes) | same | consistent, not bit-exact |
| **History** | caller keeps images | engine-owned ring; a byte budget widens the capture interval, so only imaged ticks are restorable | engine-owned ring, budgeted | engine-owned |
| **Restore to a later tick** | yes, any image the caller kept | no (forward scrub and peek-and-return are non-goals) | yes, slots after T survive a rewind until a write makes them stale | no |
| **Image usable elsewhere** | another world or process of the same build | no, in-place only | no, in-place only | no |
| **`userData` after restore** | null | restored to its value at T | restored to its value at T | unchanged |
| **Events** | cleared on restore (Box2D); not yet in box3d | cleared on restore | cleared on restore | spurious end and begin touch |
| **Geometry** | registry id per shape; needs a store (§5) | never copied; borrowed pointers stay, caller keeps blobs alive for the window | same, but blocks of created objects are held by journal entries in both directions | not recorded |
| **Engine changes** | public wrapper, event clear, lock guard, geometry store | write accessors, undo journal, three solver order changes, extended hash | rewind-only's, plus redo payloads, two-way block ownership, awake-set-exit entries, a stale-slot hook in every write path | joint mass overrides, scoped lists, solver buffers |
| **Changes results outside rollback** | no | yes, the three order changes | yes, the same three | no |
| **State** | shipped on Box2D `main`, tested | draft, unbuilt; review converged at round 21 | draft, unbuilt; review converged at round 43 | draft, unbuilt |

Box2D's snapshot is the only column with a restore-to-a-later-tick capability that needs no
engine design work, because the caller simply keeps the images. Of the ring designs, only the
two-way ring can do it, and the rewind-only ring gives it up on purpose (its open question 4).

## 7. What this changes for the designs

### 7.1 Confirmed

- **Phase 0 is sound and has a working precedent.** The rewind-only ring's first claim, that the
  serializer already gives bit-exact whole-world rewind, is what Box2D has shipped and tested. The
  keyframe and re-step tests in `box2d:test/test_recording.c` are the same property that
  `ScrubBackward` demonstrates here, checked with a deeper hash.
- **The O(world) objection stands.** Box2D did nothing to reduce capture cost and its author
  describes the same limits the perf review measured. The reason to build phases 1 and 2 is
  intact.
- **A public state hash has precedent.** The rewind-only ring asks whether
  `b3World_ComputeStateHash` should be public. Upstream made the equivalent public in Box2D and
  recommends it for detecting rollback desyncs. The ring's proposed hash covers considerably more
  than Box2D's does.
- **Clearing events on restore is necessary.** Box2D found end events leaking through a restore
  and fixed it in d710ba7. The rewind-only ring's restore already clears them. Box3d's existing
  deserializer does not, so phase 0 must add it.

### 7.2 New information

- **Upstream intends to ship this for box3d.** Whatever public API phase 0 introduces will meet an
  upstream one later. Box2D's names (`GetSnapshot`, `Restore`, `GetStateHash`) are the likely
  shape of it.
- **Phase 0 has a geometry cost the designs do not price.** Both ring designs say the phase 0
  images must share the recording registry and keep it alive. Neither mentions that every capture
  copies and hashes each referenced blob once per referencing shape (§5.3). It is not known
  whether the perf review's capture figures, 18 to 34 percent of a step, came from scenes where
  this cost was material.
- **The two approaches give the caller different `userData` contracts.** Box2D nulls it. Phase 0
  inherits that. Phase 1 restores it. A caller written against phase 0 rebinds after every
  restore, and that code becomes unnecessary at phase 1.
- **An upstream geometry store may change who owns geometry.** The ring designs put the lifetime
  of meshes, height fields and compounds on the caller for as long as the window can restore a
  shape that uses them. If upstream moves ownership into the engine (reading 1 in §5.4), that
  contract could be replaced by reference counts held by the ring.

### 7.3 Suggested actions

These are recommendations, not decisions.

1. **Build phase 0 as a port of Box2D's public layer**, with upstream's names, and put the ring
   API (`b3World_EnableHistory`, `b3World_Rewind`) on top of it. The port is small: a wrapper, the
   event clear, the lock guard, a size query. It keeps this fork close to what upstream will land.
2. **For phase 0, avoid the registry for in-memory rewind.** A ring that lives in one process and
   restores in place does not need geometry bytes at all, because the blobs are still in memory:
   meshes, height fields and compounds under the designs' borrowed-geometry rule, and hulls if the
   ring holds a reference in the hull database for each hull its images use. This has not been
   examined against the deserializer, which currently fails without a registry.
3. **Port Box2D's deep hash as the first step toward the extended hash.** It is a known-good
   intermediate and would let box3d's existing scrub tests check more than body state.
4. **Keep the ring's verification tests 2 and 3 as written.** They cover the correction flow that
   Box2D's suite does not (§3.3): API churn between rewind and re-step, and events compared as well
   as hashes.
5. **State the id reuse property in the caller contract.** The rewind-only ring says handles
   created after T are invalid after a rewind. That is true until the slot is reused (§2.6). A
   caller must drop ids from the discarded timeline.

## 8. Not determined

- Any size or timing for Box2D's snapshot or restore.
- Whether in-place restore then re-step is bit-exact for arbitrary box3d scenes. Box3d's evidence
  is `ScrubBackward`, with a bodies-only hash.
- Which mechanism upstream will choose for the geometry store, and when.
- Whether GitHub Discussions on the Box2D repository contain anything further from the author on
  rollback. Issues and pull requests were searched; discussions were not.
- The exact wording of the blog's limitations, which were read through a summarizing fetch.

## 9. Sources

| Source | Read at |
|---|---|
| https://github.com/erincatto/box2d | 956ce4e, 2026-09-24 |
| https://github.com/erincatto/box3d | 9f998c8, 2026-09-27 |
| This fork | 5643cd8 for `src/` and `include/` |
| https://github.com/erincatto/box3d/discussions/99 | all three posts, through the GitHub REST API |
| https://box2d.org/posts/2026/06/replay/ | summarizing fetch |
| https://box2d.org/documentation/md_faq.html | summarizing fetch; v3.1.0 text |
| erincatto/box2d pull request 1065, issues 839, 1109, 1110 | titles and bodies |
