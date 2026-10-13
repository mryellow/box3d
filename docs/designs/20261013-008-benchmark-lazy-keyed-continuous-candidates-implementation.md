---
title: Benchmark - lazily keyed continuous candidates, implementation measurements
status: Draft
date: 2026-10-13
follows: 20261013-005 (preflight), 20261013-001 (sweep-order implementation benchmark)
---

# Benchmark: lazily keyed continuous candidates, implementation measurements

Measurements of the implementation of 20261013-005 against the sweep-ordered head it stacks on,
item by item of that preflight's §3 checklist. The change claims no gain; it claims no cost, and
the same evaluation order wherever the order exists.

## 1. Result

Every item of the preflight's §3 checklist holds.

- Order witness (§3): on every project scenario and on `ccdbench.c` in both directions, the branch
  computes zero keys and runs zero sorts for queries with fewer than two candidates, exactly two
  keys per candidate-pair and one sort for the rest, and the hash of the evaluation order over the
  sorted queries equals the sweep head's. Same keys, same order, wherever an order exists.
- Microbenchmark (§4): within the spread in both directions, non-bullet and bullet.
- Project benchmarks (§5): total step time within the run's spread on every scenario; the
  continuous-stage clock is 1 to 6 percent lower on the branch on nine of ten scenarios, which is
  the removed work showing in a stage that is a few percent of the step.
- Tests and scope (§6): 36 of 36 tests pass on both lineages; the diff is `src/solver.c` only.

The change claims no gain and delivers none that the total step time can see. It removes the
sweep-ordered head's key, radius and sort work from the one-candidate path by construction, and the
counts show that path is every query on eight of the ten scenarios.

## 2. Setup

- Builds: Release, AVX2 kernels, `BOX3D_VALIDATE` off, static library, GCC 14.2 at the project's
  Release flags. Baseline: the sweep-ordered head `854bd28`. Branch: `78397ad`, one commit on that
  head, touching `src/solver.c` only.
- Counting builds: both heads also built with `-DB3_CCD_COUNT=1` (the measurement build of
  20261013-004) plus counters of keys computed, sorts run, queries with two or more candidates, and
  a hash of the evaluation order of those queries: for each candidate in evaluation order, the
  key's bits then the shape id, folded into a 64-bit FNV-1a hash. Equal hashes between the two
  builds mean the same keys in the same order on every query that was sorted.
- Machine: Intel i5-10400F under WSL2, otherwise idle, turbo and thermal state uncontrolled. Rows
  compare within themselves; the alternating order inside each pair is what controls the drift.
- Project scenarios: every scenario in `benchmark/main.c` with continuous-stage work, at its project
  step count, single worker. Total step time from the project benchmark binary (`-w=1 -r=1`),
  interleaved pairs alternating which build runs first. Continuous-stage time from the counting
  builds, a monotonic-clock sum around `b3SolveContinuous`, interleaved the same way.
- Microbenchmark: `ccdbench.c` (20261012-004 appendix), 4000 spheres at 300 m/s through 14,641
  static boxes, 100 steps, four sub-steps, one worker, minimum of three worlds; both travel
  directions; non-bullet runs report the total, bullet runs the continuous stage. Pairs alternate
  which build runs first.

## 3. Order witness, counting builds

Counting builds of both heads, one run each; counts are deterministic and matched between runs.

`ccdbench.c`, 4000 spheres, 100 steps, one world.

| Run | Build | Queries | Candidates | Keys | Sorts | Multi-candidate queries | Order hash |
|---|---|---|---|---|---|---|---|
| +x | sweep head | 225,400 | 2,430,923 | 2,430,923 | 396,500 | 223,400 | `2e2bfa09c347b7ed` |
| +x | branch | 225,400 | 2,430,923 | 2,428,923 | 223,400 | 223,400 | `2e2bfa09c347b7ed` |
| -x | sweep head | 226,400 | 2,480,121 | 2,480,121 | 397,600 | 224,800 | `74d20b6ae37b23a9` |
| -x | branch | 226,400 | 2,480,121 | 2,478,521 | 224,800 | 224,800 | `74d20b6ae37b23a9` |
| +x bullets | sweep head | 225,400 | 2,430,923 | 2,430,923 | 1,189,500 | 223,400 | `2e2bfa09c347b7ed` |
| +x bullets | branch | 225,400 | 2,430,923 | 2,428,923 | 223,400 | 223,400 | `2e2bfa09c347b7ed` |

The sweep head keys every collected candidate and runs the sort on every query, including the empty
kinematic and dynamic queries of a bullet. The branch keys exactly the candidates of multi-candidate
queries (candidates less the one-candidate queries: 2,430,923 - 2,000 in +x) and sorts exactly those
queries. The order hash is equal in every row pair: the same keys in the same order on every sorted
query.

Project scenarios at their step counts, one worker. Queries and candidates are equal between the
builds; the table lists them once.

| Scenario | Non-empty queries | Candidates | Multi-candidate queries | Keys, sweep head | Keys, branch | Sorts, sweep head | Sorts, branch | Order hash equal |
|---|---|---|---|---|---|---|---|---|
| mesh_drop | 283,785 | 283,785 | 0 | 283,785 | 0 | 838,848 | 0 | yes |
| spinner | 1,170,613 | 1,170,613 | 0 | 1,170,613 | 0 | 1,171,413 | 0 | yes |
| rain | 123,864 | 123,871 | 7 | 123,871 | 14 | 646,330 | 7 | yes |
| trees100 | 1,342 | 1,342 | 0 | 1,342 | 0 | 118,235 | 0 | yes |
| trees50 | 1,537 | 1,537 | 0 | 1,537 | 0 | 118,398 | 0 | yes |
| trees25 | 1,757 | 1,757 | 0 | 1,757 | 0 | 118,480 | 0 | yes |
| village | 32,296 | 32,296 | 0 | 32,296 | 0 | 58,891 | 0 | yes |
| convex_pile | 520 | 520 | 0 | 520 | 0 | 378,853 | 0 | yes |
| junkyard | 463 | 503 | 40 | 503 | 80 | 360,760 | 40 | yes |
| washer | 7,734 | 7,734 | 0 | 7,734 | 0 | 4,262,345 | 0 | yes |

The branch computes zero keys and runs zero sorts on every scenario whose queries all hold one
candidate, and exactly two keys and one sort per multi-candidate query on junkyard and rain, the two
scenarios that have any. The sweep head's sort count exceeds its query count because it also ran the
sort on every empty query. The order hash is equal on every scenario, so the forty junkyard queries
and seven rain queries that were sorted evaluated the same candidates in the same order.

## 4. Microbenchmark, both directions

Each pair runs both binaries back to back in the order shown; the machine warmed through the run,
so the sweep head's +x total rose 9 percent from the first pair to the third while the pair deltas
stayed inside 3 percent either way.

Non-bullet, total ms.

| Direction | Pair | Order | Sweep head | Branch | Delta |
|---|---|---|---|---|---|
| +x | 1 | head, branch | 4262.0 | 4220.2 | -1.0% |
| +x | 2 | branch, head | 4521.3 | 4398.4 | -2.7% |
| +x | 3 | head, branch | 4637.0 | 4570.7 | -1.4% |
| -x | 1 | head, branch | 4694.8 | 4739.9 | +1.0% |
| -x | 2 | branch, head | 4873.5 | 4794.0 | -1.6% |
| -x | 3 | head, branch | 5011.5 | 4992.8 | -0.4% |

Bullets, continuous stage ms.

| Direction | Pair | Order | Sweep head | Branch | Delta |
|---|---|---|---|---|---|
| +x | 1 | head, branch | 4590.4 | 4445.9 | -3.1% |
| +x | 2 | branch, head | 4462.4 | 4478.0 | +0.3% |
| -x | 1 | head, branch | 4460.4 | 4478.7 | +0.4% |
| -x | 2 | branch, head | 4452.9 | 4350.0 | -2.3% |

Within the spread in both directions, as the preflight expected: every query here holds about
eleven candidates, so the branch does the sweep head's work plus one shape load per candidate, and
that load is not measurable against the time-of-impact calls.

## 5. Project benchmarks, interleaved pairs

Total step time, project benchmark binary, `-w=1 -r=1`, six interleaved pairs per scenario (eight
for washer), alternating which build runs first. Medians of the per-pair deltas; the range is the
spread of those deltas.

| Scenario | Pairs | Sweep head median ms | Branch median ms | Delta median | Delta range |
|---|---|---|---|---|---|
| mesh_drop | 6 | 2133.3 | 2131.9 | -0.1% | -2.5 to +0.8 |
| spinner | 6 | 3728.4 | 3704.4 | -0.3% | -5.9 to +1.4 |
| rain | 6 | 3064.4 | 3070.3 | -0.1% | -0.6 to +0.7 |
| trees100 | 6 | 171.6 | 172.5 | +0.3% | -2.5 to +4.6 |
| trees50 | 6 | 270.2 | 270.0 | -0.3% | -2.2 to +8.8 |
| trees25 | 6 | 602.9 | 596.9 | -0.8% | -2.6 to +1.1 |
| village | 6 | 3726.7 | 3729.3 | +0.1% | -0.9 to +4.5 |
| convex_pile | 6 | 8868.9 | 8892.8 | +0.7% | -4.9 to +14.9 |
| junkyard | 6 | 21802.7 | 21975.7 | -0.2% | -4.7 to +3.5 |
| washer | 8 | 30970.2 | 31188.9 | +0.6% | -1.2 to +5.0 |

Washer ran four more pairs after its first four gave a +2.5 percent median: the continuous stage is
3 percent of washer's step and its own clock was lower on the branch, so the total could not have
moved for the stage's reason. Over eight pairs the median is +0.6 percent with pairs on both sides,
and the machine's drift across the 30-second runs (the sweep head alone spans 28.9 to 31.6 seconds)
is the spread.

Continuous-stage time, counting builds, monotonic clock around `b3SolveContinuous`, four interleaved
pairs per scenario (two for washer).

| Scenario | Pairs | Sweep head median ms | Branch median ms | Delta median | Delta range |
|---|---|---|---|---|---|
| mesh_drop | 4 | 540.4 | 522.0 | -3.3% | -4.5 to -3.2 |
| spinner | 4 | 782.8 | 733.3 | -6.2% | -8.3 to -1.8 |
| rain | 4 | 219.3 | 214.3 | -2.9% | -4.7 to -1.7 |
| trees100 | 4 | 29.23 | 27.50 | -5.9% | -6.6 to -4.1 |
| trees50 | 4 | 36.87 | 36.70 | -0.5% | -4.1 to +0.5 |
| trees25 | 4 | 65.01 | 64.88 | +1.4% | -9.4 to +2.4 |
| village | 4 | 42.77 | 40.80 | -3.5% | -9.9 to -0.1 |
| convex_pile | 4 | 87.88 | 86.46 | -1.6% | -3.1 to +1.3 |
| junkyard | 4 | 86.77 | 84.20 | -3.1% | -7.6 to -0.3 |
| washer | 2 | 1053.2 | 1042.1 | -1.1% | -1.4 to -0.7 |

The stage itself is lower on the branch on every scenario but trees25, by 1 to 6 percent of a stage
that is itself a few percent of the step. That is the removed key, radius and sort work on the
one-candidate path, visible in the stage clock and invisible in the total, as the preflight
predicted.

## 6. Tests and diff scope

- Debug build with `BOX3D_VALIDATE` on at `78397ad`: all 36 tests of the `test` target pass, run
  singly (the runner stops at the first failure, so one run of the whole target reports only the
  first; an unknown filter name runs nothing and exits 0, so the list of names was taken from
  `test/main.c`). The
  sweep-ordered order tests `ContinuousCallbackOrderTest` and `ContinuousSensorSelectionTest` and
  the layout tests of 20261012-003 are among them and pass unchanged: the order is the same.
- The same 36 tests pass singly on the design-document branch port, Debug with `BOX3D_VALIDATE` on.
- Diff scope: `git diff 854bd28 78397ad --stat` is `src/solver.c` only, 66 insertions and 27
  deletions. The candidate element type and the per-worker array are unchanged; the header is not
  touched.

## 7. Branches

The implementation lives in two lineages, as the sorted-candidate set did.

- `solver-tree-order-independent-pr` (fork of `main` at `5d83df8`): `854bd28` sweep order, then
  `78397ad` lazy keying, fast-forwarded onto the proof-tests head `8b18782`. These are the commits
  measured above.
- `solver-tree-order-independent` (the design-document branch, forked from `5643cd8`): the same two
  changes cherry-picked as `002a5ae` and `19d9ee0`. The older base lacks `b3IsShapeFast` and the
  `safetyFactor` classification and keeps the continuous stall log with its visit count, so both
  picks conflicted in `src/solver.c` around the collection callback and the per-shape setup. The
  resolution keeps the old base's visit count and omits the fast-shape test it does not have, and
  adds the `aabb.h` include the key's slab test and farthest-corner helper need, and declares the
  fast body inside the old base's per-candidate stall log, whose declaration had moved into the
  callback with the filters. The lazy-keying hunks are identical between the two lineages
  (`git diff` of the two commits, compared line by line); the sweep hunks differ only by those two
  lines.

## 8. Caveats

- One machine, one compiler, uncontrolled turbo and thermal state. Pair deltas of several percent
  either way occur on short scenarios (trees100 at 172 ms) and on the long ones that heat the
  machine (convex_pile's +14.9 percent pair); the medians and the alternating order are what the
  tables rest on.
- The counting builds carry the counters' own cost inside the stage clock, equally in both builds.
- The order hash covers the queries that were sorted. A one-candidate query has no order to
  witness; its evaluation is the sweep head's evaluation of the same single candidate.
- The microbenchmark's one-candidate queries are 2,000 of 225,400, so it measures the
  multi-candidate path's cost (one extra shape load per candidate), not the common path's saving.
