---
title: Review round 42 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round41.md
mode: full pass, on revision 38 (gate fix after the converged revision 37)
---

# Round 42

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:35:56+10:00 to 23:40:27+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1357a-17bc-7030-bbfc-673ef58a5e58`.

## Review report (Codex final message)

### 1. Summary

The proposed pair passes and migration timing account for the tree readers and moved-bit paths I found in the cited source. One stack-safety claim in B4 is incorrect for bodies with multiple shapes.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2 B4 | The “about 430 sleeping-body moves” allowance counts bodies, but `b3Body_SetTransform` moves **each shape proxy** (`src/body.c`). One callback can therefore make hundreds of unrotated inserts while traversal depth suppresses the height rebuild. A later tree walk in the same public call can exceed the 512-entry stack and silently skip branches in release builds (`src/dynamic_tree.c`). | State the allowance in **proxy inserts**, account for multi-shape bodies, and specify how walks remain safe until the outermost exit rebuilds the tree. |

### 3. Checked, no change

- The proposed self and cross passes cover the pair combinations searched by the current dynamic tree; the pair set remains keyed by shapes (`src/broad_phase.c`, `src/contact.c`).
- Sleep and wake currently leave proxies untouched; carrying leaf moved bits preserves pending pair searches (`src/solver_set.c`, `src/dynamic_tree.c`).
- The task join precedes CCD, and the sleep pass follows it, as B3 states (`src/solver.c`, `src/physics_world.c`).
- The fixed-index tree readers, proxy-key writers, and sensor queries identified in B0–B3 match the cited source.
- The churn table’s move totals and stated shape ratios are internally consistent; the endpoint method’s round-trip argument holds for the benchmark step paths described in §4.

### 4. Proposed edits

Revise B4’s stack argument to count inserts per **shape**, and replace its blanket bounded-walk claim with the actual behavior during callback writes or a concrete safeguard.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** `b3Body_SetTransform` moves every shape's proxy (`body.c`), so the
   allowance is in proxy inserts, not bodies. Fix: B4 counts it as about 430 proxy inserts,
   one per shape per move, and states what lies past it: a release build's walk in that call
   skips branches, the failure any tree write under an open walk already risks today, which B3
   leaves as it is by owner decision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 42: 1 finding (H0-M1-L0), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0 -> 0/0/0 -> 0/1/1 -> 0/0/0 -> 0/0/0 -> (gate fix, revision 38) -> 0/1/0
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B4, a consequence of
  revision 37's allowance sentence; the gate fix in B3 drew nothing. Reviewer read of round
  records or changelog: none in the raw log. Doc length: 402 -> 404 lines at revision 38.
