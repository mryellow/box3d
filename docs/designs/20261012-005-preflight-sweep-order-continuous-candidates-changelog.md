# Changelog: 20261012-005 preflight, sweep-ordered continuous candidates

## Revision 14 (2026-10-12)

Marked Canonical after the confirming clean round. No content change.

## Revision 13 (2026-10-12)

Folds round 13.

- The out-of-scope bullet on start-overlapping candidates no longer says what time of impact returns
  for them; it says only that telling them apart earlier needs a distance query.

## Revision 12 (2026-10-12)

Folds round 12.

- `r` is described as a float estimate of the shape's extent, not a strict bound; the key skips
  nothing, so nothing depends on it being one.

## Revision 11 (2026-10-12)

Folds round 10.

- The near-parallel behaviour of `b3RayCastAABB` is stated beside the rotation caveat: either can give
  a candidate key 1 and it is still evaluated.
- Key 0 means the start centroid is inside the grown box; the speculative-margin phrase is removed.
- The coarse-box limit no longer gives a fixed distance or a large-world comparison: the float box is
  absolute in both builds.
- Cheap rejection is stated as the usual convex outcome; the start-overlap bullet names solid convex
  candidates.
- Costs names the growth difference of the benchmark prototype.
- The key's three limits (rotation, near-parallel direction, coarse boxes) are one short statement
  instead of a paragraph, since each round had extended it.

## Revision 10 (2026-10-12)

Folds round 9.

- The introduction says the cost falls in the measured dense scene and can rise slightly where id
  order is already nearest-first.
- The coarse-box limit is stated for float mode (beyond about 16 km) as well as large-world builds,
  and the radius bound is qualified to boxes that resolve the shape.

## Revision 9 (2026-10-12)

Folds round 8.

- The cheap-return sentence applies to a convex candidate still separated at the interval's upper end.
- Reported memory follows allocated capacity, not the count held.
- Out of scope names solid candidates for the start-overlap case.

## Revision 8 (2026-10-12)

Folds round 7.

- Large-world builds: the float box is coarser than the shapes far from the origin, so candidates
  inside that coarseness tie at key 0 and are ordered by id; the gain is stated to apply where the
  boxes resolve the candidates.
- The cheap-rejection sentence is limited to convex candidates after the nearest solid hit.
- The memory statistics sentence says the accounting code is unchanged and the reported bytes follow
  the element size and count.
- Cost figures recomputed from 20261012-006 §3: the key between 1 percent under and 2 percent over
  the nearest-first order, 12 to 15 percent under the farthest-first order, and the machine drift
  stated against the margin over `main`. Then cut: the Costs paragraph keeps the work added and cites
  20261012-006 §1 for the measurements, since it restated that document and drew a finding in two
  rounds running.

## Revision 7 (2026-10-12)

Folds round 6.

- The key is described as a heuristic: a rotating shape's centroid follows a curve, so the endpoint
  segment can miss a box the shape hits and give key 1; the candidate is still evaluated.
- Costs figures are labelled as prototype measurements, total step time for non-bullet rows and
  continuous stage for bullet rows, with the prototype's key-before-filters difference and the bullet
  rows' result stated. "Of the stage" removed from the 10 to 20 percent swing, which 20261012-004
  measures as totals.

## Revision 6 (2026-10-12)

Owner decision: cut the static cull (B2) and land the minimal change. Folds round 4 by deleting what it
attacked.

- B2 deleted, with everything that existed only to support it: the lower-bound proof, the `M` and
  `FLT_EPSILON` growth, the two-slop growth, the `convexSolid` flag and its element growth, the
  results, custom-filter and sensor paragraphs of B2, its tests, and the commit sequence.
- The key box is the candidate's `shape->aabb` relative to `base`, grown by `r` alone, as in the
  prototype of 20261012-006. The key orders and is not a bound, so outward rounding and rounding
  allowances are gone; the NaN rule stays so the sort is total.
- The key-0 statement no longer depends on the candidate being static.
- Costs quote B1 against `main` (2 to 7 percent under) as well as against the other orders. The
  13 to 17 percent figure in 20261012-006 belongs to B1+B2 and is not claimed.
- Out of scope gains skipping any candidate from its key; Provenance gains why.
- Round 4 finding 2 (rounding proof) is moot: the proof is deleted.

## Revision 5 (2026-10-12)

Folds round 3 and the owner's decision to restrict B2 so its bound holds by construction.

- The key growth is `r + 2 * B3_LINEAR_SLOP + 8 * FLT_EPSILON * M`, with `M` the bounding radius
  plus the larger endpoint distance from the base. The two-slop term covers the 1.25-slop acceptance
  distance of `b3TimeOfImpact` whatever `B3_SPECULATIVE_DISTANCE` is set to; the epsilon term covers
  every rounding in the key for any sweep length. "Rounded outward", the rounding helpers and the
  hundred-kilometre limit leave the bound.
- B2 skips only static candidates the callback flags as convex solids; sensors, mesh, height-field
  and compound candidates are always evaluated. B2's sensor difference, its sensor test and the open
  decision are deleted; §5 holds nothing.
- The failed-return exception is stated once for any failed return, not per cause.
- A kinematic or dynamic candidate's box is described as whichever box it holds when the query runs.
- Out of scope gains a bound for mesh, height-field and compound candidates. Provenance gains the
  padding, rounding, convex-only and sensor decisions.

## Revision 4 (2026-10-12)

Folds round 2.

- B2's argument for solid results states its exception: a failed time-of-impact return (iteration cap)
  can lie below the key and was accepted before.
- B2 states that a skipped candidate gets no custom-filter call.
- Sensor paragraph corrected: only convex sensor candidates return the interval's upper end on a miss;
  mesh and height-field ones return fraction 0 and are admitted, so B2 changes their hits whatever the
  buffer holds. Behaviour-change paragraphs updated to match.
- The key-0 statement is qualified to static candidates in §1 and §6.

## Revision 3 (2026-10-12)

Folds round 1.

- The key box is grown by `r + FLT_EPSILON * segment length`. `b3RayCastAABB` treats a near-axis
  direction as parallel and would otherwise store key 1 for a candidate the sweep reaches.
- B2 no longer claims to change nothing observable: static sensors it skips no longer occupy the
  body's eight sensor entries, so the sensor hits kept can differ. Added to the behaviour-change
  paragraph, a B2 test, and the open decision on whether sensors are exempt from the stop.
- Time-of-impact cost wording scoped to convex candidates: one distance query and one separation
  evaluation.
- "Id order already nearest-first" reworded to "key order equals id order".
- Owner accepted the recommended options for the kinematic/dynamic key and the callback filters;
  both removed from the open list.

## Revision 2 (2026-10-12)

The note recorded after the benchmark becomes a full preflight. Changes from revision 1:

- The key is defined exactly: the candidate's `shape->aabb` relative to the sweep base, rounded
  outward, inflated by the fast shape's bounding radius taken from its start box; entry fraction of
  the centroid segment from `b3RayCastAABB`; miss and non-finite keys become 1; ties by shape id.
  Revision 1 named the centroid segment and `b3RayCastAABB` but left the growth and the tie rule
  open.
- B2 added: the static-tree loop stops at the first candidate whose key is at or beyond the running
  fraction. The bound argument (fat static boxes, linear centroid path, rigid motion) and the
  argument that nothing observable changes (strict accept test, sensor placeholder dropped by the
  end filter, counters are maxima over accepted candidates) are written out.
- The argument-only filters move into the collection callback; the custom filter stays in sorted
  order.
- Measurements moved to a supporting benchmark document (20261012-006), taken on a prototype of
  both changes before this revision was written: B1 reaches nearest-first in both travel
  directions at a 2 to 3 percent key cost, B1+B2 is 13 to 17 percent under `main` everywhere and
  removes a third of the time-of-impact calls.
- Tests named per change; B2 has no fail-before test and the preflight says so.
- The "revisit once the PR has landed" decision is replaced by open decisions for the owner.

## Revision 1 (2026-10-12)

Initial note recording the lead from 20261012-004 §5.
