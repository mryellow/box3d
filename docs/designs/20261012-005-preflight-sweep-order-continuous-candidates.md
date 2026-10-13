---
title: Preflight - sweep-ordered continuous candidates
status: Canonical
date: 2026-10-12
follows: solver-tree-order-independent-pr (20261012-002), 20261012-004 (benchmark)
---

# Preflight: sweep-ordered continuous candidates

One change to Box3D, in one PR after the sorted-candidate PR has landed. The continuous stage keeps
evaluating each tree query's candidates in a deterministic, layout-independent order, but the order
becomes nearest-first along the sweep instead of ascending shape id. The guarantee of the
sorted-candidate PR is unchanged; the cost of the stage falls in the measured dense scene and can rise
slightly where id order is already nearest-first. Measurements are in 20261012-006.

## 1. Why this change

`b3EvaluateContinuousCandidate` (`src/solver.c`) calls `b3ShapeTimeOfImpact` with the running best
fraction as the upper end of the sweep interval. Inside `b3TimeOfImpact` (`src/distance.c`) every
call first evaluates the distance at fraction 0 and, unless that is already a hit, advances towards
the running fraction; a convex candidate that is clearly separated at the interval's upper end
usually returns after one separation evaluation there, while a candidate that improves on it runs the
root finder to convergence. The order therefore sets how many candidates run to a root. Once the
nearest solid hit has been evaluated, later convex candidates beyond it are usually rejected cheaply. The benchmark
(20261012-004 §5) measured the swing at 10 to 20 percent between nearest-first and
farthest-first in a dense scene; ascending shape id, tree order and a hashed order all sit in between,
because none of them tracks distance.

So each query's candidates are sorted by an estimate of where along the sweep the fast shape first
reaches them. The estimate is the entry fraction of the fast centroid's segment into the candidate's
box grown by the fast shape's bounding radius: a slab test per candidate, against a time-of-impact
call that costs a distance query and a separation evaluation. A candidate whose grown box contains the
start centroid has key 0 and is evaluated first. The key only orders the candidates; it is a heuristic, not a bound, and no candidate is skipped.
A worse key costs time and never changes which candidates are evaluated. Three cases give one: a
rotating shape's centroid follows a curve, so the endpoint segment can miss a box the shape hits;
`b3RayCastAABB` treats a normalised direction component below `FLT_EPSILON` as parallel to the slab;
and a float box is coarser than the shapes it bounds where the float step at its position approaches
their size, which ties candidates at key 0 so that they are ordered by id.

Nothing else in the stage changes: the per-candidate filters, the accept tests, the sensor buffer and
its end filter, `preSolve`, the swept boxes and `fastShape->aabb` keep the sorted-candidate PR's
behaviour.

### Behaviour change, stated up front

The rollback guarantee is unchanged. The key is computed from state the two worlds of the proposition
share (the fast body's sweep relative to its own `center0`, and each candidate's `shape->aabb`), with
the same scalar float code on every worker, so equal worlds give equal keys, and the shape-id
tie-break makes the order total.

The change alters the order within a query, so relative to the sorted-candidate PR, and relative to
`main`, results are **not bit-identical** in the same cases that PR names: CCD sweeps with several
candidates in one query (impact fraction differs with no bound, `preSolve` is called for a different
set of candidates in a different order, the custom filter arrives in the new order, the eight sensor
hits kept can differ), the public CCD iteration counters. Where key order equals id order, nothing
changes. Any recording (`.b3rec`) or golden hash captured before this PR may stop validating in those
cases; the PR description says so in one line.

## 2. The change

**Files.** `src/solver.c`: `b3ContinuousContext`, `b3ContinuousQueryCallback`,
`b3EvaluateContinuousCandidates`, `b3SolveContinuous`. `src/physics_world.h`: the element type of
`continuousCandidates`.

- The candidate element becomes `{ float key; int shapeId; }`. Reserve, destroy and the memory-statistics
  code are unchanged; the element doubles from four to eight bytes, so the array's reported bytes
  follow the element size and the allocated capacity.
- Once per fast shape, before its queries: `r`, the distance from `centroid1` to the farthest corner
  of the fast shape's start box (`box1`, taken relative to `base`). Rigid motion keeps every point of
  the shape within `r` of its centroid for the whole sweep, whatever the shape type, where the stored box
  resolves the shape; `r` is a float estimate of that distance, not a strict bound.
- The tree callback applies the argument-only filters that `b3EvaluateContinuousCandidate` applies
  today (same shape, same body, sensor flags, shape filter, bullet, body filter) and drops a candidate
  that fails one. The custom filter and everything after it stay in the evaluation loop, in sorted
  order. For a candidate that passes, the callback computes the key: the candidate's `shape->aabb`
  relative to `base`, inflated on every side by `r`; `key` is the entry fraction of the segment
  `centroid1 -> centroid2` into that box from `b3RayCastAABB` (`src/aabb.h`); a segment that misses
  the box gives key 1. Any key that is not below 1, including NaN, is set to 1 before it is stored, so
  the sort never sees a value that compares unordered.
- The sort orders by ascending key, then ascending shape id. The evaluation loop is unchanged.
- The same key and order apply to all three queries. A kinematic or dynamic candidate's `shape->aabb`
  is whichever box it holds when the query runs: its end-of-step box, or the box at the fraction
  where continuous collision stopped it earlier in the stage. A static candidate's box is fixed
  through the step. The key is an estimate of the order in every case; the order is deterministic.

**Costs.** Per candidate: six subtractions, six additions for the growth, one slab test, and a
float-and-int comparison in the sort instead of an int. Per fast shape: one farthest-corner distance.
The measured cost and gain are in 20261012-006 §1, taken on a prototype whose key growth adds two
linear slops and a rounding term to `r` (20261012-006 §6).

**Tests.**
- The existing layout-independence fuzz (`ContinuousCandidateOrderTest`) passes unchanged.
- `ContinuousCallbackOrderTest` asserts the new order: a corridor whose boxes are created far-to-near
  so that ascending id and the sweep disagree, logging the custom-filter and `preSolve` calls and
  requiring ascending key, then id. Fails before the change.
- `ContinuousSensorSelectionTest` expects the eight nearest sensors by key, tie by id, with the
  sensors created in an order that differs from the sweep. Fails before the change.

## 3. Review checklist

- [ ] Diff touches only the files listed, plus tests and the docs sentence.
- [ ] The new tests fail on the sorted-candidate PR head and pass on the branch.
- [ ] Existing `test` target passes, including `test_determinism.c`, `test_tree_order.c` and
      `test_ccd_layout.c`.
- [ ] Benchmark delta quoted: `ccdbench.c` (20261012-004 appendix) in both travel directions, and the
      project benchmarks.
- [ ] PR text states the not-bit-identical-to-before consequence in one line.

## 4. Out of scope

- Skipping any candidate from its key. The key orders; it is not a bound.
- Narrow-phase and contact order. Contact pairs are already sorted by pair key and solved by colour;
  no running bound exists there, so no order is cheaper than another.
- Candidates that overlap the fast shape at the start of the step. Telling them apart earlier needs
  a distance query.
- `b3World_Explode` and the public queries, as in the sorted-candidate PR.

## 5. Open decisions for the owner

None.

## 6. Provenance

- **The key is the entry fraction of the grown box, not a projection onto the travel direction.** A
  projection is cheaper but misorders oblique approaches.
- **The bounding radius comes from the start box, not from `b3ComputeShapeExtent`.** The start box is
  already in hand for the swept query, and the distance to its farthest corner estimates the extent of every
  shape type the box resolves without per-type code.
- **The key box carries only the bounding radius.** The key orders and nothing depends on it being
  below a hit fraction, so no slop or rounding allowance is added.
- **No candidate is skipped from its key.** A skip needs the key to be at or below every fraction
  `b3TimeOfImpact` can report, and that depends on how the primitive reports a hit, which this change
  does not alter. The ordering alone takes the measured gain this change is for.
- **The key is computed in the collection callback, not after the query.** The same number of keys
  is computed either way; computing them as candidates arrive keeps the sort and the loop plain.
- **Exact distance is not used as the key.** A `b3ShapeDistance` call per candidate costs what the
  first distance evaluation of the time-of-impact call costs; the order gain cannot repay it.
