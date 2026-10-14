// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

// Tree layout tests. The broad-phase trees hand candidates to the step in a layout dependent
// order. These tests run a scene twice, perturb the layout of the second run, and check the
// step gives the same answer. A visit-order witness shows the perturbation changed the order
// the query under test sees, so a passing comparison is not vacuous.

#include "aabb.h"
#include "body.h"
#include "broad_phase.h"
#include "physics_world.h"
#include "shape.h"
#include "solver_set.h"
#include "test_macros.h"

#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/math_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct RandomState
{
	uint32_t seed;
} RandomState;

static uint32_t NextRandom( RandomState* state )
{
	// xorshift32
	uint32_t x = state->seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	state->seed = x;
	return x;
}

static float RandomFloat( RandomState* state, float lower, float upper )
{
	float u = (float)( NextRandom( state ) & 0xFFFFFF ) / (float)0x1000000;
	return lower + ( upper - lower ) * u;
}

static int RandomInt( RandomState* state, int count )
{
	return (int)( NextRandom( state ) % (uint32_t)count );
}

// Visit-order witness: the shape ids a tree query hands back, in callback order.

#define VISIT_CAPACITY 512

typedef struct VisitOrder
{
	int ids[VISIT_CAPACITY];
	int count;
} VisitOrder;

static bool VisitOrderCallback( int proxyId, uint64_t userData, void* context )
{
	MAYBE_UNUSED( proxyId );
	VisitOrder* order = context;
	if ( order->count < VISIT_CAPACITY )
	{
		order->ids[order->count] = (int)userData;
	}
	order->count += 1;
	return true;
}

// One query on the given tree of the world with the given mask. The world lock is not checked,
// so this must be called between world calls and never from a callback.
static void RecordVisitOrder( b3WorldId worldId, b3BodyType treeType, b3AABB box, uint64_t maskBits, VisitOrder* out )
{
	b3World* world = b3GetWorldFromId( worldId );
	out->count = 0;
	b3DynamicTree_Query( world->broadPhase.trees + treeType, box, maskBits, false, VisitOrderCallback, out );
}

static bool SameSequence( const VisitOrder* a, const VisitOrder* b )
{
	if ( a->count != b->count )
	{
		return false;
	}

	for ( int i = 0; i < a->count; ++i )
	{
		if ( a->ids[i] != b->ids[i] )
		{
			return false;
		}
	}

	return true;
}

static void SortVisitOrder( VisitOrder* order )
{
	// Insertion sort, the witnesses are small
	for ( int i = 1; i < order->count; ++i )
	{
		int id = order->ids[i];
		int j = i;
		while ( j > 0 && order->ids[j - 1] > id )
		{
			order->ids[j] = order->ids[j - 1];
			j -= 1;
		}
		order->ids[j] = id;
	}
}

static bool SameSet( const VisitOrder* a, const VisitOrder* b )
{
	VisitOrder sortedA = *a;
	VisitOrder sortedB = *b;
	SortVisitOrder( &sortedA );
	SortVisitOrder( &sortedB );
	return SameSequence( &sortedA, &sortedB );
}

static bool ContainsId( const VisitOrder* order, int id )
{
	for ( int i = 0; i < order->count; ++i )
	{
		if ( order->ids[i] == id )
		{
			return true;
		}
	}

	return false;
}

static void RemoveId( VisitOrder* order, int id )
{
	int count = 0;
	for ( int i = 0; i < order->count; ++i )
	{
		if ( order->ids[i] != id )
		{
			order->ids[count] = order->ids[i];
			count += 1;
		}
	}
	order->count = count;
}

// Keep only the ids in the filter, in the order they appear
static VisitOrder RestrictToSet( const VisitOrder* order, const VisitOrder* filter )
{
	VisitOrder result = { 0 };
	for ( int i = 0; i < order->count; ++i )
	{
		if ( ContainsId( filter, order->ids[i] ) )
		{
			result.ids[result.count] = order->ids[i];
			result.count += 1;
		}
	}
	return result;
}

static void PrintVisitOrder( const char* label, const VisitOrder* order )
{
	printf( "  %s:", label );
	for ( int i = 0; i < order->count; ++i )
	{
		printf( " %d", order->ids[i] );
	}
	printf( "\n" );
}

// A witness passes when the two runs saw the same candidates in a different order
static bool WitnessDiffers( const char* label, const VisitOrder* run1, const VisitOrder* run2 )
{
	bool sameSet = SameSet( run1, run2 );
	bool sameSequence = SameSequence( run1, run2 );
	if ( sameSet == false || sameSequence )
	{
		printf( "  witness %s: same set %d, same sequence %d\n", label, sameSet, sameSequence );
		PrintVisitOrder( "run 1", run1 );
		PrintVisitOrder( "run 2", run2 );
	}
	return sameSet && sameSequence == false;
}

// Full rebuild of every world tree, directly so no recording command is written. Only the layout
// changes: proxy ids, bounds and moved marks are kept. Called between world calls only.
static void PerturbTreeLayout( b3WorldId worldId )
{
	b3World* world = b3GetWorldFromId( worldId );
	for ( int i = 0; i < B3_PROXY_TREE_COUNT; ++i )
	{
		b3DynamicTree_Rebuild( world->broadPhase.trees + i, true );
	}
}

static uint32_t HashWorldState( b3WorldId worldId, const b3BodyId* bodies, int count )
{
	uint32_t hash = B3_HASH_INIT;
	for ( int i = 0; i < count; ++i )
	{
		b3WorldTransform xf = b3Body_GetTransform( bodies[i] );
		b3Vec3 v = b3Body_GetLinearVelocity( bodies[i] );
		b3Vec3 w = b3Body_GetAngularVelocity( bodies[i] );
		uint8_t awake = b3Body_IsAwake( bodies[i] ) ? 1 : 0;
		hash = b3Hash( hash, (uint8_t*)&xf, sizeof( xf ) );
		hash = b3Hash( hash, (uint8_t*)&v, sizeof( v ) );
		hash = b3Hash( hash, (uint8_t*)&w, sizeof( w ) );
		hash = b3Hash( hash, &awake, sizeof( awake ) );
	}

	int awakeCount = b3World_GetAwakeBodyCount( worldId );
	hash = b3Hash( hash, (uint8_t*)&awakeCount, sizeof( awakeCount ) );
	return hash;
}

static bool IsBodyFast( b3BodyId bodyId )
{
	b3World* world = b3GetWorld( bodyId.world0 );
	b3Body* body = b3GetBodyFullId( world, bodyId );
	b3BodySim* bodySim = b3GetBodySim( world, body );
	return ( bodySim->flags & b3_isFast ) != 0;
}

// The sweep key. The solver evaluates the candidates of one tree query in ascending key, then
// ascending shape id. The key is the fraction at which the fast shape's centroid segment enters
// the candidate's box grown by the fast shape's bounding radius, and 1 when the segment misses.

typedef struct SweepData
{
	// Sweep base, the fast body's centre at the start of the step
	b3Pos base;

	// The fast shape's centroid at the start and end of the step, relative to the base
	b3Vec3 centroid1, centroid2;

	// Distance from the start centroid to the farthest corner of the fast shape's start box
	float radius;
} SweepData;

static b3AABB RelativeBox( b3AABB box, b3Pos base )
{
	b3AABB result;
	result.lowerBound.x = (float)( box.lowerBound.x - base.x );
	result.lowerBound.y = (float)( box.lowerBound.y - base.y );
	result.lowerBound.z = (float)( box.lowerBound.z - base.z );
	result.upperBound.x = (float)( box.upperBound.x - base.x );
	result.upperBound.y = (float)( box.upperBound.y - base.y );
	result.upperBound.z = (float)( box.upperBound.z - base.z );
	return result;
}

// The one-step sweep of a fast body's shape, taken before the step as the solver takes it, with
// the given travel of the centroid over the step
static SweepData MakeSweepData( b3WorldId worldId, b3BodyId bodyId, b3ShapeId shapeId, b3Vec3 travel )
{
	b3World* world = b3GetWorldFromId( worldId );
	b3Body* body = b3GetBodyFullId( world, bodyId );
	b3BodySim* bodySim = b3GetBodySim( world, body );
	b3Shape* shape = b3Array_Get( world->shapes, shapeId.index1 - 1 );

	SweepData data;
	data.base = bodySim->center;
	b3Sweep sweep = b3MakeRelativeSweep( bodySim, data.base );
	b3Transform xf1 = { .p = b3Sub( sweep.c1, b3RotateVector( sweep.q1, sweep.localCenter ) ), .q = sweep.q1 };
	data.centroid1 = b3TransformPoint( xf1, shape->localCentroid );
	data.centroid2 = b3Add( data.centroid1, travel );
	b3Vec3 farthestCorner = b3FarthestPointOnAABB( RelativeBox( shape->aabb, data.base ), data.centroid1 );
	data.radius = b3Distance( farthestCorner, data.centroid1 );
	return data;
}

static float SweepKey( const SweepData* sweep, b3AABB candidateBox )
{
	b3AABB box = b3AABB_Inflate( RelativeBox( candidateBox, sweep->base ), sweep->radius );
	float key = 1.0f;
	float exitFraction = 1.0f;
	if ( b3RayCastAABB( box, sweep->centroid1, sweep->centroid2, &key, &exitFraction ) == false || !( key < 1.0f ) )
	{
		key = 1.0f;
	}
	return key;
}

// The given shapes in ascending sweep key, then ascending id, keyed from the box each shape holds now
static VisitOrder SortBySweepKey( b3WorldId worldId, const SweepData* sweep, const VisitOrder* shapes )
{
	b3World* world = b3GetWorldFromId( worldId );
	VisitOrder order = *shapes;
	float keys[VISIT_CAPACITY];
	for ( int i = 0; i < order.count; ++i )
	{
		keys[i] = SweepKey( sweep, b3Array_Get( world->shapes, order.ids[i] )->aabb );
	}

	// Insertion sort, the sets are small
	for ( int i = 1; i < order.count; ++i )
	{
		float key = keys[i];
		int id = order.ids[i];
		int j = i;
		while ( j > 0 && ( keys[j - 1] > key || ( keys[j - 1] == key && order.ids[j - 1] > id ) ) )
		{
			keys[j] = keys[j - 1];
			order.ids[j] = order.ids[j - 1];
			j -= 1;
		}
		keys[j] = key;
		order.ids[j] = id;
	}

	return order;
}

// Scene pair runner. The scene is built twice in two fresh worlds with identical definitions.
// Run 1 steps plainly. Run 2 perturbs the tree layout immediately before every step when asked.
// The hooks run in both runs with the world, the run index and the step index: one before each
// step, after the perturbation, and one after it, before the next perturbation. Hooks return
// nonzero to fail the test. After every step the scene's hashes are recorded for both runs.

#define MAX_SCENE_STEPS 512
#define MAX_SCENE_HASHES 4

typedef struct Scene
{
	void ( *create )( b3WorldId worldId, int runIndex, void* context );
	int ( *beforeStep )( b3WorldId worldId, int runIndex, int stepIndex, void* context );
	int ( *afterStep )( b3WorldId worldId, int runIndex, int stepIndex, void* context );
	void ( *hash )( b3WorldId worldId, int runIndex, int stepIndex, uint32_t* hashes, void* context );
	void* context;
	b3WorldDef worldDef;
	int stepCount;
	float timeStep;
	int subStepCount;
	int hashCount;
} Scene;

typedef struct PairResult
{
	uint32_t hashes[2][MAX_SCENE_STEPS][MAX_SCENE_HASHES];
} PairResult;

static int RunScenePair( const Scene* scene, bool perturbEachStep, PairResult* out )
{
	ENSURE( scene->stepCount <= MAX_SCENE_STEPS );
	ENSURE( scene->hashCount <= MAX_SCENE_HASHES );

	for ( int runIndex = 0; runIndex < 2; ++runIndex )
	{
		b3WorldId worldId = b3CreateWorld( &scene->worldDef );
		scene->create( worldId, runIndex, scene->context );

		for ( int stepIndex = 0; stepIndex < scene->stepCount; ++stepIndex )
		{
			if ( runIndex == 1 && perturbEachStep )
			{
				PerturbTreeLayout( worldId );
			}

			if ( scene->beforeStep != NULL )
			{
				ENSURE( scene->beforeStep( worldId, runIndex, stepIndex, scene->context ) == 0 );
			}

			b3World_Step( worldId, scene->timeStep, scene->subStepCount );

			if ( scene->afterStep != NULL )
			{
				ENSURE( scene->afterStep( worldId, runIndex, stepIndex, scene->context ) == 0 );
			}

			if ( scene->hash != NULL )
			{
				scene->hash( worldId, runIndex, stepIndex, out->hashes[runIndex][stepIndex], scene->context );
			}
		}

		b3DestroyWorld( worldId );
	}

	return 0;
}

// Witness corridor. A fast sphere sweeps along +x through a cluster of small solid boxes placed
// strictly inside its one-step swept box, with nothing else inside that box or its fat margin.
// The cluster is created in two parts: the first with the scene, settled by the first step, the
// second with the sphere in the after-step hook of the step before the corridor step. The
// cluster's tree then holds a settled subtree and fresh insertions at the perturbation, so the
// two runs reach the corridor step in different layouts. The candidate set of the sphere's
// query is the cluster by construction.

#define CORRIDOR_BOX_COUNT 16
#define CORRIDOR_FIRST_PART_COUNT 8
#define CORRIDOR_STEP 1

typedef struct CorridorPlan
{
	b3BodyType clusterTree;
	b3Pos origin;
	float radius;
	float speed;
	float timeStep;
	float boxHalfExtent;
	b3Vec3 boxOffsets[CORRIDOR_BOX_COUNT];
	bool firstPart[CORRIDOR_BOX_COUNT];
	bool enableCallbacks;
} CorridorPlan;

typedef struct CorridorRun
{
	b3BodyId sphereId;
	b3ShapeId sphereShapeId;
	b3BodyId boxIds[CORRIDOR_BOX_COUNT];
	b3ShapeId boxShapeIds[CORRIDOR_BOX_COUNT];
	VisitOrder onPath;
	VisitOrder witness;
	uint32_t hash;

	// The sphere's sweep, taken before the corridor step, and the cluster in the solver's
	// evaluation order, taken after it
	SweepData sweep;
	VisitOrder sweepOrder;
} CorridorRun;

typedef struct Corridor
{
	CorridorPlan plan;
	CorridorRun runs[2];
} Corridor;

// The sphere's one-step swept box, what the continuous query covers
static b3AABB CorridorSweptBox( const CorridorPlan* plan )
{
	float r = plan->radius;
	float d = plan->speed * plan->timeStep;
	b3AABB local = { { -r, -r, -r }, { d + r, r, r } };
	return b3OffsetAABB( local, plan->origin );
}

// A box covering the corridor for the witness query
static b3AABB CorridorCoverBox( const CorridorPlan* plan )
{
	float r = plan->radius;
	float d = plan->speed * plan->timeStep;
	b3AABB local = { { -4.0f * r, -4.0f * r, -4.0f * r }, { d + 4.0f * r, 4.0f * r, 4.0f * r } };
	return b3OffsetAABB( local, plan->origin );
}

static void PlanCorridor( Corridor* corridor, RandomState* random, b3BodyType clusterTree, b3Pos origin, float timeStep,
						  bool enableCallbacks )
{
	memset( corridor, 0, sizeof( Corridor ) );
	CorridorPlan* plan = &corridor->plan;
	plan->clusterTree = clusterTree;
	plan->origin = origin;
	plan->radius = 1.0f;
	plan->speed = 300.0f;
	plan->timeStep = timeStep;
	plan->boxHalfExtent = 0.05f;
	plan->enableCallbacks = enableCallbacks;

	// Every box sits on the sphere's path, scattered through the swept cylinder in all three
	// axes so the insertion layout and a full build group them differently. The boxes keep
	// their fat bounds apart so resting dynamic boxes make no pairs.
	float r = plan->radius;
	float d = plan->speed * plan->timeStep;
	float xStart = r + 0.4f;
	float xEnd = d - r - 0.4f;
	float lateral = 0.8f;
	float separation = 0.35f;
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		b3Vec3 p;
		bool accepted = false;
		while ( accepted == false )
		{
			p = (b3Vec3){ RandomFloat( random, xStart, xEnd ), RandomFloat( random, -lateral, lateral ),
						  RandomFloat( random, -lateral, lateral ) };
			accepted = p.y * p.y + p.z * p.z < lateral * lateral;
			for ( int j = 0; j < i && accepted; ++j )
			{
				accepted = b3Distance( p, plan->boxOffsets[j] ) > separation;
			}
		}
		plan->boxOffsets[i] = p;
	}

	// The first part is a random subset so the settled subtree and the insertions interleave
	int order[CORRIDOR_BOX_COUNT];
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		order[i] = i;
	}
	for ( int i = CORRIDOR_BOX_COUNT - 1; i > 0; --i )
	{
		int j = RandomInt( random, i + 1 );
		int temp = order[i];
		order[i] = order[j];
		order[j] = temp;
	}
	for ( int i = 0; i < CORRIDOR_FIRST_PART_COUNT; ++i )
	{
		plan->firstPart[order[i]] = true;
	}
}

// Reorder a planned corridor so its boxes are created far-to-near along the sweep: the first part
// is the farthest half, and each part is created in descending x. Ascending shape id is then the
// reverse of the sweep.
static void PlanCorridorFarToNear( Corridor* corridor )
{
	CorridorPlan* plan = &corridor->plan;
	for ( int i = 1; i < CORRIDOR_BOX_COUNT; ++i )
	{
		b3Vec3 p = plan->boxOffsets[i];
		int j = i;
		while ( j > 0 && plan->boxOffsets[j - 1].x < p.x )
		{
			plan->boxOffsets[j] = plan->boxOffsets[j - 1];
			j -= 1;
		}
		plan->boxOffsets[j] = p;
	}

	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		plan->firstPart[i] = i < CORRIDOR_FIRST_PART_COUNT;
	}
}

static void CreateCorridorBox( Corridor* corridor, b3WorldId worldId, int runIndex, int boxIndex )
{
	const CorridorPlan* plan = &corridor->plan;
	CorridorRun* run = corridor->runs + runIndex;

	b3BodyDef bodyDef = b3DefaultBodyDef();
	bodyDef.type = plan->clusterTree;
	bodyDef.position = b3OffsetPos( plan->origin, plan->boxOffsets[boxIndex] );
	run->boxIds[boxIndex] = b3CreateBody( worldId, &bodyDef );

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	shapeDef.enablePreSolveEvents = plan->enableCallbacks;
	shapeDef.enableCustomFiltering = plan->enableCallbacks;
	b3BoxHull box = b3MakeBoxHull( plan->boxHalfExtent, plan->boxHalfExtent, plan->boxHalfExtent );
	run->boxShapeIds[boxIndex] = b3CreateHullShape( run->boxIds[boxIndex], &shapeDef, &box.base );
}

// First part of the cluster, created with the scene
static void CreateCorridorPart1( Corridor* corridor, b3WorldId worldId, int runIndex )
{
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		if ( corridor->plan.firstPart[i] )
		{
			CreateCorridorBox( corridor, worldId, runIndex, i );
		}
	}
}

// Second part of the cluster and the fast sphere, created after the step before the corridor step
static void CreateCorridorPart2( Corridor* corridor, b3WorldId worldId, int runIndex )
{
	const CorridorPlan* plan = &corridor->plan;
	CorridorRun* run = corridor->runs + runIndex;

	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		if ( corridor->plan.firstPart[i] == false )
		{
			CreateCorridorBox( corridor, worldId, runIndex, i );
		}
	}

	// A plain fast body only queries the static tree, a bullet queries all three
	b3BodyDef bodyDef = b3DefaultBodyDef();
	bodyDef.type = b3_dynamicBody;
	bodyDef.position = plan->origin;
	bodyDef.linearVelocity = (b3Vec3){ plan->speed, 0.0f, 0.0f };
	bodyDef.isBullet = plan->clusterTree != b3_staticBody;
	run->sphereId = b3CreateBody( worldId, &bodyDef );

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	shapeDef.enablePreSolveEvents = plan->enableCallbacks;
	shapeDef.enableCustomFiltering = plan->enableCallbacks;
	b3Sphere sphere = { b3Vec3_zero, plan->radius };
	run->sphereShapeId = b3CreateSphereShape( run->sphereId, &shapeDef, &sphere );
}

static b3AABB GetLeafBound( b3World* world, const b3Shape* shape )
{
	b3BodyType treeType = B3_PROXY_TYPE( shape->proxyKey );
	int proxyId = B3_PROXY_ID( shape->proxyKey );
	return b3DynamicTree_GetAABB( world->broadPhase.trees + treeType, proxyId );
}

static VisitOrder CorridorClusterSet( const Corridor* corridor, int runIndex )
{
	VisitOrder set = { 0 };
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		set.ids[set.count] = corridor->runs[runIndex].boxShapeIds[i].index1 - 1;
		set.count += 1;
	}
	return set;
}

// Before the corridor step: the sphere's leaf bound overlaps no other leaf bound, the on-path
// boxes are found from a time of impact call over the full one-step sweep, and the static tree
// witness is recorded. The static tree is not rebuilt by the step, so its layout at the query is
// the layout now.
static int CorridorBeforeStep( Corridor* corridor, b3WorldId worldId, int runIndex )
{
	const CorridorPlan* plan = &corridor->plan;
	CorridorRun* run = corridor->runs + runIndex;
	b3World* world = b3GetWorldFromId( worldId );

	b3Shape* sphereShape = b3Array_Get( world->shapes, run->sphereShapeId.index1 - 1 );
	b3AABB sphereLeaf = GetLeafBound( world, sphereShape );
	for ( int i = 0; i < world->shapes.count; ++i )
	{
		const b3Shape* shape = world->shapes.data + i;
		if ( shape->id == B3_NULL_INDEX || shape->proxyKey == B3_NULL_INDEX || shape->id == sphereShape->id )
		{
			continue;
		}

		ENSURE( b3AABB_Overlaps( sphereLeaf, GetLeafBound( world, shape ) ) == false );
	}

	// Full one-step sweep of the sphere, relative to its start like the solver does it
	b3Body* sphereBody = b3GetBodyFullId( world, run->sphereId );
	b3BodySim* sphereSim = b3GetBodySim( world, sphereBody );
	b3Pos base = sphereSim->center;
	b3Sweep sweepB = b3MakeRelativeSweep( sphereSim, base );
	sweepB.c2 = b3Add( sweepB.c1, b3MulSV( plan->speed * plan->timeStep, (b3Vec3){ 1.0f, 0.0f, 0.0f } ) );

	run->onPath.count = 0;
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		b3Shape* boxShape = b3Array_Get( world->shapes, run->boxShapeIds[i].index1 - 1 );
		b3Body* boxBody = b3GetBodyFullId( world, run->boxIds[i] );
		b3BodySim* boxSim = b3GetBodySim( world, boxBody );
		b3Sweep sweepA = b3MakeRelativeSweep( boxSim, base );
		b3TOIOutput output = b3ShapeTimeOfImpact( boxShape, sphereShape, &sweepA, &sweepB, 1.0f );
		if ( 0.0f < output.fraction && output.fraction < 1.0f )
		{
			run->onPath.ids[run->onPath.count] = boxShape->id;
			run->onPath.count += 1;
		}
	}

	ENSURE( run->onPath.count >= 2 );

	run->sweep = MakeSweepData( worldId, run->sphereId, run->sphereShapeId, b3Sub( sweepB.c2, sweepB.c1 ) );

	if ( plan->clusterTree == b3_staticBody )
	{
		RecordVisitOrder( worldId, b3_staticBody, CorridorCoverBox( plan ), B3_DEFAULT_MASK_BITS, &run->witness );
	}

	return 0;
}

// After the corridor step: the kinematic or dynamic tree witness is recorded with the sphere's
// own shape removed, since the step rebuilt that tree before the continuous query and only
// enlarged bounds after it. The sphere must have been flagged fast, or the query never ran.
// The cluster's sweep order is keyed from the boxes the cluster holds now: a static box is fixed
// through the step, and a resting kinematic or dynamic box holds the end-of-step box the step
// wrote before the continuous stage queried it.
static int CorridorAfterStep( Corridor* corridor, b3WorldId worldId, int runIndex )
{
	const CorridorPlan* plan = &corridor->plan;
	CorridorRun* run = corridor->runs + runIndex;

	if ( plan->clusterTree != b3_staticBody )
	{
		RecordVisitOrder( worldId, plan->clusterTree, CorridorCoverBox( plan ), B3_DEFAULT_MASK_BITS, &run->witness );
		RemoveId( &run->witness, run->sphereShapeId.index1 - 1 );
	}

	ENSURE( IsBodyFast( run->sphereId ) );

	VisitOrder cluster = CorridorClusterSet( corridor, runIndex );
	run->sweepOrder = SortBySweepKey( worldId, &run->sweep, &cluster );

	b3BodyId bodies[1 + CORRIDOR_BOX_COUNT];
	bodies[0] = run->sphereId;
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		bodies[1 + i] = run->boxIds[i];
	}
	run->hash = HashWorldState( worldId, bodies, 1 + CORRIDOR_BOX_COUNT );

	return 0;
}

static const char* TreeName( b3BodyType treeType )
{
	return treeType == b3_staticBody ? "static" : ( treeType == b3_kinematicBody ? "kinematic" : "dynamic" );
}

// The witness holds exactly the cluster, same set in both runs, different sequence
static int CheckCorridorWitness( const Corridor* corridor )
{
	VisitOrder cluster = CorridorClusterSet( corridor, 0 );
	ENSURE( SameSet( &corridor->runs[0].witness, &cluster ) );
	ENSURE( SameSet( &corridor->runs[0].onPath, &corridor->runs[1].onPath ) );
	ENSURE( WitnessDiffers( TreeName( corridor->plan.clusterTree ), &corridor->runs[0].witness, &corridor->runs[1].witness ) );
	return 0;
}

// Sanity test case 1: one corridor of each kind, nothing else. The witnesses must differ and the
// spheres must be fast.

typedef struct CorridorScene
{
	Corridor corridors[3];
} CorridorScene;

static void CorridorSceneCreate( b3WorldId worldId, int runIndex, void* context )
{
	CorridorScene* scene = context;
	for ( int i = 0; i < 3; ++i )
	{
		CreateCorridorPart1( scene->corridors + i, worldId, runIndex );
	}
}

static int CorridorSceneBeforeStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	CorridorScene* scene = context;
	if ( stepIndex == CORRIDOR_STEP )
	{
		for ( int i = 0; i < 3; ++i )
		{
			ENSURE( CorridorBeforeStep( scene->corridors + i, worldId, runIndex ) == 0 );
		}
	}
	return 0;
}

static int CorridorSceneAfterStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	CorridorScene* scene = context;
	if ( stepIndex == CORRIDOR_STEP - 1 )
	{
		for ( int i = 0; i < 3; ++i )
		{
			CreateCorridorPart2( scene->corridors + i, worldId, runIndex );
		}
	}
	else if ( stepIndex == CORRIDOR_STEP )
	{
		for ( int i = 0; i < 3; ++i )
		{
			ENSURE( CorridorAfterStep( scene->corridors + i, worldId, runIndex ) == 0 );
		}
	}
	return 0;
}

static void PlanThreeCorridors( Corridor* corridors, RandomState* random, float timeStep, bool enableCallbacks )
{
	b3BodyType trees[3] = { b3_staticBody, b3_kinematicBody, b3_dynamicBody };
	for ( int i = 0; i < 3; ++i )
	{
		b3Pos origin = { 200.0 * i, 0.0, 150.0 };
		PlanCorridor( corridors + i, random, trees[i], origin, timeStep, enableCallbacks );
	}
}

static int CorridorWitnessCase( void )
{
	CorridorScene corridorScene = { 0 };
	RandomState random = { 0x51A7E11u };
	float timeStep = 1.0f / 60.0f;
	PlanThreeCorridors( corridorScene.corridors, &random, timeStep, false );

	Scene scene = { 0 };
	scene.create = CorridorSceneCreate;
	scene.beforeStep = CorridorSceneBeforeStep;
	scene.afterStep = CorridorSceneAfterStep;
	scene.context = &corridorScene;
	scene.worldDef = b3DefaultWorldDef();
	scene.worldDef.gravity = b3Vec3_zero;
	scene.worldDef.enableSleep = false;
	scene.stepCount = 2;
	scene.timeStep = timeStep;
	scene.subStepCount = 4;

	PairResult result;
	ENSURE( RunScenePair( &scene, true, &result ) == 0 );

	for ( int i = 0; i < 3; ++i )
	{
		ENSURE( CheckCorridorWitness( corridorScene.corridors + i ) == 0 );
	}

	return 0;
}

// Sanity test case 2: the explosion query. Single-shape dynamic bodies inside the explosion box,
// nothing else in the dynamic tree. Run 1 is plain, run 2 is perturbed.

#define EXPLODE_SINGLE_COUNT 10

#define EXPLODE_MULTI_SHAPE_COUNT 3
#define EXPLODE_BODY_COUNT ( EXPLODE_SINGLE_COUNT + 2 )

typedef struct ExplodeScene
{
	b3ExplosionDef def;
	b3Vec3 singleOffsets[EXPLODE_SINGLE_COUNT];
	float singleRadius;

	// A body with several shapes inside the radius, placed so their contributions are not symmetric
	b3Vec3 multiOffset;
	b3Vec3 multiShapeOffsets[EXPLODE_MULTI_SHAPE_COUNT];

	// A body whose closest surface lies beyond the radius and within the falloff
	b3Vec3 farOffset;
} ExplodeScene;

static void PlanExplodeScene( ExplodeScene* scene, RandomState* random )
{
	memset( scene, 0, sizeof( ExplodeScene ) );
	scene->def = b3DefaultExplosionDef();
	scene->def.position = (b3Pos){ 10.0, -5.0, 20.0 };
	scene->def.radius = 3.0f;
	scene->def.falloff = 1.0f;
	scene->def.impulsePerArea = 10.0f;
	scene->singleRadius = 0.2f;

	// Inside the radius, spread so no two touch
	float inner = 0.7f * scene->def.radius;
	for ( int i = 0; i < EXPLODE_SINGLE_COUNT; ++i )
	{
		scene->singleOffsets[i] = (b3Vec3){ RandomFloat( random, -inner, inner ), RandomFloat( random, -inner, inner ),
											RandomFloat( random, -inner, inner ) };
	}

	// Spread wide so the tree can group the three shapes with different neighbours
	scene->multiOffset = (b3Vec3){ 0.4f, 0.3f, -0.2f };
	scene->multiShapeOffsets[0] = (b3Vec3){ 1.5f, 0.2f, -0.3f };
	scene->multiShapeOffsets[1] = (b3Vec3){ -1.1f, 1.2f, 0.4f };
	scene->multiShapeOffsets[2] = (b3Vec3){ 0.1f, -1.3f, -1.4f };

	// Centre at radius + 0.5, surface at radius + 0.3 with the single radius
	scene->farOffset = (b3Vec3){ 0.6f * ( scene->def.radius + 0.5f ), 0.8f * ( scene->def.radius + 0.5f ), 0.0f };
}

static b3AABB ExplosionBox( const b3ExplosionDef* def )
{
	float extent = def->radius + def->falloff;
	b3AABB local = { { -extent, -extent, -extent }, { extent, extent, extent } };
	return b3OffsetAABB( local, def->position );
}

static b3BodyId CreateExplodeSingle( const ExplodeScene* scene, b3WorldId worldId, int index, bool isAwake )
{
	b3BodyDef bodyDef = b3DefaultBodyDef();
	bodyDef.type = b3_dynamicBody;
	bodyDef.position = b3OffsetPos( scene->def.position, scene->singleOffsets[index] );
	bodyDef.isAwake = isAwake;
	b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3Sphere sphere = { b3Vec3_zero, scene->singleRadius };
	b3CreateSphereShape( bodyId, &shapeDef, &sphere );
	return bodyId;
}

static int ExplodeWitnessCase( void )
{
	ExplodeScene explodeScene;
	RandomState random = { 0xE8D10DEu };
	PlanExplodeScene( &explodeScene, &random );

	VisitOrder witness[2];
	for ( int runIndex = 0; runIndex < 2; ++runIndex )
	{
		b3WorldDef worldDef = b3DefaultWorldDef();
		worldDef.gravity = b3Vec3_zero;
		worldDef.enableSleep = false;
		b3WorldId worldId = b3CreateWorld( &worldDef );

		for ( int i = 0; i < EXPLODE_SINGLE_COUNT; ++i )
		{
			CreateExplodeSingle( &explodeScene, worldId, i, true );
		}

		if ( runIndex == 1 )
		{
			PerturbTreeLayout( worldId );
		}

		RecordVisitOrder( worldId, b3_dynamicBody, ExplosionBox( &explodeScene.def ), explodeScene.def.maskBits,
						  witness + runIndex );
		b3DestroyWorld( worldId );
	}

	ENSURE( witness[0].count == EXPLODE_SINGLE_COUNT );
	ENSURE( WitnessDiffers( "explode", witness + 0, witness + 1 ) );
	return 0;
}

// Sanity test case 3: the negative control. A pile of boxes on static ground with gravity and
// sleep, no fast body, no sensor, no explosion. The perturbed run must hash the same every step.

#define PILE_SIDE 3
#define PILE_LAYERS 3
#define PILE_COUNT ( PILE_SIDE * PILE_SIDE * PILE_LAYERS )
#define PILE_STEPS 240

typedef struct PileScene
{
	b3BodyId bodies[2][PILE_COUNT];
} PileScene;

static void PileSceneCreate( b3WorldId worldId, int runIndex, void* context )
{
	PileScene* scene = context;

	b3BodyDef groundDef = b3DefaultBodyDef();
	groundDef.position = (b3Pos){ 0.0, -1.0, 0.0 };
	b3BodyId groundId = b3CreateBody( worldId, &groundDef );
	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3BoxHull ground = b3MakeBoxHull( 10.0f, 1.0f, 10.0f );
	b3CreateHullShape( groundId, &shapeDef, &ground.base );

	b3BoxHull box = b3MakeBoxHull( 0.5f, 0.5f, 0.5f );
	int index = 0;
	for ( int layer = 0; layer < PILE_LAYERS; ++layer )
	{
		for ( int i = 0; i < PILE_SIDE; ++i )
		{
			for ( int j = 0; j < PILE_SIDE; ++j )
			{
				b3BodyDef bodyDef = b3DefaultBodyDef();
				bodyDef.type = b3_dynamicBody;
				bodyDef.position = (b3Pos){ 1.1 * ( i - 1 ) + 0.05 * layer, 0.6 + 1.2 * layer, 1.1 * ( j - 1 ) - 0.05 * layer };
				b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
				b3CreateHullShape( bodyId, &shapeDef, &box.base );
				scene->bodies[runIndex][index] = bodyId;
				index += 1;
			}
		}
	}
}

static int PileSceneAfterStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	PileScene* scene = context;
	for ( int i = 0; i < PILE_COUNT; ++i )
	{
		ENSURE( IsBodyFast( scene->bodies[runIndex][i] ) == false );
	}

	if ( stepIndex == PILE_STEPS - 1 )
	{
		ENSURE( b3World_GetAwakeBodyCount( worldId ) == 0 );
	}

	return 0;
}

static void PileSceneHash( b3WorldId worldId, int runIndex, int stepIndex, uint32_t* hashes, void* context )
{
	MAYBE_UNUSED( stepIndex );
	PileScene* scene = context;
	hashes[0] = HashWorldState( worldId, scene->bodies[runIndex], PILE_COUNT );
}

static int UnchangedHashCase( void )
{
	static PileScene pileScene;
	memset( &pileScene, 0, sizeof( PileScene ) );

	Scene scene = { 0 };
	scene.create = PileSceneCreate;
	scene.afterStep = PileSceneAfterStep;
	scene.hash = PileSceneHash;
	scene.context = &pileScene;
	scene.worldDef = b3DefaultWorldDef();
	scene.stepCount = PILE_STEPS;
	scene.timeStep = 1.0f / 60.0f;
	scene.subStepCount = 4;
	scene.hashCount = 1;

	static PairResult result;
	ENSURE( RunScenePair( &scene, true, &result ) == 0 );

	for ( int step = 0; step < PILE_STEPS; ++step )
	{
		if ( result.hashes[0][step][0] != result.hashes[1][step][0] )
		{
			printf( "  pile hash differs at step %d: 0x%08X vs 0x%08X\n", step, result.hashes[0][step][0],
					result.hashes[1][step][0] );
		}
		ENSURE( result.hashes[0][step][0] == result.hashes[1][step][0] );
	}

	return 0;
}

// Continuous collision candidates must be evaluated in an order that does not depend on tree
// layout. Three tests: a seeded fuzz over a field of static boxes with corridors on every tree,
// the sensor selection with more candidates than the continuous sensor limit, and the order of
// the pre-solve and custom filter callbacks.

// Field of static boxes with fast bodies launched through it, plus one corridor per tree kind in
// regions disjoint from the field and from each other.

#define FUZZ_STATIC_COUNT 140
#define FUZZ_FAST_COUNT 60
#define FUZZ_BULLET_COUNT 40
#define FUZZ_STEPS 10

typedef struct FuzzScene
{
	Corridor corridors[3];

	b3Pos staticPositions[FUZZ_STATIC_COUNT];
	b3Vec3 staticExtents[FUZZ_STATIC_COUNT];

	b3Pos fastPositions[FUZZ_FAST_COUNT];
	b3Vec3 fastVelocities[FUZZ_FAST_COUNT];
	float fastRadii[FUZZ_FAST_COUNT];

	b3BodyId fastIds[2][FUZZ_FAST_COUNT];

	// Workload check for the four worker runs, read after the first step
	int fastBulletCount[2];
	int awakeBodyCount[2];
} FuzzScene;

static void PlanFuzzScene( FuzzScene* scene, RandomState* random, float timeStep )
{
	memset( scene, 0, sizeof( FuzzScene ) );
	PlanThreeCorridors( scene->corridors, random, timeStep, false );

	// A slab of static boxes around the origin
	float slab = 15.0f;
	for ( int i = 0; i < FUZZ_STATIC_COUNT; ++i )
	{
		scene->staticPositions[i] =
			(b3Pos){ RandomFloat( random, -slab, slab ), RandomFloat( random, -3.0f, 3.0f ), RandomFloat( random, -slab, slab ) };
		scene->staticExtents[i] =
			(b3Vec3){ RandomFloat( random, 0.2f, 0.6f ), RandomFloat( random, 0.2f, 0.6f ), RandomFloat( random, 0.2f, 0.6f ) };
	}

	// Fast bodies start above or below the slab and fly into it at speeds well above the fast
	// threshold for their radius
	for ( int i = 0; i < FUZZ_FAST_COUNT; ++i )
	{
		float side = RandomInt( random, 2 ) == 0 ? 10.0f : -10.0f;
		b3Pos start = { RandomFloat( random, -12.0f, 12.0f ), side, RandomFloat( random, -12.0f, 12.0f ) };
		b3Pos target = { RandomFloat( random, -12.0f, 12.0f ), RandomFloat( random, -2.0f, 2.0f ),
						 RandomFloat( random, -12.0f, 12.0f ) };
		b3Vec3 direction = b3Normalize( b3SubPos( target, start ) );
		scene->fastPositions[i] = start;
		scene->fastVelocities[i] = b3MulSV( RandomFloat( random, 30.0f, 90.0f ), direction );
		scene->fastRadii[i] = RandomFloat( random, 0.2f, 0.4f );
	}
}

static void FuzzSceneCreate( b3WorldId worldId, int runIndex, void* context )
{
	FuzzScene* scene = context;

	for ( int i = 0; i < 3; ++i )
	{
		CreateCorridorPart1( scene->corridors + i, worldId, runIndex );
	}

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	for ( int i = 0; i < FUZZ_STATIC_COUNT; ++i )
	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		bodyDef.position = scene->staticPositions[i];
		b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
		b3Vec3 e = scene->staticExtents[i];
		b3BoxHull box = b3MakeBoxHull( e.x, e.y, e.z );
		b3CreateHullShape( bodyId, &shapeDef, &box.base );
	}

	for ( int i = 0; i < FUZZ_FAST_COUNT; ++i )
	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		bodyDef.type = b3_dynamicBody;
		bodyDef.position = scene->fastPositions[i];
		bodyDef.linearVelocity = scene->fastVelocities[i];
		bodyDef.isBullet = i < FUZZ_BULLET_COUNT;
		b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
		b3Sphere sphere = { b3Vec3_zero, scene->fastRadii[i] };
		b3CreateSphereShape( bodyId, &shapeDef, &sphere );
		scene->fastIds[runIndex][i] = bodyId;
	}
}

static int FuzzSceneBeforeStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	FuzzScene* scene = context;
	if ( stepIndex == CORRIDOR_STEP )
	{
		for ( int i = 0; i < 3; ++i )
		{
			ENSURE( CorridorBeforeStep( scene->corridors + i, worldId, runIndex ) == 0 );
		}
	}
	return 0;
}

static int FuzzSceneAfterStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	FuzzScene* scene = context;
	if ( stepIndex == 0 )
	{
		int fastBulletCount = 0;
		for ( int i = 0; i < FUZZ_BULLET_COUNT; ++i )
		{
			fastBulletCount += IsBodyFast( scene->fastIds[runIndex][i] ) ? 1 : 0;
		}
		scene->fastBulletCount[runIndex] = fastBulletCount;
		scene->awakeBodyCount[runIndex] = b3World_GetAwakeBodyCount( worldId );
	}

	if ( stepIndex == CORRIDOR_STEP - 1 )
	{
		for ( int i = 0; i < 3; ++i )
		{
			CreateCorridorPart2( scene->corridors + i, worldId, runIndex );
		}
	}
	else if ( stepIndex == CORRIDOR_STEP )
	{
		for ( int i = 0; i < 3; ++i )
		{
			ENSURE( CorridorAfterStep( scene->corridors + i, worldId, runIndex ) == 0 );
		}
	}
	return 0;
}

static void FuzzSceneHash( b3WorldId worldId, int runIndex, int stepIndex, uint32_t* hashes, void* context )
{
	MAYBE_UNUSED( stepIndex );
	FuzzScene* scene = context;
	hashes[0] = HashWorldState( worldId, scene->fastIds[runIndex], FUZZ_FAST_COUNT );
}

typedef struct FuzzCounts
{
	int seeds;
	int field;
	int corridor[3];
} FuzzCounts;

// One seed: both runs, every witness checked, divergences counted rather than failed
static int RunFuzzSeed( uint32_t seed, int workerCount, FuzzCounts* counts )
{
	static FuzzScene fuzzScene;
	RandomState random = { seed };
	float timeStep = 1.0f / 60.0f;
	PlanFuzzScene( &fuzzScene, &random, timeStep );

	Scene scene = { 0 };
	scene.create = FuzzSceneCreate;
	scene.beforeStep = FuzzSceneBeforeStep;
	scene.afterStep = FuzzSceneAfterStep;
	scene.hash = FuzzSceneHash;
	scene.context = &fuzzScene;
	scene.worldDef = b3DefaultWorldDef();
	scene.worldDef.gravity = b3Vec3_zero;
	scene.worldDef.enableSleep = false;
	scene.worldDef.enableContinuous = true;
	scene.worldDef.workerCount = workerCount;
	scene.stepCount = FUZZ_STEPS;
	scene.timeStep = timeStep;
	scene.subStepCount = 4;
	scene.hashCount = 1;

	static PairResult result;
	ENSURE( RunScenePair( &scene, true, &result ) == 0 );

	if ( workerCount > 1 )
	{
		// Finalize and bullet continuous collision each form at least four blocks
		for ( int runIndex = 0; runIndex < 2; ++runIndex )
		{
			ENSURE( fuzzScene.awakeBodyCount[runIndex] >= 49 );
			ENSURE( fuzzScene.fastBulletCount[runIndex] >= 25 );
		}
	}

	counts->seeds += 1;

	bool fieldDiverged = false;
	for ( int step = 0; step < FUZZ_STEPS; ++step )
	{
		if ( result.hashes[0][step][0] != result.hashes[1][step][0] )
		{
			fieldDiverged = true;
		}
	}
	counts->field += fieldDiverged ? 1 : 0;

	for ( int i = 0; i < 3; ++i )
	{
		Corridor* corridor = fuzzScene.corridors + i;
		ENSURE( CheckCorridorWitness( corridor ) == 0 );
		counts->corridor[i] += corridor->runs[0].hash != corridor->runs[1].hash ? 1 : 0;
	}

	return 0;
}

static void PrintFuzzCounts( const char* label, const FuzzCounts* counts )
{
	printf( "  %s: seeds %d, field divergences %d, corridor divergences static %d kinematic %d dynamic %d\n", label,
			counts->seeds, counts->field, counts->corridor[0], counts->corridor[1], counts->corridor[2] );
}

static int CheckFuzzCounts( const FuzzCounts* counts )
{
	ENSURE( counts->field == 0 );
	ENSURE( counts->corridor[0] == 0 );
	ENSURE( counts->corridor[1] == 0 );
	ENSURE( counts->corridor[2] == 0 );
	return 0;
}

int ContinuousCandidateOrderTest( void )
{
	FuzzCounts single = { 0 };
	for ( uint32_t seed = 1; seed <= 40; ++seed )
	{
		ENSURE( RunFuzzSeed( 0x9E3779B9u * seed, 1, &single ) == 0 );
	}

	FuzzCounts parallel = { 0 };
	for ( uint32_t seed = 1; seed <= 8; ++seed )
	{
		ENSURE( RunFuzzSeed( 0x9E3779B9u * seed, 4, &parallel ) == 0 );
	}

	// Print every count before the first assertion
	PrintFuzzCounts( "one worker", &single );
	PrintFuzzCounts( "four workers", &parallel );
	ENSURE( CheckFuzzCounts( &single ) == 0 );
	ENSURE( CheckFuzzCounts( &parallel ) == 0 );
	return 0;
}

// More sensors than the continuous sensor limit, all on one static body along a bullet's path.
// The sensors kept must be the eight nearest by sweep key, tie by id, whatever the layout. The
// sensors are created farthest-first, so the eight lowest ids are not the eight nearest.

#define SENSOR_LIMIT 8
#define SENSOR_COUNT 12

static int CompareInts( const void* a, const void* b )
{
	int ia = *(const int*)a;
	int ib = *(const int*)b;
	return ia < ib ? -1 : ( ia > ib ? 1 : 0 );
}

int ContinuousSensorSelectionTest( void )
{
	RandomState random = { 0x5E4504u };
	float timeStep = 1.0f / 60.0f;
	float sensorRadius = 0.15f;
	float bulletRadius = 0.25f;
	float spacing = 0.35f;
	float lead = 0.8f;
	float distance = 2.0f * lead + spacing * ( SENSOR_COUNT - 1 );
	float speed = distance / timeStep;
	b3Pos origin = { 50.0, 50.0, 50.0 };

	b3Vec3 sensorOffsets[SENSOR_COUNT];
	for ( int i = 0; i < SENSOR_COUNT; ++i )
	{
		sensorOffsets[i] =
			(b3Vec3){ lead + spacing * i, RandomFloat( &random, -0.05f, 0.05f ), RandomFloat( &random, -0.05f, 0.05f ) };
	}

	VisitOrder witness[2];
	VisitOrder reported[2];
	VisitOrder sweepOrder[2];
	int sensorIds[2][SENSOR_COUNT];
	for ( int runIndex = 0; runIndex < 2; ++runIndex )
	{
		b3WorldDef worldDef = b3DefaultWorldDef();
		worldDef.gravity = b3Vec3_zero;
		b3WorldId worldId = b3CreateWorld( &worldDef );

		b3BodyDef sensorBodyDef = b3DefaultBodyDef();
		sensorBodyDef.position = origin;
		b3BodyId sensorBodyId = b3CreateBody( worldId, &sensorBodyDef );
		b3ShapeDef sensorDef = b3DefaultShapeDef();
		sensorDef.isSensor = true;
		sensorDef.enableSensorEvents = true;
		for ( int i = SENSOR_COUNT - 1; i >= 0; --i )
		{
			b3Sphere sphere = { sensorOffsets[i], sensorRadius };
			b3ShapeId shapeId = b3CreateSphereShape( sensorBodyId, &sensorDef, &sphere );
			sensorIds[runIndex][i] = shapeId.index1;
		}

		b3BodyDef bulletDef = b3DefaultBodyDef();
		bulletDef.type = b3_dynamicBody;
		bulletDef.position = origin;
		bulletDef.linearVelocity = (b3Vec3){ speed, 0.0f, 0.0f };
		bulletDef.isBullet = true;
		b3BodyId bulletId = b3CreateBody( worldId, &bulletDef );
		b3ShapeDef bulletShapeDef = b3DefaultShapeDef();
		bulletShapeDef.enableSensorEvents = true;
		b3Sphere bullet = { b3Vec3_zero, bulletRadius };
		b3ShapeId bulletShapeId = b3CreateSphereShape( bulletId, &bulletShapeDef, &bullet );

		if ( runIndex == 1 )
		{
			PerturbTreeLayout( worldId );
		}

		b3AABB cover = { { -1.0f, -1.0f, -1.0f }, { distance + 1.0f, 1.0f, 1.0f } };
		RecordVisitOrder( worldId, b3_staticBody, b3OffsetAABB( cover, origin ), B3_DEFAULT_MASK_BITS, witness + runIndex );

		// The sensors in the solver's evaluation order, from the bullet's one-step sweep
		VisitOrder sensors = { 0 };
		for ( int i = 0; i < SENSOR_COUNT; ++i )
		{
			sensors.ids[i] = sensorIds[runIndex][i] - 1;
		}
		sensors.count = SENSOR_COUNT;
		SweepData sweep = MakeSweepData( worldId, bulletId, bulletShapeId, (b3Vec3){ speed * timeStep, 0.0f, 0.0f } );
		sweepOrder[runIndex] = SortBySweepKey( worldId, &sweep, &sensors );

		b3World_Step( worldId, timeStep, 4 );

		ENSURE( IsBodyFast( bulletId ) );

		b3SensorEvents events = b3World_GetSensorEvents( worldId );
		reported[runIndex].count = 0;
		for ( int i = 0; i < events.beginCount; ++i )
		{
			reported[runIndex].ids[reported[runIndex].count] = events.beginEvents[i].sensorShapeId.index1;
			reported[runIndex].count += 1;
		}

		b3DestroyWorld( worldId );
	}

	ENSURE( witness[0].count == SENSOR_COUNT );
	ENSURE( WitnessDiffers( "sensors", witness + 0, witness + 1 ) );

	for ( int runIndex = 0; runIndex < 2; ++runIndex )
	{
		ENSURE( sensorIds[runIndex][0] == sensorIds[0][0] );
	}

	// The eight sensors first in sweep order, as public shape ids
	ENSURE( SameSequence( sweepOrder + 0, sweepOrder + 1 ) );
	VisitOrder expected = { 0 };
	for ( int i = 0; i < SENSOR_LIMIT; ++i )
	{
		expected.ids[i] = sweepOrder[0].ids[i] + 1;
	}
	expected.count = SENSOR_LIMIT;

	// The eight lowest sensor shape ids are a different set, so the order is not id order
	int lowest[SENSOR_COUNT];
	memcpy( lowest, sensorIds[0], sizeof( lowest ) );
	qsort( lowest, SENSOR_COUNT, sizeof( int ), CompareInts );
	VisitOrder lowestIds = { 0 };
	for ( int i = 0; i < SENSOR_LIMIT; ++i )
	{
		lowestIds.ids[i] = lowest[i];
	}
	lowestIds.count = SENSOR_LIMIT;

	PrintVisitOrder( "sweep order", sweepOrder + 0 );
	PrintVisitOrder( "expected", &expected );
	for ( int runIndex = 0; runIndex < 2; ++runIndex )
	{
		PrintVisitOrder( runIndex == 0 ? "run 1 sensors" : "run 2 sensors", reported + runIndex );
		ENSURE( reported[runIndex].count == SENSOR_LIMIT );
	}

	ENSURE( SameSet( &expected, &lowestIds ) == false );
	ENSURE( SameSet( reported + 0, &expected ) );
	ENSURE( SameSet( reported + 1, &expected ) );
	return 0;
}

// The pre-solve and custom filter callbacks are called in ascending sweep key, then ascending
// shape id, within each query. One corridor per tree kind with both callbacks enabled on the
// sphere and every cluster box. The boxes are created far-to-near, so ascending id and the sweep
// disagree.

typedef struct CallbackLog
{
	int sphereIndex[3];
	VisitOrder filterLog[3];
	VisitOrder preSolveLog[3];
} CallbackLog;

typedef struct CallbackScene
{
	Corridor corridors[3];
	CallbackLog logs[2];
} CallbackScene;

// Keep only calls with a corridor sphere, logged under that sphere as the other shape's id
static int LogCorridorCall( CallbackLog* log, VisitOrder* logs, b3ShapeId idA, b3ShapeId idB )
{
	for ( int i = 0; i < 3; ++i )
	{
		if ( idA.index1 == log->sphereIndex[i] )
		{
			VisitOrderCallback( 0, (uint64_t)( idB.index1 - 1 ), logs + i );
			return 1;
		}
		if ( idB.index1 == log->sphereIndex[i] )
		{
			VisitOrderCallback( 0, (uint64_t)( idA.index1 - 1 ), logs + i );
			return 1;
		}
	}
	return 0;
}

static bool CallbackSceneFilter( b3ShapeId idA, b3ShapeId idB, void* context )
{
	CallbackLog* log = context;
	LogCorridorCall( log, log->filterLog, idA, idB );
	return true;
}

static bool CallbackScenePreSolve( b3ShapeId idA, b3ShapeId idB, b3Pos point, b3Vec3 normal, void* context )
{
	MAYBE_UNUSED( point );
	MAYBE_UNUSED( normal );
	CallbackLog* log = context;
	LogCorridorCall( log, log->preSolveLog, idA, idB );

	// Keep the running fraction open so every on-path box reaches the callback
	return false;
}

static void CallbackSceneCreate( b3WorldId worldId, int runIndex, void* context )
{
	CallbackScene* scene = context;
	b3World_SetCustomFilterCallback( worldId, CallbackSceneFilter, scene->logs + runIndex );
	b3World_SetPreSolveCallback( worldId, CallbackScenePreSolve, scene->logs + runIndex );
	for ( int i = 0; i < 3; ++i )
	{
		CreateCorridorPart1( scene->corridors + i, worldId, runIndex );
	}
}

static int CallbackSceneBeforeStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	CallbackScene* scene = context;
	if ( stepIndex == CORRIDOR_STEP )
	{
		for ( int i = 0; i < 3; ++i )
		{
			ENSURE( CorridorBeforeStep( scene->corridors + i, worldId, runIndex ) == 0 );
		}
	}
	return 0;
}

static int CallbackSceneAfterStep( b3WorldId worldId, int runIndex, int stepIndex, void* context )
{
	CallbackScene* scene = context;
	if ( stepIndex == CORRIDOR_STEP - 1 )
	{
		for ( int i = 0; i < 3; ++i )
		{
			CreateCorridorPart2( scene->corridors + i, worldId, runIndex );
			scene->logs[runIndex].sphereIndex[i] = scene->corridors[i].runs[runIndex].sphereShapeId.index1;
		}
	}
	else if ( stepIndex == CORRIDOR_STEP )
	{
		for ( int i = 0; i < 3; ++i )
		{
			ENSURE( CorridorAfterStep( scene->corridors + i, worldId, runIndex ) == 0 );
		}
	}
	return 0;
}

int ContinuousCallbackOrderTest( void )
{
	static CallbackScene callbackScene;
	memset( &callbackScene, 0, sizeof( CallbackScene ) );
	RandomState random = { 0xCA11BACu };
	float timeStep = 1.0f / 60.0f;
	PlanThreeCorridors( callbackScene.corridors, &random, timeStep, true );
	for ( int i = 0; i < 3; ++i )
	{
		PlanCorridorFarToNear( callbackScene.corridors + i );
	}

	Scene scene = { 0 };
	scene.create = CallbackSceneCreate;
	scene.beforeStep = CallbackSceneBeforeStep;
	scene.afterStep = CallbackSceneAfterStep;
	scene.context = &callbackScene;
	scene.worldDef = b3DefaultWorldDef();
	scene.worldDef.gravity = b3Vec3_zero;
	scene.worldDef.enableSleep = false;
	scene.stepCount = 2;
	scene.timeStep = timeStep;
	scene.subStepCount = 4;

	static PairResult result;
	ENSURE( RunScenePair( &scene, true, &result ) == 0 );

	// Print every log before the first assertion
	for ( int i = 0; i < 3; ++i )
	{
		const Corridor* corridor = callbackScene.corridors + i;
		const char* name = TreeName( corridor->plan.clusterTree );
		for ( int runIndex = 0; runIndex < 2; ++runIndex )
		{
			const CallbackLog* log = callbackScene.logs + runIndex;
			printf( "  %s corridor run %d\n", name, runIndex + 1 );
			PrintVisitOrder( "sweep order", &corridor->runs[runIndex].sweepOrder );
			PrintVisitOrder( "custom filter", log->filterLog + i );
			PrintVisitOrder( "pre-solve", log->preSolveLog + i );
		}
	}

	for ( int i = 0; i < 3; ++i )
	{
		const Corridor* corridor = callbackScene.corridors + i;
		const char* name = TreeName( corridor->plan.clusterTree );
		ENSURE( CheckCorridorWitness( corridor ) == 0 );

		// The on-path boxes alone must also be visited in a different order
		VisitOrder onPath1 = RestrictToSet( &corridor->runs[0].witness, &corridor->runs[0].onPath );
		VisitOrder onPath2 = RestrictToSet( &corridor->runs[1].witness, &corridor->runs[1].onPath );
		ENSURE( WitnessDiffers( name, &onPath1, &onPath2 ) );

		// The sweep order holds the cluster, is the same in both runs, and is not ascending id
		VisitOrder cluster = CorridorClusterSet( corridor, 0 );
		SortVisitOrder( &cluster );
		const VisitOrder* sweepOrder = &corridor->runs[0].sweepOrder;
		ENSURE( SameSet( sweepOrder, &cluster ) );
		ENSURE( SameSequence( sweepOrder, &corridor->runs[1].sweepOrder ) );
		ENSURE( SameSequence( sweepOrder, &cluster ) == false );

		VisitOrder onPath = RestrictToSet( sweepOrder, &corridor->runs[0].onPath );
		for ( int runIndex = 0; runIndex < 2; ++runIndex )
		{
			const CallbackLog* log = callbackScene.logs + runIndex;
			ENSURE( SameSequence( log->filterLog + i, sweepOrder ) );
			ENSURE( SameSequence( log->preSolveLog + i, &onPath ) );
		}
	}

	return 0;
}

// The explosion wakes bodies and applies impulses in ascending shape id, so the awake set order
// and the summed impulses do not depend on the dynamic tree layout. Every body sleeps in its own
// set before the explosion. The world is stepped once after creation, with nothing awake and no
// gravity, so the shapes created asleep settle from the awake tree into the sleeping tree, which
// is the witness.

typedef struct ExplodeRun
{
	b3BodyId bodies[EXPLODE_BODY_COUNT];
	VisitOrder witness;
	VisitOrder bodyWitness;
	VisitOrder awakeOrder;
	b3Vec3 linearVelocity[EXPLODE_BODY_COUNT];
	b3Vec3 angularVelocity[EXPLODE_BODY_COUNT];
} ExplodeRun;

int ExplodeOrderTest( void )
{
	ExplodeScene explodeScene;
	RandomState random = { 0xE8D10DEu };
	PlanExplodeScene( &explodeScene, &random );
	const b3ExplosionDef* def = &explodeScene.def;

	static ExplodeRun runs[2];
	memset( runs, 0, sizeof( runs ) );

	for ( int runIndex = 0; runIndex < 2; ++runIndex )
	{
		ExplodeRun* run = runs + runIndex;

		b3WorldDef worldDef = b3DefaultWorldDef();
		worldDef.gravity = b3Vec3_zero;
		worldDef.enableSleep = true;
		worldDef.workerCount = 1;
		b3WorldId worldId = b3CreateWorld( &worldDef );

		int bodyCount = 0;
		{
			b3BodyDef bodyDef = b3DefaultBodyDef();
			bodyDef.type = b3_dynamicBody;
			bodyDef.position = b3OffsetPos( def->position, explodeScene.multiOffset );
			bodyDef.isAwake = false;
			b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
			b3ShapeDef shapeDef = b3DefaultShapeDef();
			for ( int i = 0; i < EXPLODE_MULTI_SHAPE_COUNT; ++i )
			{
				b3Sphere sphere = { explodeScene.multiShapeOffsets[i], explodeScene.singleRadius };
				b3CreateSphereShape( bodyId, &shapeDef, &sphere );
			}
			run->bodies[bodyCount++] = bodyId;
		}

		for ( int i = 0; i < EXPLODE_SINGLE_COUNT; ++i )
		{
			run->bodies[bodyCount++] = CreateExplodeSingle( &explodeScene, worldId, i, false );
		}

		{
			b3BodyDef bodyDef = b3DefaultBodyDef();
			bodyDef.type = b3_dynamicBody;
			bodyDef.position = b3OffsetPos( def->position, explodeScene.farOffset );
			bodyDef.isAwake = false;
			b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
			b3ShapeDef shapeDef = b3DefaultShapeDef();
			b3Sphere sphere = { b3Vec3_zero, explodeScene.singleRadius };
			b3CreateSphereShape( bodyId, &shapeDef, &sphere );
			run->bodies[bodyCount++] = bodyId;
		}

		ENSURE( bodyCount == EXPLODE_BODY_COUNT );
		ENSURE( b3World_GetAwakeBodyCount( worldId ) == 0 );

		b3World_Step( worldId, 1.0f / 60.0f, 4 );
		ENSURE( b3World_GetAwakeBodyCount( worldId ) == 0 );

		if ( runIndex == 1 )
		{
			PerturbTreeLayout( worldId );
		}

		// Every body was created asleep and the step settled them, so the witness is the sleeping tree
		RecordVisitOrder( worldId, B3_SLEEPING_PROXY_TYPE, ExplosionBox( def ), def->maskBits, &run->witness );

		// Each shape mapped to its body, keeping the first visit of each body, witnesses the wake order
		b3World* world = b3GetWorldFromId( worldId );
		for ( int i = 0; i < run->witness.count; ++i )
		{
			int bodyId = b3Array_Get( world->shapes, run->witness.ids[i] )->bodyId;
			if ( ContainsId( &run->bodyWitness, bodyId ) == false )
			{
				run->bodyWitness.ids[run->bodyWitness.count] = bodyId;
				run->bodyWitness.count += 1;
			}
		}

		b3World_Explode( worldId, def );

		for ( int i = 0; i < EXPLODE_BODY_COUNT; ++i )
		{
			ENSURE( b3Body_IsAwake( run->bodies[i] ) );
			run->linearVelocity[i] = b3Body_GetLinearVelocity( run->bodies[i] );
			run->angularVelocity[i] = b3Body_GetAngularVelocity( run->bodies[i] );
			ENSURE( b3LengthSquared( run->linearVelocity[i] ) > 0.0f );
		}

		b3SolverSet* awakeSet = b3Array_Get( world->solverSets, b3_awakeSet );
		for ( int i = 0; i < awakeSet->bodySims.count; ++i )
		{
			run->awakeOrder.ids[run->awakeOrder.count] = awakeSet->bodySims.data[i].bodyId;
			run->awakeOrder.count += 1;
		}

		b3DestroyWorld( worldId );
	}

	ENSURE( runs[0].witness.count == EXPLODE_SINGLE_COUNT + EXPLODE_MULTI_SHAPE_COUNT + 1 );

	// The multi-shape body's own shapes, for the record
	VisitOrder multiShapes = { { 0, 1, 2 }, EXPLODE_MULTI_SHAPE_COUNT };
	VisitOrder multi1 = RestrictToSet( &runs[0].witness, &multiShapes );
	VisitOrder multi2 = RestrictToSet( &runs[1].witness, &multiShapes );
	PrintVisitOrder( "run 1 multi-shape body", &multi1 );
	PrintVisitOrder( "run 2 multi-shape body", &multi2 );
	ENSURE( WitnessDiffers( "explode shapes", &runs[0].witness, &runs[1].witness ) );
	ENSURE( WitnessDiffers( "explode bodies", &runs[0].bodyWitness, &runs[1].bodyWitness ) );

	bool sameAwakeOrder = SameSequence( &runs[0].awakeOrder, &runs[1].awakeOrder );
	bool sameVelocities = memcmp( runs[0].linearVelocity, runs[1].linearVelocity, sizeof( runs[0].linearVelocity ) ) == 0 &&
						  memcmp( runs[0].angularVelocity, runs[1].angularVelocity, sizeof( runs[0].angularVelocity ) ) == 0;
	printf( "  awake set order equal %d, velocities equal %d\n", sameAwakeOrder, sameVelocities );
	PrintVisitOrder( "run 1 awake order", &runs[0].awakeOrder );
	PrintVisitOrder( "run 2 awake order", &runs[1].awakeOrder );
	ENSURE( sameAwakeOrder );
	ENSURE( sameVelocities );
	return 0;
}

int TreeOrderHelperTest( void )
{
	RUN_SUBTEST( CorridorWitnessCase );
	RUN_SUBTEST( ExplodeWitnessCase );
	RUN_SUBTEST( UnchangedHashCase );
	return 0;
}
