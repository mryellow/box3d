---
title: Benchmark - sorted-candidate tree-order-independent solver changes
status: Draft
date: 2026-10-12
supports: solver-tree-order-independent-pr (20261012-002)
---

# Benchmark: sorted-candidate tree-order-independent solver changes

An A/B measurement of the PR's cost. The continuous stage now collects the candidates of each tree
query into a per-worker array, sorts them by shape id and evaluates them in that order
(`src/solver.c`, commit e9bbb72). The question is whether the collection and the sort slow the
engine in real scenes, and by how much in a scene that is nothing but continuous collision.

## 1. Result

The sort does not slow the engine. In the fifteen project benchmarks the PR is within run-to-run
noise of `main`. In a scene built to be dominated by continuous queries the PR is 5 to 7 percent
faster than `main`, in every pairing and either run order.

## 2. Setup

- Builds: Release, AVX2 kernels, `BOX3D_VALIDATE` off, static library; `main` at `5d83df8` and the
  PR head at `f66ee7f` (the test-only commit 8b18782 on top of it does not touch `src/`).
  GCC 14.2, `-O3` from the project's Release flags.
- Machine: Intel i5-10400F (6 cores, 12 threads) under WSL2, otherwise idle. Turbo and thermal
  state are not controlled, which matters below.
- Project benchmarks: every scenario in `benchmark/main.c`, run as
  `benchmark -b=<name> -w=<workers> -r=<repeats>`; the reported time is the benchmark's own minimum
  over the repeats of the whole stepped run. Worker counts 1 and 4. The two builds run back to back
  per scenario and worker count.
- Microbenchmark `ccdbench.c` (appendix): 4000 dynamic spheres of radius 0.1 at 300 m/s, reset to
  their start line with `b3Body_SetTransform` every step so every step is a full sweep, through
  14,641 static 0.1 m boxes on a 0.5 m grid over 60 m by 60 m at the spheres' height. 100 steps,
  four sub-steps, one worker, minimum of three worlds. Two variants: plain dynamic bodies (continuous
  against the static tree only) and bullets, where the profile's `bullets` field times the
  continuous stage on its own.

## 3. Project benchmarks, first pass

`main` first, PR second, 4 repeats.

| Scenario | Workers | `main` ms | PR ms | PR vs `main` |
|---|---|---|---|---|
| convex_pile | 1 | 7483 | 7838 | +4.7% |
| convex_pile | 4 | 2876 | 3088 | +7.4% |
| joint_grid | 1 | 2813 | 2858 | +1.6% |
| joint_grid | 4 | 901 | 927 | +2.9% |
| junkyard | 1 | 20399 | 20694 | +1.4% |
| junkyard | 4 | 7661 | 7966 | +4.0% |
| large_pyramid | 1 | 2763 | 2692 | -2.6% |
| large_pyramid | 4 | 994 | 1016 | +2.2% |
| large_world | 1 | 9 | 8 | -1.1% |
| large_world | 4 | 68 | 68 | -0.7% |
| many_pyramids | 1 | 2997 | 3013 | +0.5% |
| many_pyramids | 4 | 1121 | 1146 | +2.3% |
| mesh_drop | 1 | 2580 | 2545 | -1.3% |
| mesh_drop | 4 | 901 | 930 | +3.3% |
| rain | 1 | 3480 | 3430 | -1.4% |
| rain | 4 | 1222 | 1326 | +8.5% |
| sleep | 1 | 1178 | 1160 | -1.5% |
| sleep | 4 | 468 | 485 | +3.7% |
| spinner | 1 | 4276 | 4271 | -0.1% |
| spinner | 4 | 1898 | 2041 | +7.5% |
| trees100 | 1 | 211 | 207 | -1.9% |
| trees100 | 4 | 139 | 139 | +0.7% |
| trees50 | 1 | 306 | 304 | -0.4% |
| trees50 | 4 | 172 | 180 | +4.9% |
| trees25 | 1 | 671 | 666 | -0.6% |
| trees25 | 4 | 322 | 334 | +3.7% |
| village | 1 | 4012 | 3973 | -1.0% |
| village | 4 | 1784 | 1902 | +6.6% |
| washer | 1 | 31070 | 30841 | -0.7% |
| washer | 4 | 11739 | 12316 | +4.9% |

Single worker: deltas of both signs, from -2.6 to +4.7 percent. The four runs of `main` alone on
`convex_pile` spanned 7483 to 7782 ms, 4 percent, so the +4.7 percent is inside one build's own
spread. Four workers: the PR is 0.7 to 8.5 percent slower on every scenario, which looked
systematic and is examined in the next pass.

## 4. Project benchmarks, reversed order

The seven scenarios with the largest four-worker deltas, PR first, `main` second, 6 repeats.

| Scenario | Workers | `main` ms | PR ms | PR vs `main` |
|---|---|---|---|---|
| convex_pile | 4 | 3275 | 3055 | -6.7% |
| convex_pile | 1 | 8764 | 8926 | +1.8% |
| rain | 4 | 1272 | 1206 | -5.2% |
| rain | 1 | 3385 | 3408 | +0.7% |
| spinner | 4 | 2030 | 1876 | -7.6% |
| spinner | 1 | 4180 | 4340 | +3.8% |
| village | 4 | 1874 | 1766 | -5.8% |
| village | 1 | 3959 | 4019 | +1.5% |
| washer | 4 | 12187 | 11503 | -5.6% |
| washer | 1 | 31368 | 30959 | -1.3% |
| junkyard | 4 | 9771 | 9254 | -5.3% |
| junkyard | 1 | 21113 | 21205 | +0.4% |
| trees50 | 4 | 186 | 175 | -6.0% |
| trees50 | 1 | 302 | 314 | +4.0% |

The four-worker deficit changed sides with the run order: whichever build runs second at four
workers is 5 to 8 percent slower, on every scenario. That is thermal or turbo drift of the machine
across a 30 to 60 second burst on four cores, not the code. The single-worker deltas stay within
-1.3 to +4.0 percent with both signs. The project benchmarks also hold few fast bodies, so the
continuous stage, the only place the PR adds work, is a small part of their step.

## 5. Continuous-collision microbenchmark

Run twice in each order. Times in ms; the continuous-stage time is the sum over the 100 steps of
the profile's `bullets` field, which is zero for non-bullet bodies.

| Order | Variant | `main` | PR | PR vs `main` |
|---|---|---|---|---|
| main, PR | non-bullet, total | 5229 | 4785 | -8.5% |
| main, PR | non-bullet, total | 5001 | 4684 | -6.3% |
| main, PR | bullet, continuous stage | 4530 | 4265 | -5.8% |
| PR, main | non-bullet, total | 5309 | 4964 | -6.5% |
| PR, main | non-bullet, total | 5361 | 5078 | -5.3% |
| PR, main | bullet, continuous stage | 4863 | 4567 | -6.1% |

The PR is faster in every pairing. The cause is not measured. Neither branch allocates per step in
the continuous path: the candidate array is reserved once per worker at world creation
(`src/physics_world.c`), so the gain is not from allocation. Three candidates remain, not separated
here:

- Memory order: ascending shape id walks the shape array and the body lookups it triggers
  sequentially, where tree order is spatial.
- Separated loops: on `main` the tree traversal is interleaved with time-of-impact solves inside the
  callback; on the PR the traversal runs as one tight loop and the solves as another.
- Running-fraction evolution: each solve brackets on the running best fraction, so the order sets
  how fast the bracket narrows and how many iterations later candidates need. Shape id order and
  tree order are both arbitrary with respect to distance, but they differ in which candidates come
  first.

Separating them would take a profiler run on both builds, or a variant that collects the candidates
and evaluates them in tree order, which isolates the sort from the loop separation. The Explode
change (f66ee7f) is not exercised by any benchmark.

## 6. Replication

A second complete pass on the same binaries, after the machine had been idle. Full sweep with the
PR first and `main` second, 4 repeats, then the microbenchmark twice in each order.

| Scenario | Workers | `main` ms | PR ms | PR vs `main` |
|---|---|---|---|---|
| convex_pile | 1 | 8541 | 8397 | -1.7% |
| convex_pile | 4 | 3247 | 3108 | -4.3% |
| joint_grid | 1 | 2779 | 2824 | +1.6% |
| joint_grid | 4 | 1024 | 1048 | +2.4% |
| junkyard | 1 | 19922 | 19656 | -1.3% |
| junkyard | 4 | 7803 | 7290 | -6.6% |
| large_pyramid | 1 | 2668 | 2733 | +2.4% |
| large_pyramid | 4 | 1001 | 954 | -4.7% |
| large_world | 1 | 9 | 8 | -3.2% |
| large_world | 4 | 67 | 71 | +5.5% |
| many_pyramids | 1 | 2976 | 2968 | -0.3% |
| many_pyramids | 4 | 1136 | 1162 | +2.3% |
| mesh_drop | 1 | 2524 | 2557 | +1.3% |
| mesh_drop | 4 | 930 | 916 | -1.5% |
| rain | 1 | 3484 | 3520 | +1.0% |
| rain | 4 | 1304 | 1268 | -2.8% |
| sleep | 1 | 1151 | 1156 | +0.4% |
| sleep | 4 | 487 | 460 | -5.6% |
| spinner | 1 | 4193 | 4263 | +1.7% |
| spinner | 4 | 2006 | 1902 | -5.2% |
| trees100 | 1 | 192 | 201 | +4.9% |
| trees100 | 4 | 136 | 133 | -2.4% |
| trees50 | 1 | 301 | 301 | -0.0% |
| trees50 | 4 | 182 | 178 | -1.9% |
| trees25 | 1 | 656 | 667 | +1.6% |
| trees25 | 4 | 336 | 336 | +0.2% |
| village | 1 | 3962 | 4000 | +1.0% |
| village | 4 | 1854 | 1788 | -3.5% |
| washer | 1 | 30676 | 30660 | -0.1% |
| washer | 4 | 12292 | 11538 | -6.1% |

Single worker: -3.2 to +4.9 percent, both signs, eleven of fifteen within 2 percent. Four workers:
with the PR now running first, nine of fifteen scenarios favour the PR by up to 6.6 percent and the
rest `main` by up to 5.5 percent, confirming that the first pass's four-worker deficit was run order.

| Order | Variant | `main` | PR | PR vs `main` |
|---|---|---|---|---|
| main, PR | non-bullet, total | 4459 | 4124 | -7.5% |
| main, PR | non-bullet, total | 4566 | 4440 | -2.8% |
| PR, main | bullet, continuous stage | 4560 | 4045 | -11.3% |
| PR, main | bullet, continuous stage | 4423 | 4196 | -5.1% |

The PR is faster in all four pairings again. Over both passes the microbenchmark gave ten pairings,
every one in the PR's favour, from -2.8 to -11.3 percent, median about -6 percent. The replication
changes no conclusion of §1.

## 7. Caveats

- One machine, one compiler, WSL2. The four-worker numbers show the machine drifts by 5 to 8 percent
  over a burst, so single-run deltas under about 5 percent are not evidence either way.
- The project scenarios are a poor probe of the continuous stage. The microbenchmark is the probe,
  and it is an extreme: every body is fast every step and every query returns many candidates.
- Not measured: worlds with many workers in the microbenchmark (the sort is per query, so per-worker
  cost does not change with worker count), and Explode.

## Appendix: `ccdbench.c`

Built against the Release static library:
`gcc -std=c17 -O2 ccdbench.c -I include -L build/src -lbox3d -lm -lpthread`. Arguments: sphere
count, steps, repeats, bullets flag.

```c
// Continuous-collision microbenchmark: many fast spheres swept through a dense static field every
// step, so every step runs a large number of continuous queries with many candidates each.
// Reports the summed continuous-stage profile time and the total step time.
#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/math_functions.h"
#include "box3d/types.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct { uint32_t s; } Rng;
static uint32_t NextRandom( Rng* r ) { uint32_t x = r->s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; r->s = x; return x; }
static float RandomFloat( Rng* r, float lo, float hi ) { return lo + ( hi - lo ) * (float)( NextRandom( r ) >> 8 ) / 16777216.0f; }

int main( int argc, char** argv )
{
	int sphereCount = argc > 1 ? atoi( argv[1] ) : 4000;
	int steps = argc > 2 ? atoi( argv[2] ) : 100;
	int repeats = argc > 3 ? atoi( argv[3] ) : 3;
	bool bullets = argc > 4 ? atoi( argv[4] ) != 0 : false;
	float speed = 300.0f, dt = 1.0f / 60.0f;

	float bestStep = 1e30f, bestCcd = 1e30f;
	int candidates = 0;
	for ( int rep = 0; rep < repeats; ++rep )
	{
		b3WorldDef worldDef = b3DefaultWorldDef();
		worldDef.gravity = b3Vec3_zero;
		worldDef.enableSleep = false;
		b3WorldId world = b3CreateWorld( &worldDef );

		// Static field: small boxes on a 0.5 m grid across 60 m x 60 m, at the spheres' height
		b3BodyDef groundDef = b3DefaultBodyDef();
		b3BodyId ground = b3CreateBody( world, &groundDef );
		b3ShapeDef boxDef = b3DefaultShapeDef();
		int n = 0;
		for ( float x = -30.0f; x <= 30.0f; x += 0.5f )
			for ( float z = -30.0f; z <= 30.0f; z += 0.5f )
			{
				b3BoxHull hull = b3MakeOffsetBoxHull( 0.05f, 0.05f, 0.05f, (b3Vec3){ x, 0.0f, z } );
				b3CreateHullShape( ground, &boxDef, &hull.base );
				n += 1;
			}

		Rng rng = { 12345u };
		b3BodyId* spheres = malloc( sphereCount * sizeof( b3BodyId ) );
		b3Vec3* starts = malloc( sphereCount * sizeof( b3Vec3 ) );
		for ( int i = 0; i < sphereCount; ++i )
		{
			starts[i] = (b3Vec3){ RandomFloat( &rng, -25.0f, 25.0f ), RandomFloat( &rng, -0.3f, 0.3f ), RandomFloat( &rng, -25.0f, 25.0f ) };
			b3BodyDef bd = b3DefaultBodyDef();
			bd.type = b3_dynamicBody;
			bd.position = b3ToPos( starts[i] );
			bd.linearVelocity = (b3Vec3){ speed, 0.0f, 0.0f };
			bd.isBullet = bullets;
			spheres[i] = b3CreateBody( world, &bd );
			b3ShapeDef sd = b3DefaultShapeDef();
			sd.enableContactEvents = false;
			b3Sphere s = { b3Vec3_zero, 0.1f };
			b3CreateSphereShape( spheres[i], &sd, &s );
		}

		float ccdMs = 0.0f;
		uint64_t ticks = b3GetTicks();
		for ( int step = 0; step < steps; ++step )
		{
			// Put every sphere back on its start line, moving fast, so every step is a continuous sweep
			for ( int i = 0; i < sphereCount; ++i )
			{
				b3Body_SetTransform( spheres[i], b3ToPos( starts[i] ), b3Quat_identity );
				b3Body_SetLinearVelocity( spheres[i], (b3Vec3){ speed, 0.0f, 0.0f } );
			}
			b3World_Step( world, dt, 4 );
			ccdMs += b3World_GetProfile( world ).bullets;
		}
		float stepMs = b3GetMilliseconds( ticks );
		if ( stepMs < bestStep ) bestStep = stepMs;
		if ( ccdMs < bestCcd ) bestCcd = ccdMs;
		candidates = n;
		free( spheres );
		free( starts );
		b3DestroyWorld( world );
	}
	printf( "spheres %d, static boxes %d, steps %d, bullets %d: continuous stage %.1f ms, total %.1f ms (min of %d)\n", sphereCount,
			candidates, steps, bullets, bestCcd, bestStep, repeats );
	return 0;
}
```
