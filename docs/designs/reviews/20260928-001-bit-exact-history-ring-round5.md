---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=high
mode: broad (round 5), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 5

## Review report (Codex final message)

## Summary

Whole-world replay is a sound direction, and the recording test demonstrates rewind and replay for the transforms and velocities its current hash covers. The proposed history ring does not yet establish the stronger bit-exact guarantee. In particular, the specified image restore can copy stale heap pointers into live records.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §6, §9 | The hot image copies `b3Contact` and `b3Island` records, then restore copies those records over live storage. They contain heap pointers. Copying a contact record *after* allocating its manifold block can replace the new pointer with the captured address; island link arrays have the same problem. The existing serializer explicitly clears and reconstructs contact pointers. `src/contact.h`, `src/island.h`, `src/container.h`, `src/world_snapshot.c` | Define pointer-free image records. Restore owned arrays and blocks, then install their live pointers; specify the same rule for journaled record writes. |
| 2 | Medium | §5.1, §7.4 | The moved list stores a proxy ID, while tree reconstruction may recreate a proxy using a different ID from the live free list. Reapplying the saved integer can mark the wrong proxy despite a valid regenerated `shape->proxyKey`. Moved flags also propagate to ancestors. `src/dynamic_tree.c`, `src/broad_phase.c`, `src/shape.c` | Store stable shape IDs for moved membership, resolve each current `proxyKey` during restore, and mark its ancestor path. |
| 3 | Medium | §12 | The proposed full hash omits graph-colour `bodySet` membership, and the cold-hash guard does not explicitly cover it. First-fit colour assignment reads these bits, so a bad restore can pass the stated immediate oracle and change a later contact’s colour. `src/constraint_graph.c`, `src/constraint_graph.h` | Hash active `bodySet` membership in both the state oracle and cold-write guard. |
| 4 | Medium | §7.4 | CCD currently admits a sensor hit against the *running* solid fraction. Collecting sensors and sorting before the eight-hit cap is insufficient unless admission is deferred: sensors beyond the final solid hit could consume cap positions ahead of valid hits. `src/solver.c` | Specify: evaluate candidates against the fixed initial fraction, determine the final accepted solid fraction, filter sensor hits against it, then sort and cap. |
| 5 | Medium | §8 | The stated budget exceptions miss a minimum-window case: two ticks’ journals plus one required image can exceed `maxBytes` even when neither one tick nor the journals alone exceeds it. Widening the interval cannot remove that last image. | Allow eviction to one imaged tick, or explicitly permit and report a minimum-window overrun. |
| 6 | Medium | §11.2, §14 | Time-sliced replay says the caller can render old poses from the ring without engine support. The sketched API exposes neither pose reads nor an old-timeline view, and branching replaces the slots needed to navigate that timeline. | Require the caller to cache old poses before branching, or add a read-only image/pose access path. State its memory cost. |
| 7 | Low | §7.1 | The heap-material hook list names `SetMaterial`, but the actual indexed setter is `b3Shape_SetMeshMaterial`. Its writes affect mesh contact mixing and are outside the shape record. `src/shape.c`, `src/mesh_contact.c`, `include/box3d/box3d.h` | Name the real setter and require an element journal entry for every indexed write. |
| 8 | Low | §14 | The correction example replays calls for `T+1..serverTick` without explicitly stepping those ticks. That matters whenever `captureInterval > 1` and `T < serverTick`. | Show a step for every tick through `serverTick` before applying the correction. |

## Checked, no change

- `ScrubBackward` seeks backward and checks replay hashes; `b3HashWorldState` currently hashes body transforms and velocities. `test/test_recording.c`, `src/recording.c`, `src/recording_replay.c`
- The broad phase sorts discovered shape-pair keys; sensor overlaps are sorted and deduplicated. `src/broad_phase.c`, `src/sensor.c`
- The current CCD query uses a running fraction and caps sensor hits at eight; the closest-ray callback overwrites equal-fraction results. `src/solver.c`, `src/physics_world.c`
- Six ID pools use a free list and bump index. The process-wide length scale and build settings are relevant to replay. `src/id_pool.c`, `src/core.c`, `CMakeLists.txt`

## Proposed edits

Resolve findings 1–4 in the restore and verification algorithms before treating bit-exactness as established. Then align the budget rule, time-sliced replay claim, journal hook names, and correction example with the behavior the API will provide.

## Unresolved / disagreements

Finding 2 is narrower than the previously declined proxy-allocator concern. It does **not** require preserving numeric proxy IDs; it requires the saved moved list to remain meaningful when those IDs change. The CCD behavior change remains an explicit owner decision in §16.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`,
foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T19:32:37+10:00, end 19:40:06+10:00, `EXIT_CODE 0` — no resume needed, returned inline.
Raw log had the final message duplicated across earlier positions; one copy kept above. Codex ran
read-only; working tree was untouched by it going in. The curated declined-findings list from
rounds 1–4 (eight items, including round 4's #8 and the CCD-algorithm-choice item) was included
in the prompt; Codex did not re-raise any of them, and explicitly flagged its own #2 as narrower
than round 1's declined proxy-numeric-id finding, which checked out on inspection.

Each finding checked against source before any doc edit:

- **#1 (image restore can overwrite a correct live pointer with a stale captured one), CONFIRMED,
  applied.** Confirmed `b3Island.bodies` (and, per its own inventory comment, `contacts`/`joints`)
  are `b3Array`s with their own heap `.data` pointer (`island.h`), and `b3Contact.manifolds` is a
  heap pointer (`contact.h`, confirmed in round 3). §9 step 3, as worded before this round, said
  "if the live slot has a manifold block of the right count, copy into it, otherwise free and
  allocate one; copy the record" — the final clause, read as a whole-struct copy, would overwrite
  the just-corrected live `manifolds` pointer with the stale pointer value captured in the image at
  T, and the same ambiguity existed for island link-array pointers with no manifold-block-style
  clause protecting them at all. Fixed by making both restores explicit: island link arrays are
  resized and their *content* copied first (mirroring the manifold-block treatment), then the rest
  of each record is copied *excluding* the pointer fields whose live values were just set
  correctly by the preceding step.

- **#2 (moved list keyed by a proxy id that restore-time proxy recreation can invalidate),
  CONFIRMED, applied.** Re-read §7.4's own restore bullet 1 (already correct, from round 3): a
  shape whose journaled body type differs from its live proxy's tree gets its proxy destroyed and
  recreated *in a different tree*, which the tree's own LIFO free list then hands a new numeric id
  from — a real, mechanical renumbering, not merely the "numeric ids need not match" point round 1
  already declined (which was about whether raw ids feed any comparison; they don't, but that
  doesn't help here, because this bullet uses the *saved* numeric id as a *live* array index to set
  a bit). Confirmed the tree also propagates the moved bit to ancestors on every ordinary path
  (`dynamic_tree.c`, "Set the moved flag on the ancestors of the proxy"; also the atomic path
  around `b3DynamicTree_EnlargeProxy`), which a raw bit-set on just the leaf would skip. Fixed by
  keying the captured moved list by shape id (§5.1) and having restore resolve each id through the
  shape's own, by-then-already-restored `proxyKey`, propagating to ancestors the same way the hot
  path does, rather than reusing the captured numeric proxy id as a live index.

- **#3 (graph-colour `bodySet` bitsets missing from the hash and cold-hash guard), CONFIRMED,
  applied.** §5.2 already lists "colour `bodySet` bitsets" as cold/journaled state (from round 1),
  but §12's full-hash inventory (extended across rounds 2, 3, and 4) never named it, and neither
  did the cold-hash guard's own enumeration — exactly the class of gap those rounds' hash-coverage
  fixes were meant to close, just missed for this one structure. Added it to both lists.

- **#4 (CCD sensor admission uses the running, not final, solid fraction), CONFIRMED, applied.**
  Read `b3ContinuousQueryCallback` (`solver.c`): a sensor candidate is admitted with
  `output.fraction <= continuousContext->fraction`, where `continuousContext->fraction` is the
  *running* best solid fraction at the moment that candidate is visited — which shrinks over the
  course of traversal and therefore depends on the *order* solid candidates are evaluated in, even
  after the "evaluate every candidate against the initial fraction" fix makes the *final* solid
  fraction itself order-independent. Two orderings that agree on the final solid fraction can still
  admit different sensor sets, because each candidate is checked against whatever the running value
  happened to be when *it* was visited, not the eventual final value. §7.4's "Change:" paragraph
  specified deterministic *cap selection* among admitted sensors but never made *admission itself*
  order-independent. Fixed by making admission a second pass, run after the final solid fraction is
  known, filtering against that fixed value rather than the transient running one.

- **#5 (`maxBytes` exception enumeration misses the combined-total case), CONFIRMED, applied.**
  Re-read §8's two stated exception conditions ("journal segments in the minimum window alone" /
  "a single tick's own required storage") against a concrete minimum window of one imaged tick
  (journal + image) plus one unimaged tick (journal only): neither condition alone covers the case
  where the *sum* of all three pieces exceeds `maxBytes` while no two-of-three subset does. Fixed
  by replacing the two partial conditions with one that covers the minimum window's total required
  storage directly (every tick's journal plus the one required image), which subsumes both
  previously-stated cases.

- **#6 (time-sliced replay implies a ring-read API that doesn't exist), CONFIRMED, applied.**
  Checked §14's API sketch: no function reads pose or image data from a specific retained tick
  without a full `Rewind` (which mutates live state and would conflict with "plus the live tick").
  §11.2 point 3's "rendering from the ring's recorded poses" therefore described a capability the
  design never specified. On reflection the mitigation doesn't need one: time-sliced replay is just
  pausing the replay-stepping loop across frames and rendering the live world's ordinary current
  state after each frame's batch — no ring read of any kind, and (since it doesn't touch images at
  all) no dependence on `captureInterval` either, which also let me drop the now-incorrect "only at
  K=1" qualifier the old wording carried for the wrong reason.

- **#7 (naming error in round 4's own material fix), CONFIRMED, applied — corrects round 4.** My
  own mistake: round 4 named the indexed material setter `SetMaterial`. Confirmed via
  `include/box3d/box3d.h` and `shape.c` that no such function exists; the real one is
  `b3Shape_SetMeshMaterial( shapeId, surfaceMaterial, index )`. Corrected the name; the substance
  of round 4's fix (per-element journaling for a multi-material shape's heap array) was already
  right and unchanged.

- **#8 (correction example omits explicit per-tick stepping), CONFIRMED, applied.** §14's example
  showed a prose comment ("replay ... for ticks T+1..serverTick") with no `b3World_Step` call,
  unlike the second loop's explicit `for` with a step call — misleading whenever `captureInterval
  > 1` makes `T < serverTick` common. Rewrote the first phase as an explicit loop matching the
  second's shape.

## Status

Round 5 of an ongoing series (rounds 1–4 committed or pending commit). All 8 findings were
genuine and applied — the first round in this series with zero declines. One (#7) is a direct,
source-confirmed correction of a naming error *I* introduced in round 4 (the second time in two
rounds a fresh pass has caught an error from my own immediately preceding fix, after round 4's
own #4 corrected a round 3 error). Finding #1 is the most consequential defect found in this
series so far: a stale-pointer bug in the restore *algorithm description* itself, present in the
document since round 1 and never caught by four prior rounds despite each auditing "the same"
mechanism from scratch — it survived because no prior round happened to trace what "copy the
record" means for a struct containing a pointer field the preceding sentence had *just* set
correctly. The finding rate (9, 8, 8, 7, 8 applied) is not decreasing, and this round found a
higher-severity class of defect (a live-state-corrupting algorithm bug, not a coverage gap) than
several recent rounds, which is a stronger non-convergence signal than a flat rate alone. Another
full-scope round is warranted, not a narrower one.
