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

## Revision 4 (after round 1)

- §10: two bullets added. Pair discovery today seeds dynamic self and dynamic-against-static and
  kinematic passes gated on moved bits, so a split needs an awake-against-non-awake cross pass and
  a moved non-awake proxy meets other non-awake proxies only through a non-awake self pass.
  Membership readers (sensors, world queries, bounds, draw) walk the trees by index and include the
  fourth.
- §10.2: layout independence qualified by the stateful-callback exception the cited designs carry;
  the explicit-rebuild entry carries `nodes`, `parents`, `proxies` and the scalars, since the
  rebuild rewrites all of them; the operation-log remove payload gains the parent index and the
  proxy slot, the free-list-or-bump bit moves to the insert payload, estimate 64 B plus 8 B per
  rotation.
- §10.3: moved bits named as state; their set and clear writes (`b3DynamicTree_ClearMoved` in the
  pair update) added to the writer list, all inside `broad_phase.c`.
- §10.5: the churn sentence states that move counts are shapes and gives shapes per flipped body
  (`rain` 1.2, `trees100` 22) instead of claiming one shape per body.

## Revision 5 (after round 2)

- §10 pair-discovery bullet: states that today a moved sleeping proxy is found against every kind
  of neighbour and a sleeping-sleeping contact goes to the disabled set, and lists every pass a
  split needs to reproduce that (awake self; awake against non-awake, static, kinematic; non-awake
  self and against static, kinematic when its root is marked moved). Waking on `SetTransform` named
  as the engine's alternative.
- §10 membership bullet: counters, capacity tracking, memory report and validation address trees
  by fixed index.
- §10.2 physical journal: growth of the `proxies` array is an entry undone by reallocating to the
  old capacity; proxy capacity is state (free list built from it, `b3Free` size goes to a user
  allocator), node capacity is not (no index changes, `nodeEnd` journaled).

## Revision 6 (after round 3)

- §10 pair-discovery bullet: non-awake cross passes run when either root is marked moved
  (`b3TestPair` accepts a moved node on either side); the self pass when the non-awake root is.
- §10: bullet added. The dynamic and kinematic rebuild is a task joined inside the solve, after
  begin-touch processing wakes sets, so under a split the engine joins it before contact state
  changes or defers the proxy moves past the join.
- §10 membership bullet: the step's reads add no order dependence beyond §5; public query
  callbacks and tied closest-hit casts see layout order.
- §10.2, without the order changes: Explode wakes inside its traversal, so this branch still
  defers the wakes until the query returns, without A3's sort.
- Series paused after round 3 (U-S1: three consecutive rounds with an H; U-S4: findings in §10
  three rounds running). Reported to the owner before any further round.
- Owner: continue the series to convergence despite U-S1 and U-S4 (U-S5). Round 4 sent at the
  same scope.

## Revision 7

- Owner: the split-tree analysis is a different subject from this collation. §10 moved to its own
  preflight, `20261013-001-preflight-awake-tree-split.md`, with its history (revisions 2 to 6 and
  rounds 1 to 3 here) as its starting point; §10 keeps a pointer and a three-line summary. This
  doc's series ends here; rounds 1 to 3 stand as records of the moved text.
