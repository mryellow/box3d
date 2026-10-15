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
