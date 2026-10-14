# Benchmark: `master` against `main`, Linux

Date: 2026-10-14. `main` is `8128d74`; `master` is `1792038`, 109 commits ahead of `main` and
0 behind (`main` is the merge base). The difference is the rollback work, the awake/sleeping
broad-phase tree split, and two extra benchmark scenes (`post_field`, `sleeping_grid`).

## 1. Summary

- **No broad regression or win.** Across the 15 scenes both branches run, the geometric-mean
  change in total time (master over main) is +0.1% at one thread and +1.2% at eight. Most cells
  are inside the noise.
- **`sleep` is slower on master: +4% to +13%** at every thread count, 95% interval excluding
  zero at all eight. This is the one clear regression and the scene the awake-tree split is most
  likely to touch.
- **`mesh_drop` is about 4–7% slower** at four or more threads (significant at t4–t8, not
  below).
- **`large_world` is faster on master at 2–3 threads (−15%, −6%)** but the scene takes 9–100 ms
  and its run-to-run spread is 8–11%, so treat that as weak.
- `spinner`, `trees25`, `trees100`, `village` and `washer` have a few isolated cells past the
  threshold, all within ±5%. With 120 paired cells, about 6 false positives are expected at 95%,
  so scattered single-cell hits are not findings. Consistent runs across thread counts
  (`sleep`, `mesh_drop`) are.
- `post_field` and `sleeping_grid` do not exist on `main`; their master timings are in §5.

Not examined here: why `sleep` and `mesh_drop` are slower. This document only measures it.

## 2. Method

- **Machine:** Intel Core i5-10400F (6 cores, 12 threads, 12 MiB L3), 7.9 GB RAM, WSL2 on Linux
  6.18, GCC 14.2 (`cc`). Because there are only 6 physical cores, **t7 and t8 run on hyperthread
  siblings**; compare those columns across branches, not against t6.
- **Build:** a detached git worktree per branch, Release, `-DBOX3D_BENCHMARKS=ON`, other options
  default (AVX2 on, `BOX3D_VALIDATE` on, which is the default and matches `build-rel`). SIMD
  width 8. Only the `benchmark` target was built.
- **Command:** `benchmark -b=<scene> -t=8 -r=1`, so each invocation yields one timing for each
  of 1..8 threads. The harness is the same `benchmark/main.c` on both sides apart from the two
  extra scene registrations.
- **Interleaving:** 10 rounds. In each round every scene runs on both builds back to back, and
  which build goes first alternates by (round + scene index), so neither side always gets the
  cold or warm slot. Samples are therefore paired by (round, scene, thread count).
- **Quiet machine:** nothing else was running (confirmed with the user before the run). Before
  each round the script waits for the 1-minute load average to drop to 1.5 or below. Round-start
  loads were 0.09–1.50. An earlier run, taken while another session was running tests, was
  discarded and not used.
- **Statistics:** per cell, the **median** of 10 paired samples per side, and the **paired
  delta** = geometric mean of the per-round ratio master/main, minus 1, with a 95% Student-t
  interval (9 degrees of freedom) on the log ratio. A cell is marked `*` when that interval
  excludes zero. Run-to-run coefficient of variation is 1–17%, highest for the shortest scenes.
- **Drift check:** the geometric-mean ratio per round, all cells, was +0.4% to +2.7% (rounds
  1–10: +0.5, +0.4, +2.7, +0.6, +1.0, +1.1, +1.7, +0.7, +1.0, +0.7). Round 3 is the largest; no
  round reversed the sign. There is no trend over the run.
- **Per-sample load is not a noise indicator here.** The CSV records `load1_before` and
  `memavail_mb_before` for every sample, but the 1-minute load includes the benchmark's own
  8-thread runs, so 2,472 of 2,560 samples exceed 1.5 without outside interference. Minimum
  available memory was 5.4 GB.

## 3. Percentage change in total time, master over main

Positive means master is slower. `*` marks a 95% interval that excludes zero. Columns are
thread counts.

| Scene | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|
| convex_pile | -0.7 | -0.2 | +0.2 | +0.2 | -2.5 | -1.9 | -0.1 | -0.5 |
| joint_grid | -0.8 | -0.4 | -0.3 | +0.1 | +0.7 | +2.0 | -0.6 | +1.2 |
| junkyard | +0.5 | -1.6 | -1.6 | -0.6 | +2.3 | +1.7 | +0.3 | +1.3 |
| large_pyramid | +1.1 | -0.0 | +1.8 | +0.5 | -0.6 | -5.0* | -2.1 | -2.2 |
| large_world | -7.9 | -14.7* | -6.1* | +1.4 | +1.7 | +2.1 | +2.1 | +2.3 |
| many_pyramids | -0.7 | -1.2 | -1.9* | -2.0 | -1.5 | -1.7 | -2.4 | -1.0 |
| mesh_drop | +0.6 | +0.4 | +1.5 | +4.3* | +6.5* | +5.7* | +3.9* | +4.6* |
| rain | +1.5 | +1.0 | -0.3 | -1.3 | -0.2 | +1.3 | +1.6 | +2.1 |
| sleep | +5.8* | +4.4* | +10.0* | +11.0* | +12.8* | +11.4* | +11.7* | +10.3* |
| spinner | +0.8 | -1.4 | +2.3* | +3.0* | +4.4* | +4.2 | +3.5 | +2.3 |
| trees100 | +0.7 | +1.3 | +1.2 | +1.5 | +1.3 | +2.2* | +2.1* | +0.8 |
| trees25 | +2.1 | +2.2 | +0.2 | +2.1 | +2.6* | +3.2* | +2.0 | +1.8* |
| trees50 | +0.2 | +0.2 | -0.6 | +2.1 | +0.9 | +0.2 | +1.8 | +1.2 |
| village | +0.7 | +0.4 | +1.6 | +1.3 | +2.1 | +1.2 | +1.1 | +4.1* |
| washer | -1.8 | -2.2* | -1.1 | -0.6 | -1.1 | -0.5 | +0.7 | +0.6 |

Geometric mean over the 15 scenes, by thread count:

| t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|
| +0.1 | −1.2 | +0.3 | +1.3 | +1.3 | +1.2 | +1.6 | +1.2 |

## 4. Median total milliseconds per scene

Median of 10 runs per cell.

| Scene | Side | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|---|
| convex_pile | main | 8125.2 | 4745.8 | 3517.5 | 2964.7 | 2703.5 | 2468.2 | 2469.3 | 2454.4 |
| convex_pile | master | 7866.3 | 4587.7 | 3437.4 | 2937.6 | 2582.0 | 2451.4 | 2416.2 | 2427.1 |
| joint_grid | main | 2922.6 | 1649.5 | 1170.3 | 937.0 | 833.2 | 774.0 | 820.4 | 795.2 |
| joint_grid | master | 2931.6 | 1629.8 | 1168.4 | 951.8 | 834.2 | 764.5 | 820.7 | 785.9 |
| junkyard | main | 22120.3 | 12887.5 | 9626.8 | 7874.5 | 7069.6 | 6703.4 | 6615.0 | 6533.3 |
| junkyard | master | 21713.8 | 12542.2 | 9415.8 | 7853.5 | 7104.2 | 6839.1 | 6599.1 | 6594.7 |
| large_pyramid | main | 2863.7 | 1602.7 | 1206.8 | 1033.1 | 960.9 | 929.8 | 946.4 | 939.2 |
| large_pyramid | master | 2885.2 | 1596.3 | 1234.2 | 1040.6 | 947.9 | 893.4 | 926.9 | 927.7 |
| large_world | main | 9.1 | 22.3 | 37.9 | 69.9 | 78.4 | 84.5 | 91.8 | 99.5 |
| large_world | master | 8.5 | 19.4 | 35.7 | 69.5 | 79.0 | 86.7 | 93.3 | 100.9 |
| many_pyramids | main | 3122.0 | 1799.3 | 1389.9 | 1178.8 | 1070.9 | 1018.6 | 1031.6 | 1017.7 |
| many_pyramids | master | 3098.3 | 1771.2 | 1360.8 | 1164.0 | 1072.2 | 1009.7 | 1016.0 | 999.7 |
| mesh_drop | main | 2660.6 | 1543.2 | 1128.7 | 935.2 | 824.2 | 763.0 | 771.1 | 764.5 |
| mesh_drop | master | 2727.4 | 1544.8 | 1131.9 | 958.3 | 854.1 | 798.7 | 797.9 | 785.9 |
| post_field | master | 5207.4 | 2926.8 | 2059.2 | 1644.3 | 1402.4 | 1264.1 | 1240.6 | 1214.2 |
| rain | main | 3641.2 | 2086.4 | 1558.4 | 1313.1 | 1178.1 | 1146.8 | 1186.4 | 1197.7 |
| rain | master | 3633.7 | 2049.4 | 1555.6 | 1327.8 | 1184.2 | 1151.7 | 1201.7 | 1213.0 |
| sleep | main | 1233.3 | 728.9 | 566.0 | 488.8 | 450.2 | 446.3 | 464.4 | 482.7 |
| sleep | master | 1290.8 | 770.1 | 609.2 | 530.2 | 504.6 | 485.1 | 513.9 | 528.1 |
| sleeping_grid | master | 25.8 | 47.4 | 75.1 | 101.4 | 139.0 | 145.9 | 154.7 | 158.1 |
| spinner | main | 4707.8 | 3046.2 | 2372.5 | 2171.7 | 2048.0 | 2105.6 | 2132.1 | 2177.2 |
| spinner | master | 4752.0 | 3006.2 | 2437.5 | 2175.4 | 2121.4 | 2145.6 | 2192.3 | 2188.7 |
| trees100 | main | 222.2 | 160.6 | 145.5 | 144.0 | 153.6 | 168.4 | 185.7 | 195.7 |
| trees100 | master | 223.6 | 160.1 | 147.7 | 148.2 | 156.0 | 171.9 | 190.0 | 197.9 |
| trees25 | main | 701.6 | 453.8 | 371.5 | 343.7 | 332.3 | 344.3 | 354.1 | 371.2 |
| trees25 | master | 712.8 | 463.7 | 371.7 | 350.8 | 337.9 | 345.8 | 363.5 | 374.8 |
| trees50 | main | 329.6 | 226.0 | 195.9 | 185.3 | 195.9 | 207.0 | 221.4 | 235.5 |
| trees50 | master | 337.1 | 227.1 | 195.9 | 191.1 | 197.1 | 210.9 | 228.2 | 236.9 |
| village | main | 4271.3 | 2692.7 | 2141.8 | 1906.4 | 1767.1 | 1725.3 | 1779.2 | 1778.0 |
| village | master | 4353.6 | 2702.7 | 2190.4 | 1934.7 | 1793.6 | 1771.7 | 1820.7 | 1869.0 |
| washer | main | 32513.2 | 18563.3 | 14266.9 | 12074.5 | 11036.1 | 10567.5 | 10413.0 | 10495.3 |
| washer | master | 32257.5 | 18490.8 | 14312.0 | 11988.0 | 10911.4 | 10349.8 | 10396.3 | 10379.5 |

## 5. Scenes only on `master`

`post_field` and `sleeping_grid` are registered only on `master`, so there is nothing to pair
against. Median of 10 runs; CV is the run-to-run spread. These numbers are not comparable with
those in the awake-tree-split closure doc, which came from a different machine.

| Scene | Stat | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|---|
| post_field | median ms | 5207.4 | 2926.8 | 2059.2 | 1644.3 | 1402.4 | 1264.1 | 1240.6 | 1214.3 |
| post_field | CV % | 6.3 | 5.0 | 4.6 | 4.3 | 5.2 | 8.8 | 5.9 | 7.1 |
| sleeping_grid | median ms | 25.8 | 47.4 | 75.1 | 101.4 | 139.0 | 145.9 | 154.7 | 158.1 |
| sleeping_grid | CV % | 7.1 | 13.5 | 9.5 | 12.1 | 7.7 | 9.4 | 6.8 | 7.4 |

To compare `sleeping_grid` against `main`, the scene would have to be cherry-picked onto
`main` (the closure doc notes it builds unchanged on the base, since it uses the public API only).
That was not done here.

## 6. Data and reproduction

All files are in `docs/designs/data/20261014-003-benchmark-main-vs-master-linux/`:

| File | Contents |
|---|---|
| `results.csv` | every sample: `round,scene,side,order,threads,run_ms,load1_before,memavail_mb_before` (2,560 rows; `order` is 1 if that side ran first in its pair) |
| `summary.csv` | per scene and thread count: medians, minima, CVs, paired delta and 95% interval |
| `raw/` | the raw stdout of all 320 benchmark invocations |
| `progress.log` | round start and end times and the load at each round start |
| `run_ab.sh`, `analyze2.py` | the driver and the analysis, unchanged from the run |

`run_ab.sh` expects detached worktrees at `wt-main/` and `wt-master/` with a Release build in
`build/`. Chart input: `results.csv` has one row per sample, and `summary.csv` has one row per
cell.
