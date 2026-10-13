---
title: Review round 44 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round43.md
mode: full pass
---

# Round 44

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:46:01+10:00 to 23:53:07+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13583-53cf-7b11-92d6-01bd27af6bc6`.

## Review report (Codex final message)

### 1. Summary

The split accounts for the fixed-index tree readers, proxy-key routing, moved-bit transfer, and the async rebuild and callback hazards I traced. Three issues remain: wake migrations can leave the awake tree tall before pair discovery, the query cost bound is too strong, and one height-test instruction contradicts B4’s hook placement.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | 2, B2–B4 | A large wake inserts proxies into the awake tree before the next pair gather. Pair discovery runs **before** the step’s rebuild, while insertion rotations provide no height bound. The claim that the awake tree stays near `log2` height therefore does not cover this new wake path; the fixed-stack pair walks can become costly or skip branches if the tree grows deep (`src/broad_phase.c`, `src/dynamic_tree.c`). | Check awake-tree height after a migration drain, or before pair gathering, and rebuild synchronously at a safe point when needed. Preserve pending moved bits. |
| 2 | L | 1 | Tree queries are described as O(log of tree plus hits). AABB overlap can visit many internal nodes even with few leaf hits; `b3DynamicTree_Query` has no such bound (`src/dynamic_tree.c`). | State query cost in terms of node visits, as section 4 already proposes measuring. |
| 3 | L | 3; 2, B4 | The tree-level test deliberately uses `b3DynamicTree_MoveProxy` until height exceeds the threshold, then the test instructions say to assert height is below the threshold after every write. B4 places the check in broad-phase hooks, which direct tree moves bypass. | Apply the post-write height assertion to the world-level path; use the tree-level path to establish the tall input. |

### 3. Checked, no change

- The fourth proxy code fits the existing two-bit key; the body-type enum remains three-valued.
- B1 covers moved sleeping proxies against awake, sleeping, static, and kinematic proxies, including static compound expansion.
- The cited fixed-index readers, sensor queries, bounds, debug draw, and bullet and explosion queries have corresponding split-tree changes.
- Deferring migrations through tree-task joins and host-callback traversals addresses the new wake-time tree writes. The churn table’s displayed totals and means are internally consistent with its stated endpoint-scan method.

### 4. Proposed edits

Add a pre-pair awake-height safeguard to B4 and its timing to B3. Replace the query complexity claim in section 1, and scope section 3’s height assertion to broad-phase writes.

### 5. Unresolved

Whether a full rebuild can bring an adversarial post-wake awake tree below the chosen height trigger needs the same explicit caveat B4 gives the non-awake tree.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted in part (M).** A wake's inserts into the awake tree are creates with rotations
   (B2, `b3CreateTreeProxyInternal`), and the pair update gathers before it enqueues the rebuild,
   so the first walk after a large wake sees a tree of rotated inserts on top of the last
   build. That is the reliance today's dynamic tree has for a spawn of as many bodies between
   steps, through the same insert, and the step's next rebuild resets it; the builder's caveat
   B4 gives the non-awake tree applies to it as to today's tree. Fix: B4 states the awake
   height as the last build's plus one step's rotated inserts, with that comparison. No awake
   height check is added (U-P11).
2. **Adopted (L).** `b3DynamicTree_Query` visits every internal node whose box overlaps; the
   cost is node visits, which section 4 measures. Fix: section 1 says so.
3. **Adopted (L).** The tree-level path bypasses the hooks. Fix: section 3's height assertion is
   on the world-level path; the tree-level path establishes the tall input.
   The Unresolved item is answered by fix 1.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 44: 3 findings (H0-M1-L2), 2 applied, 1 applied in part, 0 declined outright.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0 -> 0/0/0 -> 0/1/1 -> 0/0/0 -> 0/0/0 -> (gate fix, revision 38) -> 0/1/0 -> 0/0/1 -> 0/1/2
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B4 (finding 1, consequence
  of revision 28's "near log2" sentence), §1 (finding 2, consequence of revision 37's query
  clause), §3 (finding 3, consequence of revision 27's test). Reviewer read of round records
  or changelog: none in the raw log. Doc length: 406 -> 408 lines at revision 40.
