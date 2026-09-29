---
title: Codex review — rewind-only history ring, round 12
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 12

## Review report (Codex final message)

## Summary

The rewind and undo approach is broadly consistent with the source. Five design issues need edits before the cost and restore guarantees are ready to implement. This was a read-only review; I did not run tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §7.4, §12 | Restoring shapes can make a dynamic or kinematic tree unordered. The next step’s tree rebuild can copy retained subtrees, including sleeping proxies. The claimed replay cost and cost test do not account for this first step after rewind. | Measure that step in the million-sleeping-body case. Specify how phase 2 avoids the rebuild cost, or include it in the cost contract. |
| 2 | Medium | §7.1, §9 | The material-block entry covers a resize or replacement, but its undo does not explicitly restore the old `materialCount`; record undo and image scatter explicitly skip that field. A restored block with the newer count would be inconsistent. Current shape setters write elements, so this concerns the resize/replacement case the design itself specifies. | Store and restore the old count in that entry, or remove resize/replacement from its contract. |
| 3 | Medium | §8, §14 | Destroyed blocks held by the pending journal can persist between steps, yet are excluded from `bytesUsed`; `stagingCapacityBytes` reports only buffer capacity. Actual history-owned memory can therefore exceed the reported budget without visibility. | Report pending owned bytes separately and state how the budget treats them. |
| 4 | Medium | §5.2, §7.1 | The opaque-container plan says pointers are allocated at world creation, while sleeping sets and islands—and their nested arrays—are created later in body.c, solver_set.c, and island.c. Their wrapper allocation and lifetime across destroy, undo, and eviction are unspecified. | Define storage and ownership for dynamically created wrappers, including their allocation cost. |
| 5 | Low | §8, §14 | `captureInterval` has no stated upper bound relative to `tickCount`. If the caller supplies `captureInterval > tickCount`, tick 0 can remain the only image beyond the documented window limit. | Clamp or reject that combination, or define the longer window explicitly. |

## Checked, no change

- The existing serializer and backward-seek test support the document’s limited baseline claim; the existing state hash covers poses and velocities, not full world state.
- The sleep and wake paths support treating sleeping-set arrays as cold and show why entering the awake set needs pre-wake coverage.
- Sensor processing swaps overlap arrays each step and sorts and deduplicates current overlaps, consistent with imaging `overlaps2`.
- The proxy free list and broad-phase wrappers support journaling proxy identity separately from tree layout.
- The identified CCD and explosion traversal order dependencies are present in source.

## Proposed edits

Resolve findings 1–4 in the restore, journal, budget, and container sections; align the API window definition with finding 5. Add a first-replayed-step measurement to §12’s cost test.

## Unresolved / disagreements

The performance figures attributed to an excluded review file were not independently verified. I found no disagreement with the recommendation to make history rewind-only.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, a fresh full-scope prompt naming only the target doc and the source tree, forbidding reads of `docs/designs/reviews/`, with no curated decline list. Session 01a0eec8-06cd-76e1-b4de-85231e039086. Start 2026-09-30T06:08:00+10:00, end 06:15:25, exit code 0, no resume needed. `git status` after the run shows no change Codex could have made. Before the run the target doc's uncommitted diff (rounds 7 to 11's fixes) was read in full and was not mangled, and the mechanism sections were re-read against their sibling cases; no gap was found. AC-1 to AC-4 were applied before sending: §5.2's caller table is a worklist the compiler supersedes, the awake-set entry has one function per record kind, and no check in the doc is control flow. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** `b3DynamicTree_MoveProxy` removes and reinserts a leaf, which clears `dfsOrdered` when the result is out of DFS order (`dynamic_tree.c`), and `b3DynamicTree_Rebuild` and `b3NeedsRebuild` run on `dfsOrdered == false` even when nothing moved (`dynamic_tree.c`, `dynamic_tree.h`, `broad_phase.c`), so the first step after a restore pass that moved a proxy pays a rebuild of that tree even in a scene whose original steps did not. Kept subtrees are build leaves but the rebuild still writes the whole node array, so it is O(nodes). §5.4 already says the rebuild is the engine's own per-step cost while proxies move; §7.4 now says the pass leaves the tree unordered and the rebuild belongs to the first replayed step, §13 phase 2 says it removes the O(proxies) term from capture and `Rewind` only, and §12 test 5 reports that step separately on the sleeping dynamic and kinematic scene. Avoiding the rebuild would need a layout-preserving restore, which §7.4 rejects by making layout derived; not pursued.
- **#2 (Medium), applied, with the premise narrowed.** No source path resizes or replaces a shape's `materials` array after creation (`shape.c` allocates at create and frees at destroy; the setters write elements), so the resize case is not reachable today. The doc nonetheless specifies it, and states that counts are bound only by block entries (§7.1), so the entry must carry the count. The material block row now records the old `materialCount` and its undo sets it; the manifold row already carries a count.
- **#3 (Medium), applied as rationale only.** §8 already said blocks owned by staging-buffer entries are charged when the segment closes and are not in `bytesUsed` before then. The gap was that it did not say why that is acceptable. It now says they are one tick's own destroys, held until the next step closes the segment or a rewind undoes them, so the amount beyond the reported figure is bounded by one segment, like the staging capacity. No new `b3HistoryInfo` field: the caller cannot act on a figure that lives for one step boundary.
- **#4 (Medium), applied.** `b3SolverSet` and `b3Island` hold their arrays by value as `b3Array` (`solver_set.h`, `island.h`) and are created at run time, so §5.2's "incomplete type held by a pointer allocated at world creation" cannot apply to them. §5.2 now limits that rule to world-level containers and says record-held arrays are const-data array fields (the type the owned-blocks rule already uses for island and sensor arrays), addressed by the (tag, owner id) pair the array push and removeswap entries already record, so no wrapper exists to allocate, destroy or evict.
- **#5 (Low), declined.** §8 already defines the case: eviction runs "while the ticks from the oldest retained imaged tick to `historyTick` exceed `tickCount` and a later imaged tick remains", so tick 0 stays until the next image, and closes with "an unbounded byte budget still retains only `tickCount` ticks plus the interval to the next image". §14's `tickCount` comment says the same, "a wider capture interval can retain up to one interval more". A `captureInterval` above `tickCount` therefore has a defined window, tickCount plus that interval, and clamping it would remove a legitimate configuration.

## Status

Five findings, four Medium and one Low, against 5 the round before, with no High since round 8. Categories: a cost claim that missed the first replayed step's rebuild (#1), a journal entry that omitted a field it restores (#2), a reporting gap that was a missing rationale (#3), a container-storage rule that did not cover run-time-created records (#4) and a declined configuration question the doc already answers (#5). None was a consequence of round 11's fixes, and none reopened stated rationale. Findings are still new-category rather than repeat edge cases, and a clean round has not occurred. Next: another fresh full-scope round 13, same brief.

`Round 12: 5 findings (0H-4M-1L), 4 applied, 1 declined`

`Series total: 40 findings (7H-24M-9L) across 12 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L)`
