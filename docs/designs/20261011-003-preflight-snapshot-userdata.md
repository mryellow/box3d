---
title: Preflight - userData in the snapshot code
status: Draft
date: 2026-10-11
parent: 20261011-002-preflight-public-world-snapshot.md (B2)
---

# Preflight: userData in the snapshot code

Let an in-process world snapshot **optionally** carry the host's `userData` pointers, without
changing what a recording stores or what a snapshot taken without the option stores. This expands PR B2 of the public-snapshot preflight into its own chain of
small PRs. It touches `src/world_snapshot.c` and its callers only.

## 1. What the code does today

All of it is in `src/world_snapshot.c`:

| Place | Behaviour |
|---|---|
| `b3SerializeWorld`, bodies | copies each `b3Body`, sets `elem.userData = NULL`, writes the struct |
| `b3SerializeWorld`, joints | same for `b3Joint` |
| `b3SerShapes` | copies each `b3Shape`, nulls `materials`, `userData` and `userShape` before writing |
| `b3DeserializeIntoShell`, bodies and joints | reads the struct, then sets `userData = NULL` again |
| `b3DesShapes` | reads the struct, nulls `materials`, `userData`, `userShape` |
| `b3DesShapes`, `userShape` | the **renderer handle** is carried over from the live shape when slot and generation match, released through `destroyDebugShape` otherwise |
| `b3FreeLiveSimElements` | never frees `userData` or `userShape`: both are host-owned |

So `userData` is dropped twice (on write and on read), and the second drop means even an image
that did carry a pointer would be scrubbed on the way back in.

Not affected, and worth stating so nobody "fixes" them:

- **World `userData`** (`b3World_SetUserData`) is not part of the image; restore leaves it alone.
- **Query callbacks' `uint64_t userData`** are tree payloads (shape ids), not host pointers.
- **`b3BodyDef`, `b3ShapeDef`, `b3JointDef` `userData`** are recorded as a placeholder and replayed as
  `NULL` (`src/recording_replay.c`). That is op-stream behaviour, separate from the image.
- **`b3BodyMoveEvent.userData` and `b3JointEvent.userData`** are copied from the records at step
  time (`src/solver.c`) into per-step event arrays. They are not in the image.

## 2. Why the recording path must keep zeroing

A recording's seed snapshot and keyframes are replayed by a different process, or the same one
after the host objects are gone. A pointer in that image is meaningless at best. A reader fed a
crafted `.b3rec` that put a non-NULL pointer into it would hand the host an attacker-chosen
`userData` to dereference. So the rule is:

- **The reader, not the image, decides.** Preservation is an argument of the internal restore
  function that only the public snapshot path can set, and it sets it from the snapshot object's
  own flags (section 3), never from the image bytes. A recording image restored through the player
  is scrubbed even if its bytes hold a non-zero pointer.
- The image format does not change for recordings, so `B3_SNAP_VERSION` is not bumped and
  recordings already on disk still load.

## 3. The option, and the policy when it is on

```c
typedef enum b3SnapshotFlags
{
	b3_snapshotNone = 0,
	b3_snapshotUserData = 0x1, // capture and restore body, shape and joint userData
} b3SnapshotFlags;

B3_API b3WorldSnapshot* b3World_CreateSnapshot( b3WorldId worldId, uint32_t flags );
```

- **Opt-in, off by default.** Without the flag a snapshot behaves exactly as today's image: all
  `userData` comes back `NULL`. This is the conservative default and matches recordings; flipping
  it to on-by-default later is a one-line change, the reverse would break callers.
- **Fixed at creation.** The flags are stored in the `b3WorldSnapshot`. `b3World_RestoreSnapshot`
  takes no flags and honours the snapshot's. One snapshot cannot be restored both ways, which
  keeps "what is in the snapshot" and "what restore does" the same thing.
- **Unknown bits are rejected** (`NULL` return, assert in debug), so a later flag cannot be
  silently ignored by an older library.

Three candidates for what a restored object's `userData` becomes when the flag is set:

Three candidates for what a restored object's `userData` becomes:

| Policy | Rule | Verdict |
|---|---|---|
| **P1 verbatim** | the value captured in the snapshot | **Recommended.** Restore returns the world to its state at T, `userData` included. Simple to state and to test. |
| P2 live-wins | keep the live value when slot and generation match, else `NULL` | Rejected: an object destroyed after T comes back with `NULL`, which is the case users most need. |
| P3 hybrid | live value when slot and generation match, else captured value | Fallback if hosts re-point `userData` and expect it to stick. Two rules to document, and a result that is not "the state at T". |

P1 fits the intended user, who rolls its own entity mapping back to T alongside the world. Under P1
a host that re-points `userData` after T sees the re-point undone, the same as any other state.

Consequences of P1 to document, not to mitigate in code:

- An object created after T disappears; an object destroyed after T returns carrying its old
  pointer, which the host may have freed. The host keeps targets alive or rebinds. No automatic
  rebinding and no new enumeration API in this chain.
- `userShape` is **not** captured. It is the renderer's handle to the live world, so the existing
  carry-over by slot and generation stays exactly as it is, for both paths.
- After a restore, the event arrays of the last step (`b3World_GetBodyEvents` and the joint
  equivalent) still hold the pre-restore `userData` copies until the next step. State this next to
  the events caveat in the snapshot docs.

## 4. PR sequence

```
U1 policy plumbing (no change) ── U2 preserve userData ── U3 docs
```

### U1. Internal flag, behaviour unchanged

**Files.** `src/world_snapshot.h/.c`, `src/recording.c`, `src/recording_replay.c`.

**Change.** Add an internal bool, `preserveUserData`, to both `b3SerializeWorld` and
`b3DeserializeIntoShell` (or to the geometry-interface struct if the B0 refactor of the
public-snapshot chain has landed; use whichever exists). Every existing caller passes `false`. The
three scrub sites on each side become conditional on it.

**Tests.** `test_recording.c` unchanged and green. Add one check that a seed snapshot taken from a
world full of non-NULL `userData` is byte-identical to the same world with all `userData` NULL
(same flag value `false`), which pins the "recordings never leak pointers" property before U2 can
break it.

### U2. Preserve `userData` for bodies, joints and shapes

**Files.** `src/world_snapshot.c`, and the public-snapshot entry points
(`src/physics_world.c` or wherever B1 put them).

**Change.**
- Writer: when preserving, write `userData` as held, for **live** records only. Free slots keep
  writing `NULL` so a stale pointer left in a freed body, shape or joint never reaches the image;
  use the same liveness tests the file already uses (`shape.id == i`, `c->contactId == i`).
- Reader: when preserving, skip the forced `NULL` for bodies, joints and shapes; free slots still
  come back `NULL`.
- `materials` and `userShape` stay nulled in the image on both paths.
- Add `b3SnapshotFlags` and the `flags` parameter to `b3World_CreateSnapshot` (or, if the public
  snapshot chain has not landed, to whichever PR introduces it; see section 6). The snapshot object
  stores the flags. Create passes `flags & b3_snapshotUserData` to the serializer; restore passes
  the stored value to the reader. Nothing else passes `true`.
- Reject unknown flag bits.

**Tests.**
1. Bodies, shapes and joints with distinct tagged pointers: snapshot, mutate, restore; every
   survivor has its original pointer.
2. Destroy a body, a shape and a joint after T, restore: they return with the captured pointers.
3. Create a body after T, restore: it is gone; its slot reads `NULL` from the getter.
4. `b3Body_SetUserData` after T to a new value, restore: the captured value is back (P1).
5. Free-slot hygiene: destroy a body before the snapshot, leave a poison value in its freed record
   if the build allows it, snapshot, restore, create a body in that slot, check the pointer is the
   new body's own.
6. `userShape` behaviour unchanged: handle kept on slot and generation match, released via
   `destroyDebugShape` otherwise.
7. Recording path: start recording on a world full of `userData`, load the recording in a player,
   every body, shape and joint reads `NULL`. Repeat with an image whose bytes were patched to
   hold a non-NULL pointer; the player still yields `NULL`.
8. Flag off (`b3_snapshotNone`): same scene, snapshot, restore: every `userData` reads `NULL`,
   and the image is byte-identical to the same world with all `userData` NULL.
9. Unknown flag bits: `NULL` returned, no snapshot allocated.
10. `b3HashWorldState` (and any wider hash added later) is identical for two worlds that differ only
   in `userData`.

### U3. Documentation

In the snapshot docs added by the public-snapshot chain's B3: the P1 rule, what is and is not
captured (section 1 list), the dangling-pointer hazard, the stale event arrays, and a two-line
note that recordings deliberately drop `userData`.

## 5. Review checklist for each PR

- [ ] Recording images are unchanged byte for byte (U1 test) and old `.b3rec` files still load.
- [ ] No code path outside the public snapshot functions passes `preserveUserData = true`.
- [ ] No new public enumeration or rebinding API.
- [ ] Header comments on the snapshot functions state the P1 rule and the dangling hazard.
- [ ] No mention of rewind, rings or journals.

## 6. Open decisions for the owner

1. P1 (recommended) or P3, for when the flag is set.
2. Default when the flag is absent: off, as written. On-by-default would be friendlier to the
   rollback user but makes the zero-`userData` behaviour the opt-in.
3. Where the `flags` parameter lands. Cleanest is B1 of the public-snapshot preflight, so the
   function is born with it and never changes signature; this PR then only adds the flag's
   meaning. The public-snapshot preflight is updated to match.
