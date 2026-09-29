---
title: Codex review — rewind-only history ring, round 2
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Killed by the shell timeout, then resumed to finalize.
session: 01a0ec9c-e14d-7033-af69-6bf2f2a42645
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 2

## Review report (Codex final message)

## Summary

I reviewed §§1–16 against the cited source. The rewind-only undo rules appear coherent across creation, destruction, sleep, and wake, but the document needs clearer lifecycle and memory rules before implementation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §8, §14 | History cleanup is specified for `DisableHistory`, but not for `b3DestroyWorld`. The current destructor tears down shapes, the hull database, and manifold allocators (src/physics_world.c); retained journal entries may still own blocks that depend on them. | Require internal history cleanup before world resource teardown, including pending entries, and test destruction with history enabled. |
| 2 | Medium | §8 | Once an older image is evicted, journal segments before the next retained image—and the segment attached to that oldest image—cannot be used by `Rewind`, yet §8 still retains and charges their entries. A destroyed sleeping set could remain owned by an unreachable entry (§7.1). | When `oldestImageTick` advances, release all unreachable entries and their blocks, and subtract their charges. |
| 3 | Medium | §8, §14 | The rule doubles `captureInterval` when a minimum window exceeds `maxBytes`, even when the excess is a single slot or journal bytes that widening cannot reduce. Repeated widening can eventually overflow the API’s `int effectiveInterval`. | Define a maximum interval and stop widening when it cannot lower retained bytes; report the continuing overrun. |
| 4 | Medium | §8, §12.6, §14 | `bytesUsed` omits allocator capacity retained after journal blocks are freed. In particular, src/block_allocator.c keeps manifold backing blocks until allocator destruction, so the stated allocation baseline after `DisableHistory` need not return to its earlier value. | Report retained allocator capacity separately; test that journal ownership returns to zero rather than requiring all world allocator capacity to shrink. |
| 5 | Low | §12.2 | The proposed test resizes a live shape’s material array, but src/shape.c exposes material element setters and no resize operation. | Replace it with a supported destroy/recreate case using different material counts, or specify the new resize API the test requires. |

## Checked, no change

- The existing recording hash covers transforms and velocities only; §1 correctly limits what its backward-seek test demonstrates (src/recording.c, test/test_recording.c).
- Sensor processing visits sensors regardless of body sleep state, supporting the per-sensor image cost in §5 (src/sensor.c).
- Proxy identity needs undo entries: the trees allocate IDs through their own free lists (src/dynamic_tree.c).
- A proxy’s category bits can differ from its shape’s current filter, but §7.1’s proxy-destroy payload records those bits; this is covered by undo (src/shape.c, src/dynamic_tree.c).
- The document consistently removes forward replay: later slots are discarded on rewind, and the listed block transfers have undo or eviction paths.
- The stated CCD sensor-hit cap and explosion traversal-order risks match src/solver.c and src/physics_world.c.

## Proposed edits

Add a world-destruction cleanup rule and test; define how the ring frees journal entries that become unreachable; bound interval widening; distinguish logical history bytes from retained allocator capacity; and replace the unsupported material-resize test.

## Unresolved / disagreements

The cost targets remain estimates pending §12’s measurements. I found no separate disagreement with the rewind-only recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 1. Start 2026-09-29T20:01:37+10:00, end 2026-09-29T20:10:37+10:00, exit code 124 (killed by the shell timeout). Attempt 2, resume of session 01a0ec9c-e14d-7033-af69-6bf2f2a42645 with `-m gpt-6-sol -c model_reasoning_effort="high"` before `resume` and a finalize-only prompt, start 20:10:48, end 20:11:11, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. Codex's markdown file links were reduced to bare file names when copied here.

- **#1 (High), applied.** `b3DestroyWorld` (`physics_world.c`) destroys shapes, the hull database and the block allocators with no history step, so entries still owning manifold blocks or hulls would be freed after what they were allocated from is gone, or leak. §8 now has world destruction do `DisableHistory`'s cleanup first, and test 6 covers destruction with history enabled.
- **#2 (Medium), declined; rationale sharpened.** The oldest retained slot's own segment is never read, but slots are evicted whole and the doc's fixed-charge property (§7.1, §11.3: a charge never moves while its slot is retained) is the reason. The retained dead weight is one slot's segment. §8 now says so, so a fresh reviewer does not read the retention as an oversight.
- **#3 (Medium), applied.** §8 doubled the interval on every over-budget capture with no bound, and the doc itself says widening cannot help a single oversized slot, so the interval could double without limit and overflow `effectiveInterval`. The interval now stops at `tickCount`, beyond which a wider interval retains no fewer image bytes.
- **#4 (Low as it applies, declined; test wording sharpened).** `block_allocator.h` keeps backing blocks until the allocator is destroyed, but its `allocationCount` is a live-element count that `b3FreeElement` decrements, so "allocator counts return to baseline" is sound; §12 test 6 now says live allocation counts, not retained capacity. `bytesUsed` reporting history and not world allocator capacity is the doc's stated meaning (§8, §14).
- **#5 (Low), applied; a consequence of round 1's fix.** `shape.c` sets `materials` and `materialCount` only in create and destroy, and only element setters exist after; there is no resize. The test case round 1 added ("a material array resized") described an operation that does not exist. It is replaced by destroying a multi-material shape and creating one with a different count in its slot, which is what exercises the count binding. §7.1's material block row still names a resize or replacement as a write kind; no such write exists in the source today, and the row is left as a description of what the entry covers if one is added.

## Status

Finding count is flat versus round 1 (4 to 5), and every finding is a new category (world-destruction lifecycle, unbounded interval widening, a test naming a nonexistent operation). One finding, #5, was a consequence of round 1's own fix. Two findings (#2, #4) touched ground the doc had not yet stated rather than ground it stated wrongly, and neither engaged an existing rationale. Next: another fresh full-scope round against the edited doc.

`Round 2: 5 findings (1H-3M-1L), 3 applied, 2 declined`

`Series total: 9 findings (2H-5M-2L) across 2 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L)`
