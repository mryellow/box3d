---
title: Benchmark - sweep-ordered continuous candidates, prototype measurements
status: Draft
date: 2026-10-12
supports: 20261012-005 (preflight)
---

# Benchmark: sweep-ordered continuous candidates, prototype measurements

A measurement of sweep-key ordering (B1) and of a static-candidate cull on top of it (B2), taken on
a prototype of each, so that the preflight 20261012-005 rests on an observation rather than on the
extrapolation in 20261012-004 §5. The preflight proposes B1 alone; B2 is measured here and not
proposed (§6). The questions: does ordering by the sweep key reach
the nearest-first time in both travel directions, what does the key itself cost, and how much does
the static cull remove.

## 1. Result

Ordering by the sweep key (B1) takes the nearest-first gain in both directions: within 2 percent of
`main` to 6 percent under it with the spheres fired in +x, and 1 to 7 percent under it in -x, where
the ascending-id order of the sorted-candidate PR is 8 to 15 percent slower than `main`. In +x, where
ascending id is exactly nearest-first, B1 is 1 to 8 percent behind it: the cost of the key and the
wider comparison, inside the machine's drift. The static cull (B2) on top of B1 is the fastest build
in every pairing, 8 to 16 percent under `main` in +x and 10 to 16 percent in -x, 2 to 9 percent
under the PR in +x and 19 to 23 percent in -x. It removes a third of the time-of-impact calls. An
earlier prototype with a bare `r` growth, no filter move and a skip of every static candidate gave
the same picture (B1+B2 13 to 17 percent under `main`, B1 2 to 7 percent). The unit suite passes on
both prototypes except the test that asserts ascending-id callback order, which the preflight
replaces.

## 2. Setup

- Builds: Release, AVX2 kernels, `BOX3D_VALIDATE` off, static library, GCC 14.2 at `-O3`. Five
  binaries of `ccdbench.c` (20261012-004 appendix): `main` at `5d83df8`, the sorted-candidate PR
  head at `f66ee7f`, the descending-id variant of 20261012-004 §5 (all three the binaries of that
  document), and two prototypes on the PR head: B1 (sweep-key order, all queries) and B1+B2
  (B1 with the static cull). The prototype diff is in the appendix: filters in the callback, the
  key-1 rule, a key growth of `r` plus two linear slops plus a rounding term, a convex-solid flag on
  each candidate, and a static skip restricted to flagged candidates.
- Machine: Intel i5-10400F under WSL2, otherwise idle, turbo and thermal state uncontrolled, as in
  20261012-004. Rows compare within themselves.
- Scene: 4000 spheres at 300 m/s through 14,641 static boxes, 100 steps, four sub-steps, one worker,
  minimum of three worlds. Non-bullet runs report the total; bullet runs report the continuous stage
  (`bullets` profile field). Three rotations of the run order per direction for non-bullet runs,
  two for bullet runs.

## 3. Times

Non-bullet, total ms, spheres fired in +x (ascending id is nearest-first).

| Rotation | `main` | PR | descending | B1 | B1+B2 |
|---|---|---|---|---|---|
| main, PR, desc, B1, B1+B2 | 4479 | 4205 | 5076 | 4553 | 4112 |
| B1+B2, B1, desc, PR, main | 4954 | 4547 | 5373 | 4737 | 4166 |
| B1, B1+B2, main, PR, desc | 5055 | 4708 | 5683 | 4754 | 4299 |

Non-bullet, total ms, spheres fired in -x (ascending id is farthest-first).

| Rotation | `main` | PR | descending | B1 | B1+B2 |
|---|---|---|---|---|---|
| main, PR, desc, B1, B1+B2 | 5050 | 5802 | 4954 | 5012 | 4531 |
| B1+B2, B1, desc, PR, main | 5282 | 5682 | 4842 | 5098 | 4616 |
| B1, B1+B2, main, PR, desc | 5395 | 5863 | 5004 | 5009 | 4515 |

Bullets, continuous stage ms.

| Direction | Rotation | `main` | PR | B1 | B1+B2 |
|---|---|---|---|---|---|
| +x | main, PR, B1, B1+B2 | 4751 | 4516 | 4568 | 4003 |
| +x | B1+B2, B1, PR, main | 4631 | 4328 | 4542 | 3962 |
| -x | main, PR, B1, B1+B2 | 4724 | 5317 | 4530 | 3959 |
| -x | B1+B2, B1, PR, main | 4785 | 5563 | 4618 | 4078 |

Relative to `main` in the same row: B1 is +1.7, -4.4 and -6.0 percent in +x and -0.8, -3.5 and
-7.2 percent in -x; B1+B2 is -8.2, -15.9 and -15.0 percent in +x and -10.3, -12.6 and -16.3
percent in -x; the PR is -6.1, -8.2 and -6.9 percent in +x and +14.9, +7.6 and +8.7 percent in
-x. In the bullet rows B1 is 2 to 4 percent under `main` either way and B1+B2 is 14 to 16 percent
under it. In the bullet scene every dynamic candidate is another bullet and is dropped by the bullet
filter in the callback before it is collected (§4), so the dynamic query costs a tree walk and the
filters but no keys and no solves.

The run as a whole was 2 to 4 percent slower than the earlier prototype's run on the unchanged
`main`, PR and descending binaries, which is the machine's drift between the two bursts; the
prototype rows moved with them.

## 4. Time-of-impact calls

Two more builds of the prototypes with a counter on the candidates collected and on the calls to
`b3ShapeTimeOfImpact`, 20 steps, one world. Static queries: 80,000 (4000 spheres, 20 steps).

| Direction | Bullets | Candidates collected | Calls, B1 | Calls, B1+B2 | Removed |
|---|---|---|---|---|---|
| +x | no | 486,203 | 486,203 | 321,060 | 34% |
| -x | no | 496,041 | 496,041 | 321,120 | 35% |
| +x | yes | 486,203 | 486,203 | 321,060 | 34% |
| -x | yes | 496,041 | 496,041 | 321,120 | 35% |

About six static candidates per query reach a time-of-impact call under B1 and four under B1+B2.
The bullet runs collect the same candidates as the non-bullet runs: the other spheres fail the
bullet filter in the callback and are never collected. The earlier prototype, with a bare `r`
growth, made 320,380 and 320,540 calls; the two-slop and rounding terms admit about 700 more
candidates per 80,000 queries.

## 5. Unit tests

The PR head's suite on both prototypes, one test at a time: every test passes except
`ContinuousCallbackOrderTest`, which asserts ascending-id order of the custom-filter and pre-solve
calls and fails on both with the new order. `ContinuousCandidateOrderTest` (layout-independence
fuzz), `ContinuousSensorSelectionTest`, `DeterminismTest`, `ExplodeOrderTest` and
`TreeOrderHelperTest` pass on both. The public proof tests of 20261012-003 were not in the prototype
tree.

## 6. Caveats

- The prototype is not the preflight's change. The preflight proposes B1 with a growth of `r` alone
  and no element flag, and no skip of any candidate; its Costs paragraph points at §1 here. Read it
  against the B1 columns only: the B1+B2 columns measure a cull the preflight does not propose, and
  the B1 build differs from the preflight's B1 by the two-slop and rounding terms in the growth
  (about a centimetre on the box edge) and an unused flag in the element.
- One machine, WSL2, drift of 5 to 8 percent across a burst (20261012-004 §7); the B1+B2 margins
  exceed it in every row, the B1 margins against `main` do not in every row.
- Every candidate in this scene is a convex solid box, so the skip's restriction to flagged
  candidates removes nothing here; a scene with static meshes or sensors would evaluate those in
  full.
- The project benchmarks were not run; they hold few fast bodies (20261012-004 §4).
- Worker counts above one were not run; the key, the sort and the cull are per query.

## Appendix: prototype diff against the PR head `f66ee7f`

```diff
diff --git a/src/physics_world.h b/src/physics_world.h
index 9455b31..f9b471f 100644
--- a/src/physics_world.h
+++ b/src/physics_world.h
@@ -70,6 +70,19 @@ typedef struct b3DebugLine
 } b3DebugLine;
 
 // Per thread task storage
+// A continuous collision candidate: a sort key that estimates where along the sweep the fast shape
+// first reaches the candidate, and the shape id as the tie-break.
+typedef struct b3ContinuousCandidate
+{
+	float key;
+	int shapeId;
+
+	// A convex solid: the key is a lower bound on any hit fraction, so the static loop may skip it
+	bool convexSolid;
+} b3ContinuousCandidate;
+
+b3DeclareArrayNative( b3ContinuousCandidate );
+
 typedef struct b3TaskContext
 {
 	b3Arena arena;
@@ -80,8 +93,8 @@ typedef struct b3TaskContext
 	// Broad-phase pairs.
 	b3Array( uint64_t ) pairKeys;
 
-	// Continuous collision candidates of one tree query, evaluated in shape id order.
-	b3Array( int ) continuousCandidates;
+	// Continuous collision candidates of one tree query, evaluated in ascending sweep key then shape id.
+	b3Array( b3ContinuousCandidate ) continuousCandidates;
 
 	// These bits align with the b3ConstraintGraph::contactBlocks and signal a change in contact status
 	b3BitSet contactStateBitSet;
diff --git a/src/solver.c b/src/solver.c
index c64ca91..56d71c1 100644
--- a/src/solver.c
+++ b/src/solver.c
@@ -22,6 +22,7 @@
 #include "shape.h"
 #include "solver_set.h"
 
+#include <float.h>
 #include <limits.h>
 #include <stddef.h>
 #include <stdio.h>
@@ -332,10 +333,28 @@ typedef struct b3ContinuousContext
 	int pushBackIterations;
 	int rootIterations;
 
-	// Candidates of one tree query, collected by the callback and evaluated in shape id order
-	b3Array( int ) * candidates;
+	// Candidates of one tree query, collected by the callback with a sweep key and evaluated in
+	// ascending key, then shape id
+	b3Array( b3ContinuousCandidate ) * candidates;
+
+	// Growth of a candidate's box for the key: the fast shape's bounding radius about its centroid,
+	// two linear slops for the time-of-impact acceptance distance, and eight float epsilons of the
+	// largest coordinate the key touches for rounding
+	float fastGrowth;
+
+	// Candidates whose key is at or beyond the running fraction cannot improve it: skip them.
+	// Only sound for the static tree, whose boxes do not move during the sweep.
+	bool cullByKey;
 } b3ContinuousContext;
 
+#ifndef B3_SWEEP_CULL
+#define B3_SWEEP_CULL 0
+#endif
+#ifdef B3_TOI_COUNT
+int b3_toiCalls = 0;
+int b3_toiCandidates = 0;
+#endif
+
 // This is called from b3DynamicTree_Query for continuous collision. It only collects the
 // candidate. Evaluation happens after the query in ascending shape id, so the result does not
 // depend on the tree layout.
@@ -344,66 +363,105 @@ static bool b3ContinuousQueryCallback( int proxyId, uint64_t userData, void* con
 	B3_UNUSED( proxyId );
 
 	b3ContinuousContext* continuousContext = context;
-	b3Array_Push( *continuousContext->candidates, (int)userData );
-	return true;
-}
-
-// Evaluate one continuous collision candidate against the fast shape
-static void b3EvaluateContinuousCandidate( b3ContinuousContext* continuousContext, int shapeId )
-{
+	b3World* world = continuousContext->world;
 	b3Shape* fastShape = continuousContext->fastShape;
 	b3BodySim* fastBodySim = continuousContext->fastBodySim;
 
-	B3_ASSERT( fastShape->sensorIndex == B3_NULL_INDEX );
+	int shapeId = (int)userData;
 
 	// Skip same shape
 	if ( shapeId == fastShape->id )
 	{
-		return;
+		return true;
 	}
 
-	b3World* world = continuousContext->world;
 	b3Shape* shape = b3Array_Get( world->shapes, shapeId );
 
 	// Skip same body
 	if ( shape->bodyId == fastShape->bodyId )
 	{
-		return;
+		return true;
 	}
 
 	// Skip sensors unless both shapes want sensor events
 	bool isSensor = shape->sensorIndex != B3_NULL_INDEX;
 	if ( isSensor && ( ( shape->flags & b3_enableSensorEvents ) == 0 || ( fastShape->flags & b3_enableSensorEvents ) == 0 ) )
 	{
-		return;
+		return true;
 	}
 
 	// Skip filtered shapes
-	bool canCollide = b3ShouldShapesCollide( fastShape->filter, shape->filter );
-	if ( canCollide == false )
+	if ( b3ShouldShapesCollide( fastShape->filter, shape->filter ) == false )
 	{
-		return;
+		return true;
 	}
 
 	b3Body* body = b3Array_Get( world->bodies, shape->bodyId );
-
-	b3BodySim* bodySim = b3GetBodySim( world, body );
 	B3_ASSERT( body->type == b3_staticBody || ( fastBodySim->flags & b3_isBullet ) );
 
 	// Skip bullets
 	if ( body->flags & b3_isBullet )
 	{
-		return;
+		return true;
 	}
 
 	// Skip filtered bodies
 	b3Body* fastBody = b3Array_Get( world->bodies, fastBodySim->bodyId );
-	canCollide = b3ShouldBodiesCollide( world, fastBody, body );
-	if ( canCollide == false )
+	if ( b3ShouldBodiesCollide( world, fastBody, body ) == false )
 	{
-		return;
+		return true;
+	}
+
+	// The candidate's box relative to the sweep base, grown by the fast shape's bounding radius,
+	// the time-of-impact acceptance distance and a rounding term. The fast centroid's segment
+	// enters this box no later than any hit a convex candidate can report.
+	b3Pos base = continuousContext->base;
+	float g = continuousContext->fastGrowth;
+	b3AABB box;
+	box.lowerBound.x = (float)( shape->aabb.lowerBound.x - base.x ) - g;
+	box.lowerBound.y = (float)( shape->aabb.lowerBound.y - base.y ) - g;
+	box.lowerBound.z = (float)( shape->aabb.lowerBound.z - base.z ) - g;
+	box.upperBound.x = (float)( shape->aabb.upperBound.x - base.x ) + g;
+	box.upperBound.y = (float)( shape->aabb.upperBound.y - base.y ) + g;
+	box.upperBound.z = (float)( shape->aabb.upperBound.z - base.z ) + g;
+
+	float key = 1.0f, exitFraction = 1.0f;
+	if ( b3RayCastAABB( box, continuousContext->centroid1, continuousContext->centroid2, &key, &exitFraction ) == false )
+	{
+		key = 1.0f;
+	}
+
+	// A key that is not below 1, including NaN, is stored as 1 so the sort never compares unordered
+	if ( !( key < 1.0f ) )
+	{
+		key = 1.0f;
 	}
 
+	bool convexSolid = isSensor == false && shape->type != b3_meshShape && shape->type != b3_heightShape &&
+					   shape->type != b3_compoundShape;
+
+	b3ContinuousCandidate candidate = { key, shapeId, convexSolid };
+#ifdef B3_TOI_COUNT
+	b3_toiCandidates += 1;
+#endif
+	b3Array_Push( *continuousContext->candidates, candidate );
+	return true;
+}
+
+// Evaluate one continuous collision candidate against the fast shape
+static void b3EvaluateContinuousCandidate( b3ContinuousContext* continuousContext, int shapeId )
+{
+	b3Shape* fastShape = continuousContext->fastShape;
+
+	B3_ASSERT( fastShape->sensorIndex == B3_NULL_INDEX );
+
+	// The argument-only filters were applied when the candidate was collected
+	b3World* world = continuousContext->world;
+	b3Shape* shape = b3Array_Get( world->shapes, shapeId );
+	bool isSensor = shape->sensorIndex != B3_NULL_INDEX;
+	b3Body* body = b3Array_Get( world->bodies, shape->bodyId );
+	b3BodySim* bodySim = b3GetBodySim( world, body );
+
 	// Custom user filtering
 	if ( ( shape->flags & b3_enableCustomFiltering ) != 0 || ( fastShape->flags & b3_enableCustomFiltering ) != 0 )
 	{
@@ -412,7 +470,7 @@ static void b3EvaluateContinuousCandidate( b3ContinuousContext* continuousContex
 		{
 			b3ShapeId idA = { shape->id + 1, world->worldId, shape->generation };
 			b3ShapeId idB = { fastShape->id + 1, world->worldId, fastShape->generation };
-			canCollide = customFilterFcn( idA, idB, world->customFilterContext );
+			bool canCollide = customFilterFcn( idA, idB, world->customFilterContext );
 			if ( canCollide == false )
 			{
 				return;
@@ -424,6 +482,9 @@ static void b3EvaluateContinuousCandidate( b3ContinuousContext* continuousContex
 	b3Sweep sweepA = b3MakeRelativeSweep( bodySim, continuousContext->base );
 
 	// Time of impact versus shape. Supports all shape types
+#ifdef B3_TOI_COUNT
+	b3_toiCalls += 1;
+#endif
 	b3TOIOutput output = b3ShapeTimeOfImpact( shape, fastShape, &sweepA, &continuousContext->sweep, continuousContext->fraction );
 	if ( isSensor )
 	{
@@ -469,18 +530,20 @@ static void b3EvaluateContinuousCandidate( b3ContinuousContext* continuousContex
 // cleared for the next query.
 static void b3EvaluateContinuousCandidates( b3ContinuousContext* continuousContext )
 {
-	b3Array( int )* candidates = continuousContext->candidates;
-	int* ids = candidates->data;
+	b3Array( b3ContinuousCandidate )* candidates = continuousContext->candidates;
+	b3ContinuousCandidate* cs = candidates->data;
 	int count = candidates->count;
 
 	{
-#define LESS( i, j ) ( ids[(int)( i )] < ids[(int)( j )] )
+#define LESS( i, j )                                                                                                             \
+	( cs[(int)( i )].key < cs[(int)( j )].key ||                                                                                 \
+	  ( cs[(int)( i )].key == cs[(int)( j )].key && cs[(int)( i )].shapeId < cs[(int)( j )].shapeId ) )
 #define SWAP( i, j )                                                                                                             \
 	do                                                                                                                           \
 	{                                                                                                                            \
-		int tmp_ = ids[(int)( i )];                                                                                              \
-		ids[(int)( i )] = ids[(int)( j )];                                                                                       \
-		ids[(int)( j )] = tmp_;                                                                                                  \
+		b3ContinuousCandidate tmp_ = cs[(int)( i )];                                                                             \
+		cs[(int)( i )] = cs[(int)( j )];                                                                                         \
+		cs[(int)( j )] = tmp_;                                                                                                   \
 	}                                                                                                                            \
 	while ( 0 )
 
@@ -492,7 +555,14 @@ static void b3EvaluateContinuousCandidates( b3ContinuousContext* continuousConte
 
 	for ( int i = 0; i < count; ++i )
 	{
-		b3EvaluateContinuousCandidate( continuousContext, ids[i] );
+#if B3_SWEEP_CULL
+		if ( continuousContext->cullByKey && cs[i].convexSolid && cs[i].key >= continuousContext->fraction )
+		{
+			// A convex solid whose key is at or beyond the running fraction cannot be accepted
+			continue;
+		}
+#endif
+		b3EvaluateContinuousCandidate( continuousContext, cs[i].shapeId );
 	}
 
 	b3Array_Clear( *candidates );
@@ -608,11 +678,32 @@ static void b3SolveContinuous( b3World* world, int bodySimIndex, b3TaskContext*
 		// Each tree query is sorted and evaluated on its own, so the candidate order within a
 		// query is fixed by shape id and the tree order is static, kinematic, dynamic.
 		b3AABB sweptBox = b3AABB_Union( box1, box2 );
+
+		// Bounding radius about the centroid: the distance to the farthest corner of the start box.
+		// Rigid motion keeps every shape point within it for the whole sweep. The growth adds two
+		// linear slops for the acceptance distance and eight float epsilons of the largest coordinate
+		// the key touches for rounding.
+		{
+			b3AABB box1Rel;
+			box1Rel.lowerBound.x = (float)( box1.lowerBound.x - base.x );
+			box1Rel.lowerBound.y = (float)( box1.lowerBound.y - base.y );
+			box1Rel.lowerBound.z = (float)( box1.lowerBound.z - base.z );
+			box1Rel.upperBound.x = (float)( box1.upperBound.x - base.x );
+			box1Rel.upperBound.y = (float)( box1.upperBound.y - base.y );
+			box1Rel.upperBound.z = (float)( box1.upperBound.z - base.z );
+			b3Vec3 far = b3FarthestPointOnAABB( box1Rel, context.centroid1 );
+			float r = b3Length( b3Sub( far, context.centroid1 ) );
+			float m = r + b3MaxFloat( b3Length( context.centroid1 ), b3Length( context.centroid2 ) );
+			context.fastGrowth = r + 2.0f * B3_LINEAR_SLOP + 8.0f * FLT_EPSILON * m;
+		}
+
+		context.cullByKey = true;
 		b3DynamicTree_Query( staticTree, sweptBox, B3_DEFAULT_MASK_BITS, false, b3ContinuousQueryCallback, &context );
 		b3EvaluateContinuousCandidates( &context );
 
 		if ( isBullet )
 		{
+			context.cullByKey = false;
 			b3DynamicTree_Query( kinematicTree, sweptBox, B3_DEFAULT_MASK_BITS, false, b3ContinuousQueryCallback, &context );
 			b3EvaluateContinuousCandidates( &context );
 			b3DynamicTree_Query( dynamicTree, sweptBox, B3_DEFAULT_MASK_BITS, false, b3ContinuousQueryCallback, &context );
```
