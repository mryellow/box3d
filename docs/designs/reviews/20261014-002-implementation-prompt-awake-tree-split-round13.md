---
title: Review round 13 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round12.md
mode: full pass, same prompt and scope as round 11 (U-P4), on revision 13
---

# Round 13

## Exact request sent to Codex

Identical to round 11 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T11:12:39+10:00 to 11:17:38+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a137f7-f6ae-7331-aaea-ce12fa878bee`. The answer uses numbered bold headings.

## Review report (Codex final message)

1. **Summary**

The implementation prompt largely matches the binding design and the live code paths I checked. I found one defect in §5’s golden-hash policy: its evidence can attribute a changed query-spawn state to a tied ray hit without checking a value that actually determines the spawn.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5 | The first-divergence procedure logs the closest ray’s hit flag, shape, fraction and normal, but omits its **point**. `shared/determinism.c` uses that point to choose the spawn position. A wrong point could therefore change state and sleep goldens while the specified logs still satisfy the tie rule. | Log and compare the closest hit point. At a permitted equal-fraction tie, require the point to remain equal; log each all-hits point and normal so the selected result can be checked against its shape’s hit. |

3. **Checked, no change**

- The four-commit sequence and the proxy-key, tree-count, migration and maintenance mechanics align with the design’s §2.
- The cited broad-phase readers, snapshot tree loops, solver-set transitions and query-spawn inputs match the relevant live source.
- The B1 pair fixture’s expected contact sets follow `src/contact.c`; the B4 fail-before height check can run before the step rebuilds that tree.
- The test runner supports the named-entry fail-before procedure, and the benchmark command and scene lengths match `benchmark/main.c`.

4. **Proposed edits**

Revise §5’s logging and tie acceptance rule as described in finding 1, including its account of how `QUERY_SPAWN_HASH` may change.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

1. **Adopted (M).** `QuerySpawnOnce` (`shared/determinism.c`) sets the spawn position from the
   closest hit's point offset along its normal; the point left the log in an earlier revision. Fix:
   both logs carry point and normal; at a tie the point is equal on both sides, each side's closest
   hit matches its shape's all-hits entry, and the all-hits sets compare on all four values.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 13: 1 finding (H0-M1-L0), 1 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> 0/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5 for the sixth round running;
  this one restores a value a round 9 fix dropped (a consequence of that fix). The attribution
  proof now names every value the scene reads and every value the hash folds; one clause. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 760 -> 762 lines.
