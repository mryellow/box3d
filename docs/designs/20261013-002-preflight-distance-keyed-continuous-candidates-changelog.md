# Changelog: 20261013-002 preflight, distance-keyed continuous candidates

## Revision 3 (2026-10-13)

Marked Closed by the owner on the measurement in 20261013-003: one candidate per continuous query
on every project scenario, so the landing rule cannot be met and nothing is implemented. No content
change; no review round was run.

## Revision 2 (2026-10-13)

Folds the prototype measurement 20261013-003, taken before any review round.

- The required property no longer claims the whole `b3TOIOutput` bit for bit: the reported distance
  can differ in its last bit for a separated result, and the stage does not read it. Everything the
  stage reads was identical over 20,000 random sweeps.
- §1 cites the regime measurement: one candidate per query on every project scenario, so the order
  bound is zero there. The earlier sentence inferred this from the benchmark deltas; it is now
  observed.
- The checklist's first item points at the measurement instead of describing a test.

## Revision 1 (2026-10-13)

Initial draft. Records the lead that the time-of-impact call's own first GJK query could supply an
exact start distance for the order key. The lead as first stated, taking the distance from a
rejected candidate's output, does not work: the value exists only after the call it would order.
The draft therefore proposes the realisable form, a GJK pre-pass in the collection callback whose
simplex is handed into the time-of-impact call through a new `b3TOIInput` field, with the key as the
first conservative-advancement estimate `(d - target) / closing`. Non-convex candidates keep the
sweep key. The draft states the bound on what any order can gain (hit candidates per query, less
one), the regime where the sweep key misorders (boxes deep along the approach), and a landing rule
with the two scenes that measure cost and gain. Not yet backed by a prototype measurement; §3 asks
for one before round 1.
