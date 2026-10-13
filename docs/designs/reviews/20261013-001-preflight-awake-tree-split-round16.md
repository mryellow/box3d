---
title: Review round 16 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round15.md
mode: full pass
---

# Round 16

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T20:27:51+10:00 to 20:35:32+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The awake and non-awake pair passes, migration timing, and cost arithmetic are largely consistent with the live source. One rewind path is incomplete: the proposed migration create can bypass the proxy-identity journal, leaving a non-awake proxy behind after rewind.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2 B2; 5.2–5.4 | B2 proposes a separate migration create that calls `b3CreateTreeProxyInternal` in `src/broad_phase.c`. The cited rewind design §7.4 places proxy-create entries inside `b3BroadPhase_CreateProxy` and `DestroyProxy`; merely placing the new call in the same file does not record it. In the tree-order branch, rewind from a later sleep to a tick when the body was awake would restore the awake proxy from the image but leave its unjournaled non-awake proxy alive. | Route ordinary and migration creates through one journaled create point, preserving the old moved bit. State explicitly that migration’s `shape->proxyKey` write uses the rewind design’s shape-record accessor. |
| 2 | L | 2 B4; 4 | B4 says a height measurement in §4 confirms the new tree’s reliance on the 512-entry traversal stack. Section 4 contains churn counts and calls for height measurements in the PR; it contains no such measurement. | Remove the claim of confirmation and state the height evidence still to be reported. |

## 3. Checked, no change

- The existing proxy key has room for code 3; the identified fixed-index readers and tree walks need the fourth tree.
- The proposed pair passes cover awake, sleeping, kinematic, and static neighbours, including static compound expansion. Moved-bit carry preserves the existing pair-search timing.
- The rebuild task is joined before finalize and bullet CCD. Deferring collision-time wakes addresses that task’s tree-read window.
- The churn table’s means are consistent with its stated shape counts, including `sleep` and `trees100`. The awake-tree image size example matches the source’s node and proxy sizes.
- The non-awake restore treatment is consistent with the cited rewind design §7.4 apart from the migration-create gap above.

## 4. Proposed edits

Specify the shared journaled create path in B2 and §5.3, then add the awake-at-T, slept-after-T rewind case to §5.2. Correct B4’s reference to the height evidence.

## 5. Unresolved

None within scope.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** The rewind design's section 7.4 puts the proxy create and destroy entries
   inside `b3BroadPhase_CreateProxy` and `DestroyProxy`; B2's migration create called
   `b3CreateTreeProxyInternal` beside them and would make no entry, so a rewind from after a sleep
   to a tick when the body was awake would restore the awake proxy from the image and leave the
   non-awake one alive. Fix: there is one create point, `b3BroadPhase_CreateProxy`, which takes
   the moved mark as a parameter; ordinary callers pass today's rule and the migration passes the
   carried bit. The `shape->proxyKey` write uses the rewind design's shape accessor. 5.3 names the
   two functions, not the file, as where the journal sits.
2. **Adopted (L).** §4 calls for the height measurement; it does not contain it. Fix: B4 says the
   measurement the PR reports is the evidence.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 16: 2 findings (H1-M0-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> pending
- Tripwire: fixes add or remove a named mechanism: one removed (the separate migration create
  becomes a parameter of the existing create). Clustering: finding 1 follows from round 2's fix
  (U-T2), which introduced the separate create; finding 2 from round 15's. Reviewer read of round
  records or changelog: none in the raw log. Doc length: 392 -> 402 lines.
