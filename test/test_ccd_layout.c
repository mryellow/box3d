// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

// Continuous collision must not depend on the broad-phase tree layout. These tests use the public
// API only. Two worlds are built the same way and given the same perturbation, one move-away-and-
// back cycle per static body, world A in ascending and world B in descending body order. Public
// ids, proxy bounds and moved marks are then equal and only the static tree's layout differs. The
// visit order is witnessed with b3World_OverlapAABB, which walks the static tree with the query the
// continuous stage uses, so a passing comparison is not vacuous. Each test prints its divergence
// counts, and the first diverging seed of each observable, before its first assertion.
//
// Fail-before, on this branch with main's src checked out over it:
//   git checkout main -- src
//   cmake --build build --target test
//   for t in ContinuousLayoutImpactTest ContinuousLayoutPreSolveTest ContinuousLayoutSensorTest; do ./build/bin/test $t; done
//   git checkout solver-tree-order-independent-pr -- src

#include "test_macros.h"

#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/math_functions.h"
#include "box3d/types.h"

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
	float u = (float)( NextRandom( state ) >> 8 ) / 16777216.0f;
	return lower + ( upper - lower ) * u;
}

// A sequence of shape ids: a visit-order witness, a callback log or a sensor report

#define ID_CAPACITY 64

typedef struct IdList
{
	int ids[ID_CAPACITY];
	int count;
} IdList;

static void PushId( IdList* list, int id )
{
	if ( list->count < ID_CAPACITY )
	{
		list->ids[list->count] = id;
	}
	list->count += 1;
}

static void RemoveId( IdList* list, int id )
{
	int count = 0;
	for ( int i = 0; i < list->count; ++i )
	{
		if ( list->ids[i] != id )
		{
			list->ids[count++] = list->ids[i];
		}
	}
	list->count = count;
}

static bool SameSequence( const IdList* a, const IdList* b )
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

static void SortIds( IdList* list )
{
	// Insertion sort, the lists are small
	for ( int i = 1; i < list->count; ++i )
	{
		int id = list->ids[i];
		int j = i;
		while ( j > 0 && list->ids[j - 1] > id )
		{
			list->ids[j] = list->ids[j - 1];
			j -= 1;
		}
		list->ids[j] = id;
	}
}

static bool SameSet( const IdList* a, const IdList* b )
{
	IdList sortedA = *a;
	IdList sortedB = *b;
	SortIds( &sortedA );
	SortIds( &sortedB );
	return SameSequence( &sortedA, &sortedB );
}

static void PrintIds( const char* label, const IdList* list )
{
	printf( "    %s:", label );
	for ( int i = 0; i < list->count; ++i )
	{
		printf( " %d", list->ids[i] );
	}
	printf( "\n" );
}

static bool OverlapWitness( b3ShapeId shapeId, void* context )
{
	PushId( context, shapeId.index1 );
	return true;
}

static bool SameBodyId( b3BodyId a, b3BodyId b )
{
	return a.index1 == b.index1 && a.generation == b.generation;
}

static bool SameShapeId( b3ShapeId a, b3ShapeId b )
{
	return a.index1 == b.index1 && a.generation == b.generation;
}

// The strict perturbation: one move-away-and-back cycle per static body. The first move leaves the
// proxy's fat box, so the proxy is removed and reinserted without rotation and its bounds change
// from the creation bounds to the shape margin; the second move does the same. Both worlds get the
// same cycle, so bounds, ids and moved marks are equal, and only the reinsertion order differs.
static void MoveAwayAndBack( b3BodyId bodyId, b3Pos home )
{
	b3Vec3 away = { 0.0f, 50.0f, 0.0f };
	b3Body_SetTransform( bodyId, b3OffsetPos( home, away ), b3Quat_identity );
	b3Body_SetTransform( bodyId, home, b3Quat_identity );
}

// Corridor: a fast sphere and sixteen small static boxes scattered inside its one-step swept cylinder

#define BOX_COUNT 16
#define SEED_COUNT 200

static const float kRadius = 1.0f;
static const float kSpeed = 300.0f;
static const float kTimeStep = 1.0f / 60.0f;
static const float kHalfExtent = 0.05f;

typedef struct Plan
{
	b3Vec3 offsets[BOX_COUNT];
} Plan;

static void PlanBoxes( Plan* plan, uint32_t seed )
{
	RandomState random = { 0x9E3779B9u * seed };
	float travel = kSpeed * kTimeStep;
	float xStart = kRadius + 0.4f;
	float xEnd = travel - kRadius - 0.4f;
	float lateral = 0.8f;
	float separation = 0.35f;
	for ( int i = 0; i < BOX_COUNT; ++i )
	{
		b3Vec3 p;
		bool accepted = false;
		while ( accepted == false )
		{
			p = (b3Vec3){ RandomFloat( &random, xStart, xEnd ), RandomFloat( &random, -lateral, lateral ),
						  RandomFloat( &random, -lateral, lateral ) };
			accepted = p.y * p.y + p.z * p.z < lateral * lateral;
			for ( int j = 0; j < i && accepted; ++j )
			{
				accepted = b3Distance( p, plan->offsets[j] ) > separation;
			}
		}
		plan->offsets[i] = p;
	}
}

typedef struct Corridor
{
	b3WorldId worldId;
	b3BodyId boxIds[BOX_COUNT];
	b3ShapeId boxShapeIds[BOX_COUNT];
	b3BodyId sphereId;
	b3ShapeId sphereShapeId;
	IdList witness;
	IdList preSolveLog;
	bool preSolveAccept;
	int contactCountBefore;
	b3WorldTransform transform;
	b3Counters counters;
} Corridor;

static bool PreSolveLogger( b3ShapeId shapeIdA, b3ShapeId shapeIdB, b3Pos point, b3Vec3 normal, void* context )
{
	MAYBE_UNUSED( point );
	MAYBE_UNUSED( normal );
	Corridor* corridor = context;
	int other = shapeIdA.index1 == corridor->sphereShapeId.index1 ? shapeIdB.index1 : shapeIdA.index1;
	PushId( &corridor->preSolveLog, other );
	return corridor->preSolveAccept;
}

static void BuildCorridor( Corridor* corridor, const Plan* plan, bool descending, bool preSolveEvents, bool preSolveAccept )
{
	memset( corridor, 0, sizeof( Corridor ) );
	corridor->preSolveAccept = preSolveAccept;

	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.gravity = b3Vec3_zero;
	worldDef.enableSleep = false;
	corridor->worldId = b3CreateWorld( &worldDef );
	if ( preSolveEvents )
	{
		b3World_SetPreSolveCallback( corridor->worldId, PreSolveLogger, corridor );
	}

	b3BoxHull hull = b3MakeBoxHull( kHalfExtent, kHalfExtent, kHalfExtent );
	for ( int i = 0; i < BOX_COUNT; ++i )
	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		bodyDef.position = b3ToPos( plan->offsets[i] );
		corridor->boxIds[i] = b3CreateBody( corridor->worldId, &bodyDef );
		b3ShapeDef shapeDef = b3DefaultShapeDef();
		shapeDef.enablePreSolveEvents = preSolveEvents;
		corridor->boxShapeIds[i] = b3CreateHullShape( corridor->boxIds[i], &shapeDef, &hull.base );
	}

	for ( int k = 0; k < BOX_COUNT; ++k )
	{
		int i = descending ? BOX_COUNT - 1 - k : k;
		MoveAwayAndBack( corridor->boxIds[i], b3ToPos( plan->offsets[i] ) );
	}

	b3BodyDef sphereDef = b3DefaultBodyDef();
	sphereDef.type = b3_dynamicBody;
	sphereDef.position = b3Pos_zero;
	sphereDef.linearVelocity = (b3Vec3){ kSpeed, 0.0f, 0.0f };
	corridor->sphereId = b3CreateBody( corridor->worldId, &sphereDef );
	b3ShapeDef sphereShapeDef = b3DefaultShapeDef();
	sphereShapeDef.enablePreSolveEvents = preSolveEvents;
	b3Sphere sphere = { b3Vec3_zero, kRadius };
	corridor->sphereShapeId = b3CreateSphereShape( corridor->sphereId, &sphereShapeDef, &sphere );

	// The covering box returns every box, all of them candidates of the continuous query, and the sphere
	float travel = kSpeed * kTimeStep;
	b3AABB cover = { { -4.0f, -4.0f, -4.0f }, { travel + 4.0f, 4.0f, 4.0f } };
	b3World_OverlapAABB( corridor->worldId, cover, b3DefaultQueryFilter(), OverlapWitness, &corridor->witness );
	RemoveId( &corridor->witness, corridor->sphereShapeId.index1 );

	corridor->contactCountBefore = b3World_GetCounters( corridor->worldId ).contactCount;
}

static void StepCorridor( Corridor* corridor )
{
	b3World_Step( corridor->worldId, kTimeStep, 4 );
	corridor->transform = b3Body_GetTransform( corridor->sphereId );
	corridor->counters = b3World_GetCounters( corridor->worldId );
}

static bool SameIds( const Corridor* a, const Corridor* b )
{
	for ( int i = 0; i < BOX_COUNT; ++i )
	{
		if ( SameBodyId( a->boxIds[i], b->boxIds[i] ) == false || SameShapeId( a->boxShapeIds[i], b->boxShapeIds[i] ) == false )
		{
			return false;
		}
	}

	return SameBodyId( a->sphereId, b->sphereId ) && SameShapeId( a->sphereShapeId, b->sphereShapeId );
}

static bool Impacted( const Corridor* corridor )
{
	// A sphere that hit nothing travels the full distance
	return corridor->transform.p.x < kSpeed * kTimeStep - kRadius;
}

// Per-seed checks that make the comparison valid, counted over the seeds and asserted once. A
// rejecting preSolve callback lets the sphere through, so impacts are required where expected only.
typedef struct SceneCounts
{
	int seeds;
	int sameIds;
	int witnessComplete;
	int witnessSameSet;
	int witnessOrderDiffers;
	int contactCountsEqual;
	int bothImpacted;
	int divergedWithoutWitness;
} SceneCounts;

// Returns true when the witness orders differ
static bool CountScene( SceneCounts* counts, const Corridor* a, const Corridor* b )
{
	bool complete = a->witness.count == BOX_COUNT && b->witness.count == BOX_COUNT;
	bool sameSet = SameSet( &a->witness, &b->witness );
	bool orderDiffers = complete && sameSet && SameSequence( &a->witness, &b->witness ) == false;
	counts->seeds += 1;
	counts->sameIds += SameIds( a, b );
	counts->witnessComplete += complete;
	counts->witnessSameSet += sameSet;
	counts->witnessOrderDiffers += orderDiffers;
	counts->contactCountsEqual += a->contactCountBefore == b->contactCountBefore;
	counts->bothImpacted += Impacted( a ) && Impacted( b );
	return orderDiffers;
}

static void PrintSceneCounts( const SceneCounts* counts )
{
	printf( "  seeds %d: ids equal %d, witness complete %d, witness same set %d, witness order differs %d, "
			"contact counts equal %d, both impacted %d, diverged without differing witness %d\n",
			counts->seeds, counts->sameIds, counts->witnessComplete, counts->witnessSameSet, counts->witnessOrderDiffers,
			counts->contactCountsEqual, counts->bothImpacted, counts->divergedWithoutWitness );
}

static void PrintSeed( const char* observable, uint32_t seed, const Corridor* a, const Corridor* b )
{
	printf( "  first seed where %s differs: %u\n", observable, seed );
	PrintIds( "visit order A", &a->witness );
	PrintIds( "visit order B", &b->witness );
}

static void PrintTransform( const char* label, const Corridor* corridor )
{
	b3WorldTransform xf = corridor->transform;
	printf( "    %s: p (%.17g %.17g %.17g) q (%.9g %.9g %.9g %.9g)\n", label, (double)xf.p.x, (double)xf.p.y, (double)xf.p.z,
			xf.q.v.x, xf.q.v.y, xf.q.v.z, xf.q.s );
}

static void PrintCounters( const char* label, const Corridor* corridor )
{
	printf( "    %s: distanceIterations %d, pushBackIterations %d, rootIterations %d\n", label,
			corridor->counters.distanceIterations, corridor->counters.pushBackIterations, corridor->counters.rootIterations );
}

#define SCENE_ASSERTIONS( counts )                                                                                               \
	do                                                                                                                           \
	{                                                                                                                            \
		ENSURE( ( counts ).seeds == SEED_COUNT );                                                                                \
		ENSURE( ( counts ).sameIds == SEED_COUNT );                                                                              \
		ENSURE( ( counts ).witnessComplete == SEED_COUNT );                                                                      \
		ENSURE( ( counts ).witnessSameSet == SEED_COUNT );                                                                       \
		ENSURE( ( counts ).witnessOrderDiffers > 0 );                                                                            \
		ENSURE( ( counts ).contactCountsEqual == SEED_COUNT );                                                                   \
		ENSURE( ( counts ).divergedWithoutWitness == 0 );                                                                        \
	}                                                                                                                            \
	while ( false )

// P1: the sphere's transform after the step and each continuous iteration counter are the same in
// both layouts. No callbacks.
int ContinuousLayoutImpactTest( void )
{
	SceneCounts counts = { 0 };
	int transformDiffers = 0;
	int distanceDiffers = 0;
	int pushBackDiffers = 0;
	int rootDiffers = 0;

	for ( uint32_t seed = 1; seed <= SEED_COUNT; ++seed )
	{
		Plan plan;
		PlanBoxes( &plan, seed );

		Corridor a, b;
		BuildCorridor( &a, &plan, false, false, false );
		BuildCorridor( &b, &plan, true, false, false );
		StepCorridor( &a );
		StepCorridor( &b );

		bool orderDiffers = CountScene( &counts, &a, &b );
		bool transform = memcmp( &a.transform, &b.transform, sizeof( b3WorldTransform ) ) != 0;
		bool distance = a.counters.distanceIterations != b.counters.distanceIterations;
		bool pushBack = a.counters.pushBackIterations != b.counters.pushBackIterations;
		bool root = a.counters.rootIterations != b.counters.rootIterations;
		bool any = transform || distance || pushBack || root;
		counts.divergedWithoutWitness += any && orderDiffers == false;

		if ( transform && transformDiffers == 0 )
		{
			PrintSeed( "transform", seed, &a, &b );
			PrintTransform( "sphere A", &a );
			PrintTransform( "sphere B", &b );
		}

		if ( ( distance || pushBack || root ) && distanceDiffers + pushBackDiffers + rootDiffers == 0 )
		{
			PrintSeed( "a counter", seed, &a, &b );
			PrintCounters( "counters A", &a );
			PrintCounters( "counters B", &b );
		}

		transformDiffers += transform;
		distanceDiffers += distance;
		pushBackDiffers += pushBack;
		rootDiffers += root;

		b3DestroyWorld( a.worldId );
		b3DestroyWorld( b.worldId );
	}

	PrintSceneCounts( &counts );
	printf( "  transform differs %d, distanceIterations differs %d, pushBackIterations differs %d, rootIterations differs %d\n",
			transformDiffers, distanceDiffers, pushBackDiffers, rootDiffers );

	SCENE_ASSERTIONS( counts );
	ENSURE( counts.bothImpacted == SEED_COUNT );
	ENSURE( transformDiffers == 0 );
	ENSURE( distanceDiffers == 0 );
	ENSURE( pushBackDiffers == 0 );
	ENSURE( rootDiffers == 0 );
	return 0;
}

// P2: the sequence and the set of preSolve calls are the same in both layouts, for a callback that
// depends only on its arguments. A rejecting callback is called for every candidate on the path; an
// accepting one only for candidates that improve on the running fraction.
typedef struct PreSolveCounts
{
	SceneCounts scene;
	int nonEmpty;
	int sequenceDiffers;
	int setDiffers;
} PreSolveCounts;

static void RunPreSolvePass( PreSolveCounts* counts, bool accept )
{
	const char* pass = accept ? "accepting" : "rejecting";
	memset( counts, 0, sizeof( PreSolveCounts ) );

	for ( uint32_t seed = 1; seed <= SEED_COUNT; ++seed )
	{
		Plan plan;
		PlanBoxes( &plan, seed );

		Corridor a, b;
		BuildCorridor( &a, &plan, false, true, accept );
		BuildCorridor( &b, &plan, true, true, accept );
		StepCorridor( &a );
		StepCorridor( &b );

		bool orderDiffers = CountScene( &counts->scene, &a, &b );
		bool sequence = SameSequence( &a.preSolveLog, &b.preSolveLog ) == false;
		bool set = SameSet( &a.preSolveLog, &b.preSolveLog ) == false;
		counts->scene.divergedWithoutWitness += ( sequence || set ) && orderDiffers == false;
		counts->nonEmpty += a.preSolveLog.count > 0 && b.preSolveLog.count > 0;

		if ( sequence && counts->sequenceDiffers == 0 )
		{
			char label[64];
			snprintf( label, sizeof( label ), "the %s preSolve sequence", pass );
			PrintSeed( label, seed, &a, &b );
			PrintIds( "preSolve A", &a.preSolveLog );
			PrintIds( "preSolve B", &b.preSolveLog );
		}

		if ( set && counts->setDiffers == 0 )
		{
			char label[64];
			snprintf( label, sizeof( label ), "the %s preSolve set", pass );
			PrintSeed( label, seed, &a, &b );
			PrintIds( "preSolve A", &a.preSolveLog );
			PrintIds( "preSolve B", &b.preSolveLog );
		}

		counts->sequenceDiffers += sequence;
		counts->setDiffers += set;

		b3DestroyWorld( a.worldId );
		b3DestroyWorld( b.worldId );
	}

	printf( "  %s callback\n", pass );
	PrintSceneCounts( &counts->scene );
	printf( "  both logs non-empty %d, preSolve sequence differs %d, preSolve set differs %d\n", counts->nonEmpty,
			counts->sequenceDiffers, counts->setDiffers );
}

int ContinuousLayoutPreSolveTest( void )
{
	PreSolveCounts rejecting, accepting;
	RunPreSolvePass( &rejecting, false );
	RunPreSolvePass( &accepting, true );

	SCENE_ASSERTIONS( rejecting.scene );
	ENSURE( rejecting.nonEmpty == SEED_COUNT );
	ENSURE( rejecting.sequenceDiffers == 0 );
	ENSURE( rejecting.setDiffers == 0 );

	SCENE_ASSERTIONS( accepting.scene );
	ENSURE( accepting.scene.bothImpacted == SEED_COUNT );
	ENSURE( accepting.nonEmpty == SEED_COUNT );
	ENSURE( accepting.setDiffers == 0 );
	ENSURE( accepting.sequenceDiffers == 0 );
	return 0;
}

// P3: with more than eight sensors on a bullet's path, the eight reported are the same in both
// layouts. Each sensor is on its own static body so the perturbation reorders them.

#define SENSOR_COUNT 12
#define SENSOR_LIMIT 8

typedef struct SensorScene
{
	b3WorldId worldId;
	b3BodyId sensorBodyIds[SENSOR_COUNT];
	b3ShapeId sensorIds[SENSOR_COUNT];
	b3BodyId bulletId;
	b3ShapeId bulletShapeId;
	IdList witness;
	IdList reported;
} SensorScene;

static void BuildSensorScene( SensorScene* scene, const b3Vec3* offsets, float distance, bool descending )
{
	memset( scene, 0, sizeof( SensorScene ) );

	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.gravity = b3Vec3_zero;
	scene->worldId = b3CreateWorld( &worldDef );

	b3ShapeDef sensorDef = b3DefaultShapeDef();
	sensorDef.isSensor = true;
	sensorDef.enableSensorEvents = true;
	for ( int i = 0; i < SENSOR_COUNT; ++i )
	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		scene->sensorBodyIds[i] = b3CreateBody( scene->worldId, &bodyDef );
		b3Sphere sphere = { offsets[i], 0.15f };
		scene->sensorIds[i] = b3CreateSphereShape( scene->sensorBodyIds[i], &sensorDef, &sphere );
	}

	for ( int k = 0; k < SENSOR_COUNT; ++k )
	{
		int i = descending ? SENSOR_COUNT - 1 - k : k;
		MoveAwayAndBack( scene->sensorBodyIds[i], b3Pos_zero );
	}

	b3BodyDef bulletDef = b3DefaultBodyDef();
	bulletDef.type = b3_dynamicBody;
	bulletDef.linearVelocity = (b3Vec3){ distance / kTimeStep, 0.0f, 0.0f };
	bulletDef.isBullet = true;
	scene->bulletId = b3CreateBody( scene->worldId, &bulletDef );
	b3ShapeDef bulletShapeDef = b3DefaultShapeDef();
	bulletShapeDef.enableSensorEvents = true;
	b3Sphere bullet = { b3Vec3_zero, 0.25f };
	scene->bulletShapeId = b3CreateSphereShape( scene->bulletId, &bulletShapeDef, &bullet );

	b3AABB cover = { { -1.0f, -1.0f, -1.0f }, { distance + 1.0f, 1.0f, 1.0f } };
	b3World_OverlapAABB( scene->worldId, cover, b3DefaultQueryFilter(), OverlapWitness, &scene->witness );
	RemoveId( &scene->witness, scene->bulletShapeId.index1 );

	b3World_Step( scene->worldId, kTimeStep, 4 );

	b3SensorEvents events = b3World_GetSensorEvents( scene->worldId );
	for ( int i = 0; i < events.beginCount; ++i )
	{
		if ( events.beginEvents[i].visitorShapeId.index1 == scene->bulletShapeId.index1 )
		{
			PushId( &scene->reported, events.beginEvents[i].sensorShapeId.index1 );
		}
	}
}

int ContinuousLayoutSensorTest( void )
{
	RandomState random = { 0x5E4504u };
	float spacing = 0.35f;
	float lead = 0.8f;
	float distance = 2.0f * lead + spacing * ( SENSOR_COUNT - 1 );
	b3Vec3 offsets[SENSOR_COUNT];
	for ( int i = 0; i < SENSOR_COUNT; ++i )
	{
		offsets[i] = (b3Vec3){ lead + spacing * i, RandomFloat( &random, -0.05f, 0.05f ), RandomFloat( &random, -0.05f, 0.05f ) };
	}

	SensorScene a, b;
	BuildSensorScene( &a, offsets, distance, false );
	BuildSensorScene( &b, offsets, distance, true );

	bool sameIds = SameBodyId( a.bulletId, b.bulletId ) && SameShapeId( a.bulletShapeId, b.bulletShapeId );
	for ( int i = 0; i < SENSOR_COUNT; ++i )
	{
		sameIds =
			sameIds && SameBodyId( a.sensorBodyIds[i], b.sensorBodyIds[i] ) && SameShapeId( a.sensorIds[i], b.sensorIds[i] );
	}

	bool witnessComplete = a.witness.count == SENSOR_COUNT && b.witness.count == SENSOR_COUNT;
	bool witnessSameSet = SameSet( &a.witness, &b.witness );
	bool witnessOrderDiffers = SameSequence( &a.witness, &b.witness ) == false;
	bool reportedSameSet = SameSet( &a.reported, &b.reported );

	printf( "  ids equal %d, witness complete %d, witness same set %d, witness order differs %d, reported sets equal %d\n",
			sameIds, witnessComplete, witnessSameSet, witnessOrderDiffers, reportedSameSet );
	PrintIds( "visit order A", &a.witness );
	PrintIds( "visit order B", &b.witness );
	PrintIds( "reported A", &a.reported );
	PrintIds( "reported B", &b.reported );

	b3DestroyWorld( a.worldId );
	b3DestroyWorld( b.worldId );

	ENSURE( sameIds );
	ENSURE( witnessComplete );
	ENSURE( witnessSameSet );
	ENSURE( witnessOrderDiffers );
	ENSURE( a.reported.count == SENSOR_LIMIT );
	ENSURE( b.reported.count == SENSOR_LIMIT );
	ENSURE( reportedSameSet );
	return 0;
}
