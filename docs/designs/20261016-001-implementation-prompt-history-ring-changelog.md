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
