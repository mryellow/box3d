---
title: Review round 8 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round7.md
mode: full pass
---

# Round 8

## Exact request sent to Codex

Identical to round 1 except the one-line description of the subject, which read "splitting the
dynamic and kinematic broad-phase trees each into awake and non-awake trees" for this round
(revision 8's five-tree shape). Same scope.

Run: foreground, `timeout 540`, exit 0, 2026-10-13T19:01:58+10:00 to 19:08:03+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The five-tree pair coverage and the proposed handling of wake-related traversal hazards match the cited source. Two cost claims need correction: the first step and capture after a large sleep still pay for the awake tree’s old `nodeEnd`, and the description of refit writes is inaccurate.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | Opening, 1, 5.1 | A large sleep leaves the awake tree’s old `nodeEnd` in place. On the next step, `b3GatherMovedSiblings` scans that range **before** the queued rebuild compacts it (`src/broad_phase.c`, `src/dynamic_tree.c`). The capture immediately after sleep also copies nodes and parents to that old end: for a dense tree falling from 10,000 proxies to ten, that capture is about **960 KB**, not the stated 240 KB. | Qualify the proportional-cost claim for the first post-sleep step and capture, or compact before gathering and capture. State that 240 KB applies after compaction. |
| 2 | L | 1 | The document says rebuild and refit both rewrite every node when the root is moved or the tree is unordered. Refit returns if the root is unmoved; otherwise it scans `nodeEnd` and writes only marked internal AABBs (`src/dynamic_tree.c`). | Describe rebuild and refit separately, distinguishing nodes scanned from nodes written. |

## 3. Checked, no change

- The proposed self and cross passes cover the existing dynamic pair classes, including pairs involving moved sleeping proxies (`src/broad_phase.c`, `src/contact.c`).
- Proxy-key uses, fixed-index tree readers, and the direct solver tree accesses are accounted for in B0–B4 (`src/broad_phase.c`, `src/shape.c`, `src/solver.c`, `src/physics_world.c`, `src/sensor.c`).
- The pending rebuild and contact-state wake can overlap today; joining before wake-related migration addresses that hazard (`src/broad_phase.c`, `src/physics_world.c`, `src/island.c`).
- The section 4 move totals and averages are internally consistent with its stated endpoint method and the cited benchmark scenes (`shared/benchmarks.c`).
- Awake-tree image fields and the non-awake proxy-id undo argument are consistent with the cited rewind design’s sections 5.4, 7.1, 7.2 and 7.4.

## 4. Proposed edits

Revise the opening and section 5.1 to describe the one-step stale-`nodeEnd` cost and the larger capture immediately after sleep. Replace section 1’s “rewrite every node” statement with the refit behavior above.

## 5. Unresolved

None within the requested scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** The pair update gathers (`b3GatherMovedSiblings` to `nodeEnd`) and only then
   enqueues the rebuild (`b3UpdateBroadPhasePairs`, `broad_phase.c`), so the first step after a
   sleep scans the old `nodeEnd` once, and a capture taken before that step copies `nodes` and
   `parents` to the old end: 20,000 nodes at 36 B plus 10,000 proxy slots at 24 B is about 960 KB
   for a tree that fell from ten thousand proxies to ten. Fix: intro and 5.1 state the one-step
   cost and that 240 KB holds from the following capture.
2. **Adopted (L).** `b3DynamicTree_Refit` returns when the root is unmoved and otherwise scans every
   pair below `nodeEnd` writing only marked internal AABBs (`dynamic_tree.c`). Fix: §1 describes
   rebuild and refit separately.

## Owner reversal received with this round (U-S5)

The "No" recorded in round 7 against decision 1 meant "no, the kinematic tree need not split;
imaging it is fine". Revision 9 returns the doc to four trees (static, kinematic, awake dynamic,
non-awake dynamic): the proxy key keeps its two-bit field, B1's passes are those of revision 7,
and 5.2 states that the kinematic tree is imaged in the rewind design's non-tree-order branch.
The reason is recorded in Provenance.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 8: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no; the owner reversal removes the five-tree
  structure revision 8 added. Clustering: the awake tree's `nodeEnd` drew findings in rounds 1, 3,
  7 and 8, each narrowing when it compacts (U-T2: finding 1 follows round 7's fix). Reviewer read
  of round records or changelog: none in the raw log. Doc length: 336 -> 354 lines for revision 8;
  revision 9 removes most of that.
