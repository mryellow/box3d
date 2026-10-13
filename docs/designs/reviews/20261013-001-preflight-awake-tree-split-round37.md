---
title: Review round 37 - preflight awake tree split (confirmation)
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round36.md
mode: confirmation round (U-P9), same prompt and scope, doc unchanged since round 36
---

# Round 37

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:05:43+10:00 to 23:12:05+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1355e-6e64-7e43-8a57-dd19065e540b`.

## Review report (Codex final message)

### 1. Summary

The split accounts for the main pair passes, proxy-key readers, moved-bit transfer, and wake timing in the live source. Two height-maintenance gaps remain: the proposed intermediate commit does not reliably rebuild the non-awake tree, and the final design can wait until after a large migration batch to rebuild an unchecked tree.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, B0–B3 | The claim that the intermediate commit rebuilds the non-awake tree “on order” is inconsistent with B1’s early return. B1 tests that tree’s moved root, but not `dfsOrdered`; after pair processing clears moved bits, `b3UpdateBroadPhasePairs` can return without enqueueing `b3UpdateTreesTask`. Sleep migrations also drain after the current task has joined. Thus this commit does not provide the stated maintenance guarantee. | Include the non-awake rebuild need in the interim early-return condition and specify when a post-drain tree is maintained, or land height maintenance in the migration commit. |
| 2 | H | §2, B4 | The height test is deferred until **after** the whole step-exit migration drain. That batch has no stated height bound, and rotations do not provide one. `b3DynamicTree_Rebuild` walks the **old** tree before building a new one; its gather stack has an assertion but no release guard. A sufficiently deep, branching old tree can overflow that stack before the proposed rebuild repairs its height. The statement that the full build bounds its own walks is therefore false. | Make the rebuild’s old-tree walk safe for arbitrary height, or maintain a safe height during the drain before calling the fixed-stack rebuild. |

### 3. Checked, no change

- The existing dynamic self pass, static and kinematic cross passes, compound expansion, sorted pair keys, and sleeping-contact placement support B1’s pass plan.
- The cited proxy creators, destroyers, movers, marks, fixed-index readers, CCD queries, sensor queries, bounds, counters, and validation paths are accounted for by B0–B3.
- Carrying a leaf’s moved bit preserves pending pair discovery; wake and sleep currently leave proxies untouched.
- The collide wake occurs while the tree task may run; the solver’s join precedes finalize and bullet CCD. Explode also wakes during its tree query.
- The churn table’s shape-move arithmetic is consistent with its stated endpoint-scan method and the cited benchmark scene behavior.

### 4. Proposed edits

Revise the commit guarantee and B1 early-return rule to match the actual rebuild schedule. In B4, state that the rebuild first traverses the unchecked old tree and specify a safe way to handle a batched drain that exceeds the fixed-stack capacity.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** The interim line in `b3UpdateTreesTask` runs only when the pair update
   enqueues the task, and B1's early return tests the non-awake root's moved bit, not its order;
   the exit drain's sleep inserts land after the task joined (`physics_world.c`), so the next
   step could return early with that tree unordered. Fix: until B4's commit the early return
   also tests `b3NeedsRebuild` on the non-awake tree, two lines B4 removes together.
2. **Adopted (H).** Revision 30 ran the exit test after the drain with the depth still raised,
   so the drain's inserts, a whole sleep pass's worth, went untested until one test at the end,
   and `b3DynamicTree_Rebuild` walks the old tree with an unguarded stack. Fix: the outermost
   exit lowers the depth to zero and then drains, so each drain insert is tested like any other,
   with the tree task joined and no walk open; the mid-step drain inserts into the awake tree
   only (a wake removes from the non-awake tree), so its skipped test has nothing to test; one
   more test after the drain covers a write made under the walk. The old tree a rebuild walks is
   then at most one over the threshold on a layout the build bounds.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round found defects, so round 36's clean round
  does not stand (U-P9); a fresh clean round and confirmation are needed (U-S8).
- Round 37: 2 findings (H1-M1-L0), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0
- Tripwire: fixes add or remove a named mechanism: no; an ordering at the exit and an interim
  test. Clustering: B4 (finding 2, consequence of revision 30's exit test placement) and the PR
  plan (finding 1, consequence of revision 34's interim line). Third confirmation round to find
  what a clean round missed (28, 34, 37); every clean round so far has been followed by findings
  on the previous revision's own additions (U-T2). Reviewer read of round records or
  changelog: none in the raw log. Doc length: 391 lines, unchanged this round.
