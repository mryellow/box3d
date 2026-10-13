---
title: Review round 3 - island-scoped rollback facts
target: 20261012-001-island-scoped-rollback-facts.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-island-scoped-rollback-facts-round2.md
mode: full pass
---

# Round 3

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T17:39:24+10:00 to 17:43:42+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`. The last copy of the final message is
below.

## Review report (Codex final message)

## 1. Summary

Section 10’s split-tree analysis has four source-supported gaps. Two can miss contacts or race a tree rebuild; a third makes explosion traversal unsafe in the branch without the cited order changes.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 10, pair passes | The proposed non-awake/static and non-awake/kinematic passes run only when the non-awake root is marked moved. `b3TestPair` accepts movement on **either** side (`src/broad_phase.c`), so a newly moved static or kinematic proxy can miss a stationary sleeping neighbour. | State that each cross pass must run when either root is marked moved. |
| 2 | H | 10.4 | `b3UpdateBroadPhasePairs` can enqueue an asynchronous awake-tree rebuild; contact processing can then wake a sleeping set before that task is joined (`src/broad_phase.c`, `src/physics_world.c`, `src/solver.c`, `src/island.c`). Under the split, wake would mutate the tree being rebuilt. | Add the scheduling consequence and the required stable-tree boundary before proxy migration. |
| 3 | H | 10.2, without order changes | `ExplosionCallback` wakes a body during `b3DynamicTree_Query`. With the proposed split, wake removes proxies from the non-awake tree while the query retains its node pointer and traversal stack (`src/physics_world.c`, `src/dynamic_tree.c`). The cited preflight’s A3 defers wakeups, but this branch explicitly proceeds without those changes. | Explain how this branch completes traversal before migrating proxies. |
| 4 | M | 10, membership readers; 10.2 | “None reads order beyond section 5” overlooks world query APIs. Overlap and cast callbacks receive traversal order, and `b3World_CastRayClosest` can select a different shape for equal-distance hits when layout changes (`src/physics_world.c`, `src/dynamic_tree.c`). The stated callback exception names only `preSolve` and custom filtering. | Qualify the order-independence claim and identify query callback order and cast ties as observable exceptions. |

## 3. Checked, no change

- The two-bit proxy key has room for a fourth tree code; sleep and wake currently leave proxies untouched (`src/broad_phase.h`, `src/solver_set.c`).
- The awake-tree image fields cover the active nodes, parent indices, proxy slots and free-list state needed to restore its layout (`src/dynamic_tree.c`, `src/dynamic_tree.h`).
- The listed sleeping-proxy mutations and moved-bit clearing match the cited paths (`src/shape.c`, `src/body.c`, `src/joint.c`, `src/broad_phase.c`).
- The three cited step/explosion order dependencies and the preflight’s A3 treatment match the source and cited preflight section. The section 10.5 arithmetic is consistent with its stated per-move estimate.

## 4. Proposed edits

Correct the cross-pass gate; add the rebuild race and explosion traversal constraint to section 10; and narrow the layout-independence claim to account for public query behavior.

## 5. Unresolved

The split is not implemented, so these consequences were checked against the stated split and live call order, not by executing split-tree code. The scratch-harness measurements were not independently reproduced.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3TestPair` (`broad_phase.c`) accepts a moved node on either side, and the
   kinematic and static cross passes run today whenever the dynamic or the other root is marked
   moved. Fix: the non-awake cross passes run when either root is marked moved; the self pass when
   the non-awake root is.
2. **Adopted (H).** The step calls `b3UpdateBroadPhasePairs` (enqueues `b3UpdateTreesTask`), then
   `b3Collide`, whose contact-state pass calls `b3LinkContact` on begin touch, which wakes sleeping
   sets (`island.c`), and joins the tree task only inside `b3Solve` (`solver.c`). Today a wake
   touches no tree. Fix: a bullet in §10 stating that under a split the engine joins the rebuild
   before processing contact state changes or defers the proxy moves past the join.
3. **Adopted (H).** `ExplosionCallback` calls `b3WakeBody` inside `b3DynamicTree_Query`
   (`physics_world.c`). Under a split that wake removes leaves from the tree being traversed. Fix:
   the "without the order changes" branch of §10.2 states that Explode still defers its wakes
   until the query returns, as A3 does, without A3's sort.
4. **Adopted (M).** Public overlap and cast callbacks receive traversal order and the closest ray
   cast picks among tied hits by order (`physics_world.c`); the preflight already lists them as
   outside its guarantee. Fix: the membership-readers sentence says the step's reads add no order
   dependence beyond §5 and that public query callbacks and tied casts see layout order.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 3: 4 findings (H3-M1-L0), 4 applied, 0 declined.
- Findings: 3/2/2 -> 2/0/1 -> 3/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none for the rewind design; two engine-side
  constraints stated (rebuild join before wakes, deferred explosion wakes). Clustering: all three
  rounds' findings sit in §10 (U-S4), and findings 1 and 4 land on text rounds 1 and 2 added
  (U-T2). Three consecutive rounds each found an H (U-S1). Both stop rules fire: after these fixes
  the series pauses and reports to the owner before a further round. Reviewer read of round records
  or changelog: none in the raw log. Doc length: 372 -> 379 lines; round 2's fixes added 7 lines.
