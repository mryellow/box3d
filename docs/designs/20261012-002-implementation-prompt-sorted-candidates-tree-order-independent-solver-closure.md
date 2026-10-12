---
title: Closure - sorted-candidate tree-order-independent solver changes
status: Draft
date: 2026-10-12
implements: 20261012-002
---

# Closure: sorted-candidate tree-order-independent solver changes

Written after the three commits of the Implementation Prompt (20261012-002) landed on
`solver-tree-order-independent`. This document stays outside the PR. Nothing in the three commits
was amended for it.

## 1. Commits

Forked from `main` at `5643cd8`. The branch also carries the owner's design and prompt documents
below `docs/designs/`, which are not part of the PR.

| Commit | Message | Files |
|---|---|---|
| `a073352` | Add a tree-layout perturbation test helper with a visit-order witness | `test/test_tree_order.c` (new), `test/main.c`, `test/CMakeLists.txt` |
| `affab6a` | Evaluate continuous collision candidates in ascending shape-id order | `src/solver.c`, `src/physics_world.h`, `src/physics_world.c`, `docs/simulation.md`, `test/test_tree_order.c`, `test/main.c` |
| `ee183e7` | Apply explosion wakes and impulses in ascending shape-id order | `src/physics_world.c`, `test/test_tree_order.c`, `test/main.c` |

No other file changed. `test/test_determinism.c` was not changed: no golden moved (§3).

## 2. Suite counts per commit

Build and run per §7 of the prompt, both precision configurations, Debug, samples off.

| Commit | Float: tests run / passed | Double precision: tests run / passed |
|---|---|---|
| `a073352` (A0) | 29 / 29 | 29 / 29 |
| `affab6a` (A1) | 32 / 32 | 32 / 32 |
| `ee183e7` (A2) | 33 / 33 | 33 / 33 |

The counts are the entry functions `test/main.c` runs. The baseline on the branch before A0 was
28 / 28 in both configurations. Pre-existing failures: none.

## 3. Golden hashes (prompt §5)

`DeterminismTest` passed at every commit in both configurations with its pinned values untouched:
no `workers=`, `sleepStep=` or `hash=` diagnostic line was printed in any run. The four scenes
either perform no continuous collision with several candidates per query in an order the sort
changes, or do and reach the same result. No value in `test/test_determinism.c` was changed and
the file is not in the diff.

## 4. Compiler warnings and formatting

- The `test` target built with zero warnings in both configurations at every commit (GCC 14,
  C17, the repository's warning set).
- The Release benchmark build (`-DBOX3D_BENCHMARKS=ON`) prints twelve `-Wmaybe-uninitialized`
  warnings, ten from `xmmintrin.h:940` and two from `include/box3d/math_functions.h:957`. The same
  twelve appear building `main` the same way. They are pre-existing and none is in changed code.
- `clang-format` was not installed on the machine when the task started; the owner installed
  Debian `clang-format` 19.1.7 during A0, before the first commit. The new test file was formatted
  whole (it is new). The changed hunks of `src/solver.c`, `src/physics_world.c`,
  `src/physics_world.h` and `test/main.c` were formatted with `git clang-format` against the
  previous commit; a final `--diff` check reported nothing left to change at each commit.
- `test/main.c` carries a pre-existing violation at line 4 (`#include "test_macros.h"` before
  `box3d/base.h`), outside the changed hunks. Not touched.
- The formatter writes the pointer to an array macro as `b3Array( int ) * candidates;`. The same
  spelling already exists at `src/broad_phase.c:115` (`b3Array( uint64_t ) * pairKeys;`), so it was
  kept.

## 5. Fail-before and pass outputs

The fail-before runs used a scratch `git worktree` at the previous commit with the working tree's
`test/test_tree_order.c`, `test/main.c` and `test/CMakeLists.txt` copied in, built the same way,
each test run by its entry name. Every fail-before run fails on its divergence check. The
`test skipped:` lines of the filtered runner are omitted below.

### ContinuousCallbackOrderTest, before (worktree at `a073352`)

```
Starting Box3D unit tests
Filter: ContinuousCallbackOrderTest
======================================
  static corridor run 1
  custom filter: 7 28 31 25 1 29 4 5 27 3 24 26 0 2 6 30
  pre-solve: 7 28 31 25 1 29 4 5 27 3 24 26 0 2 6 30
  static corridor run 2
  custom filter: 27 7 4 31 28 3 0 26 30 25 2 24 6 5 1 29
  pre-solve: 27 7 4 31 28 3 0 26 30 25 2 24 6 5 1 29
  kinematic corridor run 1
  custom filter: 37 10 9 15 13 33 11 38 40 39 35 8 12 36 14 34
  pre-solve: 37 10 9 15 13 33 11 38 40 39 35 8 12 36 14 34
  kinematic corridor run 2
  custom filter: 37 10 13 15 9 11 33 38 40 39 35 12 8 36 34 14
  pre-solve: 37 10 13 15 9 11 33 38 40 39 35 12 8 36 34 14
  dynamic corridor run 1
  custom filter: 45 18 19 21 22 42 46 47 23 17 16 43 20 49 48 44
  pre-solve: 45 18 19 21 22 42 46 47 23 17 16 43 20 49 48 44
  dynamic corridor run 2
  custom filter: 21 18 45 19 22 42 46 47 23 17 16 20 43 49 48 44
  pre-solve: 21 18 45 19 22 42 46 47 23 17 16 20 43 49 48 44
condition false: SameSequence( log->filterLog + i, &cluster )
test failed: ContinuousCallbackOrderTest
```

All three tree kinds log the callbacks in visit order, different between the runs, and none is
ascending. The first assertion is the static corridor's; the kinematic and dynamic logs above fail
the same check.

### ContinuousCallbackOrderTest, after (working tree at `affab6a`)

```
Starting Box3D unit tests
Filter: ContinuousCallbackOrderTest
======================================
  static corridor run 1
  custom filter: 0 1 2 3 4 5 6 7 24 25 26 27 28 29 30 31
  pre-solve: 0 1 2 3 4 5 6 7 24 25 26 27 28 29 30 31
  static corridor run 2
  custom filter: 0 1 2 3 4 5 6 7 24 25 26 27 28 29 30 31
  pre-solve: 0 1 2 3 4 5 6 7 24 25 26 27 28 29 30 31
  kinematic corridor run 1
  custom filter: 8 9 10 11 12 13 14 15 33 34 35 36 37 38 39 40
  pre-solve: 8 9 10 11 12 13 14 15 33 34 35 36 37 38 39 40
  kinematic corridor run 2
  custom filter: 8 9 10 11 12 13 14 15 33 34 35 36 37 38 39 40
  pre-solve: 8 9 10 11 12 13 14 15 33 34 35 36 37 38 39 40
  dynamic corridor run 1
  custom filter: 16 17 18 19 20 21 22 23 42 43 44 45 46 47 48 49
  pre-solve: 16 17 18 19 20 21 22 23 42 43 44 45 46 47 48 49
  dynamic corridor run 2
  custom filter: 16 17 18 19 20 21 22 23 42 43 44 45 46 47 48 49
  pre-solve: 16 17 18 19 20 21 22 23 42 43 44 45 46 47 48 49
test passed: ContinuousCallbackOrderTest
======================================
All Box3D tests passed!
Test duration = 0.01 s
```

### ContinuousSensorSelectionTest, before (worktree at `a073352`)

```
Starting Box3D unit tests
Filter: ContinuousSensorSelectionTest
======================================
  run 1 sensors: 4 6 7 8 9 10 11 12
  run 2 sensors: 4 5 7 8 9 10 11 12
condition false: SameSet( reported + 0, &expected )
test failed: ContinuousSensorSelectionTest
```

The sensor shape ids are 1 to 12 (`index1`). Before the change each run keeps the first eight
visited, and the two runs keep different sets.

### ContinuousSensorSelectionTest, after (working tree at `affab6a`)

```
Starting Box3D unit tests
Filter: ContinuousSensorSelectionTest
======================================
  run 1 sensors: 1 2 3 4 5 6 7 8
  run 2 sensors: 1 2 3 4 5 6 7 8
test passed: ContinuousSensorSelectionTest
======================================
All Box3D tests passed!
Test duration = 0.00 s
```

### ContinuousCandidateOrderTest, before (worktree at `a073352`)

```
Starting Box3D unit tests
Filter: ContinuousCandidateOrderTest
======================================
  one worker: seeds 40, field divergences 1, corridor divergences static 23 kinematic 0 dynamic 1
  four workers: seeds 8, field divergences 0, corridor divergences static 5 kinematic 0 dynamic 0
condition false: counts->field == 0
condition false: CheckFuzzCounts( &single ) == 0
test failed: ContinuousCandidateOrderTest
```

Every witness differed in every seed (a witness failure would have stopped the run before the
counts). The recorded fail-before counts: field 1 of 40 seeds at one worker; corridor counts as
found, static 23, kinematic 0, dynamic 1 at one worker and static 5, kinematic 0, dynamic 0 over
the 8 four-worker seeds.

### ContinuousCandidateOrderTest, after (working tree at `affab6a`)

```
Starting Box3D unit tests
Filter: ContinuousCandidateOrderTest
======================================
  one worker: seeds 40, field divergences 0, corridor divergences static 0 kinematic 0 dynamic 0
  four workers: seeds 8, field divergences 0, corridor divergences static 0 kinematic 0 dynamic 0
test passed: ContinuousCandidateOrderTest
======================================
All Box3D tests passed!
Test duration = 2.49 s
```

### ExplodeOrderTest, before (worktree at `affab6a`)

```
Starting Box3D unit tests
Filter: ExplodeOrderTest
======================================
  run 1 multi-shape body: 2 0 1
  run 2 multi-shape body: 1 0 2
  awake set order equal 0, velocities equal 0
  run 1 awake order: 5 9 8 0 7 3 10 2 6 11 4 1
  run 2 awake order: 11 6 4 0 1 9 5 10 3 2 8 7
condition false: sameAwakeOrder
test failed: ExplodeOrderTest
```

### ExplodeOrderTest, after (working tree at `ee183e7`)

```
Starting Box3D unit tests
Filter: ExplodeOrderTest
======================================
  run 1 multi-shape body: 2 0 1
  run 2 multi-shape body: 1 0 2
  awake set order equal 1, velocities equal 1
  run 1 awake order: 0 1 2 3 4 5 6 7 8 9 10 11
  run 2 awake order: 0 1 2 3 4 5 6 7 8 9 10 11
test passed: ExplodeOrderTest
======================================
All Box3D tests passed!
Test duration = 0.00 s
```

### TreeOrderHelperTest

Passes at every commit; it has no behaviour to fail on before A0 (design §3). Its three witnesses
differed on the first run. For the record, the sequences on the A0 working tree with the first
placement rule (six boxes on a line, later replaced, §6.3): static `10 2 11 9 0 1` against
`10 2 11 9 1 0`, kinematic `15 14 13 5 4 3` against `5 15 14 13 3 4`, dynamic `18 8 19 7 6 17`
against `19 8 18 7 6 17`, explosion `0 3 1 5 4 8 6 7 2 9` against `5 0 3 8 4 2 9 1 6 7`.

## 6. Deviations from the prompt and the design, with reasons

1. **Scene hooks carry a context pointer and return an int.** The helper contract names the
   world, the run index and the step index as the hooks' arguments. The tests need per-run
   storage (body ids, logs, witnesses) and a way to fail from inside a hook with the usual
   `condition false:` line, so each hook also receives the scene's context pointer and returns
   nonzero to fail the pair run. The hash hook writes the scene's hash values into the pair result.
   The two runs execute one after the other in two fresh worlds; comparisons happen after both.
2. **Callback logs hold internal shape ids.** The prompt says the callbacks append the other
   shape's `index1`. The logs append `index1 - 1`, the internal shape id, so they compare directly
   with the witness and cluster sets, which are internal ids from the tree's user data. The
   assertion is the same: the custom-filter log equals the sorted cluster and the pre-solve log the
   sorted on-path set, in both runs.
3. **Corridor placement rule.** The first rule, six boxes on the sphere's path line with
   interleaved parts, gave an unchanged static-tree witness in the first fuzz seed. The second,
   eight boxes scattered in three dimensions inside the swept cylinder, gave an unchanged dynamic
   witness in the first seed. The rule was measured over 108 seeds (100 one-worker, 8 four-worker)
   with a temporary counting patch: 8 boxes, 30 witness coincidences (20 dynamic, 10 kinematic);
   12 boxes, 5 (3 dynamic, 2 kinematic); 16 boxes, none. The committed rule is 16 boxes, 8 per
   part, scattered in the cylinder with 0.35 m separation, sphere radius 1.0 at 300 m/s, box half
   extent 0.05, applied to every corridor of every kind. Every box is on the path (the contract
   asks for at least two); the on-path set is still computed from the time-of-impact call and both
   runs' sets are required equal.
4. **On-path and leaf-bound checks happen in the before-step hook of the corridor step**, after
   run 2's perturbation, using the solver's own `b3ShapeTimeOfImpact` and `b3MakeRelativeSweep`
   over the sphere's full one-step sweep. The contract does not say where; this is the last point
   before the query at which the trees are known.
5. **Explode test shape placement.** The first placement of the three shapes (`0.5 0.1 -0.2`,
   `-0.4 0.3 0.35`, `0.1 -0.5 0.4` about a body at `1.0 0.8 -0.6`) printed `velocities equal 1`
   before the change, because the three shapes were visited in the same relative order in both
   runs. Per the prompt they were moved wide apart (`1.5 0.2 -0.3`, `-1.1 1.2 0.4`,
   `0.1 -1.3 -1.4` about `0.4 0.3 -0.2`); the fail-before then prints `velocities equal 0` and the
   test also prints the multi-shape body's own visit order in each run. Neither check was dropped.
6. **High-water mark and sort cost** were read through a temporary `printf` in
   `b3DestroyWorkerContexts` and a temporary `b3GetTicks` pair around the `QSORT` call, built in a
   separate Release directory, run through the scratch harness, and reverted with `git checkout`
   before anything was committed. The `QSORT` call lives in the static helper
   `b3EvaluateContinuousCandidates` that `b3SolveContinuous` calls after each query, so the pair
   was placed there.
7. **The fast flag helper** looks the world up with `b3GetWorld( bodyId.world0 )` rather than
   `b3GetWorldFromId`, which needs the world id's generation that a body id does not carry.
8. **PR not opened.** The task was to land the three commits and report; the PR text is drafted in
   §9 for the owner to open. The samples were not built or run, so the template's samples box is
   left unticked with a sentence saying what was run.

No disagreement between the prompt and the design was found that the code had to resolve, and
the design was silent on nothing the code forced.

## 7. Measurements (prompt §7)

Machine: 12 logical cores (WSL2, Linux 6.18), GCC 14, Release builds with the prompt's options,
`main` at `5643cd8` built in its own worktree and run from its own directory, the branch at
`ee183e7`. A first attempt ran while builds and test suites shared the machine; it was stopped,
its outputs deleted, and every number below comes from one clean back-to-back run.

### 7.1 Suite benchmark, csv totals in milliseconds per thread count, `main` against the branch

Stated as a no-regression check: the suite has no bullet-heavy scene.

| Scene | Revision | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|---|
| convex_pile | main | 10479.2 | 6128.73 | 4530.41 | 3723.79 | 3282.94 | 3257.24 | 3418.26 | 3550 |
| | branch | 10724.4 | 6175.31 | 4670.3 | 3792.93 | 3391.5 | 3252.51 | 3329.41 | 3350.63 |
| | branch / main | 1.023 | 1.008 | 1.031 | 1.019 | 1.033 | 0.999 | 0.974 | 0.944 |
| joint_grid | main | 3599.79 | 1830.58 | 1253.2 | 944.44 | 831.417 | 770.624 | 818.354 | 795.446 |
| | branch | 3079.59 | 1714.01 | 1249.33 | 1012.14 | 910.975 | 832.692 | 901.526 | 854.546 |
| | branch / main | 0.855 | 0.936 | 0.997 | 1.072 | 1.096 | 1.081 | 1.102 | 1.074 |
| junkyard | main | 22633.9 | 13489.5 | 10257.2 | 8477.33 | 7614.44 | 7385.19 | 7484.07 | 7559.62 |
| | branch | 23106.5 | 13299.2 | 10202.8 | 8715.3 | 7703.5 | 7471.79 | 7396.87 | 7434.68 |
| | branch / main | 1.021 | 0.986 | 0.995 | 1.028 | 1.012 | 1.012 | 0.988 | 0.983 |
| large_pyramid | main | 3524.58 | 2002.53 | 1466.46 | 1248.62 | 1105.72 | 1055.83 | 1106.13 | 1100.11 |
| | branch | 3508.77 | 1938.82 | 1455.17 | 1247.93 | 1109.14 | 1027.73 | 1094.72 | 1081.33 |
| | branch / main | 0.996 | 0.968 | 0.992 | 0.999 | 1.003 | 0.973 | 0.990 | 0.983 |
| large_world | main | 10.4299 | 21.1582 | 37.7175 | 61.1158 | 70.6926 | 79.4723 | 83.9526 | 93.3691 |
| | branch | 10.2405 | 21.6716 | 38.8784 | 60.914 | 70.2033 | 77.3776 | 83.9687 | 90.6177 |
| | branch / main | 0.982 | 1.024 | 1.031 | 0.997 | 0.993 | 0.974 | 1.000 | 0.971 |
| many_pyramids | main | 3620.25 | 2103.52 | 1594.53 | 1352.78 | 1241.41 | 1157.32 | 1181.02 | 1171.33 |
| | branch | 3570.99 | 2085.14 | 1559.34 | 1330.96 | 1211.29 | 1118.23 | 1141.33 | 1126.98 |
| | branch / main | 0.986 | 0.991 | 0.978 | 0.984 | 0.976 | 0.966 | 0.966 | 0.962 |
| rain | main | 4044.31 | 2349.96 | 1777.12 | 1488.79 | 1355.77 | 1289.67 | 1320.15 | 1353.76 |
| | branch | 3996.98 | 2351.81 | 1753.55 | 1448.64 | 1303.24 | 1252.21 | 1312.51 | 1320.49 |
| | branch / main | 0.988 | 1.001 | 0.987 | 0.973 | 0.961 | 0.971 | 0.994 | 0.975 |
| sleep | main | 1616.27 | 941.827 | 721.577 | 611.821 | 563.184 | 545.542 | 576.597 | 596.389 |
| | branch | 1619.24 | 952.376 | 757.104 | 623.385 | 555.916 | 538.595 | 560.417 | 591.689 |
| | branch / main | 1.002 | 1.011 | 1.049 | 1.019 | 0.987 | 0.987 | 0.972 | 0.992 |
| spinner | main | 5876.68 | 3654.13 | 2915.38 | 2545.61 | 2391.42 | 2326.28 | 2461.57 | 2526.11 |
| | branch | 5823.83 | 3640.66 | 2842.32 | 2527.91 | 2372.21 | 2370.77 | 2466.88 | 2552.55 |
| | branch / main | 0.991 | 0.996 | 0.975 | 0.993 | 0.992 | 1.019 | 1.002 | 1.010 |
| trees100 | main | 358.044 | 250.886 | 217.702 | 209.137 | 220.33 | 228.802 | 244.978 | 259.518 |
| | branch | 361.912 | 252.105 | 211.097 | 210.381 | 214.867 | 224.816 | 246.179 | 249.381 |
| | branch / main | 1.011 | 1.005 | 0.970 | 1.006 | 0.975 | 0.983 | 1.005 | 0.961 |
| trees25 | main | 1433.09 | 881.819 | 669.424 | 601.395 | 570.702 | 564.15 | 572.042 | 586.066 |
| | branch | 1396.01 | 869.099 | 681.664 | 603.478 | 563.763 | 559.165 | 578.934 | 589.023 |
| | branch / main | 0.974 | 0.986 | 1.018 | 1.003 | 0.988 | 0.991 | 1.012 | 1.005 |
| trees50 | main | 600.632 | 387.923 | 325.045 | 313.724 | 311.075 | 312.733 | 334.121 | 338.953 |
| | branch | 596.984 | 392.728 | 330.056 | 332.734 | 339.466 | 318.547 | 328.525 | 337.192 |
| | branch / main | 0.994 | 1.012 | 1.015 | 1.061 | 1.091 | 1.019 | 0.983 | 0.995 |
| washer | main | 34206.5 | 19746.5 | 15088.2 | 13407.4 | 12337.6 | 11941.6 | 12149 | 11601 |
| | branch | 33205.4 | 19466.9 | 14896.2 | 13008.2 | 12198.2 | 11927.6 | 12038.7 | 11527.9 |
| | branch / main | 0.971 | 0.986 | 0.987 | 0.970 | 0.989 | 0.999 | 0.991 | 0.994 |

Run back to back on a quiet machine (browser closed, load average below 2), `main` from 13:02 to
13:27 and the branch from 13:27 to 13:53 local time, each benchmark writing its csv into its own
directory. The csv value is the minimum over the benchmark's four runs per thread count. The ratios
scatter on both sides of 1.0 by a few percent, with the largest moves in the small, noisy scenes
(`joint_grid` at one thread 0.855, `trees50` at five threads 1.091), and no scene moves one way at
every thread count. No regression attributable to A1 is visible; the suite exercises the
continuous stage lightly and has no bullet-heavy scene.

### 7.2 Scratch harness

The harness is an uncommitted program in the scratch directory built against the Release library
of each revision with only the public API. It runs the fuzz scene of `ContinuousCandidateOrderTest`
(seed 7 of the one-worker set: 140 static boxes, 60 fast bodies of which 40 are bullets, three
corridors whose second parts are created after the first step, ten steps) at one and at four
workers, and a dense scene (the sensor scene scaled to 64 sensor spheres scattered inside one
bullet's swept cylinder, the bullet reset every step so each step runs the same 64-candidate
query), printing per step the minimum over ten repeats of `b3World_GetProfile` `transforms` and
`bullets` in milliseconds and once at the end the `b3World_GetCounters` high-water marks.

**Fuzz scene, one worker.** Per step, minimum over ten repeats, milliseconds: `transforms` main / branch, `bullets` main / branch.

| Step | transforms main | transforms branch | bullets main | bullets branch |
|---|---|---|---|---|
| 0 | 0.0150 | 0.0137 | 0.0232 | 0.0211 |
| 1 | 0.0318 | 0.0326 | 0.0626 | 0.0541 |
| 2 | 0.0250 | 0.0238 | 0.0555 | 0.0480 |
| 3 | 0.0194 | 0.0180 | 0.0399 | 0.0361 |
| 4 | 0.0184 | 0.0170 | 0.0396 | 0.0315 |
| 5 | 0.0215 | 0.0196 | 0.0284 | 0.0276 |
| 6 | 0.0199 | 0.0182 | 0.0339 | 0.0279 |
| 7 | 0.0193 | 0.0177 | 0.0319 | 0.0259 |
| 8 | 0.0203 | 0.0185 | 0.0302 | 0.0285 |
| 9 | 0.0292 | 0.0263 | 0.0328 | 0.0294 |

Counters at the end, main: `distanceIterations 4 pushBackIterations 3 rootIterations 6`; branch: `distanceIterations 4 pushBackIterations 3 rootIterations 6`.

**Fuzz scene, four workers.** Per step, minimum over ten repeats, milliseconds: `transforms` main / branch, `bullets` main / branch.

| Step | transforms main | transforms branch | bullets main | bullets branch |
|---|---|---|---|---|
| 0 | 0.0114 | 0.0139 | 0.0725 | 0.0399 |
| 1 | 0.0263 | 0.0283 | 0.0629 | 0.0681 |
| 2 | 0.0206 | 0.0201 | 0.0689 | 0.0611 |
| 3 | 0.0166 | 0.0136 | 0.0672 | 0.0521 |
| 4 | 0.0133 | 0.0146 | 0.0336 | 0.0388 |
| 5 | 0.0178 | 0.0173 | 0.0633 | 0.0564 |
| 6 | 0.0149 | 0.0163 | 0.0261 | 0.0226 |
| 7 | 0.0124 | 0.0147 | 0.0609 | 0.0461 |
| 8 | 0.0130 | 0.0120 | 0.0213 | 0.0246 |
| 9 | 0.0190 | 0.0170 | 0.0560 | 0.0506 |

Counters at the end, main: `distanceIterations 4 pushBackIterations 3 rootIterations 6`; branch: `distanceIterations 4 pushBackIterations 3 rootIterations 6`.

**Dense scene, one worker, 64 sensor candidates in one query.** Per step, minimum over ten repeats, milliseconds: `transforms` main / branch, `bullets` main / branch.

| Step | transforms main | transforms branch | bullets main | bullets branch |
|---|---|---|---|---|
| 0 | 0.0002 | 0.0002 | 0.1135 | 0.1124 |
| 1 | 0.0002 | 0.0002 | 0.1131 | 0.1137 |
| 2 | 0.0002 | 0.0002 | 0.1132 | 0.1135 |
| 3 | 0.0002 | 0.0002 | 0.1130 | 0.1109 |
| 4 | 0.0002 | 0.0002 | 0.1130 | 0.1101 |
| 5 | 0.0002 | 0.0002 | 0.1131 | 0.1100 |
| 6 | 0.0002 | 0.0002 | 0.1130 | 0.1134 |
| 7 | 0.0002 | 0.0002 | 0.1131 | 0.1134 |
| 8 | 0.0002 | 0.0002 | 0.1126 | 0.1135 |
| 9 | 0.0002 | 0.0002 | 0.1125 | 0.1134 |

Counters at the end, main: `distanceIterations 0 pushBackIterations 0 rootIterations 0`; branch: `distanceIterations 0 pushBackIterations 0 rootIterations 0`.

The continuous iteration counters are identical between the revisions in all three scenes, so in
these scenes the candidate order did not change the work of any time-of-impact call that set a
high-water mark. The dense scene's counters are zero: its candidates are all sensors, whose hits
do not feed the counters. The `transforms` stage, where plain fast bodies run continuous collision
inside finalize, and the `bullets` stage both sit at tens of microseconds and differ between
revisions by less than the step to step scatter; the branch is not slower in any scene.

### 7.3 Sort cost

Temporary `b3GetTicks` pair around the `QSORT` call alone, dense scene, one worker, ten steps.
Per query: 64 candidates sorted in 1.0 to 3.0 µs (ten queries: 1.0, 1.0, 1.1, 1.2, 1.2, 1.3, 1.6,
1.6, 2.1, 3.0). The bullet's kinematic and dynamic tree queries in that scene return one
candidate (its own shape) and sort in 0.0 to 0.2 µs. Removed before commit.

### 7.4 High-water mark of `continuousCandidates`

Capacity per worker after the scene, through a temporary log line at worker-context destruction,
removed before commit. Fuzz scene, one worker: 32 (the reserve; the largest query held 17
candidates). Fuzz scene, four workers: 32 on each of the four workers. Dense scene: 64 (one growth
from 32).

### 7.5 Fuzz seeds whose field hash diverges before the change

From the fail-before run of §5: field 1 of 40 seeds at one worker, 0 of 8 at four workers; beside
it the corridor counts static 23, kinematic 0, dynamic 1 (one worker) and static 5, kinematic 0,
dynamic 0 (four workers). Supporting measurement from the placement-rule experiment with the same
corridor rule over 100 one-worker seeds: field 6 of 100.

## 8. Out-of-scope observations

- Before the change the static corridor's hash diverged in 23 of 40 seeds while the kinematic and
  dynamic corridors' diverged in 0 and 1. The static corridor's sphere is a plain fast body, the
  others are bullets; in all three the sphere hits the same nearest box, so the difference is the
  impact fraction a time-of-impact call returns under a different running limit, the unbounded
  change design §1 names. After the change all counts are zero.
- A sensor candidate whose time-of-impact call misses returns the running fraction, is admitted
  (fraction no greater than the running one) and takes one of the eight slots, then is dropped by
  the end filter (strictly below the final fraction). Design §5 states this. The sensor test keeps
  every sensor on the bullet's path for that reason.
- For small clusters the insertion layout and a full build of a tree coincide often enough to
  matter (§6.3 numbers). A test relying on a witness needs a cluster of a dozen or more leaves, or
  a structural reason for the layouts to differ.
- `docs/simulation.md` line 1941 says the pre-solve callback runs inside the parallel-for and must
  be thread-safe; the new sentence beside the pre-solve example agrees with it. The sensor passage
  about passage within a step was not touched.
- The continuous pre-solve call has no null check on the registered callback (prompt §2). Not
  changed; the callback test registers one in both worlds before any step.
- The Release build's twelve `-Wmaybe-uninitialized` warnings (§4) are pre-existing on `main`.
- `benchmark/main.c` clamps `-t` to the detected core count; the machine has 12, so `-t=8` ran
  all eight thread counts in both revisions.

## 9. PR text draft (prompt §8)

Results are not bit-identical to before in the cases the design's §1 states: a continuous
collision sweep with several candidates in one tree query, the continuous iteration counters, and
an explosion touching a multi-shape body or waking more than one sleeping set.

### Pull request template

- [x] run unit tests
- [ ] run all samples

The `test` target was built and run at each of the three commits in both the float and the
`BOX3D_DOUBLE_PRECISION` configurations, Debug, with `BOX3D_SAMPLES` off; the samples were not
built or run.

### Commits

1. `a073352` Add a tree-layout perturbation test helper with a visit-order witness. Test only. A
   helper that runs a scene twice, fully rebuilds every broad-phase tree before each step of the
   second run, records the candidate order a tree query hands back, and hashes world state. Its
   sanity test shows the order changes at the continuous and explosion queries and that a scene
   with no fast body, sensor or explosion hashes the same. This test does not fail before the
   change: it has no behaviour to fail on.
2. `affab6a` Evaluate continuous collision candidates in ascending shape-id order. The tree
   callback only collects shape ids into a per-worker array on `b3TaskContext`; after each of the
   three tree queries the array is sorted and the existing per-candidate logic runs in that
   order. `docs/simulation.md` now states where the pre-solve callback is issued from continuous
   collision and in what order. Three tests fail before and pass after.
3. `ee183e7` Apply explosion wakes and impulses in ascending shape-id order. `ExplosionCallback`
   collects shape ids with a nonzero explosion scale; after the query they are sorted and the
   existing wake and impulse logic runs per id. One test fails before and passes after.

### Measurements

Benchmark (no-regression check, the suite has no bullet-heavy scene): §7.1 above. Scratch harness
transforms and bullets stages and iteration counters: §7.2. Sort cost: §7.3. Candidate array
high-water mark: §7.4. Fuzz seeds whose field hash diverges before the change: §7.5.

### Golden hashes

None changed in `test/test_determinism.c`; both precision configurations pass at every commit.

### Stateful callbacks

`preSolve` and the custom filter are called in ascending shape id within each tree query, so a
callback that depends only on its arguments gives a layout-independent result. A callback that
keeps state stays layout-dependent: fast bodies are processed on parallel workers, so calls from
different bodies interleave as before, and the custom filter is also called during pair discovery
and sensor overlap in tree order.

### Why the A0 test does not fail before

A0 adds a test helper and its sanity test and changes no behaviour, so there is nothing for its
test to fail on at the previous commit.

## 10. Review checklist (design §3), item by item

- [x] Diff touches only the files listed for its changes, plus a test and the docs sentence. Per
      commit file lists in §1; `test/CMakeLists.txt` and `test/main.c` are the registration files
      the prompt names.
- [x] New tests fail on `main` and pass on the branch (or the PR says why one cannot fail). §5;
      `TreeOrderHelperTest` cannot and the PR text says why.
- [x] Existing `test` target passes, including `test_determinism.c`. §2 and §3.
- [x] Benchmark delta quoted for A1. §7.1.
- [x] PR text states the not-bit-identical-to-before consequence in one line. §9, first paragraph.
