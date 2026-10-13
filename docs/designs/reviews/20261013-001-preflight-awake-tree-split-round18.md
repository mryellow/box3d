---
title: Review round 18 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round17.md
mode: full pass
session: 01a134dc-37b4-7540-ba08-bca226e3b588 (resumed)
---

# Round 18

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 124 (work timeout after a long trace of real reads),
2026-10-13T20:43:29+10:00 to 20:52:29+10:00. Resumed per the workflow with `-m gpt-6-sol -c
model_reasoning_effort="high" --sandbox read-only resume <session>` and the instruction to finalize
from the research already done with no further tool calls; exit 0, 20:52:48 to 20:53:19. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high` on both calls.

## Review report (Codex final message)

## 1. Summary

The proposed tree split covers the identified proxy readers and writers, and the churn figures are consistent with the stated method. Two guarantees need tightening: a different non-awake tree layout can defeat bit-exact rewind when fixed traversal stacks overflow, and the final migration drain is not ordered against rewind capture.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2 B4; 5.2 | The non-awake tree has no height bound, while traversals in `src/dynamic_tree.c` and `src/broad_phase.c` can skip branches when their 512-entry stacks fill. An ordinary-insert rewind can restore the same leaves in a different, deeper layout; pair discovery or CCD can then miss candidates, and the restore-time moved-bit clear can leave bits uncleared. Section 5.2’s bit-exact claim therefore does not hold for every permitted tree. | Guarantee complete traversal and moved-bit clearing for the non-awake tree, or restore its physical layout. Height measurements alone cannot establish the guarantee. |
| 2 | M | 2 B3; 5.1–5.2 | The specified mid-step drain precedes finalize, but the sleep pass runs later and queues migrations until the step’s outermost exit. The design does not place that final drain relative to rewind capture. A capture before it images sleeping bodies’ proxies in the awake tree, contrary to section 5’s state-at-T assumption. | Require the final drain before any end-of-step history image or full state hash, and state that capture begins with an empty migration queue. |

## 3. Checked, no change

- The two-bit proxy key has an available fourth code; the body-type count remains three.
- The listed pair passes cover moved awake, sleeping, kinematic, and static proxies, including static compound expansion.
- Carrying each leaf’s moved bit through migration preserves the current pair-search timing.
- The cited fixed-index tree readers, sleeping-proxy writers, and solver task join match the live source.
- The section 4 arithmetic and the endpoint-scan method match the benchmark transitions described.

## 4. Proposed edits

Add the traversal-completeness requirement to B4 and qualify section 5.2’s rewind claim until it is met. Specify the final drain as a prerequisite for capture in B3 and section 5.

## 5. Unresolved

None beyond the two findings.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Declined (H), re-verified (U-V7).** Round 15's stack finding with a rewind angle. The premise
   is unchanged: a branch is skipped only when a traversal stack of 512 entries fills, which needs
   a tree of height over 500. The rewind design's section 7.4 already derives the static tree's
   layout on restore and walks it with the same stacks, so the non-awake tree is the same case it
   already carries, not a new one; rotation-balanced inserts are what both trees rely on. No fresh
   evidence that the reliance fails (U-V4). Provenance bullet extended.
2. **Adopted (M).** The sleep pass is the last thing the solve does (`solver.c`), after the
   mid-step drain, and the step holds the traversal depth, so sleep migrations queue until the
   step's exit. Fix: B3 states that the step drains at its exit, after the sleep pass and before
   it returns, and 5.1 that a capture, taken after the step returns, sees an empty queue.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8, U-V9 does not apply: one of two applied).
- Round 18: 2 findings (H1-M1-L0), 1 applied, 1 declined (finding 1, reasoning in Provenance).
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: finding 2 follows from round
  11's queue (U-T2); finding 1 repeats round 15's. Reviewer read of round records or changelog:
  none in the raw log. Doc length: 407 -> 412 lines.
