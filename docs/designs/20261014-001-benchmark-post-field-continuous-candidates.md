---
title: Benchmark - post field, the sweep key with many continuous candidates per query
status: Draft
date: 2026-10-14
supports: 20261012-005 (sweep-ordered candidates), 20261013-005 (lazy keying)
follows: 20261013-008 (lazy-keyed implementation benchmark)
---

# Benchmark: post field, the sweep key with many continuous candidates per query

Every measurement of the sweep key so far ran in one of two regimes: the project scenarios, where
a continuous query holds one candidate and no order has work to do (20261013-003 §3), and
`ccdbench.c`, where a query holds about eleven candidates and the key was worth 5 to 8 percent of
the stage against `main` (20261012-006). Neither shows what the key is for. This document adds a
scene in which a query holds tens to hundreds of candidates, most of them off the path, and measures
what each evaluation order costs there, against an oracle that knows the true order.

## 1. Result

**The scene.** `post_field`, a project benchmark scenario (`shared/benchmarks.c`, registered in
`benchmark/main.c` and as the sample "Benchmark / Post Field"): 500 spheres of radius 3 cm at
300 m/s, each put back at its start every step, swept through 25,600 thin posts (2 cm by 1 m box
hulls on a jittered 0.25 m grid over 40 m by 40 m). The sweep is 5 m per step and the query box is
the box around its two ends, so a sweep in a random horizontal direction collects every post in a
rectangle of up to 3.5 m by 3.5 m. One worker, 100 steps: 49,500 non-empty queries, 135 candidates
per query on average, 226 at most, two thirds of the queries with 100 or more, and 98 percent of
queries end in a hit. The continuous stage is 98 percent of the step. With the spheres fired along
x instead, the query box is a corridor and a query holds 19 candidates, which is `ccdbench.c`'s regime
with a longer path.

**What the order is worth.** In the random-direction scene the sweep key cuts the continuous
stage by 22 to 27 percent against `main` (§6): 5.9 s to 4.6 s over the run. The counts say why
(§5): in tree order the accepted hit is the 69th candidate evaluated on average and the running
fraction improves 2.6 times per query, so the root finder runs 5.8 million iterations; in key order
the accepted hit is the 4th candidate and the first improvement is the last, 1.2 million root
iterations. An oracle that sorts by the true time of impact needs 1.16 million: the key closes 99
percent of the gap between tree order and the best possible order. Ascending shape id is 3 percent
slower than `main` here and descending id 7 percent; the reversed key, farthest-first, is the worst
order at 65 percent slower. Along x the key is 19 to 27 percent under `main`; the id order that
is nearest-first by construction does the same work and matches the key, and the id order that is
farthest-first is 36 to 50 percent over `main`.

**What the key costs.** Nothing the pairs can separate. The one place its own work could show is
along +x, where ascending shape id is nearest-first by construction: there the key does 9 percent
fewer root iterations with 19 candidates per query and pays a slab test per candidate and a
float-and-int sort, and the two come out equal, within a point. An earlier run of the same pairs on
the one-body form of the scene had the key 2 to 7 percent behind ascending id there, so the key's
cost is at most a few percent of a stage with 19 candidates per query, and inside the spread.
Everywhere else the first improvement arriving early repays it many times over, and the PR head
with the sort removed (tree order through the collect-then-evaluate loop) matches `main` within a
point, so the loop split itself is free.

**Four workers.** The gain carries: the project benchmark binary at four workers is 22 to 27 percent
under `main` on the random-direction scene (§6).

## 2. Setup

- Builds: Release, `BOX3D_VALIDATE` off, static library, GCC 14.2 at the project's Release flags
  (`-O3`), one binary per order. `main` is `5d83df8`; the PR head is `78397ad`
  (`solver-tree-order-independent-pr`: sorted candidates, sweep key, lazy keying). The scene files
  (`shared/benchmarks.c`, `shared/benchmarks.h`, `benchmark/main.c` from `bench-post-field`) are
  copied over both heads unchanged, so every binary runs the same scene.
- Order variants are compile-time switches on the PR head's `b3EvaluateContinuousCandidates`
  (`B3_CCD_ORDER`, appendix): 0 the sweep key as landed; 1 ascending shape id (the sorted-candidate
  PR's order); 2 descending id; 3 tree order, no key and no sort; 4 oracle, each candidate keyed by
  its own time of impact against the whole sweep (a pre-pass whose calls are not counted, so its time
  is meaningless and only its counts are read); 5 the sweep key reversed, farthest first.
- Counting builds (`B3_CCD_COUNT`, appendix): non-empty queries, candidates, time-of-impact calls,
  running-fraction improvements, queries ending in a hit, the evaluation index of the accepted hit,
  the sums of the time-of-impact distance, root and push-back iterations, a histogram of candidates
  per query, and a monotonic clock around `b3SolveContinuous`. Counters are process globals; one
  worker only. Counts are deterministic and were taken once per cell.
- Harness (appendix): creates the world with the scene's capacity, one worker, sets the travel mode,
  steps once outside the clock as the benchmark app does, then 99 timed steps. Timing pairs run two
  binaries back to back, the order alternating pair by pair, from binaries without counters.
- Machine: Intel i5-10400F under WSL2, otherwise idle, turbo and thermal state uncontrolled. Pairs
  compare within themselves.
- Travel modes: random horizontal direction per sphere (the scene's default and what the benchmark
  app runs), +x and -x (`SetPostFieldTravel`), where the posts' creation order makes ascending shape
  id nearest-first in +x and farthest-first in -x.

## 3. The scene

Each post is its own static body carrying one shared box hull, so the samples app draws the field
as one geometry; posts are created row by row in x, so shape id ascends with x and, within a row,
with z. Each post is jittered by up to 10 cm from its grid point so that a sweep does not run along
a lattice line.
Sphere starts are uniform in the inner 28 m square and in y within 30 cm of the posts' mid-height,
velocity 300 m/s in the chosen direction, gravity zero, contact events off. The step function puts
each sphere back at its start with its velocity and zero angular velocity, which also resets the
sweep origin, so every step is the same 500 sweeps against the same posts. A sphere hits a post
when its centre passes within about 4 cm of the post's axis; at 16 posts per square metre that is a
free path of roughly 0.8 m, so a 5 m sweep meets a post almost always and usually within its first
fifth.

The step without continuous collision is 70 ms over 100 steps against about 5,500 ms with it, so
the benchmark app's `-nc` flag measures the stage's share directly: 98 to 99 percent.

## 4. Regime

Counts from the `main` counting build, one worker, 100 steps (99 counted). Every candidate reaches a
time-of-impact call on every build: the filters drop nothing here.

| Travel | Non-empty queries | Candidates | Per query | Max | Queries ending in a hit |
|---|---|---|---|---|---|
| random | 49,500 | 6,707,052 | 135.5 | 226 | 98.2% |
| +x | 49,203 | 935,352 | 19.0 | 101 | 97.2% |
| -x | 49,302 | 887,634 | 18.0 | 94 | 95.8% |

Candidates per query, random travel: 1 candidate 99 queries; 2 to 9: 1,881; 10 to 49: 7,029; 50
to 99: 7,029; 100 to 199: 20,394; 200 to 399: 13,068. Along x: 10 to 49 candidates in 93 percent of
queries.

## 5. Evaluation orders, counts

Same scene, same queries and candidates on every build; what differs is the order and so the work
per time-of-impact call. "Improvements" counts the times the running fraction fell; "hit index" is
the mean position, from 0, of the accepted hit in the evaluation order over queries that ended in a
hit; "gap closed" is the share of the root-iteration distance from tree order to the oracle that the
order recovers. Stage ms is the counting build's own clock, one run, for orientation only; the
timing pairs are in §6.

Random travel.

| Order | Stage ms | Improvements | Hit index | Distance iters | Root iters | Push-back iters | Gap closed |
|---|---|---|---|---|---|---|---|
| `main`, tree order | 5,936 | 126,522 | 68.3 | 9.61M | 5.79M | 2.90M | 0% |
| PR head, tree order (no sort) | 6,043 | 126,522 | 68.3 | 9.61M | 5.79M | 2.90M | 0% |
| ascending id | 6,092 | 153,648 | 65.9 | 10.02M | 6.61M | 3.31M | -18% |
| descending id | 6,378 | 168,894 | 70.9 | 10.29M | 7.14M | 3.58M | -29% |
| sweep key (PR head) | 4,359 | 48,609 | 3.0 | 7.32M | 1.21M | 0.61M | 99% |
| reversed sweep key | 9,796 | 284,823 | 133.7 | 14.58M | 15.69M | 7.86M | -214% |
| oracle, true fraction | n/a | 48,609 | 0.07 | 7.29M | 1.16M | 0.58M | 100% |

+x travel (ascending id is nearest-first).

| Order | Stage ms | Improvements | Hit index | Distance iters | Root iters | Push-back iters | Gap closed |
|---|---|---|---|---|---|---|---|
| `main`, tree order | 1,082 | 101,772 | 8.1 | 1.58M | 1.28M | 0.64M | 0% |
| PR head, tree order (no sort) | 1,028 | 101,772 | 8.1 | 1.58M | 1.28M | 0.64M | 0% |
| ascending id | 849 | 47,916 | 3.2 | 1.26M | 0.66M | 0.33M | 89% |
| descending id | 1,588 | 269,577 | 15.0 | 2.43M | 2.97M | 1.49M | -240% |
| sweep key (PR head) | 860 | 47,817 | 2.4 | 1.24M | 0.60M | 0.30M | 97% |
| reversed sweep key | 1,650 | 270,072 | 15.9 | 2.49M | 3.08M | 1.55M | -256% |
| oracle, true fraction | n/a | 47,817 | 0.07 | 1.23M | 0.58M | 0.29M | 100% |

-x travel (descending id is nearest-first).

| Order | Stage ms | Improvements | Hit index | Distance iters | Root iters | Push-back iters | Gap closed |
|---|---|---|---|---|---|---|---|
| `main`, tree order | 1,199 | 139,194 | 9.9 | 1.68M | 1.58M | 0.79M | 0% |
| PR head, tree order (no sort) | 1,143 | 139,194 | 9.9 | 1.68M | 1.58M | 0.79M | 0% |
| ascending id | 1,625 | 270,369 | 14.6 | 2.34M | 2.89M | 1.45M | -127% |
| descending id | 830 | 47,223 | 2.8 | 1.19M | 0.60M | 0.30M | 95% |
| sweep key (PR head) | 852 | 47,223 | 2.3 | 1.17M | 0.57M | 0.28M | 98% |
| reversed sweep key | 1,620 | 270,765 | 15.1 | 2.38M | 2.97M | 1.49M | -135% |
| oracle, true fraction | n/a | 47,223 | 0.06 | 1.16M | 0.55M | 0.27M | 100% |

Three things the tables show.

- The key order and the oracle accept the same hit in every query (improvements equal hit queries:
  the first candidate to improve the fraction is the accepted one). The key's three candidates ahead
  of the hit are near misses whose grown box the segment enters earlier than the hit's; each costs a
  separation test against the full sweep, which is the 4 percent of root iterations the key leaves
  above the oracle.
- Tree order on `main` and on the PR head give identical counts: the PR head's collect-then-evaluate
  loop changes nothing about the work when the sort is removed, so the difference between the PR
  head and `main` is the order alone.
- Neither id order tracks distance in the random-direction scene: both are worse than tree order,
  which has some spatial coherence from the tree. Along x each id order is nearest-first in one
  direction and the worst order but one in the other. The key is within 1 to 2 percent of the oracle
  in every travel mode.

## 6. Times, interleaved pairs

Binaries without counters. Each pair runs the two builds back to back, the order alternating pair
by pair; the delta is the sweep key's time relative to the other build in that pair. Harness rows are
total step time over 99 steps at one worker (the stage is 90 to 98 percent of it); the last row is
the project benchmark binary (`-b=post_field -w=4 -r=1`) at four workers, random travel.

| Travel | Against | Pairs | Other median ms | Sweep key median ms | Sweep key vs other, median | Range |
|---|---|---|---|---|---|---|
| random | `main` | 4 | 5,901 | 4,572 | -22.5% | -23.4 to -10.6 |
| random | ascending id | 4 | 6,226 | 4,524 | -27.3% | -28.2 to -26.4 |
| random | tree order, PR loop | 4 | 5,783 | 4,444 | -23.1% | -23.2 to -22.7 |
| +x | `main` | 4 | 1,153 | 937 | -19.1% | -20.0 to -18.0 |
| +x | ascending id | 4 | 930 | 933 | +0.2% | -0.2 to +0.7 |
| +x | tree order, PR loop | 4 | 1,120 | 929 | -17.2% | -17.3 to -15.9 |
| -x | `main` | 4 | 1,210 | 886 | -26.9% | -27.8 to -25.6 |
| -x | ascending id | 4 | 1,596 | 881 | -44.7% | -45.1 to -44.0 |
| -x | tree order, PR loop | 4 | 1,174 | 885 | -25.0% | -25.0 to -23.8 |
| random, 4 workers | `main` | 3 | 1,992 | 1,488 | -24.5% | -27.3 to -22.4 |

Read against §5: the time follows the root-iteration count. The one row without a gain is +x
against ascending id, where id order is already nearest-first with 19 candidates per query: the key
does 9 percent fewer root iterations there but pays 19 slab tests and a float-and-int sort per query,
and the two are equal within a point. The `main` and tree-order rows against the same sweep binary
agree within two points in every travel mode, so the PR head's collect-then-evaluate loop costs
nothing against `main`'s evaluate-in-callback; the whole difference is the order.

## 7. Caveats

- One machine, one compiler, WSL2, uncontrolled turbo and thermal state; the alternating pair order
  controls the drift, and the pair deltas are what the tables rest on.
- The scene is a stress case built so that every query holds many candidates. The project scenarios
  hold one (20261013-003); the key's gain on them is zero and its cost is zero by construction on the
  PR head (20261013-005). This document measures the regime the key exists for, not how often it
  occurs.
- Every candidate is a convex hull, so every call takes the GJK path. Mesh, height-field and compound
  candidates take other paths (20261013-004) and are not measured here.
- The oracle's time is not meaningful: it computes every candidate's time of impact against the whole
  sweep before evaluating any. Only its counts are read.
- The counting builds carry the counters' own cost inside the stage clock, equally in every build;
  their stage times are for orientation and are not the A/B.
- Counters are single-worker. The four-worker rows are total step time from the project benchmark
  binary.
- The scene first held every post as an offset hull on one static body; it now gives each post its
  own static body with one shared hull so that the samples app draws the field as one geometry.
  The counts are identical between the two forms except for 99 more distance iterations over the
  whole run. All tables are from the per-body form on an idle machine. The one-body form, measured
  the same way, gave the same deltas within two points except +x against ascending id, where the
  key was 2 to 7 percent behind (§1).

## 8. Files and branches

- Scene: `bench-post-field` at `d1b8e3b`, forked from `main` at `5d83df8`:
  `shared/benchmarks.c` (`CreatePostField`, `StepPostField`, `DestroyPostField`,
  `GetPostFieldCapacity`, `SetPostFieldTravel`, `SetPostFieldSphereCount`), `shared/benchmarks.h`,
  `benchmark/main.c` (`post_field`, 100 steps), `samples/sample_benchmark.cpp`
  (`BenchmarkPostField`, which runs 50 spheres so that the viewer stays interactive; the
  benchmark app runs 500).
- Measured heads: `main` `5d83df8`, PR head `78397ad`, both with the scene files copied over.
- Counting patch, order variants and harness: appendix.

## Appendix: counting patch and order variants

Applied to `src/solver.c` of each head by string replacement (`patch_count.py main|lazy`). On
`main` the per-query grouping wraps each `b3DynamicTree_Query` call; on the PR head it sits in
`b3EvaluateContinuousCandidates`. Everything is under `#ifdef B3_CCD_COUNT` except the order
switch, which defaults to 0 and leaves the landed code untouched.

```python
# Counting patch for the continuous stage, guarded by B3_CCD_COUNT. On the PR head it also adds the
# evaluation-order variants, selected by B3_CCD_ORDER (0 sweep key, 1 ascending id, 2 descending id,
# 3 tree order, 4 oracle true fraction, 5 reversed sweep key).
import sys
path, head = sys.argv[1], sys.argv[2]
s = open(path).read()

def rep(old, new, count=1):
    global s
    assert s.count(old) == count, (old[:60], s.count(old))
    s = s.replace(old, new)

GLOBALS = '''#define B2_MAX_CONTINUOUS_SENSOR_HITS 8

#ifndef B3_CCD_ORDER
#define B3_CCD_ORDER 0
#endif
#ifdef B3_CCD_COUNT
// Continuous-stage counters for measurement builds only. Single worker only.
int64_t b3_ccdQueries = 0, b3_ccdCandidates = 0, b3_ccdToiCalls = 0, b3_ccdHits = 0;
int64_t b3_ccdDistIters = 0, b3_ccdRootIters = 0, b3_ccdPushIters = 0;
int64_t b3_ccdHitQueries = 0, b3_ccdFinalHitIndex = 0, b3_ccdMaxCandidates = 0;
int64_t b3_ccdHistogram[8] = { 0 };
uint64_t b3_ccdTicks = 0;
int b3_ccdCounting = 1;
static void b3CcdEndQuery( int count, int hitIndex )
{
	if ( count == 0 ) return;
	b3_ccdQueries += 1;
	b3_ccdCandidates += count;
	if ( count > b3_ccdMaxCandidates ) b3_ccdMaxCandidates = count;
	int bucket = count == 1 ? 0 : count < 10 ? 1 : count < 50 ? 2 : count < 100 ? 3 : count < 200 ? 4 : count < 400 ? 5 : count < 800 ? 6 : 7;
	b3_ccdHistogram[bucket] += 1;
	if ( hitIndex >= 0 ) { b3_ccdHitQueries += 1; b3_ccdFinalHitIndex += hitIndex; }
}
#endif
'''
rep('#define B2_MAX_CONTINUOUS_SENSOR_HITS 8\n', GLOBALS)

# Per query bookkeeping in the context
rep('\tint distanceIterations;\n\tint pushBackIterations;\n\tint rootIterations;\n',
    '\tint distanceIterations;\n\tint pushBackIterations;\n\tint rootIterations;\n#ifdef B3_CCD_COUNT\n\tint evalIndex, hitIndex;\n#endif\n')

TOI = '\tb3TOIOutput output = b3ShapeTimeOfImpact( shape, fastShape, &sweepA, &continuousContext->sweep, continuousContext->fraction );\n'
rep(TOI, TOI + '''#ifdef B3_CCD_COUNT
	if ( b3_ccdCounting )
	{
		b3_ccdToiCalls += 1;
		b3_ccdDistIters += output.distanceIterations;
		b3_ccdRootIters += output.rootIterations;
		b3_ccdPushIters += output.pushBackIterations;
	}
#endif
''')
ACCEPT = '\t\t\tcontinuousContext->fraction = output.fraction;\n\t\t\tcontinuousContext->distanceIterations'
rep(ACCEPT, '\t\t\tcontinuousContext->fraction = output.fraction;\n#ifdef B3_CCD_COUNT\n\t\t\tif ( b3_ccdCounting ) { b3_ccdHits += 1; continuousContext->hitIndex = continuousContext->evalIndex; }\n#endif\n\t\t\tcontinuousContext->distanceIterations')

# Stage clock
rep('\tb3TracyCZoneNC( ccd, "CCD", b3_colorDarkGoldenRod, true );\n',
    '\tb3TracyCZoneNC( ccd, "CCD", b3_colorDarkGoldenRod, true );\n#ifdef B3_CCD_COUNT\n\tuint64_t ccdTicks_ = b3GetTicks();\n#endif\n')
rep('\tb3TracyCZoneEnd( ccd );\n}', '#ifdef B3_CCD_COUNT\n\tb3_ccdTicks += b3GetTicks() - ccdTicks_;\n#endif\n\tb3TracyCZoneEnd( ccd );\n}')

if head == 'main':
    # Evaluation happens in the callback: count each evaluation, group per tree query
    rep('\t// Continue query\n\treturn true;\n}', '#ifdef B3_CCD_COUNT\n\tcontinuousContext->evalIndex += 1;\n#endif\n\t// Continue query\n\treturn true;\n}')
    for tree in ('staticTree', 'kinematicTree', 'dynamicTree'):
        q = '\t\tb3DynamicTree_Query( %s, sweptBox, B3_DEFAULT_MASK_BITS, false, b3ContinuousQueryCallback, &context );\n' % tree
        pre = '#ifdef B3_CCD_COUNT\n\t\tcontext.evalIndex = 0; context.hitIndex = -1;\n#endif\n'
        post = '#ifdef B3_CCD_COUNT\n\t\tb3CcdEndQuery( context.evalIndex, context.hitIndex );\n#endif\n'
        rep(q, pre + q + post)
else:
    # Evaluation happens in b3EvaluateContinuousCandidates after the query
    rep('\tif ( count < 2 )\n\t{\n\t\tif ( count == 1 )\n\t\t{\n\t\t\tb3EvaluateContinuousCandidate( continuousContext, cs[0].shapeId );\n\t\t}\n',
        '#ifdef B3_CCD_COUNT\n\tcontinuousContext->evalIndex = 0; continuousContext->hitIndex = -1;\n#endif\n\tif ( count < 2 )\n\t{\n\t\tif ( count == 1 )\n\t\t{\n\t\t\tb3EvaluateContinuousCandidate( continuousContext, cs[0].shapeId );\n#ifdef B3_CCD_COUNT\n\t\t\tb3CcdEndQuery( 1, continuousContext->hitIndex );\n#endif\n\t\t}\n')
    KEYLOOP = '''	b3World* world = continuousContext->world;
	for ( int i = 0; i < count; ++i )
	{
		b3Shape* shape = b3Array_Get( world->shapes, cs[i].shapeId );
		cs[i].key = b3ContinuousSweepKey( continuousContext, shape );
	}
'''
    rep(KEYLOOP, '''	b3World* world = continuousContext->world;
#if B3_CCD_ORDER == 3
	// Tree order: no key, no sort
	B3_UNUSED( world );
	goto evaluate;
#endif
	for ( int i = 0; i < count; ++i )
	{
		b3Shape* shape = b3Array_Get( world->shapes, cs[i].shapeId );
#if B3_CCD_ORDER == 1
		cs[i].key = 0.0f; // ascending id
#elif B3_CCD_ORDER == 2
		cs[i].key = -(float)cs[i].shapeId; // descending id
#elif B3_CCD_ORDER == 4
		{
			// Oracle: the true time of impact against the whole sweep, not counted as work
			b3Body* body = b3Array_Get( world->bodies, shape->bodyId );
			b3BodySim* bodySim = b3GetBodySim( world, body );
			b3Sweep sweepA = b3MakeRelativeSweep( bodySim, continuousContext->base );
#ifdef B3_CCD_COUNT
			b3_ccdCounting = 0;
#endif
			b3TOIOutput oracle = b3ShapeTimeOfImpact( shape, continuousContext->fastShape, &sweepA, &continuousContext->sweep, 1.0f );
#ifdef B3_CCD_COUNT
			b3_ccdCounting = 1;
#endif
			cs[i].key = ( oracle.state == b3_toiStateHit || oracle.state == b3_toiStateOverlapped ) ? oracle.fraction : 1.0f;
		}
#elif B3_CCD_ORDER == 5
		cs[i].key = -b3ContinuousSweepKey( continuousContext, shape ); // farthest first
#else
		cs[i].key = b3ContinuousSweepKey( continuousContext, shape );
#endif
	}
''')
    EVAL = '''	for ( int i = 0; i < count; ++i )
	{
		b3EvaluateContinuousCandidate( continuousContext, cs[i].shapeId );
	}

	b3Array_Clear( *candidates );
}'''
    rep(EVAL, '''#if B3_CCD_ORDER == 3
evaluate:
#endif
	for ( int i = 0; i < count; ++i )
	{
		b3EvaluateContinuousCandidate( continuousContext, cs[i].shapeId );
#ifdef B3_CCD_COUNT
		continuousContext->evalIndex += 1;
#endif
	}
#ifdef B3_CCD_COUNT
	b3CcdEndQuery( count, continuousContext->hitIndex );
#endif

	b3Array_Clear( *candidates );
}''')
open(path, 'w').write(s)
print('patched', path)
```

## Appendix: `postfield.c`

Built against the Release static libraries:
`gcc -std=c17 -O2 [-DCOUNTERS] postfield.c -I include -I shared -L build/src -L build/shared -lshared -lbox3d -lm -lpthread`.
Arguments: travel mode (0, 1, -1), steps, repeats.

```c
// Post-field harness: steps the post_field scene single-threaded for a number of steps and reports
// the total step time, the continuous-stage clock and the stage counters of a counting build.
// Usage: postfield <travel: 0|1|-1> [steps=100] [repeats=1]
#include "benchmarks.h"
#include "box3d/box3d.h"
#include "box3d/math_functions.h"
#include "box3d/types.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef COUNTERS
extern int64_t b3_ccdQueries, b3_ccdCandidates, b3_ccdToiCalls, b3_ccdHits;
extern int64_t b3_ccdDistIters, b3_ccdRootIters, b3_ccdPushIters;
extern int64_t b3_ccdHitQueries, b3_ccdFinalHitIndex, b3_ccdMaxCandidates, b3_ccdHistogram[8];
extern uint64_t b3_ccdTicks;
#endif

int main( int argc, char** argv )
{
	int travel = argc > 1 ? atoi( argv[1] ) : 0;
	int steps = argc > 2 ? atoi( argv[2] ) : 100;
	int repeats = argc > 3 ? atoi( argv[3] ) : 1;

	float bestTotal = 1e30f;
	double bestStage = 1e30;
	for ( int rep = 0; rep < repeats; ++rep )
	{
		b3WorldDef def = b3DefaultWorldDef();
		def.workerCount = 1;
		GetPostFieldCapacity( &def.capacity );
		b3WorldId world = b3CreateWorld( &def );
		SetPostFieldTravel( travel );
		CreatePostField( world );

		// Warm step outside the clock, as the benchmark app does
		StepPostField( world, 0 );
		b3World_Step( world, 1.0f / 60.0f, 4 );
#ifdef COUNTERS
		b3_ccdQueries = b3_ccdCandidates = b3_ccdToiCalls = b3_ccdHits = 0;
		b3_ccdDistIters = b3_ccdRootIters = b3_ccdPushIters = 0;
		b3_ccdHitQueries = b3_ccdFinalHitIndex = b3_ccdMaxCandidates = 0;
		for ( int i = 0; i < 8; ++i ) b3_ccdHistogram[i] = 0;
		b3_ccdTicks = 0;
#endif
		uint64_t ticks = b3GetTicks();
		for ( int i = 1; i < steps; ++i )
		{
			StepPostField( world, i );
			b3World_Step( world, 1.0f / 60.0f, 4 );
		}
		float total = b3GetMilliseconds( ticks );
		if ( total < bestTotal ) bestTotal = total;
#ifdef COUNTERS
		double stage = (double)b3_ccdTicks / 1e6;
		if ( stage < bestStage ) bestStage = stage;
#endif
		DestroyPostField();
		b3DestroyWorld( world );
	}

	printf( "travel %+d steps %d: total %.1f ms", travel, steps, bestTotal );
#ifdef COUNTERS
	printf( "  stage %.1f ms (%.1f%%)", bestStage, 100.0 * bestStage / bestTotal );
	printf( "\nqueries %lld candidates %lld (%.1f per query, max %lld) toi %lld hits %lld hitQueries %lld (%.1f%%) finalHitIndex mean %.2f\n",
			(long long)b3_ccdQueries, (long long)b3_ccdCandidates, b3_ccdQueries ? (double)b3_ccdCandidates / b3_ccdQueries : 0.0,
			(long long)b3_ccdMaxCandidates, (long long)b3_ccdToiCalls, (long long)b3_ccdHits, (long long)b3_ccdHitQueries,
			b3_ccdQueries ? 100.0 * b3_ccdHitQueries / b3_ccdQueries : 0.0,
			b3_ccdHitQueries ? (double)b3_ccdFinalHitIndex / b3_ccdHitQueries : 0.0 );
	printf( "distIters %lld rootIters %lld pushIters %lld (per call %.2f / %.2f / %.2f)\n", (long long)b3_ccdDistIters,
			(long long)b3_ccdRootIters, (long long)b3_ccdPushIters, b3_ccdToiCalls ? (double)b3_ccdDistIters / b3_ccdToiCalls : 0.0,
			b3_ccdToiCalls ? (double)b3_ccdRootIters / b3_ccdToiCalls : 0.0, b3_ccdToiCalls ? (double)b3_ccdPushIters / b3_ccdToiCalls : 0.0 );
	printf( "histogram 1:%lld 2-9:%lld 10-49:%lld 50-99:%lld 100-199:%lld 200-399:%lld 400-799:%lld 800+:%lld", (long long)b3_ccdHistogram[0],
			(long long)b3_ccdHistogram[1], (long long)b3_ccdHistogram[2], (long long)b3_ccdHistogram[3], (long long)b3_ccdHistogram[4],
			(long long)b3_ccdHistogram[5], (long long)b3_ccdHistogram[6], (long long)b3_ccdHistogram[7] );
#endif
	printf( "\n" );
	return 0;
}
```
