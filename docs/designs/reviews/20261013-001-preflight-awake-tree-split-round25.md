---
title: Review round 25 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round24.md
mode: full pass
---

# Round 25

Series resumed by the owner after revision 26 (owner decision: the non-awake tree is rebuilt by
the step on a height trigger, not never).

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:48:15+10:00 to 21:54:17+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13517-84cc-7163-83a7-3279184433dc`.

## Review report (Codex final message)

### 1. Summary

The proposed pair passes and proxy-key routing cover the cited source paths. The non-awake tree’s height policy has two defects: it leaves public queries exposed to stack overflow between steps, and a full rebuild does not guarantee the stated height limit.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B4 | The height check runs only at the next pair update. Repeated writes to sleeping proxies can raise the non-awake tree above the fixed traversal stack limit between steps; public queries can then silently skip branches in release builds (`src/dynamic_tree.c`). Calling them “single-tree walks” does not make them safe. | Bound the tree before public walks, or give those walks a stack that cannot drop branches. |
| 2 | M | §2 B4 | A full rebuild does not guarantee height at or below 64 or “near log2.” The current builder splits at the midpoint of the centers’ extent, which can repeatedly isolate one geometrically spaced leaf (`src/dynamic_tree.c`). Such a rebuild can leave the trigger active, causing a full rebuild again on every step. | Specify and enforce a post-rebuild height bound, or revise the trigger and cost claim for persistent over-threshold trees. |

### 3. Checked, no change

- The fourth proxy code fits the existing two-bit key; the cited fixed-index readers and body-type assertions need the B0/B1 updates.
- The proposed self and cross passes cover awake, sleeping, kinematic, and static pairs, including static compound expansion.
- Carrying moved bits through sleep and wake preserves pending pair searches; the cited proxy writers route through the shape key.
- The queued migration point follows the tree-task join and precedes CCD. The cited sensor and bullet readers are accounted for.
- The churn table’s means are consistent with its stated body and shape counts; the endpoint-scan argument matches the cited wake timer and sleep-pass order.

### 4. Proposed edits

Revise B4 to state how public queries remain complete after writes between steps. Replace the claimed post-rebuild height guarantee with an enforceable bound or an explicit repeated-trigger case, and make §3’s height assertion match that policy.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** Revision 26 tested the height at the start of the pair update only. Between
   steps, `b3Body_SetTransform`, `b3ResetProxy`, enable and shape create on sleeping bodies insert
   into the non-awake tree with no test, and the public queries (`b3World_OverlapAABB`,
   `b3World_CastRay` and the rest, `physics_world.c`) walk it with the 512-entry stack that skips
   a branch when full. Each unrotated insert raises the height by at most one (`b3InsertLeaf`
   puts the new leaf beside its sibling under a new parent at the sibling's depth), so passing
   the single-walk fill from a bounded tree needs over 430 such inserts between two steps on one
   path, but the gap is a gap. Fix: the test moves to the write. `b3BroadPhase_CreateProxy` and
   `b3BroadPhase_MoveProxy` test the non-awake root height after each insert into that tree and
   rebuild in full when it is exceeded; inserts happen only while no traversal is open (B3), so
   every walk, step or public, sees a bounded tree. The pair-update test goes.
2. **Adopted (M).** `B3_TREE_HEURISTIC` is 0: `b3PartitionMid` splits the centers' extent at its
   midpoint on the longest axis (`dynamic_tree.c`). A split isolates one leaf when that leaf's gap
   exceeds the span of the rest, so a full build bounds nothing by itself and a layout it leaves
   above the threshold would rebuild on every insert into the tree. Fix: B4 states the builder's
   rule, the separation ratio a chain of isolating splits needs, and the repeated-trigger case
   with its cost; §3's assertion is on a layout the builder bounds. A layout-relative trigger
   (fire above the last build's height plus a margin, one scalar of tree state) was considered
   and not added; the reason is in Provenance.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 25: 2 findings (H1-M1-L0), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0
- Tripwire: fixes add or remove a named mechanism: no; the height test moves from the pair update
  to the insert. Clustering: B4 again (rounds 15, 18, 20, 22, 24, 25); both findings are
  consequences of revision 26's new trigger, the first round on it, not re-raises of the stack
  reliance. U-S4's third-round-running condition is met on B4; the owner directed review to
  convergence on the revised mechanism, so the series continues (U-S5). Reviewer read of round
  records or changelog: none in the raw log. Doc length: 321 -> 345 lines at revision 26.
