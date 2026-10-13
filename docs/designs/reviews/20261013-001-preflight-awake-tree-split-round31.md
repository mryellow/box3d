---
title: Review round 31 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round30.md
mode: full pass
---

# Round 31

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:28:32+10:00 to 22:35:32+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1353c-6487-7790-b382-9f8ea0f2a269`.

## Review report (Codex final message)

### 1. Summary

The tree readers, proxy-key paths, moved-bit handling, and step ordering described in B0–B3 match the cited source. I found one cost understatement in B4 for layouts that remain above the height trigger after a full rebuild.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | B4 | The document says an unbounded layout costs one full rebuild per sleep or sleeping-body move. Its proposed hook checks **after each proxy insert**. If a full build leaves the tree above 64, a sleep that migrates many shapes, or `b3Body_SetTransform` on a body with many shapes, can rebuild the entire non-awake tree after every shape. The cost is O(sleeping proxies × inserted shapes) for one operation (`src/solver_set.c`, `src/body.c`, `src/broad_phase.c`). | State the per-insert cost, or batch height checks across each multi-shape operation and test once at its end. |

### 3. Checked, no change

- B0’s fourth proxy code fits the existing two-bit key; the identified fixed-index readers and body-type checks need the changes described.
- B1’s self and cross passes cover moved pairs involving awake, sleeping, kinematic, and static proxies, including static compounds.
- B2’s migration preserves the pending moved bit; the cited proxy writers address proxies through their keys.
- B3 places the step drain after the tree task joins and before CCD; its public traversal guard addresses wakes during callbacks.
- B4’s full rebuild preserves leaf moved bits. The churn table’s arithmetic agrees with the cited benchmark scene sizes.

### 4. Proposed edits

Revise B4’s unbounded-layout cost sentence to say **one O(sleeping proxies) rebuild per inserted shape** under the current hook, and specify batching if the intended limit is one check per body operation.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M), by stating the cost.** The hook tests after each insert, so on a layout the
   full build leaves above the threshold, a sleep that migrates an island of many shapes or a
   `b3Body_SetTransform` on a many-shape body rebuilds once per shape (`solver_set.c`, `body.c`).
   On a layout the build bounds, the first rebuild in such an operation brings the height under
   the threshold and the remaining inserts test and do not fire. Fix: B4 states one
   O(sleeping proxies) rebuild per inserted shape for the unbounded layout. Batching the test to
   the end of each operation was not taken: it would move the test from the one insert choke
   point to the list of callers B2 names, which is the pattern rounds 1 to 12 showed to be the
   wrong instrument (§8), for a cost that exists only on the layout B4 already names.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 31: 1 finding (H0-M1-L0), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0
- Tripwire: fixes add or remove a named mechanism: no; one sentence corrected. Clustering: B4,
  a consequence of revision 27's move of the test to the insert. Reviewer read of round records
  or changelog: none in the raw log. Doc length: 375 -> 382 lines at revision 31.
