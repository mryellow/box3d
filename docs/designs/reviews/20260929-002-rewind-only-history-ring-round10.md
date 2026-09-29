---
title: Codex review — rewind-only history ring, round 10
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Attempt 1 completed, no resume.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 10

## Review report (Codex final message)

## Summary

The rewind-only approach is coherent, and the proposed transition entries cover the wake and sleep paths I traced. The design still needs several rules made precise before its completeness and cost claims are reviewable. I read only the permitted files and made no changes.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2, §7.4 | The compile-time completeness claim does not yet cover proxy identity. World trees use the publicly defined b3DynamicTree, and its create/destroy functions remain callable directly; journaling only in broad-phase wrappers relies on callers continuing to use those wrappers. | Make world-owned trees inaccessible to direct proxy mutation, or put the journal boundary in a tree type that distinguishes world trees from compound child trees. |
| 2 | Medium | §8 | Doubling the capture interval does not specify whether the image currently being captured is kept, skipped, or rescheduled. Existing images may also cease to follow “every K-th tick.” This leaves eviction and the next restorable tick ambiguous. | Define an image schedule and eviction algorithm for each interval change, including the triggering tick and rewind after widening. |
| 3 | Medium | §13, phase 1 | “Image the trees raw” and “replace each tree wholesale” omit heap ownership. A tree has several owned arrays and rebuild buffers; the existing tree serializer copies selected contents and reconstructs allocations explicitly. | Specify the exact phase-1 tree image fields, deep-copy/restore procedure, scratch-buffer treatment, and cleanup before calling the fallback complete. |
| 4 | Low | §12, test 6 | The stated ownership invariant is false between steps: the design says pending destroy entries own blocks in staging, while the test allows ownership only in the world or a retained slot. | Include staging entries in the invariant and check their ownership and cleanup separately. |
| 5 | Low | §4, §9, §14 | Restore cost counts journal bytes but omits tree work done while undoing proxy create/destroy entries. Each such entry can require a tree insert or removal. | Include proxy-operation tree costs in the restore bound and cost test. |
| 6 | Low | §14 | The public API does not define behavior when history is inactive or `EnableHistory` is called again. `GetRestorableTick` also needs an explicit rule for a requested tick later than `currentTick`. | State lifecycle preconditions or results for each API call and define the future-tick lookup result. |

## Checked, no change

- The existing recording hash covers transforms and velocities only; ScrubBackward supports the document’s limited claim about it.
- The proposed pre-wake entries, cold array journals, and target image address the wake, sleep, transfer, and repeated-transition cases traced in solver_set.c.
- Contact and shape destruction currently free owned blocks; the design correctly identifies paths that must transfer those blocks to undo entries, including contact manifolds and mesh caches and sensor arrays.
- The CCD sensor cap and traversal dependence, pair-key sorting, and explosion wake/impulse order are supported by solver.c, broad_phase.c, and physics_world.c.
- Imaging `overlaps2` while treating `hits` and `overlaps1` as scratch is consistent with the sensor pass: it swaps the overlap arrays, clears the destination, and consumes hits before step end.

## Proposed edits

Resolve findings 1–3 in the design’s procedures and phase gates. Align §12’s staging invariant with §8, then tighten the restore cost statement and API contract.

## Unresolved / disagreements

The §11.1 performance measurements cite a file under the excluded reviews directory, so I did not independently verify those numbers. This was a design review; I did not execute the proposed verification plan.

## Verdict

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, a fresh full-scope prompt naming only the target doc and the source tree, telling Codex not to read `docs/designs/reviews/` or other design docs, with no curated decline list. Start 2026-09-30T05:38:50+10:00, end 05:46:31, exit code 0. `git status` after the run shows the same files as before it, so Codex did not touch the working tree. The target doc had a diff (rounds 7 to 9's fixes, uncommitted) that was read in full and was not mangled. Before the round Claude re-read the whole doc and applied AC-1 to AC-4: §5.2's caller table is a migration worklist behind compile-time choke points, not a completeness claim, and finding #1 is the one place a choke point rested on a caller list. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** `world->broadPhase.trees` is a by-value array of the public `b3DynamicTree`, and `sensor.c`, `solver.c` and `physics_world.c` all take writable pointers to it (`solver.c` calls `b3DynamicTree_EnlargeProxy` and `b3DynamicTree_Refit` on it); `b3DynamicTree_CreateProxy`/`DestroyProxy` are public API taking any tree. §7.4's "already the only callers" was a call-site list (AC-2, requirement 7). It now says the world's trees are pointer-to-const outside `broad_phase.c` and every writer is a function in that file, and §5.2's table has a row for it.
- **#2 (Medium), applied.** §8 doubled the interval without saying when a tick is imaged after that, what happens to the triggering tick, or what a rewind does to the interval. §8 now defines the image schedule relative to the last imaged tick, the triggering tick's treatment, that retained slots are untouched, and that the interval is neither narrowed nor reset by a rewind. Restorable ticks were already the retained imaged ones, so `GetRestorableTick` and eviction needed no change.
- **#3 (Medium), applied.** `b3DynamicTree` owns `nodes`, `parents`, `proxies`, `swapNodes` and five rebuild buffers. The serializer's `b3SerTree`/`b3DesTree` (`world_snapshot.c`) write the scalars, `nodes` and `parents` up to `nodeEnd`, and `proxies` up to `proxyCapacity`, and free the spare array and rebuild buffers on read. The rebuild (`dynamic_tree.c`) allocates `swapNodes` when NULL and regrows the buffers when `proxyCount` exceeds `rebuildCapacity`, so they carry no state. §13 phase 1 now names the image fields and the restore's free and allocate procedure.
- **#4 (Low), applied.** §8 charges blocks owned by staging-buffer entries when the segment closes, and §8's disable frees them, so §12 test 6's invariant omitted an owner. It now names the staging buffer.
- **#5 (Low), applied.** The undo of a proxy destroy is a tree insert and the undo of a proxy create is a tree removal, each O(log n), while §4 and §14 counted journal bytes only. §4's bound and §14's Rewind comment now include proxy entries.
- **#6 (Low), applied in part.** `EnableHistory` twice, and `Rewind`, `GetRestorableTick`, `GetHistoryInfo` and `DisableHistory` with history off, were unspecified; §14 now states each (assert on a second enable, `b3_historyTickUnavailable`, `UINT64_MAX`, zeros, no-op). The future-tick lookup was already defined by "newest imaged tick <= tick" and returns the newest imaged tick; §14's comment now says so, with no rule added.

## Status

Six findings, three Medium and three Low, up from two in round 9 (2, 2, 1, 2, then 6). A fresh reviewer given no prior context found new categories: a compile-time choke point resting on a caller list (#1, the AC-2 shape), an undefined image schedule after widening (#2), the ownership of a raw tree image (#3), and API lifecycle (#6). None was a consequence of round 9's fix, and none reopened rationale the doc already stated. The rise in count is a fresh full-scope pass reaching parts of the doc (§8's interval, §13's phase 1 fallback, §14's lifecycle) that rounds 7 to 9 did not raise, not a regression in the earlier fixes. A clean round has not occurred. Next: another fresh full-scope round 11, same brief.

`Round 10: 6 findings (0H-3M-3L), 6 applied, 0 declined`

`Series total: 30 findings (7H-16M-7L) across 10 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L)`
