# Changelog: 20261013-005 preflight, lazily keyed continuous candidates

## Revision 1 (2026-10-13)

Initial draft. Follows the regime measurement in 20261013-004: one candidate per continuous query on
every project scenario. Defers the sweep key, the bounding radius and the sort to a pass after the
query that runs only for two or more candidates, so the sweep-ordered PR's cost on the common path
becomes zero by construction. No behaviour change; results bit-identical to the sweep-ordered PR.

## Revision 2 (2026-10-13)

Scoped the one-candidate premise to what is measured: junkyard averages 1.09 candidates per query, so
the zero-keys, zero-sorts requirement holds per query with fewer than two candidates, not per
scenario. The §3 counting check now also requires the sweep-ordered keys and order on every query
with two or more candidates. The §1 citation moved to 20261013-003 §3, where the regime figures are.

## Revision 3 (2026-10-13)

Stated why a collected candidate's box is stable between the query and the evaluation: non-bullet
queries see only static shapes and bullet queries drop bullet candidates in the callback, so no
worker writes a collected candidate's box. Counted the one-candidate figure over non-empty queries,
which is what the measurement counts.

## Revision 4 (2026-10-13)

Marked Canonical after the confirming clean round. Gates re-run against the four acceptance
criteria: the change deletes work on the common path and adds no structure, check or
reconciliation. No rule's cost in this series exceeded what it caught.
