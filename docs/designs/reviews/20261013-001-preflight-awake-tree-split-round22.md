---
title: Review round 22 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round21.md
mode: full pass
---

# Round 22

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:17:19+10:00 to 21:23:26+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split covers the main pair passes and fixed-index tree readers. I found one step-ordering gap that could leave an awakened body in the non-awake tree during finalization, plus two source claims that need correction.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2 B3 | A contact can begin touching and wake a sleeping set in `b3Collide` even when `b3UpdateBroadPhasePairs` returned early and enqueued no tree task. The specified drain “right after the tree task is joined” has no join in that case. Draining only at step exit leaves the newly awake body in the non-awake tree during finalize; that tree is not refitted (`src/broad_phase.c`, `src/island.c`, `src/solver.c`, `src/physics_world.c`). | Specify an **unconditional** drain after the optional join and before finalize, including the no-task path. |
| 2 | M | §2 B4 | The rotation rationale omits sleeping-proxy moves. `b3Body_SetTransform` reaches `b3DynamicTree_MoveProxyInternal`, whose insertion disables rotations. With automatic rebuild removed, repeated sleeping-body moves can degrade the non-awake tree; the proposed sleep/wake churn measurement would not exercise that path (`src/body.c`, `src/broad_phase.c`, `src/dynamic_tree.c`). | State a maintenance rule for non-awake proxy moves and measure height and node visits under repeated sleeping transforms. |
| 3 | L | §2 B0 | `b3ValidateSolverSets` does not currently match each proxy tree to its body’s solver set: it accepts either dynamic or kinematic proxy for any non-static set. `b3ValidateBroadPhase` also has no world context with which to perform the proposed check (`src/physics_world.c`, `src/broad_phase.c`). | Describe the actual existing check and specify how the new validator obtains body and shape state after migration drains. |
| 4 | L | §1 | “The rest of the step is O(awake)” contradicts the document’s own sensor exception: the sensor pass iterates every sensor (`src/sensor.c`). | Qualify the complexity claim to the relevant solver work. |

## 3. Checked, no change

- The existing two-bit proxy key has a spare code for a fourth tree (`src/broad_phase.h`).
- The proposed self and cross passes cover the pair classes produced by the current dynamic-tree passes; the static compound pass needs the stated expansion (`src/broad_phase.c`).
- Preserving a leaf’s moved bit across migration preserves pair-discovery timing; sleep and wake currently leave proxies untouched (`src/dynamic_tree.c`, `src/solver_set.c`).
- The listed sensor, query, CCD, explosion, bounds, draw, counter, capacity, and memory readers account for the fixed tree accesses found in scope.
- The §4 `sleep`, `trees100`, and `washer` means agree with their stated shape counts and step counts. The endpoint method’s no-round-trip premise is consistent with the wake timer reset and 60 Hz step (`src/solver_set.c`, `src/solver.c`, `shared/benchmarks.c`).

## 4. Proposed edits

Make the pre-finalize drain unconditional in B3. Correct B0’s validator description and state the required world context. In B4, distinguish rotated proxy creation from unrotated proxy moves and add the corresponding maintenance measurement; narrow §1’s complexity sentence.

## 5. Unresolved

Whether repeated moves of sleeping proxies keep the non-awake tree’s height and traversal cost acceptable remains unmeasured.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3UpdateBroadPhasePairs` returns before enqueuing the tree task when nothing
   moved; `b3Collide` still updates every awake contact and can set `b3_simStartedTouching`, whose
   processing wakes a set (`physics_world.c`, `island.c`); the join in `b3Solve` is conditional on
   a task (`solver.c`). Fix: B3's mid-step drain is at the join point, whether or not a task was
   enqueued.
2. **Adopted (M).** `b3DynamicTree_MoveProxyInternal` inserts with `shouldRotate = false`, and
   `b3Body_SetTransform` on a sleeping body reaches it (`body.c`, `broad_phase.c`,
   `dynamic_tree.c`). Fix: B4 states that creates rotate and moves do not, and that the height and
   node-visit measurements include repeated sleeping-body transforms; the rebuild is the remedy.
3. **Adopted (L).** `b3ValidateSolverSets` accepts a kinematic or dynamic proxy for any non-static
   set, and `b3ValidateBroadPhase` takes the broad-phase alone. Fix: B0 describes the check as it
   is and places the proxy-tree check in a world-level validation at the end of the step, after
   the exit drain.
4. **Adopted (L).** Fix: §1 says the collide and solve are O(awake), not "the rest of the step".

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 22: 4 findings (H1-M1-L2), 4 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: findings 1 and 3 follow from
  rounds 11 and 13 (U-T2); finding 2 is the fourth round on B4's balance (15, 18, 20, 22), this
  time a real gap (moves do not rotate) rather than a re-raise. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 308 -> 312 lines.
