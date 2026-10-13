---
title: Benchmark - distance-keyed continuous candidates, prototype measurements on the project scenarios
status: Draft
date: 2026-10-13
supports: 20261013-002 (preflight)
---

# Benchmark: distance-keyed continuous candidates, prototype measurements

A measurement of the preflight 20261013-002 on the project benchmark scenarios rather than on
`ccdbench.c`. Three questions: what regime the continuous stage runs in on real scenes (candidates
and hits per query), whether handing the GJK simplex into `b3TimeOfImpact` changes its result, and
what the prototype costs against the sweep-ordered PR head it is built on.

## 1. Result

**There is nothing to order.** On every project scenario each continuous query collects one
candidate (junkyard: 1.09), and the fast shape hits it in 0.2 to 25 percent of queries. An order of
candidates within a query can only save work when a query holds two or more candidates the sweep
would hit; no project scenario produces that. The preflight's §1 bound, hit candidates per query
less one, is zero here. The distance key, and every other key, is pure cost on these scenes. The
preflight's landing rule fails on its own terms before the timing is read.

**Which path runs.** Only three scenarios present convex candidates to the stage and so exercise the
distance key at all: convex_pile, junkyard and washer. The other six scenarios with continuous work
(mesh_drop, rain, spinner, trees25/50/100, village) present mesh, height-field or compound
candidates, which keep the sweep key; the prototype changes nothing on their path but the element
size.

**The handed-in simplex preserves everything the solver reads.** Over 20,000 random sweeps of
sphere, capsule and box proxies, the time-of-impact state, fraction, point, normal, iteration counts
and fallback flag are identical bit for bit between a cold start and a start from the pre-pass
simplex. The reported `distance` differs in its last bit in 75 of the 20,000, all separated
results: the cached simplex is re-solved and the barycentric arithmetic differs from the solve that
reduced it. The continuous stage does not read that field. The property the preflight states
("bit for bit") is therefore too strong as written and is restated in §5.

**Cost.** Within the run's spread everywhere (§4). Total step time: scenario medians from -0.5 to
+2.7 percent, eight of ten inside ±2 percent, with both signs. Continuous-stage time from the
counters: medians from -2.3 to +2.2 percent, both signs, including the three scenarios that take the
distance-key path (convex_pile -2.3, junkyard +1.5, washer 0.0). The warm-start hand-off keeps the
pre-pass from costing a second GJK query, as designed; what remains is inside the noise of a stage
that is 0.4 to 19 percent of the step. The prototype neither pays back nor measurably costs on
these scenes; it has no work to do on them.

**Unit tests.** All 36 tests of the prototype's Debug build pass when run one at a time, except
`ContinuousCallbackOrderTest`, which asserts the sweep-key order and fails on the new order as the
preflight says it must (the runner stops at the first failure, so the suite was run test by test).

## 2. Setup

- Builds: Release, AVX2 kernels, `BOX3D_VALIDATE` off, static library, GCC 14.2 at the project's
  Release flags. Baseline: the sweep-ordered PR head `854bd28` (`solver-sweep-order-candidates`).
  Prototype: `0689a5a` on `solver-distance-key-prototype`, forked from the same head. The
  prototype's diff is the preflight's §2 change as written: `b3SimplexCache cache` in
  `b3TOIInput`, read by `b3TimeOfImpact`; a cache parameter on `b3ShapeTimeOfImpact`'s convex path;
  the candidate element `{ key, shapeId, cache }`; the GJK pre-pass and
  `(d - target) / closing` key in the collection callback for sphere, capsule and hull candidates,
  the sweep key for the rest.
- Counting builds: both trees also built with `-DB3_CCD_COUNT=1`, which adds counters of queries
  with at least one candidate, candidates collected, time-of-impact calls, accepted hits, convex
  candidates, and a monotonic-clock sum around `b3SolveContinuous`. The `bullets` profile field
  times bullet bodies only and is zero on every project scenario; the counters are what measure the
  stage for ordinary fast bodies.
- Machine: Intel i5-10400F under WSL2, otherwise idle, turbo and thermal state uncontrolled.
- Scenarios: every scenario in `benchmark/main.c` at its project step count, single worker, run
  through a small harness that creates, steps and reads the counters; the project benchmark binary
  (`-w=1 -r=1`) for total step time.

## 3. Regime of the continuous stage, baseline counting build, one run

| Scenario | Stage ms | Share of total | Queries | Candidates per query | Hits per query | Convex candidates |
|---|---|---|---|---|---|---|
| convex_pile | 72 | 1.0% | 520 | 1.00 | 0.042 | all |
| junkyard | 71 | 0.4% | 463 | 1.09 | 0.171 | all |
| mesh_drop | 426 | 19.1% | 283,785 | 1.00 | 0.057 | none |
| rain | 186 | 6.2% | 123,864 | 1.00 | 0.015 | none |
| spinner | 666 | 17.0% | 1,170,613 | 1.00 | 0.002 | none |
| trees100 | 26 | 15.0% | 1,342 | 1.00 | 0.080 | none |
| trees50 | 30 | 11.8% | 1,537 | 1.00 | 0.073 | none |
| trees25 | 50 | 8.9% | 1,757 | 1.00 | 0.062 | none |
| village | 35 | 1.0% | 32,296 | 1.00 | 0.112 | none |
| washer | 905 | 3.3% | 7,734 | 1.00 | 0.246 | all |
| joint_grid, large_pyramid, large_world, many_pyramids, sleep | 0 | 0 | 0 | | | |

Time-of-impact calls equal candidates collected in every row: no candidate is filtered after
collection. The prototype's counting build gives the same queries, candidates and hits on every
scenario except junkyard (496 queries, 540 candidates, 87 hits against 463, 503, 79): its
multi-candidate queries are ordered differently by the two keys, the accepted fraction differs, and
the trajectories diverge, which is the behaviour change both preflights declare. The convex column
is from the prototype's counting build, which counts candidates that take the distance-key path.

## 4. Interleaved A/B, baseline against prototype

Single worker, machine idle. Each pair is one run of each build back to back, the order alternating
pair by pair. The delta of a pair is the prototype's time relative to the baseline's in that pair.

Total step time, project benchmark binary, `-r=1`.

| Scenario | Pairs | PR median ms | Prototype median ms | Delta median | Delta range |
|---|---|---|---|---|---|
| mesh_drop | 6 | 2252 | 2256 | -0.2% | -1.0 to +0.8 |
| spinner | 6 | 3989 | 4078 | +2.7% | +1.3 to +4.2 |
| rain | 6 | 3310 | 3389 | +1.8% | -3.6 to +5.4 |
| trees100 | 6 | 178.5 | 178.6 | +1.4% | -0.7 to +2.2 |
| trees50 | 6 | 277.4 | 277.5 | -0.1% | -3.4 to +1.2 |
| trees25 | 6 | 634.5 | 625.3 | -0.5% | -6.7 to +2.7 |
| village | 6 | 3813 | 3839 | +0.6% | -1.0 to +2.2 |
| convex_pile | 6 | 8435 | 8567 | +1.6% | -0.7 to +4.6 |
| junkyard | 6 | 19871 | 19980 | +0.1% | -0.7 to +1.8 |
| washer | 4 | 29490 | 29586 | +0.7% | -1.3 to +2.2 |

Continuous-stage time, counting builds, sum of the clock around `b3SolveContinuous` over the run.

| Scenario | Pairs | PR median ms | Prototype median ms | Delta median | Delta range |
|---|---|---|---|---|---|
| mesh_drop | 4 | 485.8 | 480.1 | -0.6% | -2.4 to -0.2 |
| spinner | 4 | 713.3 | 719.5 | +0.9% | +0.6 to +3.8 |
| rain | 4 | 200.0 | 200.0 | +0.1% | -0.9 to +1.6 |
| trees100 | 4 | 27.1 | 26.9 | -0.7% | -1.8 to +0.4 |
| trees50 | 4 | 32.4 | 33.1 | +2.2% | +0.8 to +5.1 |
| trees25 | 4 | 55.1 | 56.2 | +1.7% | +0.1 to +4.2 |
| village | 4 | 39.0 | 39.5 | +1.1% | -1.6 to +7.9 |
| convex_pile | 4 | 82.1 | 80.1 | -2.3% | -6.2 to -0.4 |
| junkyard | 4 | 81.0 | 82.5 | +1.5% | -1.5 to +6.6 |
| washer | 2 | 985.4 | 985.1 | -0.0% | -1.2 to +1.2 |

The spinner total-time delta (+2.7 percent, every pair positive) is not matched by its stage delta
(+0.9 percent on a stage that is 17 percent of the step), so at most a sixth of it can be the
continuous stage; the rest is drift across a 40-second burst. Spinner's candidates are all
non-convex, so the prototype's only change on its path is the 24-byte element.

## 5. Consequences for the preflight

- The regime measurement answers the preflight's landing rule: no project scenario holds a query
  with more than one candidate the sweep would hit, so no order can repay any key on them. The
  rule's first clause (at or within spread on the axis-aligned scene) is the only one the prototype
  can meet, and meeting it lands nothing.
- The required property in the preflight's §2 is to be restated: identical for every field the
  continuous stage reads (state, fraction, point, normal, iteration counts); the reported distance
  can differ in its last bit for a separated result.
- The sweep-ordered PR's own gain has the same bound and the same regime; this measurement is also
  evidence about it.

## 6. Caveats

- One machine, WSL2, one run per scenario for the regime table. The counts are deterministic and
  do not depend on the run; the stage times in §3 are from a single burst and are not an A/B.
- The counters attribute time to `b3SolveContinuous` as a whole, including the tree queries.
- Worker counts above one were not run; the stage is per fast body and the counters are not
  thread-safe, so the counting builds are single-worker only.
- The project scenarios have no bullets, so the kinematic and dynamic queries never run.
