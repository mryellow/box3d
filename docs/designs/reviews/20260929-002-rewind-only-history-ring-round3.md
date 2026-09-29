---
title: Codex review — rewind-only history ring, round 3
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Killed by the shell timeout, then resumed to finalize.
session: 01a0eca8-8ed4-7e63-b629-b13d512c5e64
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 3

## Review report (Codex final message)

## Summary

The rewind-only approach is coherent, but the specified undo rules leave two heap-owning record cases unsafe. The caller contract also needs a precise rule for restoring callback-visible host state before replay.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §9 | Shape record undo and image scatter exclude `materialCount`, relying on block entries to restore it. An inline single-material shape has no block entry. If it is destroyed after T and a multi-material shape reuses its slot, `Rewind(T)` can leave the restored inline shape with the replacement’s count. `src/shape.c` initializes inline shapes with count 1; §7.1’s pool-free payload covers a shape’s *block* and its count. | Restore `materialCount` for inline shapes independently of block ownership. Add a slot-reuse test from an inline shape to a multi-material shape. |
| 2 | High | §7.1, §9 | Contact creation can reuse a slot for a mesh contact after a convex contact. The specified reverse walk undoes the creation’s record write before its pool allocation; the old convex bytes share union storage with the new mesh contact’s `triangleCache` in `src/contact.h`. The pool-alloc payload contains no created-contact type or cache handle, so the undo rules do not ensure the new cache is freed before its pointer or type is overwritten. | Define an ownership-safe undo order or put the created cache handle and type in the allocation entry. Test convex-to-mesh and mesh-to-convex slot reuse with allocation counts. |
| 3 | High | §2, §10, §12 | `Rewind(T)` leaves `world->userData` and callback contexts at their later values, while §10 says replaying API calls is sufficient and §12 tests callbacks that depend on `userData`. If a context changes at T+2, replayed step T+1 sees the later context before its original change is replayed. `src/physics_world.c` stores `world->userData` directly. | Require the caller to restore every callback-visible host value to its value at T before the first replayed step, or include those values in the image. State this exception explicitly and test a context change partway through the replay interval. |
| 4 | Medium | §8 | Eviction is described as removing the oldest slot one tick at a time. With capture interval K>1, evicting an image leaves earlier non-image segments before the next image; §8 says those segments cannot be walked, yet they remain charged until separately evicted. This can waste budget and trigger interval widening prematurely. | Evict the unusable prefix through the next retained image as one operation, and specify the behavior when no later image exists. Test `bytesUsed` and `oldestImageTick` during that transition. |

## Checked, no change

- `src/recording.c` hashes body transforms and velocities; `test/test_recording.c` checks backward seeks against those hashes. §1 correctly limits what that test demonstrates.
- The source has six world ID pools plus separate tree proxy free lists (`src/physics_world.h`, `src/dynamic_tree.c`); §7.4 correctly treats proxy identity separately from tree layout.
- Sensor overlap processing swaps and clears its working arrays each step (`src/sensor.c`); retaining `overlaps2` as the cross-step overlap state is consistent with that path.
- The source supports the identified tree-order risks: CCD uses a running fraction and caps sensor hits, and explosions wake and apply impulses during tree traversal (`src/solver.c`, `src/physics_world.c`).
- The document consistently discards later slots on rewind. Its undo entries do not require a forward replay payload or a stale-slot write hook.

## Proposed edits

Resolve findings 1–3 in the state inventory, entry payloads, restore procedure, caller contract, and corresponding §12 tests. Clarify group eviction and accounting in §8. Keep the explicit phase-1 limitation for sleeping dynamic and kinematic proxies.

## Unresolved / disagreements

The performance figures attributed to a review file were not independently checked, in accordance with the instruction not to read `docs/designs/reviews/`. No files were changed and no tests were run.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as rounds 1 and 2. Start 2026-09-29T20:14:24+10:00, end 20:23:24, exit code 124. Attempt 2, resume of session 01a0eca8-8ed4-7e63-b629-b13d512c5e64 with `-m gpt-6-sol -c model_reasoning_effort="high"` before `resume` and a finalize-only prompt, start 20:23:31, end 20:24:09, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. Codex's markdown file links were reduced to bare file names when copied here.

- **#1 (High), applied; a consequence of round 2's and round 1's fix.** `shape.c` gives an inline shape `materialCount` 1 and no block, and a destroy frees a block only `if materials != NULL`. Excluding `materialCount` from record undo left an inline shape's count to the live slot occupant, with no block entry to restore it. The pool free entry now carries the shape's `materialCount` and the contact's `manifoldCount` whether or not a block existed. Test 2 gets the inline-to-multi-material slot reuse.
- **#2 (High), applied.** `contact.h` puts a convex contact's data and a mesh contact's `triangleCache` in one union, `contact.c` only destroys the cache and it is grown in the narrow phase with no journal entry. The reverse walk undoes a creation's record write before its pool alloc, so an older convex record's bytes were copied over a live mesh cache header and the alloc undo, which tests the recorded flags, no longer freed it. `b3CreateContact` now makes a cache entry after its record write, undone first, which frees the live cache and clears the header; the pool alloc undo no longer lists the cache, and the pool free entry carries the header. Test 2 gets convex-to-mesh and mesh-to-convex reuse.
- **#3 (High as rated, doc already covered it; applied as a sharpening).** §10 already puts host-owned state with the caller. Codex is right that it did not say when: a `world->userData` or context change after T is not undone, so a replayed step T+1 sees the later value. §10 now says the caller sets each such value to its value at T before the first replayed step, and test 3 changes a world `userData` and a context partway through the interval. Not a defect in the engine design.
- **#4 (Medium), applied.** With `captureInterval` above 1, evicting an image slot left the non-imaged slots after it retained and charged though no rewind can reach them. §8's old end now evicts them with it.
- **Found while fixing #1, not raised by Codex.** A contact with no block at wake time gets no pre-wake manifold entry (the row said "leaves a block on both sides"), yet the hot path can allocate its block unjournaled; a rewind then left the live block and count in place, since record undo skips them. The wake's pre-wake entry is now made for every contact entering the awake set, count 0 when it has no block, and its undo frees whatever block is live. Test 2 gets a contact with no block woken, touching, then rewound.
- **Unresolved item:** the perf review file was excluded from Codex's scope by the prompt, as in the earlier rounds.

## Status

Finding count is 4, but severity rose to three Highs, all in the undo rules for heap-owning records. Two of them (#1, and the wake gap found alongside it) are consequences of the earlier rounds' own fixes: excluding the count from record undo moved the hazard to cases with no block. That is a signal the block-ownership rules need a whole-picture re-read before the next round, not only per-finding edits; that re-read is the next step, then another fresh full-scope round. No finding reopened stated rationale.

`Round 3: 4 findings (3H-1M-0L), 4 applied, 0 declined`

`Series total: 13 findings (5H-6M-2L) across 3 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L)`
