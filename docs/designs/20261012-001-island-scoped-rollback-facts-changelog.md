# Changelog: 20261012-001 island-scoped rollback facts

## Revision 2

- §1: added the thread's split-tree remarks (serialize the awake tree whole; an island that slept
  and is rolled back to awake must leave the non-awake tree with that tree's order preserved).
- §10 added: split awake and non-awake trees, first analysis (image the awake tree, physical node
  journal for the non-awake tree, writers, effect on the rewind-only design, open questions).

## Revision 3

- §10 rewritten from source. Opens with what the source does today: sleep and wake touch no tree
  (`solver_set.c`); the proxy key has a spare type code but the code also selects the margin; pair
  discovery is layout-independent while CCD reaches sleeping bodies and Explode wakes them, so the
  non-awake tree is read in order; the step's rebuild rewrites every node and is triggered by any
  out-of-order insert.
- §10.1: lists exactly what the awake-tree image holds; capacity is state because the free list is
  imaged, not replayed.
- §10.2 split into two cases: with the section 5 order changes the non-awake tree needs only the
  proxy-identity journal and the thread's order-preservation requirement does not arise; without
  them every cold tree needs its exact layout back. Physical journal kept; added the constraint
  that the non-awake tree is never rebuilt by the step (a rebuild is an O(tree) entry), the
  kinematic tree's treatment, and the operation-log variant with the tight-union condition it
  depends on. Dropped the list of `dynamic_tree.c` primitives in favour of the one write accessor.
- §10.3: writer list verified against `body.c`, `shape.c`, `joint.c`; added `SetType` and the
  joint collide-connected moved-bit write; the static rebuild sentence moved to §10.2.
- §10.5: open question 1 (does anything but pair discovery read non-awake order) answered from
  source and removed; question 3 (churn) replaced by a measured table from a scratch harness that
  scans every non-static body per step through the public API on the benchmark scenes; noted that
  the section 7 journal figure is a `junkyard` estimate with no sleep churn in it.
