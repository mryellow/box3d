# Changelog: 20261013-001 preflight awake tree split

## Revision 1

- Created from §10 of `20261012-001-island-scoped-rollback-facts.md` (its revisions 2 to 6, after
  three review rounds there), reframed as a stand-alone engine preflight: §1 what the step does
  and what the split changes, §2 changes B0 to B4 in two PRs, §3 tests, §4 measured churn, §5 what
  the split gives the rewind design (awake image, non-awake treatment with and without the
  tree-order changes, journal coverage, changes to the rewind design), §6 owner decisions, §7 out
  of scope.

## Revision 2 (after round 1)

- §1: the O(awake) claim restated: holds on steps with no sleeping-proxy write; such a write costs a
  gather over the non-awake tree; the sensor pass is unchanged.
- B0: the two proxy-type guards named; they check a four-code proxy range.
- B2: sleep migration carries the leaf's moved bit (finalize marks in the same solve that sleeps;
  `SetTransform` then `SetAwake(false)` between steps); ordinary non-awake creates stay marked.
- B3: bullet CCD and Explode query both dynamic trees; Explode processes the combined list after
  both traversals.
- 5.1: image bound stated as the high-water mark (`proxyCapacity` never shrinks, `nodeEnd`
  compacts only in a rebuild), with the consequence for the rewind design's requirement 2.

## Revision 3 (after round 2)

- Intro: scaling claim limited to gather, rebuild and refit; the cross pass costs what the single
  tree's self pass costs for the same overlaps.
- B0: `b3ValidateNoMoved` added to the fixed-index readers.
- B2: migration goes through a create that takes the old leaf's moved bit, in both directions; the
  "woken pairs re-found" rule deleted; shape creation must select the tree from the body's set
  (today `body->type`); enable needs no change.
- §4: why endpoint scans count every transition (declined finding; reasoning also in §8).
- 5.3: direct tree writers outside `broad_phase.c` named, with the rewind design's section 7.4
  moving them.
- 5.4: the state hash is logical for a derived non-awake layout, physical under the byte journal.
- §8 Provenance added.

## Revision 4 (after round 3)

- §1: the "results do not depend on which tree" claim carries the stateful-callback exception.
- B0: static assert, destroy loop and recording-bounds walk named; every tree walk uses a count of
  four; the capacity hint goes to the non-awake tree and the awake tree starts at the floor.
- B1: the pair update's early return tests the non-awake root's moved bit.
- §4: a move is O(height), not O(log n); the PR measures the non-awake tree's height under churn.
- 5.1: the high-water bound starts at the floor, not the capacity hint.

## Revision 5 (after round 4)

- Intro and §4: the cross passes, awake, kinematic or static against non-awake, cost what the
  single tree's passes cost for the same overlaps; the saving is gather, rebuild and refit.

## Revision 6 (after round 5)

- B3: public queries hold the lock for the traversal, since a callback's `SetAwake` would migrate
  leaves in the tree being walked; decision 5 added (lock, or defer migrations).
- 5.4: "free lists must come back exactly" replaced by the id-sequence property and why undo in
  reverse gives it for the journaled tree (declined finding; reasoning also in §8).
- §8: proxy-pool growth needs no entry.

## Revision 7 (after round 6)

- B3: debug draw added to the locked traversals; the lock refuses `SetAwake` at runtime and
  `SetTransform` only in a validation build, so a callback's `SetTransform` is today's hazard,
  unchanged.
- §3: the explosion test's velocity equality is conditional on the tree-order changes.
- 5.2 and 5.4: the byte journal subsumes the proxy-identity entries for the trees it covers.
- 5.3: the two inline moved-bit setters in `broad_phase.h` named; out of line under the byte journal.

## Revision 8 (after round 7, with owner decisions)

- Owner: the split applies to the kinematic tree too (decision 1 settled, reasoning in §8); the
  branch base and whether the tree-order PR is applied first belong to the Implementation Prompt
  (decision 4 removed, §1 says so). Decisions renumbered 1 to 3.
- Whole doc: five trees (static, awake and non-awake kinematic, awake and non-awake dynamic); the
  proxy key's tree field widens to three bits; B1 lists the passes for five trees and the early
  return's tests; B3 bullet CCD queries all four kinematic and dynamic trees; B4 covers both
  non-awake trees; 5.1 images both awake trees; 5.2 drops the kinematic-tree caveat.
- B2: after a sleep migration the awake tree's root is marked moved so the rebuild compacts
  `nodeEnd`; an emptied tree is reset. 5.1's bound restated: nodes compact next step, the
  `proxies` array keeps its high-water mark (240 KB for ten thousand slots).
- 5.2: the operation-log remove payload names the proxy slot's 24 bytes.

## Revision 9 (after round 8, owner reversal)

- Owner: the round 7 "No" meant the kinematic tree need not split; imaging it is fine. Doc
  returned to four trees (static, kinematic, awake dynamic, non-awake dynamic): two-bit key,
  revision 7's passes, B4 for one tree, 5.1 one image, 5.2 states the kinematic tree is imaged in
  the non-tree-order branch. Reason in §8.
- §1: rebuild and refit described separately (refit scans to `nodeEnd`, writes marked internal
  AABBs only).
- Intro, B2, 5.1: the first step after a sleep still gathers to the old `nodeEnd` (gather precedes
  the enqueue), and a capture before that step is about 960 KB for a tree that fell from ten
  thousand proxies to ten; 240 KB from the capture after it.

## Revision 10 (after round 9)

- B2 and 5.1: the compaction trigger after a sleep migration is clearing `dfsOrdered`, not
  marking the root, which would mark a lone surviving proxy; an unordered tree with an unmoved
  root is rebuilt densely as one retained subtree.
- B4: files gain `physics_world.c` and `box3d.h`; the explicit rebuild is a public
  `b3World_RebuildSleepingTree` beside `b3World_RebuildStaticTree`.

## Revision 11 (after round 10)

- B3 rewritten as "Migration timing": one rule, the migration choke point queues while
  `world->locked` is set and the queue drains on unlock and after the step's tree join (before
  finalize and CCD); lock and unlock are one pair of functions; public walks and draw hold the
  lock. Covers the rebuild race, Explode and every callback wake, including `b3WakeBodyWithLock`
  whose check is assert-only. Decisions 2 and 3 removed; §8 bullet added.
- §3: the wake test also covers a wake from an overlap-query callback.

## Revision 12 (after round 11, with owner decision)

- Owner: the non-awake passes run; `b3Body_SetTransform` on a sleeping body is unchanged. Decision
  1 removed; §6 now empty; B1 states it; §8 bullet added.
- B3: the rule moves off `world->locked` to a traversal depth raised by one entry and exit pair
  around every walk with host callbacks and by the step; the queue holds body ids with
  generation; the drain, at the outermost exit and after the tree join, re-derives each proxy's
  tree from the body's set and type, so a proxy replaced by `SetType` is found in place. Explode
  queries both trees inside one entry. §8 bullet rewritten.
- B4: the sleeping-tree rebuild sits beside the static one in `physics_world.h`; neither is public.
- 5.2 and 5.4: the tree-order branch keeps the rewind design's moved-bit restore for the non-awake
  tree, since an ordinary insert marks the restored leaf moved.

## Revision 13 (after round 13)

- B0: the proxy-tree check moves from `b3ValidateSolverSets` (runs inside `b3WakeBody`, before a
  drain) to `b3ValidateBroadPhase` (after the drain).
- B2: an emptied awake tree resets node state only; proxy slots, capacity and free-list order are
  retained.
- B4 and 5.3: both explicit rebuilds call a broad-phase rebuild function.
- 5.2 and 5.4: the tree-order branch gives the non-awake tree exactly the rewind design's section
  7.4 treatment (identity journal, restored-fat-AABB move pass, moved-bit restore); the split
  removes that treatment for the awake tree only.

## Revision 14 (after round 14)

- Intro and §4: cross passes reach the same leaves; node visits depend on layout and are measured.
- B1: the non-awake-against-static pass uses the static-compound expansion and flushes before the
  non-static passes, as the awake one does.
- B3: the traversal entry and exit wrap the whole public call, not each tree walk.

## Revision 15 (after round 15)

- B4: the non-awake tree inherits the static tree's reliance on rotation-balanced height against
  the 512-entry traversal stacks; no new mechanism (declined part in §8).
- §4 labelled migration churn; 5.2 says the byte journal is also charged for the other sleeping
  body writes in B2's list, which §4 does not measure.

## Revision 16 (after round 16)

- B2 and 5.3: one create point; `b3BroadPhase_CreateProxy` takes the moved mark as a parameter and
  the migration passes the carried bit, so the proxy-identity entries inside create and destroy
  cover it; `proxyKey` written through the shape accessor.
- B4: the height the PR reports is the evidence; §4 does not contain the measurement.

## Revision 17 (after round 17)

- §2: B0 to B3 are the first PR, B4 the second.
- 5.2: per-path cost; a migration is one path in the journaled tree, a sleeping-body move two,
  rotations add swaps; bytes per move measured under §4's churn.
- 5.4: the appended-slots argument is the identity-journal branch's; the byte journal restores the
  old capacity.

## Revision 18 (after round 18)

- B3 and 5.1: the sleep pass's migrations drain at the step's exit before `b3World_Step` returns;
  a capture, taken after, sees an empty queue.
- §8: stack-reliance bullet extended with the rewind design's own derived static layout (declined
  re-raise).

## Revision 19 (after round 19)

- §4: the method counts dynamic bodies; a run counting kinematic bodies too gave identical flip
  counts. Table unchanged.

## Revision 20 (U-T6 split)

- §5 moved to `20261013-002-awake-tree-split-rewind.md` with the two Provenance bullets about the
  journaled tree; §5 keeps a three-line pointer so §6 to §8 keep their numbers. 420 -> 301 lines.

## Revision 21 (after round 20)

- B4: the non-awake tree differs from the static tree in being churned; root height is reported by
  the counters and reset by the sleeping-tree rebuild; stacks assert when full in validation
  builds. §8 bullet added (no height bound or overflow-safe traversal).

## Revision 22 (after round 21)

- B0: `types.h` in files; `sleepingTreeHeight` counter beside `treeHeight`, which is the maximum of
  the dynamic and kinematic heights.
- B1 and B3: `b3World_CollideMover` and `b3World_CastMover` named among the readers and the guarded
  public calls.

## Revision 23 (after round 22)

- §1: collide and solve are O(awake), not "the rest of the step".
- B0: `b3ValidateSolverSets` described as it is; the proxy-tree check is a world-level end-of-step
  validation after the exit drain.
- B3: the mid-step drain is at the join point whether or not a tree task was enqueued.
- B4: creates rotate, moves do not; measurements include repeated sleeping-body transforms.

## Revision 24 (after round 23)

- §1: without the tree-order changes, every bullet sweep and explosion can differ, awake or
  asleep candidates alike, since the awake tree's layout after migrations is not the single
  tree's.
- Intro and B2: the old-`nodeEnd` scan on the first post-sleep step applies when the awake tree
  stays non-empty.

## Revision 25 (after round 24)

- B4 and §8: the stack figures corrected: single-tree walks fill past height 500, the cross pair
  walk near 170; the static cross pass walks the static tree with that stack today.
- Series paused by the owner after this revision. 24 rounds, not converged; see the convergence
  summary in round 24's record and the pattern notes in rounds 10, 11 and 20.

## Revision 26 (owner decision, series resumed)

- Owner decision on B4's open question (rounds 15, 18, 20, 22, 24): the step does not stop
  rebuilding the non-awake tree; it rebuilds it at a chosen time. B4 rewritten: the pair update
  tests the non-awake root height against `B3_SLEEPING_TREE_MAX_HEIGHT` (`dynamic_tree.h`,
  initially 64) before its early return and before any walk, and rebuilds the tree in full on the
  calling thread when it is exceeded; a full build keeps leaf moved bits; every walk in the step
  sees a bounded height because migrations queue while traversals are open. The explicit
  `b3World_RebuildSleepingTree` is dropped; the rebuild function in `broad_phase.c` is the one
  write site. Threshold set against the cross walk's fill; the PR reports height, node visits and
  triggered-rebuild count.
- §3: test driving the tree past the threshold at tree and world level.
- §8: the stack-reliance bullet replaced by the triggered-rebuild decision and its reasons.
- Rewind note `20261013-002` updated in step (its revision 2).

## Revision 27 (after round 25)

- B4: the height test moves from the pair update to the insert: `b3BroadPhase_CreateProxy` and
  `b3BroadPhase_MoveProxy` test the non-awake root height after each insert into that tree and
  rebuild in full when it is exceeded, so public queries between steps see a bounded tree too.
- B4: the full build's rule stated (`b3PartitionMid`, midpoint of the centers' extent on the
  longest axis); the separation ratio a chain of isolating splits needs; the repeated-trigger
  case and its cost; the stack assert as the backstop for a layout the build cannot bound.
- §3: the test's layout evenly spaced; the assertion after every write, including between steps.
- §8: the layout-relative trigger considered and not added, with the reason.
- Rewind note `20261013-002` updated in step (its revision 3): the trigger fires on a write.

## Revision 28 (after round 26)

- Intro: the cost condition is no write to or moved mark on a sleeping body's proxy
  (`b3Joint_SetCollideConnected` marks without writing).
- B4: the insert-time height test is skipped while the traversal depth is nonzero; the exit
  drain tests once after its inserts; the callback `b3Body_SetTransform` hazard stays today's
  with no rebuild added. The cross pair walk's two stacks described (pair stack bounded by the
  shallower tree; leaf handoff to `b3CollideProxyAndSubtree`); the threshold's basis is the
  single-walk limit.
- B0: `sleepingTreeRebuildCount` in `b3Counters`; B4 and §8 cite it.

## Revision 29 (after round 28, confirmation round)

- B2: Files line lists `solver_set.c`, `shape.c`, `broad_phase.c`, `broad_phase.h`; the
  moved-mark parameter and shape creation's tree selection land in B2's commit.
- §1: the guarantee is the step's and Explode's results; public query callback order and tied
  closest-hit casts see layout order, as B1 states.

## Revision 30 (after round 29)

- B4: the outermost exit runs the height test once, after the drain, whether or not anything
  was queued, so a callback move's insert is tested; §8 follows.
- B4: stack figures stated as 64 of 512 for single walks, 192 of 512 for the pair stack with
  both trees at 64; the threshold is a trigger, a bound only where the full build reaches it.

## Revision 31 (after round 30)

- B1: the non-awake self pass gathers moved sibling pairs by descending marked internal nodes
  from the root, O(marked), not by the scan to `nodeEnd`; the awake tree keeps the scan.
- B2: the empty-tree reset is stated for either tree the migration empties.
- B0 and B3: the traversal depth and `sleepingTreeRebuildCount` live in `b3BroadPhase`, where
  B4's hooks read them.

## Revision 32 (after round 31)

- B4: on a layout the full build leaves above the threshold, the cost is one O(sleeping
  proxies) rebuild per inserted shape, stated as such.

## Revision 33 (after round 32)

- B1: `b3DynamicTree_ClearMoved` on the non-awake tree runs in the pair update after its passes,
  in the first PR; moved out of B4.
- B4: the rebuild's gather and build walks push with no release guard, today's primitive; the
  full build's height bounds them; a guard is outside this design.

## Revision 34 (after round 34, confirmation round)

- §2 PR plan: B0 to B4 are one PR of five commits; until B4's commit the step rebuilds the
  non-awake tree on order, one line in `b3UpdateTreesTask` that B4 removes.

## Revision 35 (after round 35)

- §2 PR plan: B2 and B3 are one commit; the PR is four commits, B0, B1, B2 with B3, B4.

## Revision 36 (after round 37, confirmation round)

- B3 and B4: the outermost exit lowers the depth to zero before it drains, so each drain
  insert is height-tested; the mid-step drain inserts into the awake tree only; one more test
  after the drain covers a write made under the walk.
- §2 PR plan: until B4's commit the early return tests `b3NeedsRebuild` on the non-awake tree
  as well; B4 removes both interim lines.

## Revision 37 (after round 39, confirmation round)

- B4: the height at a public call's exit is at most one over the threshold plus one per
  sleeping-body move made from a callback in that call; single walks and the rebuild's gather
  stay within their stacks up to about 430 such moves.
- §1: the O(awake) claim excludes the bullet and sensor tree queries, each O(log plus hits).

## Revision 38 (gate re-run before Canonical, U-G4 and U-G1)

- AC-1 to AC-4 re-checked by the author against revision 37. AC-4 flagged B3's queue key: the
  body generation was a staleness check used as control flow. Fix: the queue records the body
  id only, and the drain reads the live body slot, which `b3DestroyBody` leaves with no set;
  a reused slot's body already has its proxies where its set implies. AC-1 to AC-3 pass: the
  queue is pending work re-derived from the live set at the drain, not a population; the
  height test sits in the one insert path; the end-of-step validation is an assertion.
- Fresh clean round and confirmation required on this revision (U-S8).

## Revision 39 (after round 42)

- B4: the callback allowance is about 430 proxy inserts, one per shape per move, and past it a
  release build's walk in that call skips branches, today's risk for any write under a walk.

## Revision 40 (after round 43)

- B4: the 512-entry stack claim limited to the depth-first walks and the pair walk; the
  cross-seed gather's 128-entry queue and 64-seed cap named.

## Revision 41 (after round 44)

- B4: the awake tree's height at the pair walk is the last build's plus one step's rotated
  inserts, a wake's included, today's reliance for a spawn between steps; no awake check added.
- §1: a bullet's or sensor's query costs its node visits.
- §3: the height assertion is on the world-level path; the tree-level path establishes the
  tall input and asserts the full build brings it under the threshold.

## Canonical (after round 46)

- Status set to Canonical at revision 41: clean round 45 confirmed by clean round 46. 46 rounds,
  revisions 1 to 41; 71 findings, 64 applied, 4 applied in part, 3 declined outright (round 2
  #5, round 5 #2, round 18 #1).
- U-R10: no rule's cost in this series exceeded what it caught. U-P9's confirmation rounds
  overturned four clean rounds (28, 34, 37, 39) with real findings each time.
- Open items for the owner, outside this document: the rebuild's gather and build walks push
  past a compiled-out assert in release builds (`b3DynamicTree_Rebuild`, `b3BuildTree`), a
  primitive this design and today's step both call (round 32); the rewind note
  `20261013-002` has not been reviewed as its own document.

## Superseded in part (2026-10-15, after implementation)

- Owner reversed the decision of revision 12 (round 11, U-S5): `b3Body_SetTransform` now wakes a
  sleeping dynamic or kinematic body, and the non-awake tree runs no self pass. Recorded in
  `20261015-001-sleeping-tree-marks-and-set-transform-wake.md` and in `MIGRATION.md`; a note at
  the top of this document points there. B1's non-awake self pass and the "non-awake passes run"
  bullet of §8 are the parts reversed; B4's height hooks and the cross passes stand.
