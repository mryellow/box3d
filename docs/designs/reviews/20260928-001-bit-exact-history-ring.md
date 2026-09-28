---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=high
mode: broad (first round for this target)
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md

## Review report (Codex final message)

## Summary

The existing recording player provides a useful starting point, but its backward-seek test compares a hash of body transforms and velocities. It does not establish the document's stronger bit-exact claim. The proposed hot image also misses state that can change while its owner is awake, and the journal and tree restore rules do not yet support reliable backward and forward scrubbing.

I reviewed the current checkout, which differs from the commit named in the document. This was a source review; I did not run tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5–§7, §9 | An awake shape's image contains only its AABB and fat AABB. Setters can change its filter, material, geometry, flags, and heap material array. Under §7's owner rule, those awake writes are not journaled. Copying a `b3Shape` record would also leave owned material data and hull references unresolved. `shape.c`, `shape.h`, `world_snapshot.c` | Image complete awake shape state or journal every shape mutation, with explicit handling for owned data and hull references. |
| 2 | High | §5, §7, §9 | A shape can be sleeping at T, wake later, and have its AABB and fat AABB changed by subsequent steps. It is absent from image T, and waking does not journal its old AABBs. Reversing the wake therefore cannot restore those bounds at T. `solver_set.c`, `solver.c`, `shape.c` | On every cold-to-hot transition, retain the cold shape bounds before hot writes begin, or make those bounds independently reversible. Test sleep–wake–move–rewind. |
| 3 | High | §5, §7.4, §9 | Fat AABBs and moved bits do not reconstruct the tree's proxy allocator. Proxy IDs have a free list and are stored in `shape->proxyKey`; proxies also hold category bits and user data. Create/destroy through the shape journal alone does not specify how to restore those IDs and mappings. `dynamic_tree.c`, `dynamic_tree.h`, `broad_phase.c`, `shape.h`, `world_snapshot.c` | Specify a reversible proxy allocation record, or a rebuild that recreates the required IDs and updates every shape key. Include proxy metadata and validation of the mapping. |
| 4 | High | §2, §7.4, §10 | The CCD change does not remove every tree-order dependence. CCD accepts at most eight continuous sensor hits in traversal order. `b3World_Explode` also wakes bodies in query order; §10 acknowledges that its replay may differ, contrary to §2's bit-exact requirement. `solver.c`, `physics_world.c` | Gather and order CCD sensor candidates deterministically before applying the cap, and give explosion processing a deterministic shape order. Otherwise narrow the bit-exact contract explicitly. |
| 5 | High | §7.1, §9 | Several journal entries lack enough information for both directions. Undoing a pool allocation must distinguish a free-list pop from a bump-index increment. The manifold entry lists old bytes but no new bytes for redo; an old island-array length alone cannot reverse swap removal. `id_pool.c`, `contact.h`, `island.c`, `container.h` | Define old and new payloads and inverse operations for each entry kind, including array contents and lengths. Show a worked undo/redo sequence involving creation, sleep, and contact removal. |
| 6 | High | §4, §6, §9, §10 | API calls after the last completed step mutate the world before the next segment closes. `historyTick` still names the previous image, but §9 walks only closed segments. Rewind at that boundary can leave those pending mutations in place. `body.c`, `shape.c`, `physics_world.c` | Define a pending segment for between-step calls and undo it before walking completed ticks; specify when it is sealed or discarded. |
| 7 | High | §2, §8, §9 | `maxBytes` caps only the arena, while journal-owned arrays and restore allocations sit outside it. Doubling the image interval cannot make an oversized journal or one image fit a smaller budget. §9 may allocate manifolds, and array growth can allocate too, conflicting with the stated allocation-free restore. `container.h`, `contact.c`, `dynamic_tree.c` | Define what the budget counts, a minimum valid budget, and behavior when one required slot or journal exceeds it. Either reserve all restore storage ahead of time or relax the allocation-free claim. |
| 8 | Medium | §5.1, §6, §11 | Sensor overlaps are nested arrays, not one contiguous flat copy. Every sensor swaps and updates overlap arrays each step, including sensors on sleeping or static bodies. Copying all of them makes capture cost proportional to sensor count and overlaps, which need not track awake bodies. `sensor.h`, `sensor.c`, `docs/simulation.md` | Add sensors as an explicit cost term and describe a per-sensor capture scheme, or revise requirement 2 to include sensor activity. |
| 9 | Medium | §1, §3, §12–§13 | `ScrubBackward` compares `b3HashWorldState`, which hashes live bodies' transforms and velocities only. It cannot verify restored contacts, impulses, pools, islands, generations, or events. The claim that whole-world bit-exact rewind is already demonstrated is too strong. `test/test_recording.c`, `recording.c` | Describe the existing test as evidence for pose/velocity replay, then make the proposed full-state hash and structural replay tests the gate for the stronger claim. |
| 10 | Medium | §2, §9–§10 | §2 includes events in exact replay, while §9 clears all event arrays and says events for T are not delivered after restore. A caller inspecting events immediately after `Rewind(T)` sees different results from the original end of T. `physics_world.c` | State whether events at T are part of the restored observable state. If yes, retain them; if no, qualify §2 and define precisely which replayed ticks regenerate events. |
| 11 | Medium | §5, §9, §12 | "World scalars" is too broad to implement safely. The current serializer explicitly includes `stepIndex`, `compoundShapeCount`, `inv_h`, `inv_dt`, configuration flags, and more; the inventory names only examples. In particular, `compoundShapeCount` selects a broad-phase path. `physics_world.h`, `world_snapshot.c`, `broad_phase.c` | List each retained scalar and its capture/restore treatment, using the serializer's configuration list as a starting audit. |
| 12 | Medium | §11.2, §14 | Time-sliced replay assumes the ring has every old pose, but `captureInterval > 1` leaves ticks without images. Replaying also discards old future slots, so the proposed rendering source can disappear before catch-up. `recording_replay.c` | Limit that mitigation to K=1 with protected old poses, or specify a separate presentation buffer. |

## Checked, no change

- The recording player restores a serialized world into an existing shell for backward seeks, and `ScrubBackward` re-steps and compares its recorded hashes. `recording_replay.c`, `world_snapshot.c`, `test/test_recording.c`
- The engine has six ID pools, with a free-list array and bump index. `physics_world.h`, `id_pool.c`
- Awake body sims and states live in contiguous solver-set arrays; touching contacts live in graph colors, while non-touching awake contacts use `contactIndices`. `solver_set.h`, `constraint_graph.h`
- New broad-phase pair keys are sorted before contact creation. `broad_phase.c`
- Tree rebuild copies retained subtrees into a DFS-ordered node array, and CCD currently passes the running best fraction into TOI queries. `dynamic_tree.c`, `solver.c`
- Sensor overlap results are sorted and deduplicated before publication. `sensor.c`
- The build disables floating-point contraction for GCC and Clang, and the determinism tests compare runs across worker counts. `CMakeLists.txt`, `test/test_determinism.c`

## Proposed edits

(Codex's proposed wording; see Post-review verification for what was actually applied.)

1. Replace the §1 proof claim to distinguish demonstrated pose/velocity replay from the full-state claim §12 is meant to establish.
2. Revise §5–§7's classification rule so awake-owner writes to fields the hot path never rewrites are still journaled; complete the shape inventory.
3. Expand §7.1 into an invertibility specification: old/new values, allocation ownership, array count changes, and the exact undo/redo operation per entry kind.
4. Extend §7.4's tree-order-dependence list and its CCD fix to also cover the sensor-hit cap.
5. Add a pending-mutation step to §9 for calls made since the last completed step.
6. Make §8's budget claims precise about what `maxBytes` counts.
7. Clarify §2/§10's event contract for the instant right after `Rewind(T)`.

## Unresolved / disagreements

- The document's choice to keep events transient is defensible for a prediction API, provided the observable-state requirement is narrowed accordingly.
- A derived-tree approach remains plausible, but its deterministic ordering and proxy identity rules need a concrete algorithm before the O(moved · log n) restore bound can be accepted.
- The current determinism tests support repeatability across worker counts for their covered scenes. They do not by themselves establish the proposed full-state hash's cross-platform guarantee for every supported build configuration.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`,
foreground, stdin from `/dev/null`, shell-level `timeout 540`. Start 2026-09-28T12:06:25+10:00,
end 12:11:41+10:00, `EXIT_CODE 0` — no resume needed. Raw log had the final message duplicated
(streaming artifact); one copy is kept above. Codex ran read-only; working tree was untouched by
it (confirmed: target doc's `git diff` was empty going in and no other paths changed during the
run). Codex noted the checkout differs from the doc's cited commit `5643cd8` — confirmed via
`git diff --stat 5643cd8 c23ebcf -- src/ include/`, which is empty; the only difference is the
design docs themselves, so this doesn't affect the review.

Each finding checked against source before any doc edit:

- **#1 (awake shape record not journaled), CONFIRMED, applied.** `src/shape.h` shows `b3Shape`
  carries `filter`, `material`, `materialCount`/`materials` (heap array), `flags`, and the
  geometry union — none of it in §5.1's hot image, which images only `aabb`/`fatAABBs`. §5.2's
  original row gated *all* shape journaling on "non-awake owner," so an API setter on an
  awake-owned shape's filter/material/flags/geometry was neither imaged nor journaled. Fixed by
  splitting §5.2's shape row (bounds stay non-awake-gated; everything else is journaled
  unconditionally, since the hot path never rewrites it) and adding a general clause to §7.1's
  "Rule for record writes." Also closed the ownership sub-issue Codex flagged (materials array,
  hull pointer): confirmed `shape->materials` is a `b3Alloc`/`b3Free`'d heap array
  (`src/shape.c:197-212`, `1050-1054`) that a plain old/new-bytes record write can't safely
  reverse (the pointer would dangle post-free), so folded it into the existing "ownership
  transfer instead of copying" mechanism already used for sets/islands. Confirmed `shape->hull`
  is *not* shape-owned — `b3AddHullToDatabase`/`b3AddOwnedHullToDatabase` (`src/shape.c:142-146`)
  put it in the world's hull database, which outlives the shape — so a plain record write is
  fine for it; said so explicitly to close the ambiguity Codex raised.

- **#2 (sleep→wake bounds transition), DECLINED — verified not a defect.** Traced
  `b3WakeSolverSet` (`src/solver_set.c`) end to end: it touches bodies, contacts, and islands,
  never shapes, AABBs, or proxies. A sleeping body's shapes don't move while asleep, so nothing
  writes their AABB/fatAABB between sleep and wake — the cold value is trivially still correct.
  Once awake, the *first* post-wake step's image (the shape is now in the awake set) supersedes
  it. No write is missing a journal entry because no write happens at the transition. Not
  applied; no doc change needed.

- **#3 (proxy allocator/proxyKey reconstruction), DECLINED — verified not a defect.** Read
  `dynamic_tree.c`'s proxy allocator (`b3AllocateProxy`/`b3FreeProxy`, a LIFO free list over
  `tree->proxies`) and confirmed `shape->proxyKey` (set at `b3CreateTreeProxyInternal`'s return
  value, per `src/shape.c`) is the *only* place a proxy id is read back from — nothing compares
  raw proxy id values or relies on a specific numeric id surviving a rewind. §5.2's existing
  unconditional journaling of shape create/destroy (now confirmed correct per #1) is exactly
  what recreates `shape->proxyKey` with whatever id the tree's live free list hands out on
  replay; the id need not match the original run's id, only be self-consistent, since pair
  ordering is by shape-pair key (`broad_phase.c:733-748`), not proxy id. Not applied.

- **#4 (CCD tree-order dependence, second channel), CONFIRMED, applied — partially.** Grepped
  `solver.c` and found `B2_MAX_CONTINUOUS_SENSOR_HITS 8` with `sensorCount < ...` gating array
  insertion during traversal (`solver.c:313-436`) — a genuine second tree-order channel §7.4
  didn't cover; its "exactly one place" claim was wrong. Fixed by adding the channel and
  extending the CCD fix to also gather-then-cap sensor hits in shape-id order. The
  `b3World_Explode` half of this finding was **already** documented in §10 ("wakes sets in tree
  traversal order... documented, not fixed, in v1") — no new gap there, so no edit was needed
  for that part.

- **#5 (journal entry invertibility), CONFIRMED, applied.** Read `id_pool.c`: `b3AllocId` pops
  the free list when non-empty, otherwise bumps `nextIndex` — two different operations that a
  single "push back" undo can't both reverse correctly. Fixed the pool row to record which
  branch fired. Also fixed the manifold row (added new-bytes payload for redo) and the island
  array row (store the popped element's bytes, not just the old length, since the row's original
  wording lost the removed element's own content on undo).

- **#6 (pending pre-step mutations), CONFIRMED, applied.** §4 already says a slot's journal
  segment covers "the API calls between step t−1 and step t," which implies calls are logged
  into an open, not-yet-imaged segment as they happen — but §9's walk only enumerated *closed*
  segments P down to T+1, silently dropping calls made after step P but before the next step
  runs. Added a step to §9 that reverses that open segment first.

- **#7 (maxBytes scope), CONFIRMED, applied — partially.** The requirement-5 (bounded memory)
  half is real: ownership-transferred arrays (§7.1) are heap allocations referenced by, but not
  counted in, the `maxBytes` byte arena, so a large sleeping-set/island transfer isn't bounded by
  the budget. Fixed §8 to say the budget covers both. Declined the "conflicts with allocation-free
  restore" half: re-read §8's actual claim ("a capture never allocates inside the step") and
  requirement 5 — neither claims *restore* is allocation-free, so §9's manifold
  reallocate-on-mismatch isn't a contradiction of any stated property. No edit needed for that
  part.

- **#8 (sensor overlaps aren't a flat copy), CONFIRMED, applied.** `src/sensor.h` shows
  `overlaps1`/`overlaps2` are per-sensor `b3Array`s (independent heap allocations), not one
  contiguous region — §6 listed them under "flat copies (memcpy)," contradicting §5.1's own
  per-element accounting. Moved to the gather list.

- **#9 (ScrubBackward/hash scope overclaim), CONFIRMED, applied.** Read `b3HashWorldState`
  (`src/recording.c:1188`): it hashes only `sim->transform` and `state->linearVelocity`/
  `angularVelocity` per body — confirmed narrower than "checks per-step state hashes" implied.
  Reworded §1 to scope the demonstrated property to what the hash covers and point at §12 for
  the full claim.

- **#10 (events immediately after Rewind), CONFIRMED, applied.** §2 and §9/§10 weren't
  contradictory on inspection — §2's "events reproduced exactly" is about re-stepping after
  replay reaches a tick again, and §9/§10 already describe clearing/regenerating — but the doc
  never stated the momentary gap explicitly as a caller-facing contract point. Added one
  sentence to §10.

- **#11 (world scalars underspecified), CONFIRMED, applied.** Grepped `physics_world.h`/
  `world_snapshot.c` and found `stepIndex`, `compoundShapeCount`, `inv_h`, `inv_dt` are
  explicitly captured by the existing serializer and are absent from §5.1's example list; added
  them, noting `compoundShapeCount` selects a broad-phase path.

- **#12 (time-sliced replay vs. capture interval/eviction), CONFIRMED, applied.** §11.2's claim
  that "the ring already has every pose" is false whenever `captureInterval > 1` (§6), and
  §8 already documents that ordinary stepping after a rewind overwrites the very slots a
  time-sliced replay would be rendering from. Qualified the mitigation to K=1 and to rendering a
  tick before re-stepping past it.

## Status

First round for this target (round 1 of 1 so far). 9 of 12 findings were genuine and applied;
2 were verified false alarms after tracing the actual allocator/wake code paths (not just
plausible-sounding); 1 was half-confirmed (its `maxBytes` half was real, its allocation-free
half was not). No finding was a consequence of a fix made earlier in this same round — each
traces to the original text. This was a broad first-round pass per the exception for a new
target; it surfaced real cross-section gaps (awake-owner shape state, journal invertibility, the
pending-segment boundary, a second CCD order channel) rather than style nitpicks, so the
document was under-baked in exactly the ways a from-scratch source audit is supposed to catch.
Next: pause here per user request before starting another round. When review resumes, given how
much of round 1 was "the classification/journal rule had an uncovered edge," it's worth another
broad-scope round rather than narrowing yet, since the fixes just made (unconditional shape
journaling, the open-segment reversal step, the two-channel CCD fix) are new surface a
from-scratch reviewer hasn't checked against the rest of the doc.
