---
title: Review round 43 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round42.md
mode: full pass
---

# Round 43

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:40:58+10:00 to 23:45:32+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1357e-b627-7a83-8c6b-457474869215`.

## Review report (Codex final message)

### 1. Summary

The split’s pair passes, moved-bit handling, migration timing, and direct tree access paths are consistent with the live source. I found one incorrect statement in the stack-capacity discussion; it does not change the proposed design.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | 2, B4 | “Every traversal and pair walk uses a 512-entry stack” is false. `b3GatherCrossSeeds` uses a 128-entry circular queue and caps each gather at 64 seeds (`src/broad_phase.c`). | Limit the 512-entry claim to the depth-first walkers, and describe the seed gather separately. |

### 3. Checked, no change

- The fourth proxy code fits the existing two-bit key; tree and body-type counts need to remain distinct.
- The proposed self and cross passes cover moved sleeping proxies, including static compound children.
- Carrying each leaf’s moved bit through migration preserves pending pair discovery; the pair set is keyed by shapes.
- The rebuild join and migration drains place bodies woken during collide before CCD; the bullet enlargement path then addresses awake proxies.
- The churn table’s stated endpoint method is consistent with the sleep timer and benchmark scene behavior examined.

### 4. Proposed edits

In section 2 B4, replace “Every traversal and pair walk” with “The depth-first tree traversals and pair walkers”; add that cross-seed gathering uses its bounded circular queue.

### 5. Unresolved

None.

### 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (L).** `b3GatherCrossSeeds` (`broad_phase.c`) uses a circular queue of twice
   `B3_CROSS_SEED_COUNT` (64) entries and stops at 64 seeds. Fix: B4's sentence names the
   depth-first walks and the pair walk, and the seed gather with its bounded queue.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 43: 1 finding (H0-M0-L1), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0 -> 0/0/0 -> 0/1/1 -> 0/0/0 -> 0/0/0 -> (gate fix, revision 38) -> 0/1/0 -> 0/0/1
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B4; a wording present since
  revision 25, first raised. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 404 -> 406 lines at revision 39.
