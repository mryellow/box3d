---
title: Review round 30 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round29.md
mode: full pass
---

# Round 30

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:18:53+10:00 to 22:27:12+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13533-8dbc-70b1-8640-6aa851011dcf`.

## Review report (Codex final message)

### 1. Summary

The proposed pair passes and migration timing match the cited source paths, including the join before CCD. I found a cost-model defect in non-awake tree maintenance and a missing connection between B4’s height hooks and the world state they must read and update.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | 2, B4; 4 | Height-only maintenance leaves a sparse non-awake tree with a large historical `nodeEnd`. `b3GatherMovedSiblings` scans to `nodeEnd` and allocates from it, while removals do not shrink it and `b3DynamicTree_Rebuild` returns immediately for an empty tree (`src/broad_phase.c`, `src/dynamic_tree.c`). After a large sleeping population wakes, one remaining moved sleeper can therefore pay for the former population on every affected step. | Reset node state when the non-awake tree empties, and specify a compaction trigger for a sparse non-empty tree. Preserve proxy IDs and pending moved bits. |
| 2 | L | 2, B0/B3/B4 | B4 puts the height check in `b3BroadPhase_CreateProxy` and `b3BroadPhase_MoveProxy`, but their `src/broad_phase.h` interfaces receive only the broad phase. The design puts traversal depth and `sleepingTreeRebuildCount` on the world, without specifying how those hooks access either value. | Specify the context passed to the hooks, or place the depth and counter where the hooks can access them; update the stated interfaces and file list. |

### 3. Checked, no change

- The proxy key has two type bits; code 3 is available while the three body-type codes remain distinct (`src/broad_phase.h`, `include/box3d/types.h`).
- The proposed self and cross passes cover moved awake and sleeping dynamic proxies, including static compounds; contacts between two sleeping bodies enter the disabled set (`src/broad_phase.c`, `src/contact.c`).
- The cited wake, sleep, transform, shape reset, joint moved-mark, sensor, query, and bullet paths support the proposed tree selection and timing (`src/solver_set.c`, `src/body.c`, `src/shape.c`, `src/joint.c`, `src/sensor.c`, `src/physics_world.c`, `src/solver.c`).
- The tree task is joined before CCD, and island sleep follows bullet processing (`src/solver.c`). The cited tree-order preflight’s CCD and explosion changes support the stated layout-order caveats (section 2 of `20261011-001`).
- The churn table’s `sleep`, `trees100`, and `washer` means are consistent with their stated counts and scene shape counts (`shared/benchmarks.c`).

### 4. Proposed edits

Add a sparsity rule beside B4’s height trigger, including the empty-tree reset. State how the broad-phase hooks obtain traversal depth and increment the world diagnostic counter.

### 5. Unresolved

None.

### 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M), by a different fix.** `b3GatherMovedSiblings` scans every pair below `nodeEnd`
   (`broad_phase.c`), a remove never lowers `nodeEnd`, and B1 did not say how the non-awake self
   pass gathers; reused as is, a sleeping proxy marked moved after a large wake would pay the
   former population's `nodeEnd` on each such step. `b3DynamicTree_ClearMoved` (`dynamic_tree.c`)
   shows the alternative: descend from the root through marked internal nodes only, O(marked).
   Fix: B1 states the non-awake self pass gathers its moved sibling pairs by that descent; the
   awake tree keeps the scan, since its rebuild compacts `nodeEnd` every step. No reader of the
   non-awake tree then scales with `nodeEnd` (the refit and the step rebuild do not run on it;
   `b3DynamicTree_ClearMoved` descends marked nodes), so no sparsity trigger is added. B2's
   empty-tree reset is stated for either tree the migration empties.
2. **Adopted (L).** `b3BroadPhase_CreateProxy` and `b3BroadPhase_MoveProxy` take the broad phase
   (`broad_phase.h`), and the design placed the traversal depth and the rebuild count on the
   world. Fix: both live in `b3BroadPhase`; the world's entry and exit pair and
   `b3World_GetCounters` read them there. B0, B3 and B4 follow.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 30: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1
- Tripwire: fixes add or remove a named mechanism: no; the non-awake gather is specified as the
  existing marked-node descent. Clustering: B4 (finding 2, a consequence of revision 26's
  counter and revision 11's depth being on the world); finding 1 is B1's, first raised, and a
  consequence of B4 no longer rebuilding on order (revision 26). Reviewer read of round records
  or changelog: none in the raw log. Doc length: 371 -> 375 lines at revision 30.
