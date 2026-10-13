---
title: Review round 34 - preflight awake tree split (confirmation)
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round33.md
mode: confirmation round (U-P9), same prompt and scope, doc unchanged since round 33
---

# Round 34

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:52:25+10:00 to 22:56:17+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13552-4342-71e1-a224-f26967fa5d08`.

## Review report (Codex final message)

### 1. Summary

The source supports the proposed pair passes, proxy-key handling, migration timing, and cited cost method. One sequencing defect remains: the first PR would ship the non-awake tree without the height maintenance that the document says is needed for correct traversals.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2, B0–B4 | B0–B3 form a separate PR, while B4 alone adds the non-awake tree’s height trigger. During that first PR, unrotated moves and removals can leave the tree arbitrarily deep. The 512-entry walks in `src/dynamic_tree.c` and `src/broad_phase.c` can then skip branches in release builds, missing queries or pairs—the failure §2 B4 itself identifies. | Ship height maintenance with B2–B3, or add an interim per-insert safeguard to the first PR. Do not leave an independently deployable interval with an unbounded non-awake tree. |

### 3. Checked, no change

- Verified the cited proxy-key readers and writers, including shape reset, joint moved marks, sensors, world queries, bounds, counters, and memory reporting.
- Verified that the proposed pair passes cover moved sleeping proxies against awake, sleeping, static, and kinematic proxies, including static compound expansion.
- Verified the rebuild join, collide wake, CCD lookup, explosion traversal, and end-of-step sleep ordering.
- Checked the churn method against the cited sleep timer and benchmark scene code; found no supported error in the reported counts or method.

### 4. Proposed edits

Move B4’s height safeguard into the first PR’s required scope, and update the PR split and dependency text accordingly.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** The PR plan dated from revision 1, when B4 was "no step rebuild" and the
   first PR's non-awake tree was maintained like the static tree. After revision 26 B4 is what
   bounds the tree, and `b3UpdateTreesTask` (`broad_phase.c`) rebuilds the dynamic and kinematic
   trees by index only, so a first PR of B0 to B3 would ship a tree nothing rebuilds. Fix: B0 to
   B4 are one PR of five commits; until B4's commit the step rebuilds the non-awake tree on order
   as it does the dynamic tree, one line in `b3UpdateTreesTask` that B4 removes, so no commit
   leaves the tree unbounded.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round found a defect, so round 33's clean round
  does not stand (U-P9); a fresh clean round and confirmation are needed (U-S8).
- Round 34: 1 finding (H1-M0-L0), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0
- Tripwire: fixes add or remove a named mechanism: no; the PR boundary changes and one interim
  line is named. Clustering: the PR plan, first raised; a consequence of revision 26, which
  made B4 load-bearing. Second confirmation round to find something a clean round missed (28,
  34). Reviewer read of round records or changelog: none in the raw log. Doc length: 388
  lines, unchanged this round.
