# Changelog: 20261013-005 preflight, lazily keyed continuous candidates

## Revision 1 (2026-10-13)

Initial draft. Follows the regime measurement in 20261013-004: one candidate per continuous query on
every project scenario. Defers the sweep key, the bounding radius and the sort to a pass after the
query that runs only for two or more candidates, so the sweep-ordered PR's cost on the common path
becomes zero by construction. No behaviour change; results bit-identical to the sweep-ordered PR.
