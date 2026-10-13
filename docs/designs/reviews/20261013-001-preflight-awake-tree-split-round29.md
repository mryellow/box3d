---
title: Review round 29 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round28.md
mode: full pass
---

# Round 29

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:14:06+10:00 to 22:18:05+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1352f-2e9a-7401-8764-6308f636741a`.

## Review report (Codex final message)

### 1. Summary

The split covers the pair combinations and the main wake, query, and solver readers. Section B4 has a maintenance gap for sleeping-body moves made from query callbacks, and its stack headroom claim is numerically wrong.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | B4 | `b3Body_SetTransform` can move a sleeping proxy during a public query callback. B4 skips the height check while traversal depth is nonzero and says no rebuild is added for that write. Its exit check is described as following queued migration inserts, so a callback that only moves a proxy can leave the tree above the threshold for the next query. This contradicts B4’s between-walk height claim. | Record a pending height check for every skipped sleeping-tree insert or move, and run it at the outermost traversal exit, including when the migration queue is empty. |
| 2 | L | B4 | B4 says a threshold of 64 keeps every walk under a quarter of the 512-entry stack. Its own cross-pair bound is up to three entries per level: `3 × 64 = 192`, above a quarter (128). A full build can also leave a tree above 64, as B4 acknowledges. | Correct the headroom statement and distinguish the height trigger from a guaranteed height bound. |

### 3. Checked, no change

- The awake self pass and the specified cross passes cover the pair classes in `src/broad_phase.c`; the static compound pass needs its separate expansion and flush.
- Proxy keys have two tree-code bits, and the cited tree readers and proxy-key users can address a fourth tree. The bullet proxy enlargement in `src/solver.c` remains correctly tied to the awake dynamic tree.
- The rebuild, moved-bit, sleep/wake, sensor, and Explode claims match the cited source. The churn table’s shape counts are consistent with the scene construction in `shared/benchmarks.c`; I found no demonstrated flaw in its stated endpoint-scan method.

### 4. Proposed edits

Specify the pending height-check behavior at traversal exit in B4. Replace the “under a quarter” stack claim with the actual cross-pair bound and retain the stated exception for layouts that a full build cannot bring below the threshold.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** Revision 28 tied the exit-time test to the drain's own inserts, and a
   callback `b3Body_SetTransform` on a sleeping body writes the tree directly with nothing
   queued, so its insert went untested until the next insert outside a traversal. Fix: the
   outermost exit runs the O(1) test once, after the drain, whether or not anything was queued.
   No pending flag is needed, since the test costs one root read.
2. **Adopted (L).** "Under a quarter of its fill" held for the single walks (64 of 512) and for
   the pair stack against an awake tree near log2 of its count, not for the pair stack with both
   trees at 64 (192 of 512). Fix: B4 states the three figures and that the threshold is a
   trigger, a bound only on a layout the full build brings under it.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 29: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B4 both; finding 1 is a
  consequence of revision 28's depth-gated test, finding 2 of revision 28's stack paragraph.
  Round 27 found nothing in this text and round 29 found two things in it; the text was the
  same both times. Reviewer read of round records or changelog: none in the raw log. Doc length:
  367 -> 371 lines at revision 29.
