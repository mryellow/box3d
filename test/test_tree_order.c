// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

// Tree layout tests. The broad-phase trees hand candidates to the step in a layout dependent
// order. These tests run a scene twice, perturb the layout of the second run, and check the
// step gives the same answer. A visit-order witness shows the perturbation changed the order
// the query under test sees, so a passing comparison is not vacuous.

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
	b3DynamicTree_Rebuild( world->broadPhase.trees + b3_staticBody, true );
	b3DynamicTree_Rebuild( world->broadPhase.trees + b3_kinematicBody, true );
	b3DynamicTree_Rebuild( world->broadPhase.trees + b3_dynamicBody, true );
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

#define CORRIDOR_BOX_COUNT 6
#define CORRIDOR_FIRST_PART_COUNT 3
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
	plan->radius = 0.25f;
	plan->speed = 300.0f;
	plan->timeStep = timeStep;
	plan->boxHalfExtent = 0.05f;
	plan->enableCallbacks = enableCallbacks;

	// Every box sits on the sphere's path. One box per slot along x, with a gap that keeps the
	// boxes' fat bounds apart, and a lateral jitter small enough that the sphere hits each one.
	float r = plan->radius;
	float d = plan->speed * plan->timeStep;
	float start = r + 0.4f;
	float end = d - r - 0.4f;
	float slot = ( end - start ) / CORRIDOR_BOX_COUNT;
	float jitter = 0.09f;
	for ( int i = 0; i < CORRIDOR_BOX_COUNT; ++i )
	{
		float x = start + slot * ( i + RandomFloat( random, 0.15f, 0.85f ) );
		float y = RandomFloat( random, -jitter, jitter );
		float z = RandomFloat( random, -jitter, jitter );
		plan->boxOffsets[i] = (b3Vec3){ x, y, z };
	}

	// The first part is a random subset so the settled subtree and the insertions interleave along x
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

	if ( plan->clusterTree == b3_staticBody )
	{
		RecordVisitOrder( worldId, b3_staticBody, CorridorCoverBox( plan ), B3_DEFAULT_MASK_BITS, &run->witness );
	}

	return 0;
}

// After the corridor step: the kinematic or dynamic tree witness is recorded with the sphere's
// own shape removed, since the step rebuilt that tree before the continuous query and only
// enlarged bounds after it. The sphere must have been flagged fast, or the query never ran.
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

typedef struct ExplodeScene
{
	b3ExplosionDef def;
	b3Vec3 singleOffsets[EXPLODE_SINGLE_COUNT];
	float singleRadius;
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

int TreeOrderHelperTest( void )
{
	RUN_SUBTEST( CorridorWitnessCase );
	RUN_SUBTEST( ExplodeWitnessCase );
	RUN_SUBTEST( UnchangedHashCase );
	return 0;
}
