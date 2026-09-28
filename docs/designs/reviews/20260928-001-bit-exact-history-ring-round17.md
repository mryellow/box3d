---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 17), fresh session, resumed after a timeout
session: 01a0e7ce-5acc-7293-9d09-baaa11c37e35
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 17

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the source paths checked. Two issues remain: restoring a raw debug shape handle can resurrect a freed pointer, and FIFO eviction can leave retained ticks with no earlier image from which to replay.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2, §9 | The generic shape record includes `userShape`. If a shape alive at T is destroyed later, `shape.c` destroys its debug handle; copying T’s record back can restore that freed pointer. `physics_world.c` later passes it to the draw callback, and `world_snapshot.c` handles this pointer separately during its own restore. | Exclude `userShape` from image and journal record copies. Reconcile live debug handles with restored shape identities and recreate them when needed. |
| 2 | Medium | §2, §6, §8 | Evicting only the oldest slot can remove an image while retaining the following journal-only ticks. With images at ticks 0 and 4, evicting slot 0 leaves ticks 1–3 retained but without an image at or before them, contrary to the promise that every retained tick is reachable. | Evict through the tick before the next image, or define the reachable window as starting at the oldest surviving image and exclude earlier slots from the retained-tick contract. |

## Checked, no change

- `sensor.c` swaps overlap buffers each step and marks changed `overlaps2` content through its event bitset, supporting the journal rule in §5.2.
- `id_pool.c` uses a free list and bump cursor as described in §3 and §7.1.
- `recording.c` hashes transforms and velocities, matching §1’s limited description of the existing hash.
- `world_snapshot.c` clears body, shape, and joint `userData` on restore, supporting the Phase 0 caveat in §13.
- `solver.c` currently uses the running CCD fraction and caps sensor hits during traversal, supporting the order dependence identified in §7.4.

## Proposed edits

1. In §5.2 and §9, specify that shape record copies omit `userShape`, and add restore steps for disposing of handles belonging to removed shapes and creating handles for restored shapes when drawn.
2. In §8, state and enforce the image anchor invariant: every tick advertised as retained and reachable has a surviving image at or before it. Align `oldestImageTick` and `GetRestorableTick` in §14 with that rule.

## Unresolved / disagreements

None from the source paths checked.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation and resume.** First attempt: `codex exec --sandbox read-only "<prompt>"` (CLI
default model/effort), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash tool
`timeout: 600000`. Start 2026-09-28T21:37:35+10:00; hit the shell-level `timeout` at 540s,
`EXIT_CODE 124` — a genuine stall/long round, not a tool-configuration mistake this time (the Bash
tool parameter was set correctly and the call ran foreground throughout). Per the user's
mid-session request, the round was paused at that point without resuming, and picked back up in a
later turn on explicit instruction to resume and apply. Resumed per WORKFLOW.md:
`codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium" resume
01a0e7ce-5acc-7293-9d09-baaa11c37e35 "<short finalize prompt>"`, pinning the exact model and
effort the killed run's own banner reported (`gpt-6-sol`, `medium`) so the finalize turn didn't
silently fall back to a different default. Foreground, stdin from `/dev/null`, Bash tool
`timeout: 600000` again. Start 2026-09-28T21:47:34+10:00, end 21:48:53+10:00, `EXIT_CODE 0` —
returned inline in under 90s, using the research the killed run had already done. Codex ran
read-only both times; the working tree was untouched by either going in. The curated
declined-findings list from rounds 1–7 (nine items, plus the round-14 pool-`nextIndex` item) was
included in the original prompt; Codex neither re-raised nor disputed any of them.

Both findings verified directly against source:

- **#1 (a shape's `userShape` debug-draw handle is swept up by the generic "shapes[id] fields
  other than aabb/fatAABBs" journaled record, so undo/redo of an unrelated setter, or restoring
  across a destroy, can write a since-freed handle pointer back into a live shape, which
  `b3World_Draw` then hands to the caller's draw callback), CONFIRMED, applied.** Confirmed
  `userShape` is a `b3Shape` field (`shape.h`) populated *lazily*, only when drawing
  (`physics_world.c`: `if (shape->userShape == NULL && world->createDebugShape != NULL) ...`), and
  destroyed explicitly by `b3DestroyShapeAllocationForShapeChange` (`shape.c`) whenever the shape
  is destroyed or its geometry changes — the live code frees the handle and nulls the field in the
  same operation a structural journal entry would capture. §5.2's row for this field previously
  named no exclusion, unlike `proxyKey` (round 14). Traced the exact hazard: destroy a shape whose
  `userShape` is live → the live call frees the handle and journals the destroy (old bytes include
  the now-freed pointer value) → `Rewind` to before the destroy copies those old bytes back →
  `shape->userShape` now holds a dangling pointer → the next draw call passes it straight to
  `DrawShapeFcn`. This is the same defect *shape* — a heap/host-owned resource encoded as a plain
  pointer field that a byte-level record copy cannot safely undo/redo — as hull data (§7.1),
  materials arrays, triangle caches, and sensor arrays; `userShape` was simply the one instance of
  it not yet given equivalent treatment. Found the exact right precedent already solved in this
  codebase: `b3DesShapes` (`world_snapshot.c`) already carries a live debug handle over when a
  restored shape's id *and generation* match what's still live in that slot (avoiding a needless
  GPU-resource teardown/rebuild), and destroys handles belonging to slots whose identity changed.
  Applied that same pattern rather than Codex's plainer "recreate when needed": excluded
  `userShape` from the generic shape-record row (§5.2, alongside `proxyKey`), and added a restore
  step (§9 step 5) that destroys a live handle only when the slot's restored generation doesn't
  match the live one, otherwise leaving the live handle untouched — cheaper than Codex's proposed
  unconditional reconcile-and-recreate, and consistent with how this exact problem is already
  solved elsewhere in this codebase.

- **#2 (oldest-first FIFO eviction can remove the one image several younger, still-retained,
  image-less ticks depend on, leaving them advertised as "retained" but not actually reachable per
  requirement 4's own definition), CONFIRMED, applied.** Constructed the exact scenario the
  finding describes: `captureInterval=4`, images at ticks 0 and 4, window presently 0–7. A new
  capture at tick 8 forces eviction of the oldest slot (tick 0, imaged) to make room. Post-
  eviction, ticks 1–7 (plus new tick 8) remain physically in the arena, but the newest imaged tick
  at or before 1, 2, or 3 no longer exists in the retained set (tick 0's image is gone; tick 4's is
  *after* them) — `b3World_GetRestorableTick(1|2|3)` has no valid answer within the window, and
  requirement 4's "every retained tick... is reachable by restoring the newest imaged tick at or
  before it" is false for exactly those three ticks, even though their journal bytes are still
  sitting in the arena, not yet overwritten. Confirmed this is a real gap in the *stated contract*,
  not merely an implementation detail: nothing in §8 as written before this round said eviction
  ever removes more than one slot at a time, so a naive one-slot-per-wraparound implementation
  genuinely produces this state. Chose the simpler of Codex's two proposed fixes (redefining the
  contract rather than batch-evicting through the next image): requirement 4 now explicitly scopes
  "reachable" to ticks at or after the oldest *surviving image*, and §8 states plainly that
  eviction can leave younger image-less slots as inert, unreachable bytes awaiting physical
  overwrite — not a correctness gap, since `oldestImageTick` (already the documented "oldest
  restorable tick" in §14, unchanged) is the authoritative boundary callers must respect. This
  avoids adding batch-eviction bookkeeping to §8's otherwise-simple one-slot-at-a-time mechanism
  for a case (an isolated capture interval far larger than the window immediately after a resize)
  that costs only a temporarily smaller-than-K-implies effective window, not incorrect behavior.

## Status

Round 17 of an ongoing series (rounds 1–16 committed). Both findings genuine and applied, none
declined; this is the first round run across two separate turns (a mid-session pause on explicit
user request between the timeout and the resume), handled per WORKFLOW.md's resume procedure
without losing the round's own result. Finding #1 is the *sixth* distinct field/structure this
series has found needing ownership-transfer-or-exclusion treatment instead of a plain record copy
(after hull data, materials arrays, mesh triangle caches, sensor arrays, and — as an exclusion
rather than a transfer — `proxyKey`), and the first one whose consequence is a caller-side
use-after-free in rendering code rather than a wrong physics result — a reminder that "generic
record write, unless excluded" as §7.1's default rule keeps needing new named exceptions, each
found by a different round, rather than converging on a rule that catches the whole class up
front (every pointer-typed field of `b3Shape`, not just the ones found so far, is a candidate for
the same hazard, and a future round should check whether any remain). Finding #2 is a new failure
mode for this series: not a missing journal write or a wrong restore step, but the *eviction*
mechanism itself breaking a requirement stated elsewhere in the document, caught only by explicitly
constructing a multi-tick timeline rather than checking any single restore in isolation. Given both
findings are Medium severity, source-confirmed, and touch mechanisms (generic record journaling,
ring eviction) central enough that they're unlikely to be the last gap in either, another
full-scope round is warranted, with attention to whether other pointer-typed shape/body/joint
fields share finding #1's hazard shape.
