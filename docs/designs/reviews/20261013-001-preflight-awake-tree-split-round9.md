---
title: Review round 9 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round8.md
mode: full pass
---

# Round 9

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope; the one-line description reverted with the
doc to "splitting the dynamic-body broad-phase tree into awake and non-awake trees").

Run: foreground, `timeout 540`, exit 0, 2026-10-13T19:25:21+10:00 to 19:31:54+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split covers the main tree readers, proxy-key paths, and rebuild race, but it has one pair-discovery edge case and one missing route for the promised explicit rebuild. I found no separate rewind restoration gap in the two treatments described in section 5.2.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | 2, B2 | After a sleep leaves **one** proxy in the awake tree, that proxy is the root leaf. Marking the root moved to trigger compaction marks the surviving proxy moved, contradicting “a clear bit stays clear.” The next pair update searches its overlaps and can call a custom filter or create a pair that sleep alone would not have prompted (`src/dynamic_tree.c`, `src/broad_phase.c`). | Specify a compaction trigger that does not change a leaf’s moved bit; handle the singleton case directly or track maintenance separately. |
| 2 | L | 2, B4 | B4 promises an explicit non-awake rebuild through `b3World_RebuildStaticTree` or a sibling, but its file list cannot provide either route: the existing function in `src/physics_world.c` rebuilds only the static tree. It is also absent from `include/box3d/`. | Add the entry-point work to B4’s scope and state whether the rebuild is internal or exposed in the public API. |

## 3. Checked, no change

- The fourth proxy code fits the existing two-bit key; the cited fixed-index readers and tree-count loops need the extensions identified in B0, B1, and B3.
- The proposed self and cross passes cover moved sleeping proxies against sleeping, awake, static, and kinematic proxies (`src/broad_phase.c`).
- The pending rebuild can race with a contact-triggered wake, and the described join or deferral addresses that race (`src/physics_world.c`, `src/solver.c`, `src/island.c`).
- The awake-tree image sizes and high-water explanation are consistent with the tree’s node, parent, and proxy sizes (`src/dynamic_tree.c`).
- Reverse proxy-identity undo plus the restored moved-proxy set supports the layout-independent non-awake restore; the physical journal covers the layout-sensitive branch (rewind design §§5.4, 7.1, 7.4).

## 4. Proposed edits

Revise B2’s root-marking rule and its claim that migration preserves clear moved bits. Extend B4’s file scope and name the explicit rebuild entry point.

## 5. Unresolved

None.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** With one proxy left the root is that proxy's leaf, and its moved bit is the
   bit `b3TestPair` and the pair passes read. `b3DynamicTree_Rebuild` also runs when `dfsOrdered`
   is false with an unmoved root, taking the whole retained tree as one build leaf and copying it
   densely (`dynamic_tree.c`); `b3NeedsRebuild` includes that flag, so the pair update's early
   return lets it through. Fix: B2 clears `dfsOrdered` after a sleep migration instead of marking
   the root, which touches no leaf's moved bit; 5.1's compaction sentence follows.
2. **Adopted (L).** `b3World_RebuildStaticTree` (`physics_world.c`, `box3d.h`) rebuilds the static
   tree only, and B4 listed neither file. Fix: B4 adds a public `b3World_RebuildSleepingTree`
   beside it, in those files.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 9: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: the compaction trigger changes from a moved mark
  to the ordered flag; one public entry point named. Clustering: finding 1 is a consequence of
  round 7's fix (U-T2), the fourth round in a row on the awake tree's compaction; the fix this
  time removes the rule's side effect rather than adding to it. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 354 -> 357 lines (revision 9 reverted the five-tree
  text and added round 8's two fixes).
