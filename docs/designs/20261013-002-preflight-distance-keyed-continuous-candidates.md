---
title: Preflight - distance-keyed continuous candidates
status: Closed
date: 2026-10-13
follows: 20261012-005 (sweep-ordered candidates), 20261013-001 (its benchmark)
---

# Preflight: distance-keyed continuous candidates

One change to Box3D, in one PR after the sweep-ordered candidate PR has landed, and only if the
measurement in §3 shows it earns its cost. The continuous stage keeps evaluating each tree query's
candidates in a deterministic, layout-independent, nearest-first order. For a convex candidate the
order key stops being the entry of the fast centroid's segment into a grown box and becomes the
first conservative-advancement estimate of the impact fraction: the GJK distance between the two
shapes at the start of the sweep divided by their closing speed along the separating normal. The
GJK query that produces it is the one `b3TimeOfImpact` would run first anyway; its simplex is handed
into the time-of-impact call so the distance is computed once.

## 1. Why this change, and what it can and cannot pay back

`b3TimeOfImpact` (`src/distance.c`) begins every call with a GJK distance query at fraction 0 from a
zero simplex cache. If that distance is above the target, it builds a separation function from the
resulting simplex and evaluates the minimum separation at the upper end of the sweep interval. A
candidate separated there returns `b3_toiStateSeparated` at once; one that is not runs the root
finder. The upper end of the interval is the running best fraction, so the order in which a query's
candidates are evaluated decides which of them reach the root finder.

Two facts about that structure bound what any order can gain:

- A candidate the sweep never reaches costs one GJK query and one separation evaluation whatever
  the interval is. No order makes a rejection cheaper.
- A candidate the sweep does reach is cheap only when the running fraction is already below its
  impact fraction. The gain of an order is therefore bounded by the number of candidates per query
  that the fast shape would hit, less one.

On the project benchmarks every continuous query collects one candidate (20261013-003 §3), so the
bound is zero there: the sorted-candidate PR and the sweep-ordered PR measure as noise on them, and
no key can pay back on them. The place an order pays
is a query with several hit candidates, as in `ccdbench.c` (20261012-004 appendix). There the sweep
key already orders hit candidates nearly as the impact fractions do, because every candidate is an
axis-aligned box of one size on a straight path. The sweep key misorders where a candidate's box is
much larger than the candidate along the approach: hulls rotated off the axes, elongated hulls and
capsules approached obliquely, a thin plate seen edge-on. For those the segment enters the grown box
well before the shape is reached, and the key is too small by the depth of the box.

The distance key does not have that error: GJK measures the shape, not its box. It costs more than a
slab test, so the change pays back only where the box error is common. §3 states the measurement
that decides.

### Behaviour change, stated up front

The rollback guarantee is unchanged. The key is computed from state the two worlds of the
proposition share (both sweeps relative to the fast body's `center0`, both shapes' geometry), with
the same scalar code on every worker, and the shape-id tie-break makes the order total.

The time-of-impact result for a candidate is identical whether its GJK simplex is handed in or
built cold (§2). So, relative to the sweep-ordered PR, results differ only where the two keys order
a query's candidates differently, in the cases the sorted-candidate PR names: CCD sweeps with several
candidates in one query (impact fraction differs with no bound, `preSolve` is called for a different
set of candidates in a different order, the custom filter arrives in the new order, the eight sensor
hits kept can differ), and the public CCD iteration counters. Where the orders agree, results are
**bit-identical** to the sweep-ordered PR. Any recording (`.b3rec`) or golden hash captured before
this PR may stop validating in those cases; the PR description says so in one line.

## 2. The change

**Files.** `include/box3d/types.h`: `b3TOIInput`. `src/distance.c`: `b3TimeOfImpact`.
`src/shape.h`, `src/shape.c`: `b3ShapeTimeOfImpact`. `src/solver.c`: `b3ContinuousContext`,
`b3ContinuousQueryCallback`, `b3EvaluateContinuousCandidate`. `src/physics_world.h`: the element
type of `continuousCandidates`.

- **The primitive accepts a simplex.** `b3TOIInput` gains a `b3SimplexCache cache` field.
  `b3TimeOfImpact` starts its first distance query from it instead of from a zero cache. A zero
  field is the cold start the function performs today, so every existing caller that
  zero-initialises its input is unchanged in behaviour; `b3ShapeTimeOfImpact`'s convex path, which
  does not, sets the field. `b3ShapeTimeOfImpact` gains a `const b3SimplexCache*` parameter that its
  convex path copies into the input and its compound, mesh and height-field paths ignore; `NULL`
  is the cold start.
- **Required property: a handed-in simplex changes nothing the stage reads.** When the cache holds
  the simplex a cold GJK query terminated with, at the same relative transform, `b3ShapeDistance`
  rebuilds that simplex from the same vertices, finds the same metric, solves it and terminates on
  its first iteration. The state, fraction, point, normal and iteration counts `b3TimeOfImpact`
  returns are the cold call's bit for bit; its reported distance can differ in the last bit for a
  separated result, because the rebuilt simplex is re-solved, and the continuous stage does not
  read that field. Observed over 20,000 random sweeps in 20261013-003 §1. This is the property that
  keeps the behaviour change in §1 to order alone.
- **The candidate element** becomes `{ float key; int shapeId; b3SimplexCache cache; }`. Reserve,
  destroy and the memory-statistics code are unchanged; the reported bytes follow the element size.
- **The key, computed in the collection callback.** The callback applies the argument-only filters
  as today. For a candidate that passes, it branches on the candidate's shape type once:
  - *Convex (sphere, capsule, hull), solid or sensor:* the candidate's sweep is
    `b3MakeRelativeSweep` of its body sim against the same base as the fast shape's; both sweeps are
    evaluated at fraction 0 with `b3GetSweepTransform`, the GJK input is the relative transform of
    the fast shape in the candidate's frame with `useRadii` false, the proxies are
    `b3MakeShapeProxy` of each shape, and the query runs from a zero cache into the element's
    `cache`. With `d` the distance returned, `target` the acceptance distance `b3TimeOfImpact`
    derives from the two proxy radii, `n` the returned normal rotated into the base frame, and
    `v` the fast shape's centroid displacement over the sweep less the candidate's centre
    displacement, the closing speed is `-dot(v, n)` and the key is `(d - target) / closing`.
    `d <= target` gives key 0. A closing speed of zero or less, and any key that is not below 1,
    including NaN, gives key 1. The key is an estimate of the impact fraction, not a bound: it
    ignores rotation of either shape and treats the normal as fixed over the sweep.
  - *Compound, mesh, height field:* the sweep key of the sweep-ordered PR, unchanged, and a zero
    `cache`. These shapes have no single proxy; their time-of-impact calls build their own.
  Both keys estimate the same quantity, the impact fraction, so one sort orders them together.
- **The evaluation loop** passes the element's `cache` to `b3ShapeTimeOfImpact`. Nothing else in it
  changes: the custom filter, the accept tests, the sensor buffer and its end filter, `preSolve`,
  the swept boxes and `fastShape->aabb` keep the sweep-ordered PR's behaviour.
- The same key and order apply to all three queries. No candidate is skipped from its key.

**Costs.** Per convex candidate, against the sweep-ordered PR: the cold GJK query moves from the
time-of-impact call into the callback and is not repeated; the time-of-impact call's first query
becomes a warm start that terminates on its first iteration, which is one simplex solve and one
support search on each proxy; one quaternion rotation, one dot product and one division for the key;
twelve more bytes per element; the slab test of the sweep key is no longer done. Per non-convex
candidate: twelve more bytes per element. The order gain is bounded as §1 states. Both are measured
in §3; neither is claimed here.

**Tests.**
- `b3TimeOfImpact` with the cache a cold `b3ShapeDistance` query at fraction 0 wrote returns the same
  state, fraction, point, normal and iteration counts, with exact float equality, as the cold call,
  over sweeps that hit, separate, overlap and fail. Fails if the required property above does not
  hold.
- The existing layout-independence fuzz (`ContinuousCandidateOrderTest`) passes unchanged.
- `ContinuousCallbackOrderTest` asserts the new order: a corridor of thin hulls rotated so that
  their boxes are deep along the approach and their shapes are reached in the opposite order to
  their box entries, logging the custom-filter and `preSolve` calls and requiring ascending key,
  then id. Fails on the sweep-ordered PR head.
- `ContinuousSensorSelectionTest` expects the eight nearest convex sensors by key, tie by id, with
  the sensors placed so that box entry and distance disagree. Fails on the sweep-ordered PR head.

## 3. Review checklist

- [ ] The required property of §2 is confirmed by observation on a prototype before round 1
      (20261013-003 §1).
- [ ] Benchmark deltas quoted against the sweep-ordered PR head and `main`, same machine protocol
      as 20261013-001: `ccdbench.c` in both travel directions, where the two keys agree and the
      branch measures only its cost; and a variant of it whose static field is thin hulls rotated 45
      degrees about the vertical, where the box key misorders and the branch measures its gain.
- [ ] Landing rule: the branch is within the run's spread of the sweep-ordered PR in the
      axis-aligned scene and under it by more than the spread in the rotated scene. Failing either,
      this document is set to Closed with the figures recorded, and nothing is implemented.
- [ ] Diff touches only the files listed, plus tests and the docs sentence.
- [ ] The new tests fail on the sweep-ordered PR head and pass on the branch.
- [ ] Existing `test` target passes, including `test_determinism.c`, `test_tree_order.c`,
      `test_ccd_layout.c` and `test_distance.c`.
- [ ] PR text states the not-bit-identical-to-before consequence in one line.

## 4. Out of scope

- Skipping any candidate from its key. The key orders; it is not a bound.
- A distance key for compound, mesh and height-field candidates.
- Any memory of distances or simplices across steps or across queries.
- The hit path of `b3TimeOfImpact` and how it reports a hit.
- `b3World_Explode` and the public queries.

## 5. Open decisions for the owner

None. The landing rule in §3 decides whether the change is implemented.

## 6. Provenance

- **The distance comes from a pre-pass with the simplex handed on, not from the time-of-impact
  output.** `b3TOIOutput.distance` of a separated candidate is its start distance, but it exists
  only after the call it would have ordered, and it says nothing about the other candidates.
- **No distance is kept across steps.** A per-pair memory of last step's distance would be a side
  table over the shape population that the rollback state would have to carry or rebuild; the key
  is derived from live state at the point of use.
- **The GJK query is not run twice.** A `b3ShapeDistance` call per candidate that left the
  time-of-impact call to start cold would cost a second cold query per candidate; handing the
  simplex on makes the second query one iteration.
- **The key divides by closing speed; it is not the bare distance.** A candidate beside the path is
  near but never reached; the bare distance would rank it ahead of a farther candidate on the path.
  Ranking rejected candidates costs nothing, but ranking a hit candidate behind one is the whole
  cost the order exists to avoid.
- **The cache is a field of `b3TOIInput`, not a parameter of `b3TimeOfImpact`.** A zero field is
  the cold start, so the public callers that zero-initialise their input need no change and the
  function keeps its signature.
- **Non-convex candidates keep the sweep key rather than key 0.** Both keys estimate the impact
  fraction, so the sort stays one comparison with one meaning; key 0 would put every terrain mesh
  first with the whole interval open.
- **The landing rule is in the document.** The sweep-ordered PR's gain was measured in one scene
  and did not appear elsewhere; this change states before it is built where it must win and where
  it may not lose.
