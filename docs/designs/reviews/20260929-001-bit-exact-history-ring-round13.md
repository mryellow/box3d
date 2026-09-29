---

title: Codex review — bit-exact history ring, round 13
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 13

## Review report (Codex final message)

## Summary

The hot image and cold journal approach fits much of the source tree, but the draft does not yet establish bit-exact restore across all proposed phases. The main gaps are static-tree order in phase 1, the pending journal after a rewind, and the concrete enforcement of journaled writes. This was a read-only source review; no files were changed or tests run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §13, Phase 1; §7.4 | Phase 1 images the dynamic and kinematic trees but never the static tree, while postponing the CCD order change to phase 2. CCD queries the static tree and currently passes a running fraction to TOI (`solver.c`). Replaying proxy operations does not restore its former layout, so phase 1 cannot claim bit-exact replay after static-tree changes. | Image the static tree in phase 1, move the relevant order change into phase 1, or explicitly limit phase 1’s exactness claim. |
| 2 | High | §§2.7, 5.2, 7.3 | The promised compiler-enforced write boundary needs a concrete migration design. World arrays and records are directly accessible across translation units (`physics_world.h`); `b3GetShapeMaterials` returns a writable pointer even from a const shape (`shape.h`). An accessor that returns a writable record pointer cannot ensure later assignments are journaled. | Specify opaque storage or copy-and-commit mutation APIs, including sim arrays, materials, and joint setters. State how compilation will reject each remaining direct write. |
| 3 | Medium | §§6, 8 | A rewind undoes the unclosed pending segment, but the draft does not say that the staging entries are removed. If they are closed by the next step, an undone API operation could enter the new branch’s history. Entries that own blocks also need disposal rules. | Define pending-segment rollback, ownership transfer, and buffer reset before another write or capture. Test rewind with pending creation and destruction calls. |
| 4 | Medium | §§6, 8, 14 | With `captureInterval > 2`, retaining a minimum of two tick slots can leave only journal slots and no image. History would report a retained window with no restorable tick. | Evict in image-anchored groups or reserve at least one usable image when widening the interval; specify the behavior when that exceeds `maxBytes`. |
| 5 | Medium | §7.1 | The sensor `array push` entry carries element bytes, while the later ownership rule says undoing a sensor creation hands its three allocated arrays to the entry. The push payload and undo/redo description omit those handles (`shape.c`, `sensor.c`). | Add ownership handles and sizes to sensor push entries and specify their transfer on undo, redo, eviction, and branch truncation. |
| 6 | Medium | §§8, 2.5 | “Bounded memory” conflicts with admitting an oversized slot or minimum window indefinitely. The draft correctly describes `maxBytes` as a target, but its requirement and API comment still read like a cap. | Describe the guarantee as a soft budget and state the possible overrun behavior in the requirement and API. |
| 7 | Low | §10 | The claim that a shape alive at both ends of a journal walk keeps `userShape` is too broad. Shape geometry setters can destroy that debug handle while keeping the shape alive (`shape.c`); rewind cannot revive it. | Narrow the promise and define how a restored shape obtains a replacement debug handle, if needed. |

## Checked, no change

- The existing recording test does seek backward and compare hashes after replay; `b3HashWorldState` covers body transforms and velocities only (`test/test_recording.c`, `recording.c`).
- The serializer records the six world id pools, solver sets, sensors, islands, broad phase, and graph; it clears host `userData` on object restore (`world_snapshot.c`).
- `sensor->overlaps2` is the active overlap state. Restoring it supplies the previous overlaps when the next step swaps buffers (`sensor.c`).
- Proxy ids have a separate tree free list, and broad-phase proxy creation and destruction are the world-tree call sites identified by the draft (`dynamic_tree.c`, `broad_phase.c`).
- Explosion impulses currently accumulate in tree query order; the proposed shape-id ordering addresses a real order dependence (`physics_world.c`).

## Proposed edits

Resolve findings 1–5 in the design before treating phase 1 as an exact implementation plan. Then align the memory and host-handle contracts with the behavior described elsewhere in the draft. Add targeted tests for static-tree churn, pending-segment rewind, sensor creation across a branch, and interval widening without an image.

## Unresolved / disagreements

The measured performance figures could not be verified: their cited file is under `docs/designs/reviews/`, which the review instructions exclude. The proposed order changes and capture-cost target also remain untested in this source tree.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:34:14+10:00, ended 12:40:03+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after the call was identical, so the working tree was
untouched by Codex. Fresh full-scope prompt (the doc and the source only, the reviews directory
excluded, file-only citations stated as by design), no declined-findings list: rounds 6 to 12
declined nothing and the earlier declines were answered in the doc. Before the round `git diff` of
the target was empty, the doc's mechanism sections were re-read against their siblings, and
AC-1 through AC-4 were checked with no failure. That sibling check found two gaps in §7.1's entry
inventory, fixed before the prompt: the sensor destruction's ownership handles were absent from the
array removeswap row, and §7.4's proxy create/destroy entry had no row. The final message above is
the last copy in the raw log, unchanged.

**Findings verified against source.**

1. **Applied.** `solver.c` queries the static tree in the CCD sweep for every fast shape
   (`b3DynamicTree_Query( staticTree, ...)`), and the doc's own §7.4 says the static tree is never
   imaged. Phase 1 as written kept the CCD order change in phase 2, so a static proxy replayed
   through the journal could sit in a different layout than at T while CCD still read it in
   traversal order. §13 now puts the §7.4 order changes in phase 1 and runs the restore-time tree
   pass on the static tree there; phase 2 keeps only the layout reconstruction that replaces the
   kinematic and dynamic raw images. §7.4's fallback sentence, which made the CCD change optional,
   now says the order changes are required in every phase because the static tree is world-sized and
   never imaged.
2. **Applied as a sharpening.** `b3GetShapeMaterials` (`shape.h`) does return a writable
   `b3SurfaceMaterial*` from a `const b3Shape*`, and the material setters in `shape.c` write through
   it, so the const-only claim for the materials block needed a named replacement. §5.2 already
   said write accessors journal before assigning the new value; it now says an accessor takes the
   completed value, never returns a record pointer, and names the const-returning replacement and
   `b3Shape_WriteMaterial`. The rest of the finding restates the module-boundary approach the doc
   already gives; no design change.
3. **Applied.** §6 said `Rewind` walks the pending entries but not what becomes of them. Left in the
   staging buffer they would close into the next tick's segment and replay an undone call, and the
   blocks they own (undo of a create hands blocks to the entry) would have no owner. §6 now discards
   the walked entries, freeing their blocks as truncation does, and resets the staging buffer before
   any further write or capture.
4. **Applied.** §8's minimum window was "say 2 ticks", and slots exist per tick while images exist
   only every K-th tick, so with K > 2 the window could hold journal slots and no image. It is now
   two imaged ticks with the journal segments between them.
5. **Applied; a consequence of this session's own pre-round fix.** The removeswap row carried the
   sensor's array handles after the pre-round edit, but a push is the create side and the same
   ownership transfer applies (undo of a create detaches the arrays into the entry). The row now says
   push or removeswap.
6. **Applied as a comment fix.** Requirement 5 and §8 already say `maxBytes` is a target and that an
   oversized slot or minimum window is admitted; Codex's own text concedes that. Only the API
   sketch's field comment still read as a hard cap and now says target and cross-references §8.
7. **Applied.** `b3DestroyShapeAllocationForShapeChange` (`shape.c`) calls `destroyDebugShape` and
   nulls `userShape` on a shape that stays alive (geometry change), and no journal walk can revive
   it, so "a shape alive at both ends keeps its handle" over-promised. §10 now says it keeps its live
   handle, which is null if a shape-change setter released it after T.

7 applied, 0 declined.

## Status

Finding count rose to 7 (2H-4M-1L) from 4, the first increase since round 4, and both Highs are
new categories: the phase plan leaves the static tree's layout unaddressed (1), and the write
boundary's concrete accessor shape was unstated (2, a sharpening). Findings 3 and 4 are lifecycle
gaps at rewind and at interval widening that no earlier round touched; 5 sits next to round 12's
finding 1 (ownership handles per owner, one row over) and was half-closed by a fix made just before
this round; 6 and 7 are wording over-promises. None reopened stated rationale, and none was a
consequence of round 12's fixes. The series is not converging: the count is up and the findings are
still mechanism-level.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 13: 7 findings (2H-4M-1L), 7 applied, 0 declined`

`Series total: 83 findings (37H-42M-4L) across 13 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L)`
