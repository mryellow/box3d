---
title: Review round 24 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round23.md
mode: full pass
---

# Round 24

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:30:04+10:00 to 21:34:11+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`. The answer uses numbered bold headings.

## Review report (Codex final message)

1. **Summary**  
The source supports the proposed tree split, migration timing, pair passes, and the churn table’s arithmetic. One safety claim needs correction: the new cross pair passes can exhaust their fixed stack at a tree height well below 500.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2 B4; §8 | The “height over 500” threshold is false for cross pair walks. `b3CollideCrossPairs` in `src/broad_phase.c` can add several pending node pairs at each level; a full stack skips pairs in release builds. Leaving the non-awake tree without step rebuilding adds an unbounded-height input to these passes. | Correct the stack analysis and specify how pair discovery remains complete under sleep/wake churn: bound the relevant stack use, rebuild before an unsafe walk, or make the walk overflow-safe. |

3. **Checked, no change**

- The fourth key code fits the existing two-bit field; the cited fixed-index readers and proxy-key paths need the updates listed in B0–B3.
- The proposed passes cover moved awake, sleeping, kinematic, and static proxies, including static compound expansion. Contact placement for two sleeping bodies matches `src/contact.c`.
- The tree task joins before finalize and CCD, and the sleep pass is last. Carrying moved bits preserves pending pair work.
- The churn table’s means agree with its counts. The sleep timer and benchmark callbacks support endpoint sampling for the described scenes; I did not rerun the scratch harness.

4. **Proposed edits**  
Replace the height-based stack assurance in B4 and §8 with a bound for cross pair traversal, and state the chosen safeguard and its validation criterion.

5. **Unresolved**  
Worst-case cross pair stack use under the proposed non-awake tree churn has not been established.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted in part (M).** `b3CollideCrossPairs` pops one node pair and visits four child
   combinations, each of which may push, so its stack grows by up to three per level and the
   512-entry stack fills near a height of 170; `b3DynamicTree_Query` and
   `b3CollideProxyAndSubtree` push at most one net per level and fill past 500 (`broad_phase.c`,
   `dynamic_tree.c`). The "over 500" figure in B4 and Provenance was wrong for the cross walks.
   The reliance class is unchanged: today's dynamic-against-static cross pass walks the static
   tree with that stack. Fix: B4 and the Provenance bullet carry both figures. No bound, rebuild
   trigger or overflow-safe walk is added, for the reason already in Provenance (U-P11, U-V4).

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8). Series paused by the owner after this round's
  fixes.
- Round 24: 1 finding (H0-M1-L0), 1 applied in part, 0 declined outright.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> paused
- Tripwire: fixes add or remove a named mechanism: no. Clustering: fifth round on B4's stack
  reliance (15, 18, 20, 22, 24); this one corrected a number the earlier declines rested on.
  Reviewer read of round records or changelog: none in the raw log. Doc length: 319 -> 321 lines.
