# Changelog: 20261012-002 implementation prompt, sorted-candidates tree-order-independent solver

## Revision 1: first draft

- Drafted from 20261012-001 (Canonical) after reconciling its claims against the source tree on the
  branch `solver-tree-order-independent`. Every named function, field, macro and file was found as
  the design states.
- Two facts the design leaves open are decided here. The visit-order witness is a direct
  `b3DynamicTree_Query` from the test over a covering box, because the traversal in
  `src/dynamic_tree.c` hands leaves back in a layout-only order that a query box prunes but never
  reorders. The benchmark report is built from the existing suite plus an uncommitted scratch
  measurement, because no scene in `shared/benchmarks.c` creates bullets and the design's file list
  excludes `benchmark/`.
- A golden-hash policy for `test/test_determinism.c` is added, since the design's §1 says goldens may
  stop validating but its §3 requires that file to pass.
- Not yet reviewed.

## Revision 2 (after round 1)

- Fuzz: the whole-field witness is removed; the test counts diverging seeds instead of stopping at
  the first, and its non-vacuity evidence is the pre-change count. Sanity, sensor and Explode
  fixtures state that every candidate the covering box returns is inside the query under test.
- One entry function per test (four), each registered in `test/main.c` by the commit that adds it;
  `test/main.c` joins the A1 and A2 file lists.
- §7: build commands configure with samples off and build the `test` and `benchmark` targets by
  name (verified live); benchmark measurements taken from the `-s` per-step files' `transforms`
  column and the csv totals; `bullets`, counters and the dense scene from a scratch harness; the
  sort cost timed around `QSORT` alone.
- PRNG idiom named xorshift32.
- 284 -> 308 lines.

## Revision 3 (after round 2)

- §1 opens with the `ready-for-pr` issue prerequisite; §8 requires the PR template filled.
- Sensor test: a test-local constant of 8, shape ids compared by `index1`, the eight lowest sensor
  ids asserted after the change, and different reported sets required before it.
- Explode test: nonzero `impulsePerArea` set explicitly; one body in the falloff band; the
  falloff-branch claim corrected.
- Docs sentence: the pre-solve call's conditions stated.
- §2 facts added for each of the above; §7 iteration counters labelled high-water marks.
- 308 -> 331 lines.

## Revision 4 (after round 3)

- Fuzz: a filtered witness body per seed whose exact swept box, the union of its shape AABB before
  and after each step, drives a direct static-tree witness every step; the pair runner gains an
  after-step hook. The diverging seed count stays as fail-before evidence. §2 states the two source
  facts the witness rests on.
- Fail-before runs move from a stash chain to a scratch worktree at the last commit with the working
  tree's test files copied in, each build and test status printed.
- §5 covers every pinned value kind in a CCD scene, with cause reported; untraceable changes stop.
- §3 exempts the closure doc; §8 ticks the samples box only after running the samples.
- 331 -> 362 lines.

## Revision 5 (after round 4, owner decision (b))

- Fuzz witness: the filtered witness body and the after-step hook are removed; each seed builds a
  witness corridor, a fast sphere sweeping a cluster of solid boxes placed inside its swept AABB with
  nothing else there, so the candidate set is known by construction and the query reaches the hash.
- New test `ContinuousCallbackOrderTest`: one corridor with argument-only logging callbacks; the
  custom-filter log must be the cluster in ascending order and the `preSolve` log equal across runs.
- §1 requires an approved issue, not a Discussion. Docs sentence carries the pair-discovery and
  sensor-overlap caveat. Counters are high-water marks since the worker contexts were last created.
  Mapping states the determinism suite's worker counts. The closure doc stays outside the PR.
- §2 facts replaced: corridor construction and the callback call sites in place of the AABB getter
  and mask facts.
- 362 -> 384 lines.

## Revision 6 (after round 5)

- §7 fail-before script runs a test list, with the A2 list stated.
- Callback test: at least two on-path boxes, `preSolve` returns false to keep the sweep open, and its
  log is asserted ascending and equal across runs.
- Fuzz reruns sixteen seeds at four workers. Sensors sit on one static body. The perturbation helper
  states its unlocked-world and no-recording precondition. The §2 CCD-only custom-filter claim is
  qualified to a scene without sensors.
- 384 -> 392 lines.

## Revision 7 (after round 6)

- Fuzz: the field and the corridor are hashed separately; the field's diverging seed count is the
  design's number, the corridor's is printed beside it.
- Four-worker rerun: worlds created with a worker count of four in the definition and forty fast
  bodies, half bullets, so finalize and bullet CCD each span several blocks; §2 states the thresholds
  and that the worker-count setter adds no scheduler.
- U-T6: the document crosses 400 lines; it decides one thing, so no split.
- 392 -> 401 lines.

## Revision 8 (after round 7)

- Suite benchmark reports csv totals only; the per-step files accumulate minima across runs, thread
  counts and scenes and are not used. §2 states this.
- Fuzz: the corridor hash is the assertion tied to the witnessed query and must diverge before the
  change in at least one seed; the field hash stays the design's broad fuzz with its count recorded.
- Callback test: the witness must also differ when restricted to the on-path boxes, so both logs
  differ before the change.
- 401 -> 405 lines.

## Revision 9 (after round 8)

- Fuzz: three witness corridors per seed, static, kinematic and dynamic, each with its own hash and
  nonzero pre-change count; kinematic and dynamic witnesses recorded after the step, with the §2
  fact on the step's tree rebuild and post-CCD enlargement that makes that the right moment.
- Explode test requires every body in range awake with nonzero velocity before the comparison.
- Four-worker wording narrowed to eligibility for parallel use.
- 405 -> 419 lines.

## Revision 10 (after round 9)

- §7 benchmark invocation caps threads at eight on both runs, with the reason beside the commands.
- 419 -> 422 lines.

## Revision 11 (after round 10)

- Pair runner regains an after-step hook; the kinematic and dynamic corridor witnesses name it.
- Every corridor has at least two on-path boxes, with the reason that makes the result
  order-sensitive; placement guidance corrected.
- Four-worker rerun uses forty-eight fast bodies so both stages form four blocks.
- Explode fixture starts from the default explosion definition, the witness takes the definition's
  mask, and the band body's closest surface lies beyond the radius. `RecordVisitOrder` takes a mask.
- 422 -> 434 lines.

## Revision 12 (owner decision, after round 10)

- Fixture detail cut to contracts (U-T3): numbers and placement geometry (box, body, seed, step,
  sensor and cluster counts) are removed and left to the implementer under the existing
  adjust-until rules, with the counts recorded in the closure doc. What stays is what each test
  must build, witness, assert and show before the change.
- The witness corridor moves into the A0 helper contract as `CreateCorridor`; the sanity test's CCD
  case witnesses one corridor of each kind.
- Four-worker workload stated as the derived thresholds (at least 49 awake bodies and 25 bullets).
- Part B not reconsidered: no finding in the series named a shape it forbids.
- 434 -> 420 lines.

## Revision 13 (after round 11)

- Sanity test settings stated per case; the negative control asserts no body fast at any step.
- Every corridor asserts its sphere flagged fast after the corridor step; §2 states the flag and
  how a test reads it.
- Explode witness also requires a different first-visit order over distinct bodies.
- §7: at least eight detected cores, or the shared thread range is reported.
- 420 -> 432 lines.

## Revision 14 (after round 12)

- Explode fixture sets `radius` positive and places bodies relative to it.
- Fuzz: each corridor hash is compared at its witnessed step only; from the second step the
  corridor bodies join the field hash.
- 432 -> 435 lines.

## Revision 15 (after round 13)

- Fuzz field hash is over field bodies alone at every step; corridor bodies are hashed at their
  corridor step only.
- §7 fail-before block is a shell function taking the test list, called for A1 and for A2, with a
  quoted scratch variable in place of the placeholder.
- 435 -> 438 lines.

## Revision 16 (after round 14)

- Corridor hash divergence before the change is recorded, not required; the corridor contract says
  why it may not occur. The callback test runs one corridor per tree kind and is the guaranteed
  fail-before evidence for A1's three sort passes.
- §6: the fuzz row claims the two witnessed layouts per seed; a new row maps the general rule to
  inspection of the three sort-and-evaluate passes.
- §7 fail-before function records every status, always removes its worktree, returns failure.
- 438 -> 446 lines.

## Revision 17 (after round 15)

- Callback test logs keep only calls involving a corridor sphere; the CCD-only claim is stated for
  those calls.
- 446 -> 448 lines.

## Revision 18 (after round 16)

- §7 fail-before function prints a build line and one exit line per test; its status is 0 only when
  the build succeeded and every named test failed.
- §7 benchmark runs each revision in its own scratch directory; §2 states that the csv files land in
  the benchmark's working directory.
- 448 -> 456 lines.

## Revision 19 (after round 17)

- The corridor sphere's precondition is its fat AABB overlapping no other shape's fat AABB at step
  start, asserted from `world->fatAABBs`; §2 states that pair discovery overlaps fat AABBs and that
  `b3Shape_GetAABB` returns the tight box; the callback test cites the assertion.
- §7 benchmark builds main in a scratch worktree and runs each revision's own binary from its own
  results directory.
- 456 -> 466 lines.

## Revision 20 (after round 18)

- §7 fail-before counts an exit of exactly 1 as the expected result.
- Fuzz four-worker runs require at least 25 bullets flagged fast at the first step; §2 states that
  the bullet stage runs over the bullets finalize flagged fast; the §6 row claims at least four
  blocks available per stage.
- Explode fixture separates bodies by more than the speculative distance, asserts a distinct sleeping
  set per body before the explosion, and checks the awake-set sequence before the velocities.
- 466 -> 470 lines.

## Revision 21 (after round 19, owner decisions)

- Owner decisions (U-S5): the witness problem is solved by scene preparation in this document, not by
  a different perturbation primitive or a mock tree; the §7 shell functions are withdrawn, the
  fail-before and benchmark procedures are stated as requirements.
- §2 states that an inserted proxy marks its path, that the step's rebuild of a just-inserted tree
  builds what a full rebuild builds, and that the build marks no internal node, so the two runs differ
  at a kinematic or dynamic tree query only after an insertion since the last step or in a world
  never stepped.
- Corridor contract: the sphere is created in the after-step hook of the step before the corridor
  step; the sanity test, fuzz and callback test step twice with the second as the corridor step.
- Explode test: bodies are created asleep and the world is never stepped; the sleep loop and the
  speculative-distance spacing are deleted.
- §3 states the fail-before requirement in prose: the test file is copied into a scratch worktree at
  the previous commit, the build succeeds, each test fails on its divergence check, then passes in
  the working tree. §7 keeps the plain build, test and benchmark commands only.
- 470 -> 453 lines.

## Revision 22 (after round 20)

- §7 builds and runs the suite a second time with `BOX3D_DOUBLE_PRECISION` on; §2, §3, §5 and the
  §6 row state that both precision configurations are run and that the golden policy applies to each.
- 453 -> 460 lines.

## Revision 23 (after round 21)

- Callback test registers both callbacks in both worlds before stepping; §2 states that CCD calls the
  pre-solve callback with no null check.
- §2: `b3Shape_GetAABB` returns the shape's box with the speculative margin, not a tight box.
- §7 suite benchmark counts run from one to the shared maximum, at most eight.
- 460 -> 465 lines.

## Revision 24 (after round 22)

- Corridor contract: each cluster is created in two parts in its own tree, one with the scene and one
  with the sphere after the first step; the sanity test and fuzz say the same.
- §2 rebuild fact restated: internal nodes inherit moved marks on insertion and on build; the two
  runs differ at a query when the tree holds settled subtrees and insertions at the perturbation.
- 465 -> 469 lines.

## Revision 25 (after round 23)

- Corridor contract defines an on-path box by its time-of-impact fraction, strictly between zero and
  one; the callback test's pre-solve log holds exactly the on-path boxes; the §6 row cites it.
- 469 -> 470 lines.

## Revision 26 (after round 24)

- §4 and the §6 A0 row state why the fuzz field's queries are not witnessed and that the nonzero
  fail-before count is their evidence of a changed order.
- 470 -> 472 lines.

## Revision 27 (after round 25)

- A1 and A2 code paragraphs name the `qsort.h` include for `src/solver.c` and
  `src/physics_world.c`; the A1 moved range starts at the fast-shape sensor assertion.
- §2: only kinematic and dynamic proxies enter marked, static ones when pair creation is forced.
- §8: the samples box is ticked only after running every sample; otherwise the PR says what was run.
- 472 -> 476 lines.

## Revision 28 (after round 26)

- Fuzz paragraph cites design §2 A1 for the fail-before count as the fuzz's evidence.
- Explode test computes and prints both between-run comparisons before asserting either.
- A1 include clause no longer says `src/broad_phase.c` is the only user of `qsort.h`.
- 476 -> 477 lines.

## Revision 29 (after round 28)

- Corridor precondition and the §2 pair-discovery fact are stated over tree leaf bounds, read with
  `b3DynamicTree_GetAABB`; §2 says a bullet's leaf can exceed its `world->fatAABBs` entry.
- §2 states the finalize and bullet block thresholds independently.
- 477 -> 478 lines.

## Canonical (after round 31)

- Status set to Canonical after the clean round 30 and its confirmation, round 31, both at full
  scope against revision 29.
- U-R10: U-C10, the Implementation Prompt's exemption from U-C1 and the U-T3 fix-size rule; under it
  the §7 shell block grew across revisions 13 to 20 and drew five findings in rounds 14 to 19 before
  the owner withdrew it in revision 21.
