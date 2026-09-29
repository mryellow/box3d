---

title: Codex review — bit-exact history ring, round 4
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 4

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible, and the document correctly limits what the existing recording tests prove. The proposed restore procedure still has gaps that can change replay results or restore invalid pointers. I recommend revisions before implementation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.4, §9 | Undoing a shape destruction reverses the shape write before the proxy destruction. The proposed shape restore hook would call `MoveProxy` while that proxy is absent. This follows the operation order in `shape.c`. | Apply journal changes without tree updates, then reconcile proxies in a separate pass after the journal walk and image copy. |
| 2 | High | §5.1, §7.4 | Moved state is not stored in `b3Shape`. `Joint collision changes` can mark a proxy moved without writing a shape or creating a proxy, so the stated image and journal cannot reconstruct that state. | Add an explicit moved-proxy state entry or capture the complete moved-proxy set, including marks made between steps. |
| 3 | High | §5.2, §7.1 | Only hull pointee lifetime is handled. Mesh, height-field, and compound shapes store borrowed geometry pointers; restoring a shape after its caller has released that geometry can leave a dangling pointer. See `shape creation`. | Define a retained-geometry ownership contract or keep geometry alive through the history window. Include it in memory accounting. |
| 4 | High | §7.4 | The proxy journal describes create/destroy as an ID-pool-shaped inverse, but the `tree proxy allocator` grows capacity and adds a chain of free IDs. Undoing a create by destroying its proxy does not restore the earlier free-list and capacity state. | Specify and test exact proxy-pool restoration across growth, including alternate timelines after rewind. |
| 5 | High | §2, §10 | The stated v1 exception for explosion wake order permits different overflow constraint order after restore. That conflicts with the central claim that unchanged inputs replay bit for bit. | Make explosion wake order deterministic before claiming bit-exact v1, or narrow the headline requirement and acceptance criteria explicitly. |
| 6 | Medium | §14 | The correction example replays calls for `T+1..serverTick` without stepping those ticks. It applies the server correction while the world is still at T. | Step each tick through `serverTick` before applying an end-of-tick correction; specify the correction’s tick boundary. |
| 7 | Medium | §11.2 | Time-sliced replay assumes the ring has every old pose. With `captureInterval > 1`, it does not; stepping after rewind also overwrites future slots (§8). The API exposes no pose lookup. | Describe a separate presentation-pose buffer or restrict this example to a retained, unmodified image at every rendered tick. |
| 8 | Medium | §5, §8, §10 | The world’s `name cache` is append-only and absent from the inventory. Record `nameId` values can rewind, but names introduced on discarded timelines remain allocated, outside `bytesUsed`. | State that names persist and account for their memory, or journal/cache them with an eviction policy. |
| 9 | Medium | §12 | A cross-platform “full state hash” cannot hash raw records directly: records contain addresses and heap-backed arrays, including `shape pointers`. The hash plan does not say how these are canonicalized. | Specify field-wise hashing of semantic values, geometry contents, pool order, and array contents while excluding pointers and padding. |

## Checked, no change

- `ScrubBackward` restores and re-steps, and its `current hash` covers body transforms and velocities only.
- The `serializer` captures six ID pools, solver sets, sparse records, sensors, trees, pair set, and graph colors.
- Sensors are evaluated each step regardless of body sleep state; their current overlaps are sorted and deduplicated in `sensor.c`.
- Pair keys are sorted before contact creation in `broad_phase.c`. CCD’s running fraction and eight-hit sensor cap remain traversal-sensitive in `solver.c`, as the design says.
- The documented whole-world resimulation cost and the phase-1 raw-tree fallback are consistent with the proposed scope.

## Proposed edits

Resolve findings 1–5 in the state model and restore algorithm, then update the hash specification, correction example, time-sliced replay guidance, and memory accounting. Add targeted tests for proxy growth, moved marks without shape writes, destroyed borrowed geometry, and explosion replay.

## Unresolved / disagreements

The document deliberately accepts an explosion case that can break bit-exact replay. I disagree with calling v1 bit-exact while that case remains. I did not read anything under `docs/designs/reviews/` or modify files.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only`, model `gpt-6-sol`, reasoning effort `medium`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T11:23:08+10:00, ended 11:27:23+10:00, exit code 0 — no resume needed. Codex ran
read-only; the working tree was untouched by the call. Raw transcript stays in the scratchpad.
Same prompt as round 3: target doc and source only, no access to `docs/designs/reviews/`,
file-only citations stated as by design, no declined-findings list (round 3's one decline was
answered by sharpening the doc, so it is not carried). The final message's file links are reduced
to plain backticked names, with no other change.

**Before the round.** `git diff` of the target was empty. Round 3's watch item — restore's "clear
all moved bits" looked O(proxies) — was fixed before the round by replacing it with a per-record
moved state. That replacement was wrong (see findings 1 and 2) and was reverted in this round.
Re-checking sibling cases turned up a gap Codex did not raise, fixed below (see "Found by Claude").

**Findings verified against source.**

1. **Applied.** Confirmed in `shape.c`: destruction destroys the proxy and then writes the record,
   so undo (reverse order) restores the record before re-creating the proxy, and a restore hook
   that calls `b3DynamicTree_MoveProxy` per record write would hit an absent proxy. §7.4 now has
   restore-time record writes append to a restore list and touch no tree; one pass after the
   journal walk and image scatter moves each listed shape's proxy, when every proxy exists.
2. **Declined; the original design stands.** `b3DynamicTree_ClearMoved` (`dynamic_tree.c`)
   descends only nodes marked moved, so it is O(moved), not O(proxies) — the round 3 watch item
   was wrong, and the original "clear each tree's moved bits, then set them from image T's moved
   list" bullet is restored (with `ClearMoved`'s cost stated). Wholesale clearing also covers
   Codex's scenario: marks made by API calls (`b3Body_*` setters, `shape.c`) are set after the end
   of a step, are consumed by the next step's pair update and rebuild (`broad_phase.c`,
   `dynamic_tree.c`), and are cleared wholesale on restore, so bits present at the end of step T
   are the hot-path enlargements of awake shapes (`solver.c`), which the image's moved list
   captures. A per-shape moved state does not exist in `b3Shape`, so the earlier replacement had
   no basis.
3. **Applied.** Confirmed: `shape.c` stores mesh, height-field and compound pointers as given, and
   `box3d.h` documents that the mesh and height field "must remain valid for the lifetime of this
   shape". Restoring a shape after the caller freed its geometry would dangle. Added a caller
   contract to §10; the engine does not own this geometry, so the ring does not retain it.
4. **Declined.** `dynamic_tree.c` grows the proxy array when the free list is exhausted, appending
   a chain of new ids in ascending order starting at the old capacity. Undoing a create pushes the
   id back at the free-list head, which restores head order; extra capacity left by a discarded
   timeline holds the same ascending chain a replayed growth would append, so the id sequence
   handed out is identical. Capacity is not observable state. The doc now says so in §7.4.
5. **Applied, by changing the design.** This reopens round 2's finding 8 (the explosion exception)
   and does not engage the doc's stated position, but the doc's position was "documented, not
   fixed" with no reason for deferral. The fix is small and matches the impulse-accumulation fix
   already accepted: the explode callback collects hit shapes, and sets are woken and impulses
   applied in shape-id order afterwards. The overflow-colour exception is removed from §2, §7.4,
   §10 and §16.
6. **Applied.** Confirmed: the §14 example replayed inputs for T+1..serverTick without stepping
   them. Rewritten as one loop from T+1 that steps every tick and applies the correction after
   `serverTick` (or immediately if T equals it).
7. **Applied.** Confirmed: §8 says stepping after a rewind overwrites later slots, and with
   `captureInterval` above 1 only every K-th tick has an image, so the ring cannot serve every
   old pose. §11.2 (3) now renders from the caller's own recorded poses.
8. **Applied.** Confirmed `world->names` in `physics_world.h`, populated by `b3AddName` in
   `body.c` and `shape.c` and referenced by `nameId` in records. Added to §5.3 as diagnostic,
   append-only, outside the budget, and excluded from the hash.
9. **Applied.** §12 said "hashing float bit patterns" but records hold pointers and padding.
   Specified field-wise hashing over semantic values with pointees hashed by content and
   padding, addresses and `nameId` excluded.

**Found by Claude, not raised by Codex.** `solver_set.c` never touches shape records, while
`solver.c` writes `shape->aabb` and `fatAABBs` in place for awake bodies. A body asleep at T and
awake at P therefore loses those shape fields on undo: the wake's own record writes do not cover
them. The same holds for a contact's manifold block and mesh cache, which hold hot-written data
behind a pointer the wake's contact record write only captures by address. §7.1 now requires the
one function that moves a body, and the one that moves a contact, into the awake set to journal
those pre-wake bytes.

7 applied, 2 declined.

## Status

Finding count 9 after 8 (5H-4M-0L), so the count is not falling steadily; Highs rose from 4 to 5.
New categories again, not edge cases: proxy restore ordering (1), borrowed geometry lifetime (3),
the name cache (8), correction example and time-sliced replay (6, 7), hash canonicalisation (9).
Finding 1 is a direct consequence of round 3's fix (the restore hook added for §7.4's layout
bullet). Finding 2 and 4 were wrong readings of the source. Finding 5 reopened ground: it re-argued
the alternative without engaging the stated position, and I resolved it by removing the
exception, not by declining. The wake gap Claude found is the same class as rounds 1–3's
findings: state hot-written while awake that an undo does not capture.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 4: 9 findings (5H-4M-0L), 7 applied, 2 declined`

`Series total: 40 findings (23H-17M-0L) across 4 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L)`
