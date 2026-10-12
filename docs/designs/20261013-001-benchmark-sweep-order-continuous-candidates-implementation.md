---
title: Benchmark - sweep-ordered continuous candidates, implementation measurements
status: Draft
date: 2026-10-13
supports: 20261012-005 (preflight), branch solver-sweep-order-candidates
---

# Benchmark: sweep-ordered continuous candidates, implementation measurements

The A/B measurement of the implementation of 20261012-005, commit `854bd28` on
`solver-sweep-order-candidates`, forked from the sorted-candidate PR head `8b18782`. The prototype
measured in 20261012-006 differed from the preflight by a two-slop and rounding growth of the key box
and an unused element flag; this is the change as it will land. The questions are the preflight's
review checklist: the continuous-collision microbenchmark in both travel directions, and the project
benchmarks.

## 1. Result

The implementation takes the nearest-first time in both travel directions. In the dense scene of
`ccdbench.c` it is 2.6 to 3.8 percent under `main` with the spheres fired in +x, where ascending id
was already nearest-first and the sorted-candidate PR is fastest, and 4.2 to 5.6 percent under `main`
in -x, where ascending id is farthest-first and the PR is 11 to 12 percent over `main`. Against the
PR head it lands on, the branch is 2.6 to 2.9 percent slower in +x and 13.6 to 13.9 percent faster in
-x. Its own time is the same in both directions to within 1 percent, which is the property the change
is for: the cost no longer depends on the correlation between shape id and distance. The bullet rows,
which time the continuous stage alone, show the same picture with a 3 percent cost in +x and a 15
percent gain in -x against the PR.

The project benchmarks are within run-to-run noise of the PR head. Single worker: -2.3 to +3.1
percent with both signs. Four workers: the sign of every delta follows which build ran first in the
burst, as 20261012-004 §4 found, so the four-worker rows measure the machine and not the code.

## 2. Setup

- Builds: Release, `BOX3D_VALIDATE` off, static library, GCC 14.2 at the project's Release flags.
  Three libraries: `main` at `5d83df8`, the sorted-candidate PR head at `8b18782`, and the branch
  at `854bd28`. `ccdbench.c` (20261012-004 appendix) compiled once against each.
- Machine: Intel i5-10400F under WSL2, otherwise idle for the whole run, turbo and thermal state
  uncontrolled. Rows compare within themselves. A first attempt with a browser active on the machine
  was discarded and the whole run repeated idle.
- Microbenchmark scene: 4000 spheres at 300 m/s through 14,641 static boxes, 100 steps, four
  sub-steps, one worker, minimum of three worlds. Non-bullet runs report the total; bullet runs report
  the continuous stage (`bullets` profile field). Three rotations of the run order per direction for
  non-bullet runs, two for bullet runs.
- Project benchmarks: every scenario in `benchmark/main.c`, `benchmark -b=<name> -w=<workers> -r=2`,
  the reported time is the benchmark's own minimum over its repeats. Each scenario and worker count
  ran as a burst of four back to back: PR, branch, branch, PR. The four-worker scenarios ran a second
  burst in the reversed order: branch, PR, PR, branch.

## 3. Microbenchmark times

Non-bullet, total ms, spheres fired in +x (ascending id is nearest-first).

| Rotation | `main` | PR | branch | branch vs `main` | branch vs PR |
|---|---|---|---|---|---|
| main, PR, branch | 4830 | 4583 | 4706 | -2.6% | +2.7% |
| branch, PR, main | 4868 | 4551 | 4681 | -3.8% | +2.9% |
| PR, branch, main | 4797 | 4553 | 4674 | -2.6% | +2.6% |

Non-bullet, total ms, spheres fired in -x (ascending id is farthest-first).

| Rotation | `main` | PR | branch | branch vs `main` | branch vs PR |
|---|---|---|---|---|---|
| main, PR, branch | 4882 | 5437 | 4679 | -4.2% | -13.9% |
| branch, PR, main | 4985 | 5455 | 4705 | -5.6% | -13.7% |
| PR, branch, main | 4913 | 5448 | 4706 | -4.2% | -13.6% |

Bullets, continuous stage ms.

| Direction | Rotation | `main` | PR | branch | branch vs `main` | branch vs PR |
|---|---|---|---|---|---|---|
| +x | main, PR, branch | 4427 | 4111 | 4243 | -4.1% | +3.2% |
| +x | branch, PR, main | 4418 | 4140 | 4267 | -3.4% | +3.1% |
| -x | main, PR, branch | 4446 | 5027 | 4258 | -4.2% | -15.3% |
| -x | branch, PR, main | 4468 | 4983 | 4255 | -4.8% | -14.6% |

The machine was steadier than in the earlier documents: the spread of one binary across its three
rotations is under 1 percent in every column except `main` in -x (2 percent). The branch's six
non-bullet times span 4674 to 4706 ms across both directions.

These figures match 20261012-006 §3 for its B1 column within the drift between the two runs: the
prototype was 1.7 percent over to 6 percent under `main` in +x and 0.8 to 7.2 percent under in -x.
The bare `r` growth of the implementation did not move the result measurably against the prototype's
padded growth.

## 4. Project benchmarks

Single worker, minimum of the burst's two runs per build, ms.

| Scenario | PR | branch | branch vs PR |
|---|---|---|---|
| convex_pile | 8277 | 8340 | +0.8% |
| joint_grid | 2745 | 2722 | -0.9% |
| junkyard | 19901 | 19876 | -0.1% |
| large_pyramid | 2628 | 2618 | -0.4% |
| large_world | 9 | 8 | -6.2% |
| many_pyramids | 2898 | 2904 | +0.2% |
| mesh_drop | 2501 | 2514 | +0.5% |
| rain | 3328 | 3430 | +3.1% |
| sleep | 1114 | 1106 | -0.7% |
| spinner | 4033 | 4106 | +1.8% |
| trees100 | 196 | 191 | -2.3% |
| trees50 | 295 | 290 | -1.6% |
| trees25 | 628 | 647 | +3.0% |
| village | 3750 | 3797 | +1.3% |
| washer | 29310 | 29601 | +1.0% |

`large_world` runs for under 10 ms and its percentage is noise on a millisecond. The others lie in
-2.3 to +3.1 percent with both signs; `rain` and `trees25`, the two scenarios past +3 percent, each
have one branch run at or under the slower PR run of the same burst (rain 3430 against 3511, trees25
647 against 641).

Four workers, both bursts, minimum of each build's two runs within the burst, ms.

| Scenario | First burst, PR | branch | delta | Reversed burst, PR | branch | delta |
|---|---|---|---|---|---|---|
| convex_pile | 2925 | 3045 | +4.1% | 2837 | 2595 | -8.5% |
| joint_grid | 877 | 908 | +3.6% | 870 | 891 | +2.4% |
| junkyard | 7268 | 7464 | +2.7% | 7275 | 6932 | -4.7% |
| large_pyramid | 931 | 960 | +3.1% | 980 | 979 | -0.1% |
| large_world | 65 | 72 | +10.7% | 72 | 66 | -7.7% |
| many_pyramids | 1117 | 1104 | -1.2% | 1112 | 1115 | +0.2% |
| mesh_drop | 876 | 891 | +1.7% | 921 | 925 | +0.4% |
| rain | 1195 | 1248 | +4.5% | 1333 | 1323 | -0.8% |
| sleep | 434 | 460 | +5.9% | 489 | 487 | -0.5% |
| spinner | 1816 | 1939 | +6.8% | 2010 | 2051 | +2.0% |
| trees100 | 127 | 134 | +5.4% | 142 | 143 | +0.6% |
| trees50 | 166 | 174 | +4.7% | 182 | 184 | +1.2% |
| trees25 | 312 | 331 | +6.0% | 340 | 337 | -0.8% |
| village | 1686 | 1743 | +3.4% | 1863 | 1873 | +0.5% |
| washer | 12425 | 11627 | -6.4% | 11797 | 11814 | +0.1% |

In the first burst the PR held the first slot and the branch is 2 to 7 percent behind on twelve of
fifteen scenarios; in the reversed burst the branch held the first slot and the deltas sit in -0.8 to
+2.4 percent on twelve scenarios, with the three largest scenes swinging to the branch's side. Within
every burst the fastest run of either build is the first run, so the four-worker figures are the
first-slot turbo effect of 20261012-004 §4. The reversed burst as a whole ran 5 to 10 percent slower
than the first, which is the same drift across the twenty minutes between them. The project
benchmarks hold few fast bodies, so the continuous stage, the only place the change adds work, is a
small part of their step.

## 5. Caveats

- One machine, WSL2, no control of turbo or thermal state. The microbenchmark margins against the PR
  exceed the run's spread in every row; the margins against `main` in +x (2.6 to 3.8 percent) are
  above this run's spread but below the 5 to 8 percent drift the earlier documents saw.
- Every candidate in the microbenchmark scene is a convex solid box of one size; the key's
  limits (rotation, near-axis directions, coarse float boxes) are not exercised.
- Worker counts above one were not run for the microbenchmark; the key and the sort are per query.
- The microbenchmark's `main` column is the same binary as in 20261012-004 and 20261012-006 and
  landed within 3 percent of both.
