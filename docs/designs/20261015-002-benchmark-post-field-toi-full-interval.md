---
title: Benchmark - post_field on main with the time of impact always run over [0, 1]
status: Draft
date: 2026-10-15
follows: 20261014-001 (post field, the sweep key), 20261014-003 (master against main, Linux)
---

# Benchmark: `post_field` on `main` with the time of impact always run over [0, 1]

Question (Erin): how much does `post_field` slow down if the time of impact is run over the full
interval [0, 1] for every candidate, with no other change?

## 1. Result

**`post_field` runs 61 to 67 percent slower** when every candidate's time of impact is run over
[0, 1] instead of [0, best fraction so far]. At one thread the step total goes from 5.24 s to
8.40 s over the 100-step run; at eight threads from 1.17 s to 1.86 s. Every thread count's 95
percent interval excludes zero by a wide margin, and the smallest of the 80 paired ratios is
+46 percent.

| Threads | main median ms | [0, 1] median ms | Change % | 95% interval | Per-round range % | CV main / [0, 1] % |
|---|---|---|---|---|---|---|
| 1 | 5237.8 | 8399.3 | +61.0 | [+57.7, +64.4] | +53.0 to +68.8 | 2.2 / 2.1 |
| 2 | 2836.4 | 4574.3 | +63.1 | [+58.1, +68.3] | +53.6 to +77.8 | 4.2 / 3.7 |
| 3 | 1946.5 | 3282.8 | +67.1 | [+61.5, +72.9] | +54.0 to +79.6 | 5.0 / 6.0 |
| 4 | 1563.9 | 2589.2 | +66.4 | [+60.0, +72.9] | +56.6 to +79.5 | 6.7 / 6.0 |
| 5 | 1303.7 | 2197.2 | +66.6 | [+59.4, +74.1] | +55.6 to +80.1 | 7.2 / 6.6 |
| 6 | 1179.2 | 1914.5 | +63.9 | [+55.6, +72.6] | +46.0 to +80.8 | 8.6 / 6.6 |
| 7 | 1142.1 | 1894.1 | +64.9 | [+55.3, +75.1] | +47.0 to +85.9 | 7.7 / 7.9 |
| 8 | 1167.6 | 1859.0 | +60.8 | [+52.1, +70.0] | +47.9 to +82.2 | 8.3 / 8.0 |

Positive means the [0, 1] build is slower. Change is the geometric mean of the ten per-round
ratios, minus one; the interval is a Student-t 95 percent interval (9 degrees of freedom) on the
log ratio. Medians are of ten samples per side.

Why it costs this much: on `main` the running best fraction is passed to `b3ShapeTimeOfImpact` as
`maxFraction`, which becomes `tMax` in `b3TimeOfImpact`. A candidate whose shapes are still apart
at `tMax` exits as separated after one or two distance queries. With `tMax` fixed at 1 every
candidate runs its root finder to its own impact, or to the end of the sweep, whether or not an
earlier hit already makes the result irrelevant. In this scene a query holds 135 candidates on
average and 98 percent of queries hit (20261014-001 §4), so almost every query pays the full
solve on more than a hundred candidates that the shrinking interval would have dismissed. The
accepted hit is the same in both builds: the acceptance test
`0 < output.fraction < continuousContext->fraction` is unchanged, so the [0, 1] build does more
work to reach the same answer. The contact count printed at the end of every run is identical on
both sides (354).

Not measured here: the sweep key (20261014-001), which is on `master` and not on `main`. This
record is `main` against `main` plus the one-line change, as asked.

## 2. The change

One line in `src/solver.c`, in `b3ContinuousQueryCallback` (`main` evaluates each candidate in
the tree query callback; there is no candidate array or sweep key on `main`):

```c
-	b3TOIOutput output = b3ShapeTimeOfImpact( shape, fastShape, &sweepA, &continuousContext->sweep, continuousContext->fraction );
+	b3TOIOutput output = b3ShapeTimeOfImpact( shape, fastShape, &sweepA, &continuousContext->sweep, 1.0f );
```

`maxFraction` also bounds the swept box used by the compound and mesh paths of
`b3ShapeTimeOfImpact`; `post_field` has only hull posts and sphere bullets, so the convex path is
the one measured. The patch is `data/.../toi-full-interval.patch`.

## 3. Method

- **Machine:** Intel Core i5-10400F (6 cores, 12 threads), WSL2 on Linux 6.18, GCC 14.2, CMake
  3.31. The same machine as 20261014-003. t7 and t8 run on hyperthread siblings.
- **Builds:** two detached worktrees of `main` (`8128d74`). Both have `d1b8e3b` (the
  `post_field` scene) cherry-picked without commit, because the scene is not registered on
  `main`; the scene uses the public API only and applies cleanly. The `full` worktree has the one
  line above on top. Release, `-DBOX3D_BENCHMARKS=ON`, samples and unit tests off, other options
  default (AVX2, SIMD width 8, `BOX3D_VALIDATE` on). Only the `benchmark` target was built.
- **Command:** `benchmark -b=post_field -t=8 -r=1`, one timing per thread count 1..8 per
  invocation.
- **Interleaving:** 10 rounds; in each round both builds run back to back and the side that goes
  first alternates by round. Samples are paired by (round, thread count): 160 samples, 80 pairs.
- **Quiet machine:** nothing else ran. Before each round the script waits for the 1-minute load
  to drop to 1.5 or below; round-start loads were 0.93 to 1.47. Run 08:27 to 08:41.
- **Order effect:** the side that runs second in a round is slower on both builds. The rounds
  where the [0, 1] build ran first give +53 to +59 percent, those where it ran second +67 to +78.
  The alternation puts five rounds on each side, so the per-thread change above is balanced; the
  per-round range column shows the spread this causes.
- **Baseline sanity:** `main` at one thread is 5238 ms here, against 5207 ms for `master` in
  20261014-003 §5 on the same machine, so the `main` baseline sits where the earlier record puts
  this scene.

## 4. Data and reproduction

All files are in `docs/designs/data/20261015-002-benchmark-post-field-toi-full-interval/`:

| File | Contents |
|---|---|
| `results.csv` | every sample: `round,scene,side,order,threads,run_ms,load1_before,memavail_mb_before` (160 rows; `side` is `base` for `main`, `full` for [0, 1]; `order` is 1 if that side ran first) |
| `summary.csv` | per thread count: medians, minima, CVs, paired change, 95 percent interval, per-round ratio range |
| `raw/` | the raw stdout of all 20 benchmark invocations |
| `progress.log` | round start and end times and the load at each round start |
| `toi-full-interval.patch` | the one-line change |
| `run_ab.sh`, `analyze.py` | the driver and the analysis, unchanged from the run |

`run_ab.sh` expects detached worktrees at `wt-base/` and `wt-full/` with a Release build in
`build/`; the scratchpad path at its top is the only thing to edit. Written from branch
`broad-phase-migration-inserts` at `55a40d1`; the branch's own code was not measured.
