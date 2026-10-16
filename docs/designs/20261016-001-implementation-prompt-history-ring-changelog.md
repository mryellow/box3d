# Changelog: 20261016-001 implementation prompt, rewind-only world history

## Revision 1: first draft

- Drafted from the design (`20260929-002`, Canonical at revision 60), the ring doc (`20261015-003`,
  Canonical at revision 6) and the trees doc (`20261015-004`, Canonical at revision 7) after
  reconciling their claims against `master` at 60ea633 (`src/` and `include/` as of 55a40d1).
  Every function, field, macro and file the designs name was found as the designs state, with
  these differences and additions recorded in §2:
  - `b3Shape_SetFilter` assigns the new filter before comparing category bits, so every filter
    change recreates the proxy; the design §12 test 2's "filter change that does not touch the
    proxy" cannot be produced on this base and is recorded as an observation, not fixed.
  - `b3SplitIsland` detaches the base island's arrays and frees them by hand; H3c leaves them on the
    record for the destroy and H4's destroy entry takes them.
  - The serializer scrubs every `userData` word and carries `userShape` over live same-generation
    slots, so the design §13 phase 0's two options are new (a header flag and a reader parameter,
    `B3_SNAP_VERSION` 12); no `.b3rec` file is committed, so no migration.
  - A restored mesh, height-field or compound shape points at the registry's bytes, so a phase-0
    ring owns a registry and reader for the retained window and a phase-0 restore repoints those
    shapes at copies; H5 removes it. The registry's bytes are reported in the closure doc, not in
    `b3HistoryInfo`.
  - `b3World` carries no mark for a player's world and no mark for the inside of a step; the two
    assertions the design names (`b3World_EnableHistory` refuses a player's world; the hot accessor
    asserts the step) need one `bool` each, read by assertions only.
  - `b3HashWorldState` is the recording's divergence check and is left as it is; the design's
    `b3World_ComputeStateHash` is a new function.
  - The discarded-qualifier diagnostic is a per-compiler flag (GCC, Clang, MSVC spellings).
  - `b3Alloc` adds `(int)size` to the byte count, the ring doc §6's named change, made in H1.
  - `traversalDepth`, `b3BroadPhase_BeginTraversal`/`EndTraversal`, the tree functions, the
    `b3IdPool` pop-or-bump behaviour and the four outside tree writers all match the designs.
- Decisions the designs leave to the prompt: phases 0, 1 and 2 in one prompt as nine commits on
  `world-history-ring` forked from `master`; H0 the state hash and H1 the phase-0 ring with the
  design §12 tests 2 to 4 in full; H2, H3a, H3b, H3c as behaviour-free boundary commits evidenced
  by compile-boundary checks, the suite, the determinism goldens and the H1 history goldens; H4 the
  journal before the in-place image, tested directly on cold-only churn through the two halves of
  the design §9 step 2; H5 the hot image, shown by the H1 tests passing unchanged and a 256 KB
  per-tick bound the serializer path cannot meet; H6 the derived kinematic tree with the kinematic
  bound; one test file `test/test_history.c` with every entry named `History...`; 25 (T, P) pairs
  per scene in the suite and 1,000 under `BOX3D_HISTORY_SOAK`; a 3% benchmark regression threshold
  with history disabled; no golden may change on this branch.
- Length 1263 lines.

## Revision 2: after review round 1

Round 1: 9 findings (H3-M5-L1), 9 applied, 0 declined. Findings: 3/5/1 -> pending.

- The compile-boundary checks of H2, H3a, H3b and H3c name statements that exist at the previous
  commit (`b3GetBodySim` taken into a non-const pointer, the joint sim's `constraintHertz`, the
  record arrays' `data`, `b3GetShapeMaterials`) with the diagnostic each produces, and separately
  the statements through the new const getters that fail at the commit alone; `drawScale` is a
  record field, not a sim field. §3 admits the second kind.
- `HistoryRingTest` reads its byte-count baseline before the world is created and checks it after
  the disable and the destroy, since the world's own arrays grow across the churn.
- Every test that rewinds names its `tickCount`: `HistoryReplayTest` 90, the golden scene 120,
  `SegmentWalkCase` 16, `HistoryTreeTest` 30, `HistoryUndoSufficiencyTest` 60.
- `HistoryRingTest` cases 2 to 4 set `maxBytes` to one and a half images, where the ring doc §6's
  minimum window is over budget, and state the interval sequence, the imaged ticks and the window
  that follow; the invariant check reads imaged flags from the directory.
- H3b's file list names `src/container.h`.
- §3's fail-before procedure covers every commit that adds an entry or a case to the existing
  test file.
- The §6 row for the design §7.3 names inspection of each journal function's first statement
  beside the benchmark.
- §7's capture percentage is over the step timed in the harness, since `b3World_GetProfile().step`
  ends before the capture point.
- §2 counts seven traversal-wrapped public queries and casts.
- Length 1263 -> 1284 lines.

## Revision 3: after review round 2

Round 2: 5 findings (H2-M3-L0), 5 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> pending.

- §2 corrects the revision 1 observation on `b3Shape_SetFilter`: the proxy is destroyed and
  recreated only under `invokeContacts` true (always, since the category comparison follows the
  assignment); with `invokeContacts` false the proxy is left as it is, which is the design §12
  test 2's untouched-proxy case. `HistoryRewindTest` names the two cases; §8's observation is
  narrowed to the `invokeContacts` true path.
- H3a: the serializer's reader with `releaseUserShapes` off keeps its carry-over by detaching the
  live handle through `b3Shape_SetUserShape` before the restore write and putting it back after;
  `b3Shape_Restore` releases unconditionally.
- H3c's compile-boundary statement uses `invMass`.
- `SegmentWalkCase` records the scalars struct per tick and copies the target tick's back before
  comparing, since the step writes `inv_h`, `inv_dt` and `endEventArrayIndex` with no awake body;
  the tick-0 target runs on a fresh run, since the walk starts at `historyTick`.
- `OwnershipCase` drops the exact byte-count equalities after the undo and after the eviction;
  the baseline after the destroy is the check that every evicted block was freed.
- Length 1284 -> 1294 lines.

## Revision 4: after review round 3

Round 3: 4 findings (H0-M4-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> pending.

- §2 and H3a name `b3Shape_SetFriction` and `b3Shape_SetRestitution` as material writers that go
  through `b3Shape_WriteMaterial`; the journal and rewind tests exercise them.
- §2 lists the two `static inline` mark functions of `src/broad_phase.h` and the standalone
  broad-phase test's direct clears among the tree writes outside `src/broad_phase.c`; H4 moves the
  mark bodies into the file and names its clear function for the test.
- `HashSensitiveCase` uses the wake alone, since a wake followed by a sleep can hash as before.
- H1 declares two read-only directory functions in `src/world_history.h` for the tests, H4 a third
  for owned blocks; §9's hook stop condition excepts them.
- Length 1294 -> 1309 lines.

## Revision 5: after review round 4

Round 4: 6 findings (H3-M3-L0), 6 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> pending.

- `HashRoundTripCase` compares only at frames whose recorded migration queue count is 0, since
  the serializer does not write the queue and the hash reads it; at least 100 of 120 frames must
  qualify; §9's stop condition names a qualifying frame.
- §2 names `b3TrySleepIsland`'s four `b3ClearBit` calls on the colours' body sets; H4 routes them
  through the graph's journaled clear.
- H3c makes `world->sensors` and `world->constraintGraph` pointers to incomplete types with const
  element access outside their files; H4 makes `b3BroadPhase.pairSet` a pointer-to-const to
  storage of `src/broad_phase.c`; the compile-boundary checks gain a header-write statement each.
- `HistoryRewindTest` states its sampling rule: P in (current, current + 60], T in
  [max( 0, P − 60 ), P].
- `HistoryUndoSufficiencyTest`'s interval-4 run places T on a multiple of 4 and checks it is
  restorable before the rewind.
- The slot reader returns the charge's parts (image, journal, owned-block bytes), which §7's
  measurements read.
- Length 1309 -> 1329 lines.

## Revision 6: after review round 5

Round 5: 4 findings (H2-M1-L1), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> pending.

- H3a, H3b, H3c and H4 list `src/CMakeLists.txt`, which enumerates sources and headers by name.
- §2 lists, from the source, the API calls that wake a body (`b3Body_SetTransform`, `b3Body_SetType`,
  the destroy and disable of a body with neighbours, a joint with an awake body, a filter change
  with contacts, a shape created on a sleeping body) and the cold ones; the three direct-walk
  journal cases use the cold list only, and the wake, sleep and bulk placement stay with the
  rewinding tests.
- `ProxyIdentityCase` uses a lone sleeping body's destroy for the sleeping-tree entry; the id reuse
  across a step is a named `HistoryRewindTest` case.
- §2 counts eighteen `Create*` in `shared/benchmarks.h` and seventeen table rows; the rewind test
  runs the table's seventeen.
- Length 1329 -> 1351 lines.

## Revision 7: after review round 6

Round 6: 4 findings (H2-M2-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> pending.

- The mesh-contact body's destroy wakes (a touching contact's destroy wakes both bodies), so
  `ColdChurnUndoCase` drops it and `OwnershipCase` tests the contact's blocks through
  `b3World_Rewind` in a second part; §2's cold rule states the reason.
- The slot pool holds the id pool and two functions the record file installs, push-zeroed and
  truncate, so the record arrays' headers stay private to their files.
- H4 names `b3BroadPhase_LoadPairSet` for the serializer's reader beside the tree load.
- The phase-0 writer sizes an imaged slot with a count-only serialization before the budget steps
  and writes into a buffer of exactly that capacity.
- Length 1351 -> 1361 lines.

## Revision 8: after review round 7

Round 7: 3 findings (H1-M1-L1), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> pending.

- `SleepingDynamicCostCase` and §7's million-body scene create their sleeping bodies in batches
  with a step after each, since a shape created on a sleeping body enters the awake tree, whose
  proxy capacity never shrinks and is imaged; the case checks the capacity before the enable.
- The event digest hashes every named field of each public event struct, contact ids, user
  material ids and the `userData` words included.
- §2 names all four `splitIslandId` writers; H2 refers to them.
- Length 1361 -> 1370 lines.

## Revision 9: after review round 8

Round 8: 5 findings (H2-M2-L1), 5 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> pending.

- `b3BroadPhase.pairSet` is the design's `b3JournaledPairSet*`, incomplete outside
  `src/broad_phase.c`, with contains, count and bytes readers, in place of a pointer-to-const
  generic set.
- The static and kinematic leaf assertion applies to forward writes only, since an undo inserts
  the recorded box before the record's entry is restored; H5's validation checks the leaves after
  the restore pass. The sentence's citation into the trees doc's changelog and §6's and §8's
  references to the changelogs' gate entries are removed.
- §2's wake bullet is qualified by body type; the rewind test's filter churn names the shape
  kinds it runs on.
- §2 no longer claims a pre-branch recording replays; a version-11 recording is rejected by the
  version-12 reader.
- §7 names `large_world` as a table row, not an addition.
- Length 1370 -> 1380 lines.

## Revision 10: after review round 9

Round 9: 3 findings (H2-M1-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> pending.

- The serializer's reader with `releaseUserShapes` off detaches every saved handle before any
  restore write, puts the matching ones back and releases each remaining one once.
- H4 states the restore routes: every journaled container and the hull database offer a load that
  makes no entry, used by the serializer's reader from H4 and the image scatter from H5;
  `HistoryRingTest` checks `pendingBytes` is 0 after every successful rewind.
- §2 names `b3DestroyBody` as the third `compoundShapeCount` writer; H2 refers to the three.
- Length 1380 -> 1389 lines.

## Revision 11: after review round 10

Round 10: 3 findings (H1-M1-L1), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> pending.

- The phase-0 slot carries each live mesh, height-field and compound shape's geometry pointer
  beside the image and the rewind repoints the restored shape at it (the design §10's
  borrowed-geometry rule makes it valid), so no shape points into the ring's registry and disable
  frees it; `HistoryReplayTest` disables after a rewind and steps on with casts; the decision is in
  §10 with the two alternatives not taken.
- The H4 pair-set compile-boundary statement is a member access on the incomplete type, since
  Clang warns rather than errors on an incompatible pointer type; §2 names the diagnostic classes
  the boundary statements rely on.
- H2 counts five `splitIslandId` sites.
- Length 1390 -> 1411 lines.
- Tenth round without a clean round: the project's criteria review threshold. Recorded in the
  round record's Status for the owner.

## Revision 12: after review round 11

Round 11: 7 findings (H2-M4-L1), 7 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> pending. The owner chose to
continue the series at the same scope after round 10's threshold (B8).

- H1's rewind writes the recorded geometry pointer into the shape record directly; H3a's
  `b3Shape_Restore` takes the write over.
- `b3World_DisableHistory` and `b3DestroyWorld` share `b3DisableHistoryInternal( b3World* )`, since
  the destructor has already locked the world when it stops the recording.
- H3b's `b3Contact_Hot` returns the record alone; `b3Contact_HotManifolds` and `b3Contact_HotCache`
  return the writable block and cache views the narrow phase and store impulses need.
- H4 names `b3SolverSet_PushAwakeContact` as the push onto the awake set's `contactIndices`, with
  its two callers; the end-touch move is named as one of the no-entry container moves and a merge
  between non-awake sets as the journaled push.
- `HistoryTreeTest` records a visit-order witness over the case's tree before and after the
  perturbation and asserts the two differ; each case names its tree.
- §7 times the fixed-offset rewinds on the unbounded ring and one resolved-tick rewind under the
  quarter budget.
- §2's `manifoldCount` inventory adds creation, destruction and the mesh path's reset.
- Length 1411 -> 1440 lines.

## Revision 13: after review round 12

Round 12: 4 findings (H1-M3-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> pending.

- H4's segment walk discards each slot as it undoes its segment (entry, charge, write position,
  `historyTick`), since the undo hands the slot's blocks back to the world; the design §9 step 5's
  slot removal is thereby done in step 2. `SegmentWalkCase` checks the ring agrees with the world
  after the walk.
- §9's hook stop admits the two halves of the walk H4 names.
- §5 and §7's scratch harness compare the event digest beside the state hash, base against branch
  and enabled against disabled, since H0's hash excludes the event arrays.
- §7's restore figure is absolute microseconds beside awake count, sensor overlap count and undone
  segment bytes; the million-body scene gains 64 static sensors so the sensor column is not zero
  everywhere.
- Length 1440 -> 1460 lines.

## Revision 14: after review round 13

Round 13: 4 findings (H1-M3-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> pending.

- H0's hash mixes each colour contact spec's `manifoldCount` beside its contact id; only
  `manifoldStart` is scratch.
- `HistoryTreeTest`'s cases sit in trees their queries reach: static and sleeping candidates for
  the bullet sweep, static visitors with sensor events enabled for the sensor sweep, sleeping bodies
  for both explosions (`b3World_Explode` reads the awake and sleeping trees only).
- §7's quarter-budget rewind falls back to the newest restorable tick when the nine-tick target has
  no image below it.
- §2's wake inventory adds the conditional wakes of the velocity, target-transform and force,
  torque and impulse setters; the rewind churn names which velocity write wakes.
- Length 1460 -> 1476 lines.

## Revision 15: after review round 14

Round 14: 2 findings (H0-M1-L1), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
pending.

- A third assertion-only `bool`, the restore mark, set by `b3DeserializeIntoShell` and by
  `b3World_Rewind`'s steps 2 and 3, is what the restore writers assert (the design §5.2); §3's
  exception list and §10's marks entry count three.
- §6's exclusion row names the four assertion sites and their predicates in place of a count.
- Length 1476 -> 1484 lines.

## Revision 16: after review round 15

Round 15: 6 findings (H2-M4-L0), 6 applied, 0 declined; 1 Unresolved item (a design question,
recorded). Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 ->
2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> pending.

- Each boundary commit gives the serializer's reader the file-local loads for the containers it
  hides (load-count functions for the record arrays in H3a to H3c; set, island, colour and sensor
  array loads in H3c); H4 adds the pair set's, the trees' and the bitsets' and routes the undo
  through all of them.
- The restore mark is set and cleared by the functions that write through the restore writers
  (the deserializer, the two walk halves, the scatter), so the journal test's direct calls carry
  it.
- `b3SetJointSim` dispatches to a graph-local writer for an awake joint, whose sim is in a colour.
- `HistoryRewindTest` asserts the world's `userData` survived each rewind before setting it back.
- `HistoryTreeTest` gains a kinematic layout witness: the original run's visit order at T against
  the order after the restore pass's full rebuild, then replay equality.
- §7's million-body fixture scripts a kinematic move and a 4,096-body island wake before the timed
  rewind; a tick with no awake body reports an absolute figure outside the per-awake ratio.
- Length 1484 -> 1521 lines.

## Revision 17: after review round 16

Round 16: 6 findings (H0-M5-L1), 6 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> pending.

- H0's contact hash skips the union and the cache header's pointer and capacity, hashing the active
  member's count and elements.
- The collide pass's write of a mesh contact's spec `manifoldCount` is named in H0 and routed
  through a colour hot element accessor in H3c.
- `HistoryTreeTest`'s witness compares the original run's visit order at T with the order after
  the rewind and the perturbation, the trees doc's condition; the kinematic case uses the same
  witness (the restore pass moves proxies and leaves rebuilds to the next step).
- §2 counts six force, torque and impulse calls and names them.
- §7's slab sleeps as one island before the enable, asserted through the internal headers.
- Length 1521 -> 1534 lines.

## Revision 18: after review round 17

Round 17: 4 findings (H2-M2-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> pending.

- H3a names the shape material block's one writer and the reader's and teardown's routes through
  it; H3b names the contact blocks' restore form and teardown; both under the restore mark, no
  entry.
- H4's slot pool takes detach and attach callbacks from the shape and contact files, so the pool
  free and its undo move owned blocks through the owning file.
- The kinematic filter case checks the leaf's filter payload (the recreated proxy takes the same
  id back); a new case destroys a kinematic shape and creates another whose proxy reuses the id.
- §7's rebuild timing is reported from the million-body scene only when the harness observed the
  threshold crossing, otherwise from the rewind test's height-trigger case.
- Length 1534 -> 1555 lines.

## Revision 19: after review round 18

Round 18: 3 findings (H0-M2-L1), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> pending.

- H2's file list carries the graph files its joint-sim writer lives in.
- `HistoryRingTest` case 2 separates the images written over the run from the images the window
  retains at each step, with the two-image and one-image spans stated.
- §2's target-transform wake names its `wake` and time-step conditions.
- Length 1555 -> 1562 lines.

## Revision 20: after review round 19

Round 19: 5 findings (H1-M4-L0), 5 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> pending.

- H4 lists `src/world_hash.c` and gives the hash and the memory log their read routes: pair-set
  capacity and key walk, the slot pool's const id-pool getter.
- `HistoryRingTest` case 2's window spans corrected: two images only at the step that writes one.
- The budget cases run on a variant with equal image charges (bodies apart, gravity zero, sleep
  off), asserted per slot.
- `HistoryRewindTest` bounds T by `oldestImageTick` read at P.
- §7's fallback rebuild timing is a scene of the harness, since the measurement build has no test
  target.
- Length 1562 -> 1574 lines.

## Revision 21: after review round 20

Round 20: 2 findings (H0-M2-L0), 2 applied, 0 declined; 1 Unresolved item (a design question,
recorded, the prompt stating its reading). Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 ->
2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 ->
2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> pending.

- H4's proxy choke points take `b3World*`; the standalone broad-phase case of the awake-tree test
  moves onto a world.
- H5 counts moved leaves with a new descend-moved-only count before sizing the gather's scratch
  array; the tree files join H5's list.
- H5's gather parallel-for names its items, every sensor among them, as this document's reading of
  the design §6's wording; recorded for the owner.
- Length 1574 -> 1588 lines.
- Round 20 is the end of the second block the owner authorised; the series has not converged.

## Revision 22: after review round 21

Round 21: 3 findings (H1-M1-L1), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> pending. The owner chose to
continue the series after round 20.

- The serializer's reader saves and detaches every live shape's renderer handle before the array's
  length changes under both options, releasing all of them with `releaseUserShapes` on, so a slot
  a shorter image truncates away leaks nothing.
- `HistoryRingTest` reads the image charge from a probe enable, since the budget is fixed at enable.
- §2 qualifies the filter's always-recreate path to shapes with a live proxy.
- Length 1588 -> 1596 lines.

## Revision 23: after review round 22

Round 22: 5 findings (H1-M3-L1), 5 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> pending.

- The event digest tags each event array with its kind and count before its events.
- The sensor fixtures set `enableSensorEvents` on the sensor and its visitors and assert the
  overlap count; §7's million-body sensors likewise.
- The budget variant creates every body awake and steps once before the probe, so tick 0's image
  carries no queue bytes.
- §2's wake inventory adds `b3Body_EnableSleep( false )`, `b3World_EnableSleeping( false )`,
  `b3Joint_WakeBodies` and `b3Shape_ApplyWind` with `wake`.
- H2 names H3c as the commit that moves the step's stages onto the hot accessors.
- Length 1596 -> 1611 lines.

## Revision 24: after review round 23

Round 23: 2 findings (H1-M1-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> pending.

- `HistoryReplayTest` and the tree test's bullet case set the per-shape callback flags and count
  the callbacks' calls and rejections, asserted above zero.
- §7's million-body capture-target scene keeps the slab out; the slab is a scene of its own whose
  capture figure is reported but not held to the per-awake target.
- Length 1611 -> 1623 lines.

## Revision 25: after review round 24

Round 24: 4 findings (H1-M3-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> pending.

- A rewind sets the ring's last-imaged tick to T; the walk's slot discard keeps it at the newest
  retained image.
- `b3World_Rewind` asserts the traversal depth before the disabled return.
- `HistoryRingTest` cases 6 to 10 run on a 60-tick unbounded ring stepped 20 ticks.
- §7 records sensor count apart from overlap count and runs the million-body scene at 64 and 256
  sensors.
- Length 1623 -> 1631 lines.

## Revision 26: after review round 25

Round 25: 3 findings (H1-M2-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
pending.

- H3c gives the colour's five per-step scratch fields a setter of their own under the step mark.
- H4 gives the serializer's writer a const read route to the pair set's hash set.
- §2's inventory adds the four geometry setters (a parked leaf without a wake for a contactless
  sleeping body) and `b3World_Explode`.
- Length 1631 -> 1642 lines.

## Revision 27: after review round 26

Round 26: 1 finding (H0-M1-L0), 1 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> pending.

- `HistoryTreeTest`'s sensor sweep is a bullet crossing at least nine sensors in one step, the
  continuous stage's eight-hit limit asserted; the overlap-pass case stays as extra coverage. The
  kinematic sensor case likewise.
- Length 1642 -> 1644 lines.

## Revision 28: after review round 27

Round 27: 3 findings (H2-M1-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> pending.

- H3c's scratch setter covers the overflow colour's two constraint pointers as well.
- H4's detach and attach are ownership transfers of the record files, free of the restore mark;
  the restore-only loads stay for the deserializer and the undo.
- Every owning destroy hands its blocks to one journal function whose disabled branch frees them
  as today and whose enabled branch records the owning entry.
- Length 1644 -> 1655 lines.

## Revision 29: after review round 28

Round 28: 3 findings (H0-M1-L2), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> pending.

- §2 names the step's four writers through the manifold pointer, two in `src/contact_solver.c`
  and two in `src/contact_solver_wide.inl` compiled through `src/wide4.c` and `src/wide8.c`;
  H3b's file list and companion sentence cover the `.inl`.
- §2's wake inventory says `b3Body_SetTransform` wakes a sleeping body and names the disabled
  body's non-awake writes.
- §2 states that the benchmark prints per-run times and writes only each thread count's minimum
  to the csv; §7 tees the console to a log and reads the spread from it and the minimum from the
  csv.
- Length 1655 -> 1667 lines.

## Revision 30: after review round 29

Round 29: 3 findings (H1-M2-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> pending.

- H3a's restore mark is saved, set and put back by each holder so holders nest; `b3World_Rewind`
  holds it around steps 2 to 5, and the rewind's geometry repoint and `bodyMoveIndex` clears are
  restore-writer writes from H3a.
- H1's op log records each creation's id and resolves ordinals to it, `ENSURE`ing replayed
  creations return it; the corrected replay drops a setter op.
- `HistoryRewindTest` schedules every structural sub-scene per scene, counts its runs and
  `ENSURE`s every count above zero.
- Length 1667 -> 1672 lines.

## Revision 31: after review round 30

Round 30: 3 findings (H0-M3-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> pending.

- H4's slot keeps the geometry pointer words and step 3's repoint until H5.
- The hull database's three functions take the owning shape's id from H4; their callers in
  `src/shape.c` are named and the serializer's two calls are the restore add and remove.
- `HistoryRingTest`'s H4 extension fails before H4 by not building.
- Length 1672 -> 1676 lines.

## Revision 32: after review round 31

Round 31: 1 finding (H0-M1-L0), 1 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> pending.

- §7's million-body kinematic timing is the raw-image baseline at H5 and the restore-pass rebuild
  at H6, with the harness confirming the move and the rebuild.
- Length 1676 -> 1680 lines.

## Revision 33: after review round 32

Round 32: 1 finding (H0-M1-L0), 1 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> pending.

- H0's hash names the contact cache member each shape pair selects, with its fields, at both
  cache locations; `HashSensitiveCase` writes a field of each member and of a triangle cache
  element, through the accessor and the block mutator from H3b.
- Length 1680 -> 1691 lines.

## Revision 34: after review round 33

Round 33: 4 findings (H1-M2-L1), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> pending.

- H3c's set and colour pushes take completed values; the constructors build a local and push
  once, in today's order; H4's journaling pushes are their awake forms.
- `HashSensitiveCase`'s mesh-cache probe is compared against a hash taken after the sleep it
  needs.
- `OwnershipCase` enables at `tickCount` 4; the step after the destroy writes the owning slot
  and five more steps evict it.
- `SleepingKinematicCostCase` says bodies created asleep start in a sleeping set.
- Length 1691 -> 1703 lines.

## Revision 35: after review round 34

Round 34: 3 findings (H0-M3-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> pending.

- H3a exposes a const fat AABB getter for the readers outside the hot path; H4's leaf assertion
  reads through it.
- `HistoryCrossWorkerTest` creates its worlds at four workers and switches the count with
  `b3World_SetWorkerCount`.
- `HistoryReplayTest` counts callback calls and rejections per tick, on both sides of the tick-50
  change, and compares them on replay.
- Length 1703 -> 1711 lines.

## Review round 35: clean, revision 35 stands

Round 35: 0 findings (H0-M0-L0), 0 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> pending
confirmation.

- No edit. One Unresolved item about the trees doc §7's pointer-to-const wording recorded for the
  owner in the round record.

## Revision 36: after review round 36

Round 36: 4 findings (H1-M3-L0), 4 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
pending.

- H5's restore scatters contact records through `b3Contact_Restore` before resizing their blocks.
- §2 and H3b name the collide task's manifold-point writes in `src/physics_world.c`.
- H0's cache rule takes a compound's member from its child; `HashSensitiveCase` probes a compound
  contact's SAT cache.
- `HistoryRewindTest` draws T before a scheduled sub-scene's first mutation, so every counted run
  is rewound across.
- Length 1711 -> 1720 lines.

## Revision 37: after review round 37

Round 37: 2 findings (H1-M1-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> pending.

- `HistoryRewindTest` runs `large_world` at `tickCount` 2 until H5 and expresses its pair bounds
  in `tickCount`; H5 moves the scene to 60; §7's phase-0 harness runs at `tickCount` 1.
- `HashLayoutCase` records each tree's visit order before and after the rebuild and requires a
  difference on at least one tree.
- Length 1720 -> 1725 lines.

## Revision 38: after review round 38

Round 38: 3 findings (H1-M2-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> pending.

- `HashSensitiveCase`'s compound is static with a dynamic hull resting on its child; each probed
  contact is `ENSURE`d touching first.
- H1's reader slots hold their own copies of the registry entries' bytes.
- `HistoryReplayTest`'s corrected replay drops a named velocity setter at tick 70 and checks the
  tick-70 hash.
- Length 1725 -> 1730 lines.

## Revision 39: after review round 39

Round 39: 4 findings (H0-M3-L1), 3 applied, 1 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> pending.

- H4 lists `test/test_broad_phase.c` and routes its pair-set read through `b3PairSet_Contains`.
- H0's simplex cache rule hashes `count` and the first `count` entries; the probes write an
  active index.
- H3c names `b3CreateJoint` and the typed constructors' handoff of a completed sim to the push.
- §2 names the seven public queries and casts that open a traversal; §10 records why the count is
  seven (the series' first declined finding).
- Length 1730 -> 1738 lines.

## Revision 40: after review round 40

Round 40: 2 findings (H0-M1-L1), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> pending.

- §2 names the solver's three read-only `b3GetBodySim` calls; H2 makes their locals const.
- §7 measures eviction under the quarter budget and widening under a budget below the minimum
  window, reporting the observed interval for each.
- Length 1738 -> 1743 lines.

## Review round 41: clean, revision 40 stands

Round 41: 0 findings (H0-M0-L0), 0 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> pending confirmation.

- No edit.

## Revision 41: after review round 42

Round 42: 3 findings (H0-M2-L1), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> pending.

- `HashLayoutCase` and `HistoryTreeTest` stage their shapes in two parts around a step for the
  layout witness; the retry clause is gone.
- §7's cost harness enables history after ten warm-up steps; the quarter budget is expected to
  evict at interval 1, the observed interval reported.
- `HistoryRingTest` case 7 skips the churn's tick-11 body create on the re-step.
- Length 1743 -> 1749 lines.

## Revision 42: after review round 43

Round 43: 3 findings (H0-M3-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> pending.

- `HistoryReplayTest`'s callback counters are atomic and read into the record after each step.
- H0's hash rule says a filter joint has no union member; `HashSensitiveCase` holds one.
- `HistoryRingTest`'s H4 extension checks the manifold allocators' count after a disable on the
  live world.
- Length 1749 -> 1755 lines.

## Revision 43: after review round 44

Round 44: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> pending.

- §3's compiler-named-file exception covers H4 for the containers it hides.
- §7 defines the phase-0 registry bytes as the entry and slot byte-count sums and reads them in a
  scratch build; H1 points at §7.
- Length 1755 -> 1759 lines.

## Revision 44: after review round 45

Round 45: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> pending.

- `HistoryRewindTest` runs every scene at `tickCount` 60 again: the Debug suite's `large_world`
  is the 32 by 32 floor; revision 37's window limit is withdrawn. §2 states the Debug and Release
  sizes; §7 adds one Release run of the entry on the million-body scene at H5 and H6, which §6
  names.
- `HashSensitiveCase` probes the hull database with an isolated reference add and remove in place
  of the shape destroy.
- Length 1759 -> 1769 lines.

## Revision 45: after review round 46

Round 46: 3 findings (H0-M2-L1), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> pending.

- §2 names the solver setup's `manifoldStart` write; H3c routes it through the hot spec accessor.
- `OwnershipCase` checks the owned-block count per kind after each destroy; §6 cites it.
- §7 and §8 call the phase-0 figure geometry payload bytes.
- Length 1769 -> 1772 lines.

## Revision 46: after review round 47

Round 47: 3 findings (H1-M0-L2), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 ->
pending.

- H4's phase-0 reader adds the image's hulls before releasing the old references, so a hull live
  at T keeps the pointer the retained entries hold; `HistoryRingTest` gains the two-rewind hull
  scenario.
- §2 names the recording's and the player's direct reads of the body array; H3a lists both files.
- §7's payload figure is read by a scratch-build function of `src/world_history.c`.
- Length 1772 -> 1783 lines.

## Review round 48: clean, revision 46 stands

Round 48: 0 findings (H0-M0-L0), 0 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 ->
0/0/0 -> pending confirmation.

- No edit.

## Revision 47: after review round 49

Round 49: 3 findings (H2-M1-L0), 3 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 ->
0/0/0 -> 2/1/0 -> pending.

- §2 states the three material storage cases; inline storage is told by the pointer.
- H3a splits the shape's block route into the forward writer and the restore-only load and free;
  H4 names the two block mutators that journal.
- H4's pair set and slot pools expose value views with const element pointers in place of raw
  header getters.
- Length 1783 -> 1790 lines.

## Revision 48: after review round 50

Round 50: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 ->
0/0/0 -> 2/1/0 -> 0/2/0 -> pending.

- H6's four kinematic cases are witnessed by the restore pass's own move, counted after the
  rewind, since the step rebuilds the kinematic tree in part every tick.
- `ProxyIdentityCase` exercises the sleeping-tree proxy move entry through an in-traversal
  `b3Body_SetTransform`; §6 names it.
- Length 1790 -> 1798 lines.

## Revision 49: after review round 51

Round 51: 2 findings (H1-M1-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 ->
0/0/0 -> 2/1/0 -> 0/2/0 -> 1/1/0 -> pending.

- `ProxyIdentityCase`'s in-traversal move is undone through `b3World_Rewind`, the queued wake
  having migrated the leaf into the imaged awake tree.
- H6's kinematic cases check the restored moved set, leaf boxes and proxy ids and claim no layout
  witness over a tree the step rebuilds every tick.
- Length 1798 -> 1800 lines.

## Revision 50: after review round 52

Round 52: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings: 3/5/1 -> 2/3/0 -> 0/4/0 ->
3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> 1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 ->
2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> 0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 ->
0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 -> 0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 ->
1/1/0 -> 1/2/0 -> 0/3/1 -> 0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 ->
0/0/0 -> 2/1/0 -> 0/2/0 -> 1/1/0 -> 0/2/0 -> pending.

- `OwnershipCase` counts the non-null handles each destroy hands over, read from the record's
  headers before it, instead of the kinds' array totals; the handover bullet says a NULL header
  is no block.
- `HistoryTreeTest` claims layout witnesses over the static and sleeping trees; the §6 row says
  so, and a §10 item records the kinematic clause of the trees doc §10 test 1 for the owner.
- Length 1800 -> 1820 lines.
