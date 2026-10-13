---
title: Review round 28 - preflight awake tree split (confirmation)
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round27.md
mode: confirmation round (U-P9), same prompt and scope, doc unchanged since round 27
---

# Round 28

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:08:48+10:00 to 22:13:16+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1352a-5459-7a01-9e2f-f3162b00846e`.

## Review report (Codex final message)

### 1. Summary

The split’s pair passes, moved-bit handling, and rebuild timing are broadly consistent with the live source. Two design defects remain: the stated B2 file scope cannot implement its shape-creation rule, and the behavior guarantee is broader than the document’s own query caveat.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2 B2 | B2 lists only `solver_set.c`, but it also requires changing sleeping-body shape creation in `shape.c` and the moved-mark parameter in `broad_phase.c` and `broad_phase.h`. As specified, a newly created shape on a sleeping dynamic body still receives the dynamic tree code. Moving that routing into B0 would put such shapes in a tree B0 does not yet search. | Add the required files and routing to B2’s commit scope; state when the proxy API change lands. |
| 2 | L | §1; §2 B1 | The statement that results do not depend on proxy tree membership when the tree-order changes land is unqualified. B1 says public query callback order and tied closest-hit casts remain layout-dependent, consistent with the cited design’s §4. | Limit the guarantee to step and explosion results, and retain the public-query caveat alongside it. |

### 3. Checked, no change

- The proposed self and cross passes cover moved sleeping proxies against awake, sleeping, static, and kinematic proxies; the static compound pass needs the stated expansion.
- Carrying leaf moved bits through sleep and wake preserves pending pair searches. The stated join point precedes finalize’s CCD work.
- Fixed-index readers, sensor queries, bullet queries, proxy-key lookups, and the awake-only bullet enlargement path were checked against the cited source.
- The rebuild, refit, height-trigger rationale, and benchmark scene descriptions are consistent with the cited implementations. I found no demonstrated error in the churn harness method or table.

### 4. Proposed edits

Revise B2’s file list and commit boundary to include sleeping-body shape routing and the proxy API change. Qualify §1’s result guarantee to step and explosion behavior.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** B2's Files line names `solver_set.c` only, while its text changes
   `b3BroadPhase_CreateProxy`'s signature (`broad_phase.c`, `broad_phase.h`) and shape creation's
   tree selection (`shape.c`). B0 to B3 are one PR, so the gap is the commit's stated scope, not
   the PR's. Fix: B2 lists the four files and states that the parameter and the routing land in
   its commit, after B1's passes search the non-awake tree.
2. **Adopted (L).** Section 1's guarantee named the tree-order preflight's `preSolve` and filter
   exception only; B1 names public query callback order and tied closest-hit casts as well. Fix:
   section 1 limits the guarantee to the step and Explode and carries B1's caveat.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round found defects, so round 27's clean round
  does not stand (U-P9); a fresh clean round and confirmation are needed (U-S8).
- Round 28: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1
- Tripwire: fixes add or remove a named mechanism: no. Clustering: neither finding is in B4;
  neither is a consequence of a previous fix (B2's file line dates from revision 1; the §1
  guarantee from revision 20's split). Reviewer read of round records or changelog: none in the
  raw log. Doc length: 367 lines, unchanged this round.
