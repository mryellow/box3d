---
title: Review round 14 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round13.md
mode: full pass
session: 01a134b9-cdd8-7601-86cb-2fc0f37e3120 (resumed)
---

# Round 14

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 124 (work timeout after a long trace of real reads),
2026-10-13T20:05:54+10:00 to 20:14:54+10:00. Resumed per the workflow with `-m gpt-6-sol -c
model_reasoning_effort="high" --sandbox read-only resume <session>` and the instruction to finalize
from the research already done with no further tool calls; exit 0, 20:15:07 to 20:15:39. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high` on both calls. The resumed answer uses
numbered bold headings instead of `##` headings; it is otherwise the requested shape.

## Review report (Codex final message)

1. **Summary**

The source supports the proposed proxy-key split, moved-bit handling, and the awake-tree rewind image. Three points need correction: migration timing across multi-tree public queries, compound pair handling in the new static cross pass, and the claim that cross-pass cost is unchanged.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | 2, B3 | The specified entry/exit around *each tree walk* can drain migrations between trees in one public query. In `physics_world.c`, overlap and cast APIs walk trees sequentially; a callback that wakes a sleeper can move its proxy out of a tree still to be visited, omitting a hit. Sleeping an awake body can produce a duplicate hit. | Hold traversal depth across the **entire public query call**, as B3 explicitly does for Explode. |
| 2 | M | 2, B1 | The new non-awake/static cross pass needs the static-compound expansion. In `broad_phase.c`, `b3CrossPairsTask` enables `b3EmitCompoundPairs` only for its static subrange; an ordinary cross pass would not produce compound child contact keys for a moved sleeping proxy. B1 specifies the tree pair but misses this special handling. | State that both awake/static and non-awake/static passes use compound expansion and flush before non-static passes. |
| 3 | L | Opening, 4 | “Cross passes [are] unchanged in cost” and “visit the same overlapping nodes as today” are false as stated. Splitting changes internal-node layout, and B4 leaves the non-awake tree incrementally maintained with no height bound (`dynamic_tree.c`); cross-pass node visits can therefore differ, including under churn. | Qualify the cost claim and report cross-pass node visits alongside the planned tree-height and `pairs` measurements. |

3. **Checked, no change**

- The fourth proxy code fits the existing two-bit key; the identified fixed-index tree readers and validators need the stated four-tree updates.
- Moved-bit carry, the first-step gather before rebuild, and the empty-awake-tree reset match `dynamic_tree.c` and the step order.
- The solver joins the tree task before CCD; sleeping proxies must remain in bullet and sensor queries.
- Section 5’s awake image and non-awake proxy-identity, AABB, and moved-bit restore treatments are consistent with the cited rewind sections.
- The section 4 counts are consistent with the cited benchmark scene construction; I found no defect in the stated endpoint-scan method for those scenes.

4. **Proposed edits**

Clarify whole-call traversal scopes in B3, add the compound rule to B1, and replace the fixed cross-pass cost claim with a layout-dependent cost statement and measurement requirement.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

1. **Adopted (M).** The public overlap and cast functions loop over the trees and query each in
   turn (`physics_world.c`). An exit between two of those walks would drain a wake out of a tree
   not yet visited, or a sleep into one already visited. Fix: B3's entry and exit wrap the whole
   public call, as it already said for Explode.
2. **Adopted (M).** `b3CrossPairsTask` runs the static seeds with `checkCompounds` set and flushes
   before the kinematic seeds run with it clear (`broad_phase.c`), because compound shapes are
   static only. Fix: B1 states that the non-awake-against-static pass uses the compound expansion
   and flushes before the non-static passes, as the awake-against-static pass does.
3. **Adopted (L).** The leaves a moved box overlaps are the same, but the internal nodes visited
   depend on each tree's layout, and the non-awake tree's height is unbounded under B4. Fix: the
   intro and §4 say the same leaves are reached and the node visits depend on layout, with
   cross-pass node visits added to the PR's measurements.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 14: 3 findings (H0-M2-L1), 3 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: finding 1 follows from round
  11's entry-and-exit pair (U-T2); finding 3 is the fourth narrowing of the cost claim (rounds 2,
  3, 4, 14). Reviewer read of round records or changelog: none in the raw log. Doc length: 378 ->
  386 lines.
