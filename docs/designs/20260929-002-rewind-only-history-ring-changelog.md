# Changelog: 20260929-002 rewind-only history ring

## Revisions 1 to 21 (2026-09-29 to 2026-09-30)

- Derived from `20260927-002-bit-exact-history-ring.md` by removing forward scrub. Twenty-one
  review rounds against source as of `5643cd8`, recorded in `docs/designs/reviews/`; the series
  converged at round 21. Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L)
  -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L)
  -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L)
  -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L). The tallies
  lived in the front matter until revision 22.

## Revision 22 (2026-10-15)

Reconciled with the tree changes on `master` through `55a40d1` (U-I1): the sorted continuous
candidates and explosion order (`20261012-001`, `20261012-005`, `20261013-005`), the awake and
sleeping dynamic trees (`20261013-001`), the parked leaf and `b3Body_SetTransform` wake
(`20261015-001`), the unrotated and bulk migration inserts. Folds in
`20261013-002-awake-tree-split-rewind.md`, now Closed. Reopens the doc at full scope: status is
the bare word Draft and the finding tallies moved here.

- Front matter: status, code baseline.
- §2 requirement 1, §3, §4 table and cost line: tree layout is not simulation-affecting on
  today's engine; the awake tree and the migration queue are hot; the static, kinematic and
  sleeping layouts are derived.
- §5.1: rows for the awake tree, the migration queue and the kinematic tree's moved proxies
  replace the moved-proxies row; the awake tree's bytes join the per-tick estimate.
- §5.2 table: proxy-id row names the single and bulk create choke points and the two journaled
  trees.
- §5.4 rewritten for four trees and the two rebuild rules.
- §6: the awake tree and the queue are flat copies; the moved-proxy gather is the kinematic
  tree's.
- §7.1: proxy entries name their trees and carry the leaf's box; the sleep path's proxy creates
  are entries. §7.2: one entry per migrated shape, churn measured in the preflight.
- §7.4 rewritten as "Trees": which tree holds what, the awake tree image with the queue, the
  derived trees' restore pass and moved-bit restore, leaf box equals fat AABB entry, proxy
  identity with the bulk create, height-triggered rebuilds, phase-1 kinematic image. The three
  order changes, the CCD two-pass sensor collection and the explode description are gone, since
  the engine now sorts.
- §9 step 6: the step's proxy-tree check. §11.3: order changes removed from the engine-change
  list.
- §12: test 1 hashes four proxy pools and the queue; test 2 adds the parked, shape-on-sleeping,
  bulk and height-trigger cases; test 5 reports the sleeping tree's rebuild; test 7 is written
  for the four trees.
- §13: phase 1 images the awake tree and the kinematic tree, journals the static and sleeping
  trees; phase 2 derives the kinematic tree.
- §16: open question on the order changes removed, the rest renumbered.
- §17 Provenance added: awake tree imaged, sleeping tree derived, pool growth, no height bound,
  queue imaged.

## Revision 23 (2026-10-15)

Owner decisions on four of §16's five open questions, folded in (U-S5); the doc stays Draft at
full scope, unreviewed since revision 22.

- §7.3: journaling runs only while history is enabled; a disabled world pays one null check per
  structural write.
- §12 test 1: `b3World_ComputeStateHash` is public, not "worth exposing".
- §16: questions 1 to 4 removed (hash public; journaling enabled with history; no return to the
  present without re-stepping, the forward journal being `20260927-002`'s own design; the
  recording player keeps its own ring). Question 5 is now question 1, with the per-record mark
  assigned to measurement after phase 1 under test 5 rather than built in phase 1.
- §17: four Provenance bullets carry the four decisions and their reasons.

## Revision 24 (2026-10-15, after round 22)

Round 22, the first against the reconciled doc: 6 findings (2H-3M-1L), all applied.

- §5 hot row, §5.1, §6, §7.4: the moved-proxy list covers the kinematic and static trees. A
  static leaf is marked by between-step API calls and cleared by the next pair update, so the
  static list is empty after any step and non-empty only in tick 0's image, which
  `b3World_EnableHistory` takes after such calls. Rewind(0) previously cleared those marks and
  the first replayed step could miss pairs.
- §7.4: the tree pass skips a listed shape id at or beyond the restored shape array's length,
  which an undone bump allocation truncated away, without reading it.
- §3, §10: the engine runs SIMD width 4, or 8 under AVX2, and `test_determinism.c` holds the two
  bit-identical; the per-world `b3World_EnableSSE2Fallback` width and the process-wide width are
  host configuration, left by `Rewind` and outside the hash. "Width-4 SIMD everywhere" removed.
- §2 requirement 2: the awake tree's proxy array is its second exception, imaged to a capacity
  that never shrinks, 24 B per slot of the high-water mark, matching what §7.4 already said.
- §5.2: the compile-time claim is narrowed to element and count assignment; the world's container
  pointers are assignable but have no producer outside the container's file.
- §1, §8: `src/recording_replay.c` is `src/replay.c`. The same stale path fixed in
  `20260927-002` and `20260929-003`.

## Revision 25 (2026-10-15, owner request)

Retained memory stated alongside per-tick cost, so the ring's RAM is a stated figure and not
only its capture time.

- §2 requirement 2: the awake tree's high-water term is retained once per imaged tick in the
  window. Requirement 5: retained memory is the per-tick slot times the imaged ticks in the
  window; on the all-awake stress scenes a 60-tick window at interval 1 is 0.3 to 3 GB, and
  `maxBytes` is what holds it to hundreds of megabytes at the price of a wider interval.
- §5.1: the retained total is the per-tick image times the imaged ticks.
- §12 test 5: reports retained bytes and the effective interval at a 60-tick window, under an
  unbounded budget and under one the window does not fit; acceptance adds `bytesUsed` within
  `maxBytes` except in §8's overrun cases.

## Revision 26 (2026-10-15, after round 23)

Round 23: 3 findings (0H-1M-2L), all applied, plus one unresolved item the author confirmed as a
defect and fixed.

- §7.4, §5.2: the drain's parked-flag write on each placed body goes through `b3Body_Write`'s
  structural entry; the moving flag is set and cleared inside one drain. `b3PlaceBodies` joins the
  body worklist.
- §10: within a CCD sweep `preSolve` and the custom filter are called in sorted candidate order;
  the layout-dependent cases are the custom filter in pair discovery and sensor queries, and the
  interleaving of parallel workers.
- §5.1: the static tree's mark sources are the broad phase's three mark setters, with the API
  callers, `b3Joint_SetCollideConnected` among them, in parentheses.
- §7.1, §5.2, §7.4, §12, §13: a bullet's leaf can be larger than its fat AABB entry, since the
  enlarge leaves a leaf that already contains the new entry, and the sleep migration carries that
  box into the sleeping tree. The sleeping tree's leaves are restored from entries alone: the
  destroy entry's box and a new proxy move entry (sleeping tree only, inside
  `b3BroadPhase_MoveProxy`) for a leaf `b3ResetProxy` writes in place. The restore pass runs on
  the static tree in phase 1 and the kinematic tree from phase 2, where the leaf equals its entry.
  Test 1's hash reads the leaf box as its own value; test 2 adds the slept-bullet case.
- §16: question 2 records the engine-side alternative, a bullet enlarge that replaces the leaf
  whenever it differs from the entry, deferred to its own preflight if phase 1 shows the proxy
  move entry worth removing.

## Revision 27 (2026-10-15, after round 24)

Round 24: 4 findings (1H-1M-2L), all applied.

- §2, §7.4: the awake tree's non-awake-proportional term is the most awake proxies and parked leaves
  held at once. §17 records that the image is not compacted (proxy ids).
- §4, §14: the restore cost includes the sleeping tree's height-triggered full rebuild.
- §5.2: `b3PlaceBodies` joins the `shapes[id]` worklist (`proxyKey`).
- §10: body, shape and joint `userData` restored; `world->userData` is host configuration.

## Revision 28 (2026-10-15, after round 25)

Round 25: 2 findings (1H-1M-0L), 2 applied, 0 declined.

- §13: phase 0's prerequisites gain the migration queue, carried in each ring slot beside its
  serialized image and copied back on restore (the serializer does not write it).
- §7.4, §17: the queue is imaged by stored count and order; each body once holds after a step's
  drain, and the tick-0 image can hold repeats and destroyed bodies' ids.

## Revision 29 (2026-10-15, after round 26)

Round 26: 2 findings (1H-1M-0L), 2 applied, 0 declined.

- §9, §14: `Rewind` and `EnableHistory` assert no broad-phase traversal is open (not callable from a
  query or cast callback). §17 records that tick 0 images the queue as stored.
- §14: the closing guarantee holds when the same API calls, time steps and host inputs are replayed.

## Revision 30 (2026-10-15, after round 27)

Round 27: 2 findings (1H-0M-1L), 2 applied, 0 declined.

- §9, §10: the lock refuses world-level API entries; `destroyDebugShape` runs during the restore and
  must not call the API, as at an ordinary destroy. §17 records why the destruction is not deferred.
- §1, §15: pair discovery's cost is stated for the awake and sleeping trees (moved siblings and cross
  passes, whole world when all move).

## Revision 31 (2026-10-15, after round 28)

Round 28: 2 findings (0H-1M-1L), 2 applied, 0 declined.

- §7.1: the material-block undo for an element write writes the old element back at the index; a
  resize or replacement reallocates and copies.
- §7.4, §12: a restore-pass kinematic move can leave the tree unordered or its root marked moved;
  the first replayed step can pay the rebuild.

## Round 29 (2026-10-15, no revision)

Round 29: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The first clean round on revision 30; the confirmation round follows.

## Round 30 (2026-10-15, no revision)

Round 30: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The confirmation round of round 29, clean; the series is settled at
  revision 30 and the status word remains Draft.

## Revision 31 (2026-10-15, after round 30, owner gate re-run)

Not a review round. The gates re-run after the confirmation round found two doc defects, and the
owner ruled on two others.

- §2, §11.1, §5.1: the citations of the perf review record are removed; the figures are cited as
  §11.1's table.
- §5.2: `b3CreateShapeProxy` joins the `shapes[id]` worklist (`proxyKey`).
- §17: owner decision recorded: §12 stays in the design (the tests are part of what it decides).
- Owner ruling, no edit: the migration queue is live engine state (AC-1 holds); the restore pass
  and the doc's length stay as they are.

## Revision 32 (2026-10-15, after round 31)

Round 31: 1 findings (1H-0M-0L), 1 applied, 0 declined.

- §5.2: a body's or joint's journal entry holds its sim and state only when the record's
  coordinates name a non-awake set at the time of the write; a write whose coordinates name the
  awake set holds the record alone.

## Revision 33 (2026-10-15, after round 32)

Round 32: 1 findings (0H-1M-0L), 1 applied, 0 declined.

- §5.2, §7.1: a non-awake set holds no `bodyStates`; a body's entry holds its sim only. The record
  inventory row, the container row and the array push tag drop `bodyStates`.

## Round 33 (2026-10-15, no revision)

Round 33: 2 findings (1H-1M-0L), 0 applied, 0 declined.

- No change to the design. Both findings are recorded and unfolded; the series is stopped for an
  owner decision on how a body's or joint's sim is journaled (U-S3, U-S4).

## Revision 33 (2026-10-15, after round 33, owner-directed)

Round 33's two findings applied after the owner asked for the choices to be evaluated. Of the
ways to journal a non-awake sim (a capture point before each detach site; a new sim-write entry
kind; the record entry reading the sim through a caller-supplied pointer; reordering the engine's
detach sites to write coordinates first), the one taken uses what the journal already has: a
sim is an element of its set's journaled array, and that array's existing `set`, push, removeswap
and set-destroy entries hold its bytes at every write and move. No new entry kind, no per-site
list, no rule on callers.

- §5.2: a body's or joint's record entry never holds a sim; the accessor takes the record only. A
  function that writes both makes two writes, the record through its accessor and the sim through
  its set's array `set` at the record's `localIndex`. The bodies and joints rows drop the sims;
  the container row names the in-place sim writers.
- §7.1: the sufficiency argument extended to a non-awake sim through its set's array entries.
  Round 31's clause on the entry's coordinates is deleted with the payload it qualified.
- §12: test 2's churn adds a body-type change and a destroy of a sleeping body.
- §17: records why the record entry never carries a sim.
- §2, §5.1 (round 33 #2, confirmed: `CreateLargeWorld` creates a million static shapes with
  contact creation forced, `shared/benchmarks.c`): tick 0's image holds the static tree's moved
  proxies and is the third exception to requirement 2; `large_world`'s few KB holds after its first
  step.

## Revision 34 (2026-10-15, after round 34)

Round 34: 2 findings (0H-1M-1L), 2 applied, 0 declined.

- No change to the design. Both findings are recorded and unfolded; the series is stopped for an
  owner decision on how a body's or joint's sim is journaled (U-S3, U-S4).

## Revision 35 (2026-10-15, after round 35)

Round 35: 1 findings (0H-1M-0L), 1 applied, 0 declined.

- §14: the correction example replays the API calls made after step `now`, which the rewind undid,
  after the catch-up loop.

## Revision 36 (2026-10-15, after round 36)

Round 36: 3 findings (2H-1M-0L), 3 applied, 0 declined.

- §5.2, §7.1: `b3Shape_Write` takes the record and its `fatAABBs` entry together; one entry holds
  both.
- §8: the writer never wraps into the minimum window; the arena grows instead, so a sole image is
  retained until a second is captured.
- §9: the lock refuses the entries that take an unlocked world and no others; the callback contract
  is unchanged.

## Revision 37 (2026-10-15, after round 36, split)

Split under the split check (U-T6) after the series' convergence pattern was reviewed: §8 moves
to `docs/designs/20261015-003-history-ring-budget-and-eviction.md`, which decides the ring alone.
§8 keeps its number and becomes the cross-reference, so every `§8` citation resolves to the ring
doc. Requirement 5 shrinks to its requirement; §6's capture-interval paragraph, §12 test 6 and
test 5's budget acceptance move with the ring. No decision changed.

## Round 37 (2026-10-15, no revision)

Round 37: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The first clean round after the split; the confirmation round follows.

## Revision 38 (2026-10-15, after round 38)

Round 38: 1 findings (0H-1M-0L), 1 applied, 0 declined.

- §14: the correction example sets the host state its callbacks read back to its value at T before
  the correction and the replay.

## Revision 39 (2026-10-15, after round 39)

Round 39: 1 findings (1H-0M-0L), 1 applied, 0 declined.

- §10, §17: a recording player's world and history exclude each other, asserted at enable and at
  the player's seek.
- §5.2, §13: restore's writer also serves `b3DeserializeIntoShell`; the serializer's reader
  migrates to it in phase 1.

## Revision 40 (2026-10-15, after round 39, second split)

Split again under the split check (U-T6): §5.4 and §7.4 move to
`docs/designs/20261015-004-history-ring-broad-phase-trees.md`, with §12 test 7, §16 question 2
and the seven tree decisions of §17. Both section numbers stay as cross-references, so every
`§5.4` and `§7.4` citation resolves to the trees doc. No decision changed.

## Round 40 (2026-10-15, no revision)

Round 40: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The first clean round after the second split; the confirmation round
  follows.

## Round 41 (2026-10-15, no revision)

Round 41: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The confirmation round of round 40, clean; the series is settled at
  revision 40 and the status word remains Draft.

## Revision 41 (2026-10-15, after round 41, correction)

Round 34's three fixes were never written: its script failed before the first edit, its commit
holds only the changelog and the record, and the record's resolution sections are round 33's
text. The revision 34 entry above is therefore wrong about what changed. Found when the trees
doc's round 2 re-raised the `version` omission in text the split had moved. Applied now:

- §14: the hash's comment says engine state, host state being the caller's to hash.
- §6: capture runs after the step's final migration drain, not only after the end-event flip.
- The `version` clause is in the trees doc (its revision 3).

Revision 41 differs from the revision rounds 40 and 41 passed, so the series is not settled; a
fresh clean round and its confirmation are needed (U-S8).

## Revision 42 (2026-10-15, after round 42)

Round 42: 3 findings (0H-2M-1L), 3 applied, 0 declined.

- §5.2: `b3UpdateBodyMassData` and `b3Body_SetMassData` join the contacts worklist (the attached
  contacts' recycling flag); the debug draw's `userShape` assignments migrate to the `shape.c`
  writer; velocity setters on an awake body write its `bodyStates` element through the awake set's
  array.

## Revision 43 (2026-10-15, after round 43)

Round 43: 1 findings (0H-0M-1L), 1 applied, 0 declined.

- §5.2: the islands worklist row names `b3RemoveBodyFromIsland` (`body.c`) and the solver-set
  transitions (`solver_set.c`) with their routes.

## Revision 44 (2026-10-15, after round 44)

Round 44: 2 findings (1H-0M-1L), 2 applied, 0 declined.

- §5.2, §7.3: the sim getters of `body.h` and `joint.h` become const-returning; `b3SyncBodyFlags`
  joins the in-place sim writers.
- §2: the callback-order exception covers worker interleaving as well as layout.

## Revision 45 (2026-10-15, before round 45, the U-S4 cut)

No round. §5.2 produced findings in rounds 42, 43 and 44, each a caller missing from the
migration worklist table; the owner chose the cut on offer.

- §5.2: the worklist table loses its caller column and keeps each structure's write route. The
  prose that pointed at the list (`body.c`'s sim-writing functions "listed below", the table as
  the migration's starting worklist) now names the route instead.

## Revision 45 (2026-10-15, after round 45)

Round 45: 2 findings (0H-2M-0L), 1 applied, 0 declined, 1 carried forward (U-S3, the owner
decides whether phase 0's restore is public before it meets §10).

- §5.2, §13: the write boundary is the file that holds a structure's storage; the write accessor's
  own file is an outside file that builds the value in a local copy and enters the container's
  write function; phase 1 removes direct-write access from every translation unit but each
  container's own.
- Series stopped: §5.2 produced a finding in the first round after its cut (U-S4).

## Revision 46 (2026-10-15, owner decision, no round)

The fork round 45 carried forward, resolved by the owner: phase 0's restore meets §10 rather
than staying internal.

- §13: the serializer takes two options, off for a recording and on for the ring, under which the
  image carries and the reader restores every body, shape and joint `userData` word, and the
  reader releases a live `userShape` and carries none across a restore. Tests 2, 3 and 4 are
  written in full from phase 0.

## Round 46 (2026-10-15, no revision)

Round 46: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The first clean round on revision 46; round 47 is its confirmation.

## Revision 47 (2026-10-15, after round 47)

Round 47: 1 findings (0H-0M-1L), 1 applied, 0 declined.

- §15: the rejected alternative is keeping the adapted serializer as the product, not reusing it
  as-is; it is correct with §13's phase-0 adaptations. The confirmation of round 46 was not clean,
  so a fresh clean round and its confirmation follow.

## Round 48 (2026-10-15, no revision)

Round 48: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The first clean round on revision 47; round 49 is its confirmation.

## Revision 49 (2026-10-15, after round 49)

Round 49: 1 findings (0H-1M-0L), 1 applied, 0 declined.

- §2, §10, §14: callback registrations, their contexts and `world->userData` are host inputs the
  caller sets back to their values at T and replays at their original ticks, like any API call;
  a mid-timeline swap is no longer outside the contract. The confirmation of round 48 was not
  clean, so a fresh clean round and its confirmation follow.

## Revision 50 (2026-10-15, after round 50)

Round 50: 1 findings (1H-0M-0L), 1 applied, 0 declined.

- §5.2, §7.3: `b3GetJointSimCheckType` joins the const-returning sim getters, and the box3d
  target compiles a discarded qualifier as an error, C otherwise making the conversion a warning.

## Revision 51 (2026-10-15, after round 51)

Round 51: 2 findings (1H-0M-1L), 2 applied, 0 declined.

- §5.2: the const-data array type's `data`, `count` and `capacity` fields are const-qualified, so
  a record write carries an array header unchanged and only the container's file writes one.
- §3: custom filter calls follow layout order in pair discovery and sensor queries, sorted
  candidate order with worker interleaving in continuous sweeps, as §10 says.
