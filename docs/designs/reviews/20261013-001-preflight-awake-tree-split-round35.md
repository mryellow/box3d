---
title: Review round 35 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round34.md
mode: full pass
---

# Round 35

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:56:58+10:00 to 23:01:30+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13556-6d73-71b2-b97d-925fb9631b69`.

## Review report (Codex final message)

### 1. Summary

The completed B0–B4 design accounts for the tree readers, proxy-key users, and moved-bit paths I found in the cited source. Its commit order has one safety defect: B2 enables migration before B3 protects active traversals and the async rebuild.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2, B2–B3 | B2 is specified as a separate commit before the migration guard. At that commit, `ExplosionCallback` can wake and migrate a proxy while `b3DynamicTree_Query` holds the tree’s node array; a collide wake can also migrate while the enqueued tree rebuild is running (`src/physics_world.c`, `src/dynamic_tree.c`, `src/solver.c`). Migration can invalidate the traversal or race the rebuild. | Land B3’s queue, traversal guard, and rebuild join before enabling B2 migration, or combine B2 and B3 into one commit. |

### 3. Checked, no change

- The four-code proxy key fits the existing two-bit field; the body-type count remains three (`src/broad_phase.h`, `src/broad_phase.c`).
- The proposed self and cross pair passes cover moved sleeping proxies against awake, sleeping, kinematic, and static proxies, including static compounds (`src/broad_phase.c`, `src/contact.c`).
- The listed fixed-index readers and the solver’s bullet proxy access are consistent with draining wakes before finalization (`src/solver.c`, `src/physics_world.c`, `src/sensor.c`).
- The moved-bit preservation, empty-tree reset need, and height-trigger rationale match the tree operations (`src/dynamic_tree.c`).
- The churn table’s totals and means are consistent with its stated endpoint-scan method; I found no specific transition that method would miss in the cited step order (`src/solver.c`, `src/solver_set.c`).

### 4. Proposed edits

Reorder §2 so traversal protection and the drain are in place before sleep and wake begin moving proxies. Update the B0–B4 sequence and commit descriptions to match.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** Revision 34 made the commit order explicit, and B2's commit then precedes
   B3's: at B2's commit a wake from `ExplosionCallback` (`physics_world.c`) or from the collide
   migrates while a walk holds the node array or the tree task runs. Fix: B2 and B3 are one
   commit, since a migration is unsafe without the guard and the guard has nothing to do
   without migrations; the PR is four commits.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 35: 1 finding (H1-M0-L0), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0
- Tripwire: fixes add or remove a named mechanism: no. Clustering: the PR plan, second round
  running; a consequence of revision 34's "in order". Reviewer read of round records or
  changelog: none in the raw log. Doc length: 388 -> 390 lines at revision 34.
