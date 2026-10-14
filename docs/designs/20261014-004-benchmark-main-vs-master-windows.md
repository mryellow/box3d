# Benchmark: `master` against `main`, Windows

Date: 2026-10-14. Same comparison as [the Linux document](20261014-003-benchmark-main-vs-master-linux.md):
`main` is `8128d74`; `master` is `1792038`, 109 commits ahead and 0 behind. This run is native
Windows on the same physical machine as the Linux (WSL2) run.

## 1. Summary

- **`sleep` is slower on master here too: +4% to +11% at 2–8 threads** (95% interval excludes
  zero at all seven; at one thread it is +2.1% and not significant). This agrees with the Linux
  result (+4% to +13% at 1–8).
- **`mesh_drop` is only +1.6% to +2.5% at 2–5 threads** and shows nothing at 6–8, against
  +4% to +7% at 4–8 on Linux. The Linux finding did not reproduce at the same size.
- **No broad shift.** Geometric mean over the 15 shared scenes: −0.6% at one thread, +0.4% to
  +1.3% at 2–8. Run-to-run spread is lower than on Linux (CV mostly 1–4%).
- The other flagged cells (`convex_pile`, `large_world`, `rain`, `spinner`, `trees100`,
  `village`, `washer`, `joint_grid`) are scattered single or paired cells within ±4%, except
  `large_world` (−11% at t1, −8% at t4; the scene takes 8–25 ms and the spread on `main` is
  11–17%). With 120 paired cells, about 6 false positives are expected at 95%.
- `post_field` and `sleeping_grid` exist only on `master`; see §5.

## 2. Method

- **Machine:** the same Intel Core i5-10400F as the Linux run (6 cores, 12 threads), Windows 10
  (build 19045). t7 and t8 run on hyperthread siblings.
- **Build:** Visual Studio 18 CMake, MSVC 19.51 (`cl`, toolset 14.51), Windows SDK 10.0.26100.0,
  Release (`/O2 /Ob2 /MT`), `-DBOX3D_BENCHMARKS=ON`, other options default (AVX2 on,
  `BOX3D_VALIDATE` on). SIMD width 8. Only the `benchmark` target was built, in separate trees
  `E:\src\box3d-bench\main` and `E:\src\box3d-bench\master`, rsynced from the Linux worktrees
  without `.git` or build output. `E:\src\box3d` was not touched.
- **Command:** `benchmark.exe -b=<scene> -t=8 -r=1` launched through `cmd.exe /c` from WSL, one
  timing for each of 1..8 threads per invocation.
- **Interleaving, statistics:** identical to the Linux document (10 rounds, alternating first
  side per (round + scene index), paired by round, median and geometric-mean paired delta with a
  95% t-interval).
- **Quiet machine:** nothing else was running. Before each round the script waits for Windows
  CPU load of 15% or less; round-start loads were 0–8% (2, 2, 3, 8, 4, 3, 4, 8, 1, 0) and no
  round had to wait. There is no per-sample load column worth using, so the CSV carries the
  round-start value in `win_cpu_pct_round_start`.
- **Drift check:** geometric-mean ratio per round, all cells: +0.6, +0.4, +0.1, +0.9, +0.7,
  +1.4, +0.7, +0.7, +0.4, +0.3%. No round reversed the sign and there is no trend.

## 3. Percentage change in total time, master over main

Positive means master is slower. `*` marks a 95% interval that excludes zero. Columns are
thread counts.

| Scene | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|
| convex_pile | +0.4 | -0.9 | +2.8 | +3.3 | +2.8* | +4.2 | +3.4* | +3.7 |
| joint_grid | -0.5 | +1.0 | +1.8 | -0.7 | -0.7 | -3.7* | +0.4 | -1.3 |
| junkyard | -0.5 | -0.7 | -1.8 | -1.7 | -0.1 | -3.5 | -1.6 | +1.0 |
| large_pyramid | +0.0 | +0.1 | +0.0 | -0.5 | -1.1 | +1.5 | -1.5 | -0.7 |
| large_world | -10.8* | -3.2 | +0.3 | -8.4* | -1.5 | -2.4 | -1.9 | +0.1 |
| many_pyramids | -0.7 | -0.0 | +1.5 | +0.7 | +2.7 | +0.6 | -0.9 | -2.1 |
| mesh_drop | +1.0 | +1.6* | +1.9* | +1.9* | +2.5* | +0.3 | -0.8 | +0.5 |
| rain | +0.9 | +0.8* | +1.2* | +0.8 | -0.6 | -0.8 | -0.1 | -1.4 |
| sleep | +2.1 | +4.2* | +7.1* | +8.4* | +8.0* | +11.0* | +10.3* | +10.8* |
| spinner | +0.0 | -2.5* | -2.2 | -0.2 | -0.4 | +2.6 | +0.1 | +1.5 |
| trees100 | +1.1 | +1.7 | +1.4 | +1.4 | +1.4 | +1.5 | +3.1* | +0.4 |
| trees25 | +0.3 | -0.7 | +1.4 | -0.7 | +0.1 | -0.2 | +0.6 | +1.7 |
| trees50 | -1.5 | -1.1 | -1.6 | +2.4 | -0.6 | +0.8 | +1.2 | +3.4 |
| village | +0.2 | +0.2 | +1.3* | +2.2 | +2.6* | +0.5 | +1.2 | +0.4 |
| washer | +0.3 | +1.6* | +0.4 | -0.2 | +0.4 | +0.8 | -0.6 | +0.5 |

Geometric mean over the 15 scenes, by thread count:

| t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|
| −0.6 | +0.4 | +0.9 | +0.7 | +1.1 | +0.9 | +1.0 | +1.3 |

## 4. Median total milliseconds per scene

Median of 10 runs per cell.

| Scene | Side | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|---|
| convex_pile | main | 9881.1 | 5479.2 | 3948.0 | 3225.3 | 2817.3 | 2595.7 | 2602.0 | 2533.0 |
| convex_pile | master | 9881.0 | 5382.7 | 3956.5 | 3264.5 | 2859.4 | 2636.4 | 2624.2 | 2544.5 |
| joint_grid | main | 2957.7 | 1533.2 | 1085.8 | 869.2 | 755.4 | 697.0 | 746.4 | 712.2 |
| joint_grid | master | 2911.3 | 1524.7 | 1089.3 | 861.0 | 762.4 | 692.7 | 753.7 | 703.4 |
| junkyard | main | 21120.0 | 11489.9 | 8734.7 | 7190.0 | 6376.7 | 6105.7 | 5871.9 | 5728.1 |
| junkyard | master | 20545.8 | 11456.8 | 8571.9 | 7129.3 | 6355.4 | 5943.3 | 5849.9 | 5837.8 |
| large_pyramid | main | 2702.4 | 1431.4 | 1066.9 | 885.5 | 794.2 | 745.6 | 771.6 | 756.4 |
| large_pyramid | master | 2712.1 | 1438.7 | 1068.8 | 885.6 | 794.9 | 745.1 | 768.2 | 753.1 |
| large_world | main | 8.9 | 11.9 | 14.2 | 15.6 | 14.9 | 15.7 | 16.7 | 17.9 |
| large_world | master | 8.1 | 11.8 | 14.4 | 14.5 | 14.9 | 15.2 | 16.3 | 18.0 |
| many_pyramids | main | 2882.4 | 1580.4 | 1201.1 | 1027.3 | 945.3 | 890.7 | 912.0 | 882.9 |
| many_pyramids | master | 2850.4 | 1587.1 | 1207.1 | 1033.1 | 955.4 | 896.0 | 903.8 | 875.7 |
| mesh_drop | main | 2638.7 | 1411.8 | 1039.2 | 853.1 | 745.8 | 687.1 | 691.2 | 668.5 |
| mesh_drop | master | 2667.2 | 1441.2 | 1059.2 | 871.3 | 768.9 | 701.3 | 705.5 | 686.8 |
| post_field | master | 5700.1 | 2959.0 | 2094.3 | 1664.3 | 1427.1 | 1280.8 | 1267.0 | 1251.3 |
| rain | main | 3512.9 | 1902.4 | 1395.6 | 1153.1 | 1014.4 | 932.2 | 965.8 | 936.4 |
| rain | master | 3520.2 | 1911.4 | 1411.6 | 1154.9 | 1015.0 | 928.1 | 969.8 | 940.9 |
| sleep | main | 1239.3 | 673.7 | 504.8 | 428.4 | 382.6 | 363.1 | 375.0 | 376.5 |
| sleep | master | 1272.1 | 707.5 | 537.7 | 466.4 | 420.2 | 403.3 | 416.1 | 414.4 |
| sleeping_grid | master | 25.6 | 25.5 | 29.5 | 33.3 | 36.4 | 37.2 | 37.7 | 37.8 |
| spinner | main | 4669.3 | 2816.9 | 2197.5 | 1888.6 | 1773.0 | 1673.3 | 1665.7 | 1677.6 |
| spinner | master | 4646.5 | 2788.2 | 2161.5 | 1882.9 | 1726.5 | 1749.5 | 1679.7 | 1690.5 |
| trees100 | main | 216.3 | 138.2 | 116.2 | 106.8 | 102.0 | 99.9 | 104.0 | 106.8 |
| trees100 | master | 221.2 | 140.7 | 119.7 | 108.7 | 103.8 | 100.2 | 107.8 | 107.3 |
| trees25 | main | 692.8 | 407.3 | 319.8 | 283.2 | 264.0 | 251.5 | 258.8 | 257.9 |
| trees25 | master | 690.7 | 409.5 | 323.7 | 284.1 | 264.4 | 249.9 | 257.7 | 261.9 |
| trees50 | main | 325.4 | 202.1 | 163.2 | 142.4 | 137.3 | 134.7 | 139.6 | 138.1 |
| trees50 | master | 326.0 | 200.7 | 161.5 | 147.8 | 135.8 | 134.0 | 138.9 | 141.7 |
| village | main | 4268.7 | 2523.5 | 2006.8 | 1732.3 | 1599.3 | 1533.8 | 1582.7 | 1549.5 |
| village | master | 4248.0 | 2519.0 | 2018.9 | 1766.8 | 1634.9 | 1557.1 | 1590.6 | 1551.1 |
| washer | main | 31060.5 | 17176.1 | 13024.0 | 11077.0 | 9859.1 | 9285.1 | 9221.0 | 8953.9 |
| washer | master | 31036.3 | 17399.0 | 13101.1 | 11003.2 | 9872.9 | 9377.6 | 9219.4 | 9033.2 |

## 5. Scenes only on `master`

Median of 10 runs; CV is the run-to-run spread.

| Scene | Stat | t1 | t2 | t3 | t4 | t5 | t6 | t7 | t8 |
|---|---|---|---|---|---|---|---|---|---|
| post_field | median ms | 5700.1 | 2959.0 | 2094.3 | 1664.3 | 1427.1 | 1280.8 | 1267.0 | 1251.3 |
| post_field | CV % | 1.1 | 2.2 | 1.7 | 1.8 | 2.2 | 3.5 | 3.0 | 2.5 |
| sleeping_grid | median ms | 25.6 | 25.5 | 29.5 | 33.3 | 36.4 | 37.2 | 37.7 | 37.8 |
| sleeping_grid | CV % | 3.6 | 2.4 | 4.1 | 3.3 | 2.0 | 2.2 | 3.9 | 2.7 |

## 6. Windows against Linux

Same machine, same sources, different OS and compiler (MSVC against GCC 14.2; native Windows
threads against WSL2).

- **Branch comparison agrees on `sleep`:** master slower by 2–11% on Windows, 4–13% on Linux.
  The two platforms differ on `mesh_drop` (Windows −0.8% to +2.5%, Linux up to +6.5%) and on the
  scattered single-cell hits, which do not repeat across platforms.
- **Single-thread speed is close.** Ratio of Windows to Linux `main` median at one thread:
  0.92–1.01 for 14 of 15 scenes and 1.22 for `convex_pile` (Windows
  slower).
- **Eight threads favour Windows.** Windows is 10–82% faster for 14 scenes at t8 (ratio
  0.18–0.90) and 3% slower for `convex_pile` (1.03). The extremes: `large_world` 0.18, `trees100` 0.55,
  `trees50` 0.59, `trees25` 0.69. `sleeping_grid` on master is 37.8 ms at t8 on Windows
  against 158.1 ms on Linux, and is flat from t2 to t8 on Windows, where it rises steadily on
  Linux. I did not investigate the cause; thread wake-up cost under WSL2 is one possibility but
  it is a guess, not a finding.

## 7. Data and reproduction

All files are in `docs/designs/data/20261014-004-benchmark-main-vs-master-windows/`:

| File | Contents |
|---|---|
| `results.csv` | every sample: `round,scene,side,order,threads,run_ms,win_cpu_pct_round_start` (2,560 rows) |
| `summary.csv` | per scene and thread count: medians, minima, CVs, paired delta and 95% interval (same columns as the Linux file) |
| `raw/` | raw stdout of all 320 invocations |
| `progress.log` | round start and end times and the Windows CPU load at each round start |
| `run_ab_win.sh` | the driver, run from WSL |
| `build-bench.bat` | configures and builds both trees with VS 18 CMake |
| analysis | `analyze2.py` in the Linux data folder, unchanged |
