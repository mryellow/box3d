---
title: Review round 23 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round22.md
mode: full pass
---

# Round 23

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:24:31+10:00 to 21:28:59+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The proposed pair passes, proxy-key routing, and migration timing are broadly consistent with the cited source. I found two defects in the document’s claims about behavior and when the gather saving begins.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §1, Behaviour change | Without the tree-order changes, bullet and explosion results can change even when every target is awake. Removing sleeping leaves changes the awake tree’s layout and traversal order; the order-sensitive paths identified in the cited preflight can then visit awake candidates differently (`src/dynamic_tree.c`, `src/solver.c`, `src/physics_world.c`; cited preflight §1). | Broaden the behavior-change statement to cover awake-only candidates after tree migration. |
| 2 | L | §2, B2; introduction | “The first step after the sleep still gathers to the old `nodeEnd`” conflicts with B2’s empty-tree reset. If migration empties the awake tree, B2 resets `nodeEnd` to 2, so the next gather has no old range to scan (`src/broad_phase.c`, `src/dynamic_tree.c`). | Qualify the second-step saving claim to migrations that leave the awake tree nonempty. |

## 3. Checked, no change

- Proxy-key readers and fixed-index tree walks in the scoped source are accounted for by B0–B4.
- The proposed self and cross passes cover awake, sleeping, kinematic, and static pairs, including static compound expansion.
- Carrying moved bits across migration preserves pair-search timing.
- The proposed join before CCD and exit drain address wakes during collide, Explode, and public tree traversals.
- The churn table’s shape-count arithmetic is consistent with the described benchmark scenes.

## 4. Proposed edits

Revise §1’s behavior-change paragraph to include awake-only bullet and explosion cases. In the introduction and B2, state that the old-`nodeEnd` scan occurs after a sleep only when the awake tree remains nonempty.

## 5. Unresolved

None within scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** The awake tree after migrations has a layout the single tree never had, so
   without the tree-order changes the order-dependent paths (`b3ContinuousQueryCallback`,
   `ExplosionCallback`) can visit awake candidates in a different order too. Fix: §1's behaviour
   change covers every bullet and explosion, not only those reaching sleeping bodies.
2. **Adopted (L).** B2 resets an emptied awake tree's node state, so the old-`nodeEnd` scan on the
   first step after a sleep happens only when the tree stays non-empty. Fix: intro and B2
   qualified.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 23: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: finding 2 follows from rounds
  8 and 13 (U-T2); finding 1 is a qualification of §1 text from revision 1. Reviewer read of round
  records or changelog: none in the raw log. Doc length: 312 -> 319 lines.
