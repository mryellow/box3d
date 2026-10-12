---
title: Preflight - evaluate continuous candidates in sweep order
status: Draft
date: 2026-10-12
follows: 20261012-004 (benchmark), solver-tree-order-independent-pr (20261012-002)
---

# Preflight: evaluate continuous candidates in sweep order

A lead from the benchmark (20261012-004 §5), recorded so it is not lost. It is a separate change
from the sorted-candidate PR and should land after it, if at all.

## 1. What the benchmark showed

In a scene of 4000 fast spheres swept through 14,641 static boxes every step, the order in which
the continuous stage evaluates a query's candidates moved the stage's time by 10 to 20 percent in
either direction. Nearest-first along the sweep was fastest, farthest-first slowest, and any order
unrelated to distance (tree order, shape id, a hash of the id) sat in between at the same cost.
The mechanism is in `b3EvaluateContinuousCandidate` (`src/solver.c`): each candidate's
`b3ShapeTimeOfImpact` runs with the running best fraction as its upper bracket, so once the true
nearest hit has been evaluated every later candidate either converges in fewer iterations or is
rejected outright. The PR fixed the order at ascending shape id, which is layout independent but
no better than tree order for this effect.

## 2. Proposal

After the PR's collection step, sort each query's candidates by a key that estimates where along
the sweep the candidate is first reached, ascending, with shape id as the tie-break. Evaluate in
that order. Everything after the sort is unchanged.

Key: the entry fraction of the fast body's centroid segment `centroid1 -> centroid2` (already in
the context, relative to `base`) against the candidate's world box `shape->aabb` grown by the fast
shape's extent, from `b3RayCastAABB` (`src/aabb.h`). A candidate the segment misses, which the
grown box makes rare since the query already overlapped the swept box, takes key 1. The key is a
float computed from state the two worlds of the proposition share (transforms, shapes), so equal
worlds give equal keys and the tie-break makes the order total.

Cost per candidate: one slab test, a few dozen flops, against a time-of-impact solve of hundreds.
The sort already exists; its comparison grows from one integer compare to a float compare and an
integer compare.

## 3. What changes for users

Deterministic and layout independent, as before. But the order is different from the PR's, so:

- The accepted fraction can differ from the PR's by the same few ulps that §2 of 20261012-003
  describes between any two orders. Not a regression against `main`, where the order was
  arbitrary; a change against the PR's ascending-id results.
- An accepting `preSolve` callback is called for fewer candidates, since fewer improve on the
  running fraction once the nearest is first. The set of calls is still deterministic. A rejecting
  callback is still called for every on-path candidate, in the new order.
- The sensor selection past eight hits keeps the first eight in the new order, which is now
  nearest-first rather than lowest id first. That is arguably the better choice.
- The three continuous counters change value, as they are maxima over accepted candidates.

The PR's public proof tests (`test/test_ccd_layout.c`) and internal tests (`test/test_tree_order.c`)
compare two layouts of the same world, so they pass unchanged; the ascending-id property test in
`test_tree_order.c` would need to assert the new order instead.

## 4. Measure

The microbenchmark `ccdbench.c` (20261012-004 appendix) in both travel directions. Expected:
sweep order matches the better of the ascending and descending variants in both directions, about
5 to 8 percent under `main` in the dense scene, and parity in the project benchmarks, which hold
few fast bodies. If sweep order does not reach the nearest-first variant's time in the +x direction,
the key is not tracking the true hit order and needs refining (for example the fast shape's
leading point rather than its centroid).

## 5. Risks and open questions

- The key is an estimate. Rotating fast bodies and long thin shapes can reach a candidate before
  the centroid segment enters its grown box. The order is still deterministic; only the gain
  shrinks. The safety factor the existing fast-body test uses could grow the box the same way.
- NaN or infinite keys from degenerate sweeps must sort deterministically; clamp to 1 before the
  compare.
- Double precision: the key is computed in the base-relative float frame the sweep already uses,
  so it does not reintroduce far-from-origin error.
- Whether the accepting `preSolve` call-set change is acceptable to users who rely on seeing every
  improving candidate. The PR's decision record (20261012-001 §6) narrowed the guarantee to
  argument-only callbacks, which this keeps.

## 6. Decision

Not part of the sorted-candidate PR. Revisit once that PR has landed, if the dense-scene
continuous cost matters to a consumer.
