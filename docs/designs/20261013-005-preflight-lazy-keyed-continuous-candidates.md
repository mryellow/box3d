---
title: Preflight - lazily keyed continuous candidates
status: Draft
date: 2026-10-13
follows: 20261012-005 (sweep-ordered candidates), 20261013-002 (distance key, Closed), 20261013-004 (measurement)
---

# Preflight: lazily keyed continuous candidates

One change to Box3D, in one PR after the sweep-ordered candidate PR has landed. The continuous stage
keeps evaluating each tree query's candidates in ascending sweep key, then shape id, exactly as that
PR orders them. What changes is when the key is computed: the collection callback stores only the
shape id, and the key, the bounding radius and the sort are computed after the query, and only when
the query collected two or more candidates. A query with one candidate evaluates it directly.

## 1. Why this change

The order of a query's candidates only matters when there are at least two of them. On every
project scenario each continuous query collects one candidate (20261013-004 §1), so on those scenes
the sweep-ordered PR computes a bounding radius per fast shape, and a relative box, an inflate and a
slab test per candidate, for an order that is never consulted. The measured cost is inside the
run-to-run spread (20261013-001 §4 and §6); this change makes it zero by construction rather than
small by measurement, and leaves the gain of the order where it exists, in queries with several
candidates. It is a deletion of work on the common path, not a new mechanism.

### Behaviour change, stated up front

None. For a query with one candidate the sorted order and the evaluation order are the same thing.
For a query with two or more, the keys are the same values computed from the same inputs
(`shape->aabb`, the fast shape's start box and centroid segment, all unchanged between the end of the
query and the start of evaluation, since the stage mutates only `fastShape->aabb` before the query),
so the order is the sweep-ordered PR's order. Results are **bit-identical** to the sweep-ordered PR.
The rollback guarantee is unchanged.

## 2. The change

**Files.** `src/solver.c`: `b3ContinuousContext`, `b3ContinuousQueryCallback`,
`b3EvaluateContinuousCandidates`, `b3SolveContinuous`.

- The callback applies the argument-only filters as today and pushes the element with the shape id
  and no key. The element type and the per-worker array are unchanged.
- `b3EvaluateContinuousCandidates` branches once on the count. One candidate: evaluate it. Two or
  more: compute the fast shape's bounding radius if this is the first query of the fast shape to
  need it, compute each element's key from its shape's box exactly as the sweep-ordered PR's
  callback does, sort, evaluate. Zero: return.
- The bounding radius is computed from the start box relative to the base, as today, but at the
  point of first need rather than before the first query. The fast shape's start box is already
  held for the swept query.
- The three queries of a bullet each decide for themselves from their own count.

**Costs.** Per candidate of a one-candidate query: the shape load that the second pass would have
done is the one the evaluation does anyway, so nothing is added; the relative box, the inflate and
the slab test are removed. Per candidate of a multi-candidate query: one extra shape load against
the sweep-ordered PR, since the key pass and the evaluation each load the shape. Per fast shape with
no multi-candidate query: the farthest-corner distance is removed.

**Tests.** The sweep-ordered PR's order tests (`ContinuousCallbackOrderTest`,
`ContinuousSensorSelectionTest`) and the layout-independence fuzz pass unchanged: the order is the
same. The change is not observable through the public API, so it has no fail-before test; its
evidence is the measurement in §3.

## 3. Review checklist

- [ ] Counting build shows zero keys computed and zero sorts on every project scenario, and the
      same keys and order as the sweep-ordered PR head on `ccdbench.c`.
- [ ] Project benchmarks against the sweep-ordered PR head, interleaved pairs, single worker: within
      the run's spread everywhere. The change claims no gain; it claims no cost.
- [ ] `ccdbench.c` in both travel directions against the sweep-ordered PR head: within the spread,
      since every query there has many candidates and the work is the same plus one shape load.
- [ ] Existing `test` target passes.
- [ ] Diff touches only `src/solver.c`.

## 4. Out of scope

- The key itself, the order it gives, and any skip from it.
- The fast-body classification that decides how many queries run.
- The mesh, height-field and compound time-of-impact paths (20261013-007).

## 5. Open decisions for the owner

None.

## 6. Provenance

- **The threshold is two candidates, not a count of hits.** The order exists for hit candidates,
  but which candidates hit is unknown before evaluation; two collected candidates is the first
  count at which the order can matter.
- **Keys are computed in a pass after the query, not in the callback with a flag to skip the sort.**
  Keying in the callback is the cost this change removes; a flag would keep it.
- **The bounding radius moves to first need rather than staying before the first query.** It exists
  only for the key; on a fast shape whose queries hold one candidate each it was computed for
  nothing.
