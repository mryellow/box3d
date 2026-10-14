// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

// Awake and sleeping dynamic tree tests. The broad-phase keeps the proxies of sleeping dynamic
// bodies in their own tree behind proxy code 3. The sleeping tree never holds a marked leaf and is
// never searched for its own: b3Body_SetTransform wakes a sleeping body, and every other write that
// marks a sleeping body's proxy parks the leaf in the awake tree until the next step has searched
// it. These tests check the tree exists, is searched as the unmoved side, is filled and emptied by
// sleep and wake, is kept shallow, and that the parked leaves find their pairs and settle.

#include "body.h"
#include "broad_phase.h"
#include "contact.h"
#include "dynamic_tree.h"
#include "physics_world.h"
#include "shape.h"
#include "solver_set.h"
#include "test_macros.h"

#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/math_functions.h"

#include <stdio.h>
#include <string.h>

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

// The fourth tree takes the dynamic shape hint and the awake tree starts at the floor. A proxy
// created with code 3 decodes, reports its shape and overlaps a proxy of code 2.
static int SleepingProxyTreeCase( void )
{
	b3Capacity capacity = { 0 };
	capacity.dynamicShapeCount = 100;

	b3BroadPhase bp;
	b3CreateBroadPhase( &bp, &capacity );
	ENSURE( bp.trees[B3_SLEEPING_PROXY_TYPE].proxyCapacity == 100 );
	ENSURE( bp.trees[b3_dynamicBody].proxyCapacity == 16 );

	b3AABB boxA = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
	b3AABB boxB = { { 0.5f, 0.5f, 0.5f }, { 1.5f, 1.5f, 1.5f } };
	int shapeIndexA = 7;
	int shapeIndexB = 11;
	int keyA = b3BroadPhase_CreateProxy( &bp, b3_dynamicBody, boxA, B3_DEFAULT_CATEGORY_BITS, shapeIndexA, false );
	int keyB = b3BroadPhase_CreateProxy( &bp, B3_SLEEPING_PROXY_TYPE, boxB, B3_DEFAULT_CATEGORY_BITS, shapeIndexB, false );

	ENSURE( B3_PROXY_TYPE( keyA ) == b3_dynamicBody );
	ENSURE( B3_PROXY_TYPE( keyB ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( B3_PROXY_ID( keyA ) == 0 );
	ENSURE( B3_PROXY_ID( keyB ) == 0 );
	ENSURE( b3BroadPhase_GetShapeIndex( &bp, keyA ) == shapeIndexA );
	ENSURE( b3BroadPhase_GetShapeIndex( &bp, keyB ) == shapeIndexB );
	ENSURE( b3BroadPhase_TestOverlap( &bp, keyA, keyB ) );

	// A dynamic create marks its leaf, so clear both trees before the no-moved check
	b3DynamicTree_ClearMoved( bp.trees + b3_dynamicBody );
	b3DynamicTree_ClearMoved( bp.trees + B3_SLEEPING_PROXY_TYPE );
	b3ValidateBroadPhase( &bp );
	b3ValidateNoMoved( &bp );

	b3BroadPhase_DestroyProxy( &bp, keyA );
	b3BroadPhase_DestroyProxy( &bp, keyB );
	b3DestroyBroadPhase( &bp );
	return 0;
}

// A fresh world reports the fourth tree empty with no rebuilds, and a sleeping body with one
// step changes neither. Which tree holds the shape is not asserted here.
static int SleepingTreeCountersCase( void )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	b3WorldId worldId = b3CreateWorld( &worldDef );

	b3Counters counters = b3World_GetCounters( worldId );
	ENSURE( counters.sleepingTreeHeight == 0 );
	ENSURE( counters.sleepingTreeRebuildCount == 0 );

	b3BodyDef bodyDef = b3DefaultBodyDef();
	bodyDef.type = b3_dynamicBody;
	bodyDef.position = (b3Pos){ 0.0, 5.0, 0.0 };
	bodyDef.isAwake = false;
	b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3BoxHull box = b3MakeBoxHull( 0.5f, 0.5f, 0.5f );
	b3CreateHullShape( bodyId, &shapeDef, &box.base );

	b3World_Step( worldId, 1.0f / 60.0f, 4 );

	b3World* world = b3GetWorldFromId( worldId );
	b3ValidateBroadPhase( &world->broadPhase );

	counters = b3World_GetCounters( worldId );
	ENSURE( counters.sleepingTreeHeight == 0 );
	ENSURE( counters.sleepingTreeRebuildCount == 0 );

	b3DestroyWorld( worldId );
	return 0;
}

// Find the contact between two bodies through the first body's contact list, or NULL
static b3Contact* FindContact( b3World* world, b3BodyId bodyIdA, b3BodyId bodyIdB )
{
	b3Body* bodyA = b3GetBodyFullId( world, bodyIdA );
	int otherId = bodyIdB.index1 - 1;

	int contactKey = bodyA->headContactKey;
	while ( contactKey != B3_NULL_INDEX )
	{
		int contactId = contactKey >> 1;
		int edgeIndex = contactKey & 1;
		b3Contact* contact = b3Array_Get( world->contacts, contactId );
		if ( contact->edges[edgeIndex ^ 1].bodyId == otherId )
		{
			return contact;
		}

		contactKey = contact->edges[edgeIndex].nextKey;
	}

	return NULL;
}

static b3BodyId CreateBox( b3WorldId worldId, b3BodyType type, b3Pos position, float extent, bool isAwake )
{
	b3BodyDef bodyDef = b3DefaultBodyDef();
	bodyDef.type = type;
	bodyDef.position = position;
	bodyDef.isAwake = isAwake;
	b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3BoxHull box = b3MakeBoxHull( extent, extent, extent );
	b3CreateHullShape( bodyId, &shapeDef, &box.base );
	return bodyId;
}

// A wide static box whose top face is at y = 0
static b3BodyId CreateGround( b3WorldId worldId )
{
	b3BodyDef bodyDef = b3DefaultBodyDef();
	bodyDef.position = (b3Pos){ 0.0, -0.5, 0.0 };
	b3BodyId groundId = b3CreateBody( worldId, &bodyDef );

	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3BoxHull box = b3MakeBoxHull( 20.0f, 0.5f, 20.0f );
	b3CreateHullShape( groundId, &shapeDef, &box.base );
	return groundId;
}

// The proxy code the body's first shape carries
static b3BodyType GetProxyType( b3World* world, b3BodyId bodyId )
{
	b3Body* body = b3GetBodyFullId( world, bodyId );
	b3Shape* shape = b3Array_Get( world->shapes, body->headShapeId );
	return B3_PROXY_TYPE( shape->proxyKey );
}

// Whether the leaf of the body's first shape is marked moved
static bool IsProxyMarked( b3World* world, b3BodyId bodyId )
{
	b3Body* body = b3GetBodyFullId( world, bodyId );
	b3Shape* shape = b3Array_Get( world->shapes, body->headShapeId );
	const b3DynamicTree* tree = world->broadPhase.trees + B3_PROXY_TYPE( shape->proxyKey );
	int proxyId = B3_PROXY_ID( shape->proxyKey );
	return b3IsNodeMoved( tree->nodes + tree->proxies[proxyId].node );
}

typedef struct PairScene
{
	b3WorldId worldId;
	b3World* world;
	b3BodyId staticId;
	b3BodyId kinematicId;
	b3BodyId sleeper1;
	b3BodyId sleeper2;
	b3BodyId sleeper3;
	b3BodyId sleeper4;
} PairScene;

// A static box, an awake kinematic box and four boxes created asleep, stepped once so the sleeping
// boxes settle into the sleeping tree
static void CreatePairScene( PairScene* scene )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	worldDef.enableSleep = true;
	scene->worldId = b3CreateWorld( &worldDef );
	scene->world = b3GetWorldFromId( scene->worldId );

	float extent = 0.5f;
	scene->staticId = CreateBox( scene->worldId, b3_staticBody, (b3Pos){ 0.0, 0.0, 0.0 }, extent, true );
	scene->kinematicId = CreateBox( scene->worldId, b3_kinematicBody, (b3Pos){ 20.0, 0.0, 0.0 }, extent, true );
	scene->sleeper1 = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ -40.0, 0.0, 0.0 }, extent, false );
	scene->sleeper2 = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ -60.0, 0.0, 0.0 }, extent, false );
	scene->sleeper3 = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ -80.0, 0.0, 0.0 }, extent, false );
	scene->sleeper4 = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ 40.0, 0.0, 0.0 }, extent, false );
}

// Overlap each target by a fifth of the extent along y: sleeper 1 over the static box, sleeper 2
// over sleeper 4, sleeper 3 over the kinematic box
static void MovePairSleepers( const PairScene* scene )
{
	float lift = 1.8f * 0.5f;
	b3Body_SetTransform( scene->sleeper1, (b3Pos){ 0.0, lift, 0.0 }, b3Quat_identity );
	b3Body_SetTransform( scene->sleeper2, (b3Pos){ 40.0, lift, 0.0 }, b3Quat_identity );
	b3Body_SetTransform( scene->sleeper3, (b3Pos){ 20.0, lift, 0.0 }, b3Quat_identity );
}

// A shape created asleep is parked in the awake tree, marked, with the body asleep and queued; the
// first step searches it and moves it to the sleeping tree, unmarked
static int CreatedAsleepCase( void )
{
	PairScene scene;
	CreatePairScene( &scene );
	b3World* world = scene.world;

	ENSURE( b3Body_IsAwake( scene.kinematicId ) );
	ENSURE( b3Body_IsAwake( scene.sleeper1 ) == false );
	ENSURE( b3Body_IsAwake( scene.sleeper4 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper1 ) == b3_dynamicBody );
	ENSURE( IsProxyMarked( world, scene.sleeper1 ) );
	ENSURE( GetProxyType( world, scene.sleeper4 ) == b3_dynamicBody );
	ENSURE( IsProxyMarked( world, scene.sleeper4 ) );
	ENSURE( world->broadPhase.migrationQueue.count == 4 );
	b3ValidateBroadPhase( &world->broadPhase );

	b3World_Step( scene.worldId, 1.0f / 60.0f, 4 );
	b3ValidateBroadPhase( &world->broadPhase );
	ENSURE( b3World_GetAwakeBodyCount( scene.worldId ) == 1 );
	ENSURE( world->broadPhase.migrationQueue.count == 0 );
	ENSURE( GetProxyType( world, scene.sleeper1 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsProxyMarked( world, scene.sleeper1 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper4 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsProxyMarked( world, scene.sleeper4 ) == false );
	ENSURE( b3HasTreeMoved( world->broadPhase.trees + B3_SLEEPING_PROXY_TYPE ) == false );

	b3DestroyWorld( scene.worldId );
	return 0;
}

// b3Body_SetTransform wakes a sleeping box. Moved into a static box, another sleeping box and an
// awake kinematic box, the three woken boxes find their pairs from the awake tree, every contact in
// the awake set, and the begin touch of the pair with sleeper 4 wakes it too.
static int MovedSleeperWakesCase( void )
{
	PairScene scene;
	CreatePairScene( &scene );
	b3World* world = scene.world;
	float timeStep = 1.0f / 60.0f;

	b3World_Step( scene.worldId, timeStep, 4 );
	ENSURE( b3World_GetAwakeBodyCount( scene.worldId ) == 1 );

	MovePairSleepers( &scene );
	ENSURE( b3Body_IsAwake( scene.sleeper1 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper2 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper3 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper4 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper1 ) == b3_dynamicBody );
	ENSURE( GetProxyType( world, scene.sleeper2 ) == b3_dynamicBody );
	ENSURE( GetProxyType( world, scene.sleeper3 ) == b3_dynamicBody );
	ENSURE( GetProxyType( world, scene.sleeper4 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( b3HasTreeMoved( world->broadPhase.trees + B3_SLEEPING_PROXY_TYPE ) == false );
	b3ValidateBroadPhase( &world->broadPhase );

	b3World_Step( scene.worldId, timeStep, 4 );
	b3ValidateBroadPhase( &world->broadPhase );

	b3Contact* contact1 = FindContact( world, scene.sleeper1, scene.staticId );
	ENSURE( contact1 != NULL );
	ENSURE( contact1->setIndex == b3_awakeSet );
	ENSURE( b3GetBodyFullId( world, scene.sleeper1 )->contactCount == 1 );

	b3Contact* contact2 = FindContact( world, scene.sleeper2, scene.sleeper4 );
	ENSURE( contact2 != NULL );
	ENSURE( contact2->setIndex == b3_awakeSet );
	ENSURE( b3GetBodyFullId( world, scene.sleeper2 )->contactCount == 1 );
	ENSURE( b3GetBodyFullId( world, scene.sleeper4 )->contactCount == 1 );

	b3Contact* contact3 = FindContact( world, scene.sleeper3, scene.kinematicId );
	ENSURE( contact3 != NULL );
	ENSURE( contact3->setIndex == b3_awakeSet );
	ENSURE( b3GetBodyFullId( world, scene.sleeper3 )->contactCount == 1 );

	ENSURE( b3Body_IsAwake( scene.sleeper1 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper2 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper3 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper4 ) );
	ENSURE( GetProxyType( world, scene.sleeper4 ) == b3_dynamicBody );

	b3DestroyWorld( scene.worldId );
	return 0;
}

// The same moves followed by b3Body_SetAwake( false ) before the step. Each box is asleep again at
// once, its marked leaf parked in the awake tree. The step finds the same pairs the single tree
// gave, in the same sets: disabled for the two sleeping pairs, awake for the kinematic pair, whose
// begin touch wakes sleeper 3. The two boxes that stay asleep are then in the sleeping tree,
// unmarked.
static int MovedSleeperParkedCase( void )
{
	PairScene scene;
	CreatePairScene( &scene );
	b3World* world = scene.world;
	float timeStep = 1.0f / 60.0f;

	b3World_Step( scene.worldId, timeStep, 4 );
	ENSURE( b3World_GetAwakeBodyCount( scene.worldId ) == 1 );

	MovePairSleepers( &scene );
	b3Body_SetAwake( scene.sleeper1, false );
	b3Body_SetAwake( scene.sleeper2, false );
	b3Body_SetAwake( scene.sleeper3, false );
	ENSURE( b3World_GetAwakeBodyCount( scene.worldId ) == 1 );
	ENSURE( b3Body_IsAwake( scene.sleeper1 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper1 ) == b3_dynamicBody );
	ENSURE( IsProxyMarked( world, scene.sleeper1 ) );
	ENSURE( GetProxyType( world, scene.sleeper2 ) == b3_dynamicBody );
	ENSURE( GetProxyType( world, scene.sleeper3 ) == b3_dynamicBody );
	ENSURE( GetProxyType( world, scene.sleeper4 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( world->broadPhase.migrationQueue.count == 3 );
	ENSURE( b3HasTreeMoved( world->broadPhase.trees + B3_SLEEPING_PROXY_TYPE ) == false );
	b3ValidateBroadPhase( &world->broadPhase );

	b3World_Step( scene.worldId, timeStep, 4 );
	b3ValidateBroadPhase( &world->broadPhase );
	ENSURE( world->broadPhase.migrationQueue.count == 0 );

	b3Contact* contact1 = FindContact( world, scene.sleeper1, scene.staticId );
	ENSURE( contact1 != NULL );
	ENSURE( contact1->setIndex == b3_disabledSet );
	ENSURE( b3GetBodyFullId( world, scene.sleeper1 )->contactCount == 1 );

	b3Contact* contact2 = FindContact( world, scene.sleeper2, scene.sleeper4 );
	ENSURE( contact2 != NULL );
	ENSURE( contact2->setIndex == b3_disabledSet );
	ENSURE( b3GetBodyFullId( world, scene.sleeper2 )->contactCount == 1 );
	ENSURE( b3GetBodyFullId( world, scene.sleeper4 )->contactCount == 1 );

	b3Contact* contact3 = FindContact( world, scene.sleeper3, scene.kinematicId );
	ENSURE( contact3 != NULL );
	ENSURE( contact3->setIndex == b3_awakeSet );
	ENSURE( b3GetBodyFullId( world, scene.sleeper3 )->contactCount == 1 );

	ENSURE( b3Body_IsAwake( scene.sleeper3 ) );
	ENSURE( b3Body_IsAwake( scene.sleeper1 ) == false );
	ENSURE( b3Body_IsAwake( scene.sleeper2 ) == false );
	ENSURE( b3Body_IsAwake( scene.sleeper4 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper1 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsProxyMarked( world, scene.sleeper1 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper2 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsProxyMarked( world, scene.sleeper2 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper3 ) == b3_dynamicBody );
	ENSURE( b3HasTreeMoved( world->broadPhase.trees + B3_SLEEPING_PROXY_TYPE ) == false );

	b3DestroyWorld( scene.worldId );
	return 0;
}

// A kinematic box moved by b3Body_SetTransform onto a sleeping box wakes, and the pass of the
// sleeping tree against the kinematic tree finds the pair from the kinematic side; its begin touch
// wakes the sleeping box. A static box created over a sleeping box is found from the static side
// and the contact goes to the disabled set.
static int MovedNeighbourCase( void )
{
	PairScene scene;
	CreatePairScene( &scene );
	b3World* world = scene.world;
	float timeStep = 1.0f / 60.0f;

	b3World_Step( scene.worldId, timeStep, 4 );
	int sleepSteps = 0;
	while ( b3Body_IsAwake( scene.kinematicId ) && sleepSteps < 600 )
	{
		b3World_Step( scene.worldId, timeStep, 4 );
		sleepSteps += 1;
	}
	ENSURE( b3Body_IsAwake( scene.kinematicId ) == false );
	ENSURE( b3World_GetAwakeBodyCount( scene.worldId ) == 0 );

	float lift = 1.8f * 0.5f;
	b3Body_SetTransform( scene.kinematicId, (b3Pos){ 40.0, lift, 0.0 }, b3Quat_identity );
	ENSURE( b3Body_IsAwake( scene.kinematicId ) );
	ENSURE( GetProxyType( world, scene.kinematicId ) == b3_kinematicBody );
	ENSURE( b3Body_IsAwake( scene.sleeper4 ) == false );

	b3BodyId staticOver = CreateBox( scene.worldId, b3_staticBody, (b3Pos){ -40.0, lift, 0.0 }, 0.5f, true );

	b3World_Step( scene.worldId, timeStep, 4 );
	b3ValidateBroadPhase( &world->broadPhase );

	b3Contact* kinematicContact = FindContact( world, scene.sleeper4, scene.kinematicId );
	ENSURE( kinematicContact != NULL );
	ENSURE( kinematicContact->setIndex == b3_awakeSet );
	ENSURE( b3Body_IsAwake( scene.sleeper4 ) );

	b3Contact* staticContact = FindContact( world, scene.sleeper1, staticOver );
	ENSURE( staticContact != NULL );
	ENSURE( staticContact->setIndex == b3_disabledSet );
	ENSURE( b3Body_IsAwake( scene.sleeper1 ) == false );
	ENSURE( GetProxyType( world, scene.sleeper1 ) == B3_SLEEPING_PROXY_TYPE );

	b3DestroyWorld( scene.worldId );
	return 0;
}

int SleepingTreePairTest( void )
{
	RUN_SUBTEST( CreatedAsleepCase );
	RUN_SUBTEST( MovedSleeperWakesCase );
	RUN_SUBTEST( MovedSleeperParkedCase );
	RUN_SUBTEST( MovedNeighbourCase );
	return 0;
}

int SleepingProxyTreeTest( void )
{
	RUN_SUBTEST( SleepingProxyTreeCase );
	RUN_SUBTEST( SleepingTreeCountersCase );
	return 0;
}

// Sleep and wake move proxies. A pile on static ground beside a kinematic box sleeps: every dynamic
// shape is then in the sleeping tree and the kinematic shape stays in the kinematic tree. Waking the
// pile and the kinematic box puts the dynamic shapes back in the awake tree. The sequence hashes the
// same with the tree layout perturbed before every step.

#define MIGRATION_PILE_COUNT 6
#define MIGRATION_STEP_CAP 600
#define MIGRATION_BODY_COUNT ( MIGRATION_PILE_COUNT + 2 )

typedef struct MigrationRun
{
	uint32_t hashes[MIGRATION_STEP_CAP + 1];
	int hashCount;
} MigrationRun;

static int RunMigrationScene( bool perturbEachStep, MigrationRun* run )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	worldDef.enableSleep = true;
	b3WorldId worldId = b3CreateWorld( &worldDef );
	b3World* world = b3GetWorldFromId( worldId );

	float extent = 0.5f;
	b3BodyId bodies[MIGRATION_BODY_COUNT];
	b3BodyId* pile = bodies;
	for ( int i = 0; i < MIGRATION_PILE_COUNT; ++i )
	{
		pile[i] = CreateBox( worldId, b3_dynamicBody, (b3Pos){ 0.0, extent + 2.0f * extent * i, 0.0 }, extent, true );
	}

	// Resting against the side of the bottom box with no velocity
	b3BodyId kinematicId = CreateBox( worldId, b3_kinematicBody, (b3Pos){ 2.0f * extent, extent, 0.0 }, extent, true );
	bodies[MIGRATION_PILE_COUNT] = kinematicId;
	bodies[MIGRATION_PILE_COUNT + 1] = CreateGround( worldId );

	run->hashCount = 0;
	bool allAsleep = false;
	for ( int step = 0; step < MIGRATION_STEP_CAP && allAsleep == false; ++step )
	{
		if ( perturbEachStep )
		{
			PerturbTreeLayout( worldId );
		}

		b3World_Step( worldId, 1.0f / 60.0f, 4 );
		b3ValidateBroadPhase( &world->broadPhase );
		run->hashes[run->hashCount++] = HashWorldState( worldId, bodies, MIGRATION_BODY_COUNT );

		allAsleep = b3Body_IsAwake( kinematicId ) == false;
		for ( int i = 0; i < MIGRATION_PILE_COUNT; ++i )
		{
			allAsleep = allAsleep && b3Body_IsAwake( pile[i] ) == false;
		}
	}

	ENSURE( allAsleep );
	ENSURE( b3World_GetAwakeBodyCount( worldId ) == 0 );

	for ( int i = 0; i < MIGRATION_PILE_COUNT; ++i )
	{
		ENSURE( GetProxyType( world, pile[i] ) == B3_SLEEPING_PROXY_TYPE );
	}
	ENSURE( GetProxyType( world, kinematicId ) == b3_kinematicBody );

	b3Body_SetAwake( pile[0], true );
	b3Body_SetAwake( kinematicId, true );
	b3ValidateBroadPhase( &world->broadPhase );

	for ( int i = 0; i < MIGRATION_PILE_COUNT; ++i )
	{
		ENSURE( b3Body_IsAwake( pile[i] ) );
		ENSURE( GetProxyType( world, pile[i] ) == b3_dynamicBody );
	}
	ENSURE( b3Body_IsAwake( kinematicId ) );
	ENSURE( GetProxyType( world, kinematicId ) == b3_kinematicBody );
	ENSURE( b3World_GetAwakeBodyCount( worldId ) == MIGRATION_PILE_COUNT + 1 );

	run->hashes[run->hashCount++] = HashWorldState( worldId, bodies, MIGRATION_BODY_COUNT );

	b3DestroyWorld( worldId );
	return 0;
}

int AwakeTreeMigrationTest( void )
{
	static MigrationRun runs[2];
	memset( runs, 0, sizeof( runs ) );

	ENSURE( RunMigrationScene( false, runs + 0 ) == 0 );
	ENSURE( RunMigrationScene( true, runs + 1 ) == 0 );

	printf( "  slept after %d steps\n", runs[0].hashCount - 1 );
	ENSURE( runs[0].hashCount == runs[1].hashCount );
	for ( int i = 0; i < runs[0].hashCount; ++i )
	{
		ENSURE( runs[0].hashes[i] == runs[1].hashes[i] );
	}

	return 0;
}

// Whether a shape's leaf is marked moved
static bool IsShapeMarked( b3World* world, const b3Shape* shape )
{
	const b3DynamicTree* tree = world->broadPhase.trees + B3_PROXY_TYPE( shape->proxyKey );
	int proxyId = B3_PROXY_ID( shape->proxyKey );
	return b3IsNodeMoved( tree->nodes + tree->proxies[proxyId].node );
}

// The writers of a sleeping body's proxy that wake nothing. A shape created on a sleeping body is
// parked in the awake tree, marked, and settles into the sleeping tree at the next step; a filter
// change that recreates the proxy does the same when no contact is destroyed, and follows the wake
// when one is.
int SleepingWriterTest( void )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	worldDef.enableSleep = true;
	b3WorldId worldId = b3CreateWorld( &worldDef );
	b3World* world = b3GetWorldFromId( worldId );

	float extent = 0.5f;
	CreateGround( worldId );
	b3BodyId dropped = CreateBox( worldId, b3_dynamicBody, (b3Pos){ 0.0, 1.0, 0.0 }, extent, true );
	b3BodyId sleeper = CreateBox( worldId, b3_dynamicBody, (b3Pos){ 10.0, 5.0, 0.0 }, extent, false );

	int step = 0;
	while ( b3Body_IsAwake( dropped ) && step < 600 )
	{
		b3World_Step( worldId, 1.0f / 60.0f, 4 );
		step += 1;
	}
	ENSURE( b3Body_IsAwake( dropped ) == false );
	ENSURE( GetProxyType( world, dropped ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( b3GetBodyFullId( world, dropped )->contactCount == 1 );
	ENSURE( GetProxyType( world, sleeper ) == B3_SLEEPING_PROXY_TYPE );

	// Shape creation on a sleeping body: parked, then settled by a step
	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3BoxHull box = b3MakeBoxHull( extent, extent, extent );
	b3ShapeId sleeperShape = b3CreateHullShape( sleeper, &shapeDef, &box.base );
	b3Shape* shape = b3Array_Get( world->shapes, sleeperShape.index1 - 1 );
	ENSURE( B3_PROXY_TYPE( shape->proxyKey ) == b3_dynamicBody );
	ENSURE( IsShapeMarked( world, shape ) );
	ENSURE( b3Body_IsAwake( sleeper ) == false );
	ENSURE( world->broadPhase.migrationQueue.count == 1 );
	b3ValidateBroadPhase( &world->broadPhase );

	b3World_Step( worldId, 1.0f / 60.0f, 4 );
	ENSURE( B3_PROXY_TYPE( shape->proxyKey ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsShapeMarked( world, shape ) == false );
	ENSURE( b3Body_IsAwake( sleeper ) == false );
	ENSURE( world->broadPhase.migrationQueue.count == 0 );
	b3ValidateBroadPhase( &world->broadPhase );

	// A filter change recreates the proxy. Nothing touches the sleeper, so nothing wakes; the
	// marked leaf is parked and settles at the next step.
	b3Filter filter = b3Shape_GetFilter( sleeperShape );
	filter.maskBits = filter.maskBits ^ 1;
	b3Shape_SetFilter( sleeperShape, filter, true );
	ENSURE( B3_PROXY_TYPE( shape->proxyKey ) == b3_dynamicBody );
	ENSURE( IsShapeMarked( world, shape ) );
	ENSURE( b3Body_IsAwake( sleeper ) == false );
	b3ValidateBroadPhase( &world->broadPhase );

	b3World_Step( worldId, 1.0f / 60.0f, 4 );
	ENSURE( B3_PROXY_TYPE( shape->proxyKey ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsShapeMarked( world, shape ) == false );
	ENSURE( b3Body_IsAwake( sleeper ) == false );
	b3ValidateBroadPhase( &world->broadPhase );

	// A category change recreates the proxy through the move path. Same outcome.
	filter = b3Shape_GetFilter( sleeperShape );
	filter.categoryBits = filter.categoryBits ^ 2;
	b3Shape_SetFilter( sleeperShape, filter, true );
	ENSURE( B3_PROXY_TYPE( shape->proxyKey ) == b3_dynamicBody );
	ENSURE( IsShapeMarked( world, shape ) );
	ENSURE( b3Body_IsAwake( sleeper ) == false );
	b3ValidateBroadPhase( &world->broadPhase );

	b3World_Step( worldId, 1.0f / 60.0f, 4 );
	ENSURE( B3_PROXY_TYPE( shape->proxyKey ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( IsShapeMarked( world, shape ) == false );
	ENSURE( b3Body_IsAwake( sleeper ) == false );
	b3ValidateBroadPhase( &world->broadPhase );

	// The same change on the dropped box destroys its touching ground contact, which wakes it
	b3ShapeId droppedShapes[1];
	ENSURE( b3Body_GetShapes( dropped, droppedShapes, 1 ) == 1 );
	filter = b3Shape_GetFilter( droppedShapes[0] );
	filter.maskBits = filter.maskBits ^ 1;
	b3Shape_SetFilter( droppedShapes[0], filter, true );
	ENSURE( GetProxyType( world, dropped ) == b3_dynamicBody );
	ENSURE( b3Body_IsAwake( dropped ) );
	b3ValidateBroadPhase( &world->broadPhase );

	b3DestroyWorld( worldId );
	return 0;
}

// A begin touch inside the collide wakes a sleeping box while the rebuild task is out. The
// migration waits for the join and the box is in the awake tree by the end of the step. The
// scene hashes the same with one worker and with four.

#define REBUILD_STEP_CAP 120

typedef struct RebuildRun
{
	uint32_t hashes[REBUILD_STEP_CAP];
	int stepCount;
	int wakeStep;
} RebuildRun;

static int RunWakeUnderRebuild( int workerCount, RebuildRun* run )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = workerCount;
	worldDef.enableSleep = true;
	b3WorldId worldId = b3CreateWorld( &worldDef );
	b3World* world = b3GetWorldFromId( worldId );

	float extent = 0.5f;
	CreateGround( worldId );

	// Asleep on the ground, with a box dropped onto it from just above
	b3BodyId sleeper = CreateBox( worldId, b3_dynamicBody, (b3Pos){ 0.0, extent, 0.0 }, extent, false );
	b3BodyId dropper = CreateBox( worldId, b3_dynamicBody, (b3Pos){ 0.0, 3.2f * extent, 0.0 }, extent, true );

	// Moved before every step so the awake tree's root is marked and the pair update enqueues the
	// rebuild every step. It never sleeps and has no gravity, so its velocity stays zero.
	b3BodyDef moverDef = b3DefaultBodyDef();
	moverDef.type = b3_dynamicBody;
	moverDef.position = (b3Pos){ 50.0, 10.0, 0.0 };
	moverDef.enableSleep = false;
	moverDef.gravityScale = 0.0f;
	b3BodyId mover = b3CreateBody( worldId, &moverDef );
	b3ShapeDef shapeDef = b3DefaultShapeDef();
	b3BoxHull box = b3MakeBoxHull( extent, extent, extent );
	b3CreateHullShape( mover, &shapeDef, &box.base );

	b3BodyId bodies[3] = { sleeper, dropper, mover };

	run->stepCount = 0;
	run->wakeStep = -1;
	for ( int step = 0; step < REBUILD_STEP_CAP; ++step )
	{
		// More than the extent plus margin along x, so the new box leaves the fat box
		b3Pos position = { 50.0 + 2.0 * step, 10.0, 0.0 };
		b3Body_SetTransform( mover, position, b3Quat_identity );
		ENSURE( b3HasTreeMoved( world->broadPhase.trees + b3_dynamicBody ) );

		b3World_Step( worldId, 1.0f / 60.0f, 4 );
		b3ValidateBroadPhase( &world->broadPhase );

		bool awake = b3Body_IsAwake( sleeper );
		ENSURE( GetProxyType( world, sleeper ) == ( awake ? b3_dynamicBody : B3_SLEEPING_PROXY_TYPE ) );

		run->hashes[run->stepCount++] = HashWorldState( worldId, bodies, 3 );

		if ( awake && run->wakeStep == -1 )
		{
			run->wakeStep = step;
		}

		if ( run->wakeStep != -1 && step >= run->wakeStep + 5 )
		{
			break;
		}
	}

	ENSURE( run->wakeStep != -1 );

	b3DestroyWorld( worldId );
	return 0;
}

int WakeUnderRebuildTest( void )
{
	RebuildRun runs[2] = { 0 };
	ENSURE( RunWakeUnderRebuild( 4, runs + 0 ) == 0 );
	ENSURE( RunWakeUnderRebuild( 1, runs + 1 ) == 0 );

	printf( "  woke on step %d\n", runs[0].wakeStep );
	ENSURE( runs[0].wakeStep == runs[1].wakeStep );
	ENSURE( runs[0].stepCount == runs[1].stepCount );
	for ( int i = 0; i < runs[0].stepCount; ++i )
	{
		ENSURE( runs[0].hashes[i] == runs[1].hashes[i] );
	}

	return 0;
}

// Wakes from a query callback. The walk is open, so the migration waits for the query to return.

typedef struct CallbackScene
{
	b3WorldId worldId;
	b3World* world;
	b3BodyId bodyA;
	b3BodyId bodyB;
	b3BodyId newBody;
	int callbackCount;
	b3BodyType keys[2];
} CallbackScene;

static void CreateCallbackScene( CallbackScene* scene )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	worldDef.enableSleep = true;
	worldDef.gravity = b3Vec3_zero;
	scene->worldId = b3CreateWorld( &worldDef );
	scene->world = b3GetWorldFromId( scene->worldId );

	float extent = 0.5f;
	scene->bodyA = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ 0.0, 0.0, 0.0 }, extent, false );
	scene->bodyB = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ 5.0, 0.0, 0.0 }, extent, false );
	scene->newBody = b3_nullBodyId;
	scene->callbackCount = 0;

	// One step with nothing awake settles the shapes created asleep into the sleeping tree
	b3World_Step( scene->worldId, 1.0f / 60.0f, 4 );
}

static b3AABB CallbackQueryBox( void )
{
	return (b3AABB){ { -2.0f, -2.0f, -2.0f }, { 7.0f, 2.0f, 2.0f } };
}

// Whichever shape comes first, wake A and read its key; read it again on the second callback
static bool WakeCallback( b3ShapeId shapeId, void* context )
{
	MAYBE_UNUSED( shapeId );
	CallbackScene* scene = context;
	if ( scene->callbackCount == 0 )
	{
		b3Body_SetAwake( scene->bodyA, true );
	}

	if ( scene->callbackCount < 2 )
	{
		scene->keys[scene->callbackCount] = GetProxyType( scene->world, scene->bodyA );
	}

	scene->callbackCount += 1;
	return true;
}

static int WakeFromQueryCase( void )
{
	CallbackScene scene;
	CreateCallbackScene( &scene );

	b3QueryFilter filter = b3DefaultQueryFilter();
	b3World_OverlapAABB( scene.worldId, CallbackQueryBox(), filter, WakeCallback, &scene );

	// Queued while the walk was open, including between the two callbacks
	ENSURE( scene.callbackCount == 2 );
	ENSURE( scene.keys[0] == B3_SLEEPING_PROXY_TYPE );
	ENSURE( scene.keys[1] == B3_SLEEPING_PROXY_TYPE );

	// Placed at the exit
	ENSURE( GetProxyType( scene.world, scene.bodyA ) == b3_dynamicBody );
	ENSURE( b3Body_IsAwake( scene.bodyA ) );
	b3ValidateBroadPhase( &scene.world->broadPhase );

	b3BodyId bodies[2] = { scene.bodyA, scene.bodyB };
	uint32_t hashAfterWake = HashWorldState( scene.worldId, bodies, 2 );
	b3World_Step( scene.worldId, 1.0f / 60.0f, 4 );
	uint32_t hashAfterStep = HashWorldState( scene.worldId, bodies, 2 );
	b3DestroyWorld( scene.worldId );

	// The same wake between calls gives the same state
	CallbackScene plain;
	CreateCallbackScene( &plain );
	b3Body_SetAwake( plain.bodyA, true );
	b3BodyId plainBodies[2] = { plain.bodyA, plain.bodyB };
	ENSURE( HashWorldState( plain.worldId, plainBodies, 2 ) == hashAfterWake );
	b3World_Step( plain.worldId, 1.0f / 60.0f, 4 );
	ENSURE( HashWorldState( plain.worldId, plainBodies, 2 ) == hashAfterStep );
	b3DestroyWorld( plain.worldId );

	return 0;
}

// Wake A, destroy it and create a new awake body in its slot, all inside the walk
static bool DestroyCallback( b3ShapeId shapeId, void* context )
{
	MAYBE_UNUSED( shapeId );
	CallbackScene* scene = context;
	if ( scene->callbackCount == 0 )
	{
		b3Body_SetAwake( scene->bodyA, true );
		b3DestroyBody( scene->bodyA );
		scene->newBody = CreateBox( scene->worldId, b3_dynamicBody, (b3Pos){ 0.0, 20.0, 0.0 }, 0.5f, true );
	}

	scene->callbackCount += 1;
	return true;
}

static int DestroyFromQueryCase( void )
{
	CallbackScene scene;
	CreateCallbackScene( &scene );

	b3QueryFilter filter = b3DefaultQueryFilter();
	b3World_OverlapAABB( scene.worldId, CallbackQueryBox(), filter, DestroyCallback, &scene );

	// The pool popped the freed slot, so the drain sees the new body there
	ENSURE( scene.callbackCount >= 1 );
	ENSURE( scene.newBody.index1 == scene.bodyA.index1 );
	ENSURE( GetProxyType( scene.world, scene.newBody ) == b3_dynamicBody );
	b3ValidateBroadPhase( &scene.world->broadPhase );

	b3World_Step( scene.worldId, 1.0f / 60.0f, 4 );
	b3ValidateBroadPhase( &scene.world->broadPhase );

	b3DestroyWorld( scene.worldId );
	return 0;
}

// Wake A and make it kinematic inside the walk, which recreates its proxies
static bool ReplaceCallback( b3ShapeId shapeId, void* context )
{
	MAYBE_UNUSED( shapeId );
	CallbackScene* scene = context;
	if ( scene->callbackCount == 0 )
	{
		b3Body_SetAwake( scene->bodyA, true );
		b3Body_SetType( scene->bodyA, b3_kinematicBody );
	}

	scene->callbackCount += 1;
	return true;
}

static int ReplaceFromQueryCase( void )
{
	CallbackScene scene;
	CreateCallbackScene( &scene );

	b3QueryFilter filter = b3DefaultQueryFilter();
	b3World_OverlapAABB( scene.worldId, CallbackQueryBox(), filter, ReplaceCallback, &scene );

	// The drain finds the kinematic proxy where a kinematic body's belongs and leaves it
	ENSURE( scene.callbackCount >= 1 );
	ENSURE( GetProxyType( scene.world, scene.bodyA ) == b3_kinematicBody );
	b3ValidateBroadPhase( &scene.world->broadPhase );

	b3DestroyWorld( scene.worldId );
	return 0;
}

int WakeFromCallbackTest( void )
{
	RUN_SUBTEST( WakeFromQueryCase );
	RUN_SUBTEST( DestroyFromQueryCase );
	RUN_SUBTEST( ReplaceFromQueryCase );
	return 0;
}

// An explosion over sleeping bodies wakes the same set with the same velocities as over awake ones.
// This is the explode scene of the tree order tests: one body with three shapes, ten single-shape
// bodies inside the radius and one in the falloff. The world is never stepped. This test and its
// helpers use the public API and the awake set's body list only, so it also runs at the base.

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

#define EXPLODE_SINGLE_COUNT 10
#define EXPLODE_MULTI_SHAPE_COUNT 3
#define EXPLODE_BODY_COUNT ( EXPLODE_SINGLE_COUNT + 2 )

typedef struct ExplodeScene
{
	b3ExplosionDef def;
	b3Vec3 singleOffsets[EXPLODE_SINGLE_COUNT];
	float singleRadius;
	b3Vec3 multiOffset;
	b3Vec3 multiShapeOffsets[EXPLODE_MULTI_SHAPE_COUNT];
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

	scene->multiOffset = (b3Vec3){ 0.4f, 0.3f, -0.2f };
	scene->multiShapeOffsets[0] = (b3Vec3){ 1.5f, 0.2f, -0.3f };
	scene->multiShapeOffsets[1] = (b3Vec3){ -1.1f, 1.2f, 0.4f };
	scene->multiShapeOffsets[2] = (b3Vec3){ 0.1f, -1.3f, -1.4f };

	// Centre at radius + 0.5, surface at radius + 0.3 with the single radius
	scene->farOffset = (b3Vec3){ 0.6f * ( scene->def.radius + 0.5f ), 0.8f * ( scene->def.radius + 0.5f ), 0.0f };
}

typedef struct ExplodeRun
{
	b3BodyId bodies[EXPLODE_BODY_COUNT];
	int awakeOrder[EXPLODE_BODY_COUNT];
	int awakeCount;

	// The lowest shape id of each body, indexed by body id
	int lowestShapeId[EXPLODE_BODY_COUNT];
	b3Vec3 linearVelocity[EXPLODE_BODY_COUNT];
	b3Vec3 angularVelocity[EXPLODE_BODY_COUNT];
} ExplodeRun;

// The lowest shape id of a body, through the public shape list
static int LowestShapeId( b3BodyId bodyId )
{
	b3ShapeId shapes[EXPLODE_MULTI_SHAPE_COUNT];
	int count = b3Body_GetShapes( bodyId, shapes, EXPLODE_MULTI_SHAPE_COUNT );
	int lowest = shapes[0].index1;
	for ( int i = 1; i < count; ++i )
	{
		lowest = b3MinInt( lowest, shapes[i].index1 );
	}
	return lowest;
}

static int RunExplodeScene( const ExplodeScene* scene, bool isAwake, ExplodeRun* run )
{
	const b3ExplosionDef* def = &scene->def;

	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.gravity = b3Vec3_zero;
	worldDef.enableSleep = true;
	worldDef.workerCount = 1;
	b3WorldId worldId = b3CreateWorld( &worldDef );

	int bodyCount = 0;
	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		bodyDef.type = b3_dynamicBody;
		bodyDef.position = b3OffsetPos( def->position, scene->multiOffset );
		bodyDef.isAwake = isAwake;
		b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
		b3ShapeDef shapeDef = b3DefaultShapeDef();
		for ( int i = 0; i < EXPLODE_MULTI_SHAPE_COUNT; ++i )
		{
			b3Sphere sphere = { scene->multiShapeOffsets[i], scene->singleRadius };
			b3CreateSphereShape( bodyId, &shapeDef, &sphere );
		}
		run->bodies[bodyCount++] = bodyId;
	}

	for ( int i = 0; i < EXPLODE_SINGLE_COUNT; ++i )
	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		bodyDef.type = b3_dynamicBody;
		bodyDef.position = b3OffsetPos( def->position, scene->singleOffsets[i] );
		bodyDef.isAwake = isAwake;
		b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
		b3ShapeDef shapeDef = b3DefaultShapeDef();
		b3Sphere sphere = { b3Vec3_zero, scene->singleRadius };
		b3CreateSphereShape( bodyId, &shapeDef, &sphere );
		run->bodies[bodyCount++] = bodyId;
	}

	{
		b3BodyDef bodyDef = b3DefaultBodyDef();
		bodyDef.type = b3_dynamicBody;
		bodyDef.position = b3OffsetPos( def->position, scene->farOffset );
		bodyDef.isAwake = isAwake;
		b3BodyId bodyId = b3CreateBody( worldId, &bodyDef );
		b3ShapeDef shapeDef = b3DefaultShapeDef();
		b3Sphere sphere = { b3Vec3_zero, scene->singleRadius };
		b3CreateSphereShape( bodyId, &shapeDef, &sphere );
		run->bodies[bodyCount++] = bodyId;
	}

	ENSURE( bodyCount == EXPLODE_BODY_COUNT );
	ENSURE( b3World_GetAwakeBodyCount( worldId ) == ( isAwake ? EXPLODE_BODY_COUNT : 0 ) );

	b3World_Explode( worldId, def );

	for ( int i = 0; i < EXPLODE_BODY_COUNT; ++i )
	{
		ENSURE( b3Body_IsAwake( run->bodies[i] ) );
		run->linearVelocity[i] = b3Body_GetLinearVelocity( run->bodies[i] );
		run->angularVelocity[i] = b3Body_GetAngularVelocity( run->bodies[i] );
		ENSURE( b3LengthSquared( run->linearVelocity[i] ) > 0.0f );

		int bodyIndex = run->bodies[i].index1 - 1;
		ENSURE( 0 <= bodyIndex && bodyIndex < EXPLODE_BODY_COUNT );
		run->lowestShapeId[bodyIndex] = LowestShapeId( run->bodies[i] );
	}

	b3World* world = b3GetWorldFromId( worldId );
	b3SolverSet* awakeSet = b3Array_Get( world->solverSets, b3_awakeSet );
	ENSURE( awakeSet->bodySims.count == EXPLODE_BODY_COUNT );
	run->awakeCount = awakeSet->bodySims.count;
	for ( int i = 0; i < run->awakeCount; ++i )
	{
		run->awakeOrder[i] = awakeSet->bodySims.data[i].bodyId;
	}

	b3DestroyWorld( worldId );
	return 0;
}

int SleepingExplodeTest( void )
{
	ExplodeScene scene;
	RandomState random = { 0xE8D10DEu };
	PlanExplodeScene( &scene, &random );

	static ExplodeRun runs[2];
	memset( runs, 0, sizeof( runs ) );
	ENSURE( RunExplodeScene( &scene, false, runs + 0 ) == 0 );
	ENSURE( RunExplodeScene( &scene, true, runs + 1 ) == 0 );

	ENSURE( memcmp( runs[0].linearVelocity, runs[1].linearVelocity, sizeof( runs[0].linearVelocity ) ) == 0 );
	ENSURE( memcmp( runs[0].angularVelocity, runs[1].angularVelocity, sizeof( runs[0].angularVelocity ) ) == 0 );

	// The sleeping run wakes bodies in ascending shape id, so its awake set is ordered by each
	// body's lowest shape id
	for ( int i = 1; i < runs[0].awakeCount; ++i )
	{
		int previous = runs[0].awakeOrder[i - 1];
		int current = runs[0].awakeOrder[i];
		ENSURE( runs[0].lowestShapeId[previous] < runs[0].lowestShapeId[current] );
	}

	uint32_t hash = B3_HASH_INIT;
	hash = b3Hash( hash, (uint8_t*)runs[0].awakeOrder, runs[0].awakeCount * (int)sizeof( int ) );
	hash = b3Hash( hash, (uint8_t*)runs[0].linearVelocity, sizeof( runs[0].linearVelocity ) );
	hash = b3Hash( hash, (uint8_t*)runs[0].angularVelocity, sizeof( runs[0].angularVelocity ) );
	hash = b3Hash( hash, (uint8_t*)runs[1].linearVelocity, sizeof( runs[1].linearVelocity ) );
	hash = b3Hash( hash, (uint8_t*)runs[1].angularVelocity, sizeof( runs[1].angularVelocity ) );
	printf( "  sleeping explode hash 0x%08X\n", hash );

	return 0;
}

// The sleeping tree is kept shallow. Moves into a nested sequence of boxes, each enclosing every
// box before it, deepen one path per insert. At the tree level the full build brings the tall tree
// under the threshold; at the world level the drain's inserts are tested, so the tree is under the
// threshold after every step, and the pairs are still found.

#define HEIGHT_PROXY_COUNT ( B3_SLEEPING_TREE_MAX_HEIGHT + 16 )

// Box i of the initial row: small, disjoint, centers evenly spaced along x
static b3AABB RowBox( int index )
{
	float x = 4.0f * index;
	return (b3AABB){ { x - 0.5f, -0.5f, -0.5f }, { x + 0.5f, 0.5f, 0.5f } };
}

// Box i of the nested arrangement: encloses every nested box before it
static b3AABB NestedBox( int index )
{
	float extent = 1.0f + 0.5f * index;
	return (b3AABB){ { -extent, -extent, -extent }, { extent, extent, extent } };
}

static int SleepingTreeHeightTreeCase( void )
{
	b3DynamicTree tree = b3DynamicTree_Create( 16 );

	int proxyIds[HEIGHT_PROXY_COUNT];
	for ( int i = 0; i < HEIGHT_PROXY_COUNT; ++i )
	{
		proxyIds[i] = b3DynamicTree_CreateProxy( &tree, RowBox( i ), B3_DEFAULT_CATEGORY_BITS, (uint64_t)i );
	}

	for ( int i = 0; i < HEIGHT_PROXY_COUNT; ++i )
	{
		b3DynamicTree_MoveProxy( &tree, proxyIds[i], NestedBox( i ) );
	}

	b3DynamicTree_Validate( &tree );
	int tallHeight = b3DynamicTree_GetHeight( &tree );
	printf( "  tree level: height %d after the moves, threshold %d\n", tallHeight, B3_SLEEPING_TREE_MAX_HEIGHT );
	ENSURE( tallHeight > B3_SLEEPING_TREE_MAX_HEIGHT );

	b3DynamicTree_Rebuild( &tree, true );
	b3DynamicTree_Validate( &tree );
	int builtHeight = b3DynamicTree_GetHeight( &tree );
	printf( "  tree level: height %d after the full build\n", builtHeight );
	ENSURE( builtHeight <= B3_SLEEPING_TREE_MAX_HEIGHT );

	b3DynamicTree_Destroy( &tree );
	return 0;
}

// Every body created asleep, apart along x, so the first step settles each into the sleeping
// tree; then, one per step, moved to the origin and put back to sleep at once, so its marked leaf
// waits in the awake tree and the step's drain inserts it into the sleeping tree, where it encloses
// every box before it. The drain tests the height after its inserts, so the tree is under the
// threshold after every step. A static box created over the first body, unmarked, is found through
// the parked leaf's mark before the leaf settles.
static int SleepingTreeHeightWorldCase( void )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	worldDef.enableSleep = true;
	worldDef.gravity = b3Vec3_zero;
	b3WorldId worldId = b3CreateWorld( &worldDef );
	b3World* world = b3GetWorldFromId( worldId );
	b3DynamicTree* tree = world->broadPhase.trees + B3_SLEEPING_PROXY_TYPE;

	// Each body carries a box of its nested size, placed apart along x and away from the origin,
	// asleep, so every move below leaves the fat box and marks the leaf
	b3BodyId bodies[HEIGHT_PROXY_COUNT];
	for ( int i = 0; i < HEIGHT_PROXY_COUNT; ++i )
	{
		b3AABB nested = NestedBox( i );
		float extent = nested.upperBound.x;
		b3Pos position = { 100.0 * ( i + 1 ), 0.0, 0.0 };
		bodies[i] = CreateBox( worldId, b3_dynamicBody, position, extent, false );
	}

	b3World_Step( worldId, 1.0f / 60.0f, 4 );
	b3ValidateBroadPhase( &world->broadPhase );
	ENSURE( b3World_GetAwakeBodyCount( worldId ) == 0 );
	ENSURE( tree->proxyCount == HEIGHT_PROXY_COUNT );

	// A static box over the origin, unmarked, so the pair with each moved body is found only
	// through that body's parked leaf
	b3BodyDef staticDef = b3DefaultBodyDef();
	staticDef.position = (b3Pos){ 0.0, 0.0, 0.0 };
	b3BodyId staticId = b3CreateBody( worldId, &staticDef );
	b3ShapeDef shapeDef = b3DefaultShapeDef();
	shapeDef.invokeContactCreation = false;
	b3BoxHull box = b3MakeBoxHull( 0.5f, 0.5f, 0.5f );
	b3CreateHullShape( staticId, &shapeDef, &box.base );

	int rebuildCount = 0;
	int maxHeight = 0;
	for ( int i = 0; i < HEIGHT_PROXY_COUNT; ++i )
	{
		b3Body_SetTransform( bodies[i], (b3Pos){ 0.0, 0.0, 0.0 }, b3Quat_identity );
		ENSURE( b3Body_IsAwake( bodies[i] ) );
		b3Body_SetAwake( bodies[i], false );
		ENSURE( b3Body_IsAwake( bodies[i] ) == false );
		ENSURE( GetProxyType( world, bodies[i] ) == b3_dynamicBody );
		ENSURE( IsProxyMarked( world, bodies[i] ) );
		ENSURE( tree->proxyCount == HEIGHT_PROXY_COUNT - 1 );
		b3ValidateBroadPhase( &world->broadPhase );

		b3World_Step( worldId, 1.0f / 60.0f, 4 );
		b3ValidateBroadPhase( &world->broadPhase );
		ENSURE( b3Body_IsAwake( bodies[i] ) == false );
		ENSURE( GetProxyType( world, bodies[i] ) == B3_SLEEPING_PROXY_TYPE );
		ENSURE( IsProxyMarked( world, bodies[i] ) == false );
		ENSURE( tree->proxyCount == HEIGHT_PROXY_COUNT );

		b3Contact* contact = FindContact( world, bodies[i], staticId );
		ENSURE( contact != NULL );
		ENSURE( contact->setIndex == b3_disabledSet );

		b3Counters counters = b3World_GetCounters( worldId );
		maxHeight = b3MaxInt( maxHeight, counters.sleepingTreeHeight );
		ENSURE( counters.sleepingTreeHeight <= B3_SLEEPING_TREE_MAX_HEIGHT );
		rebuildCount = counters.sleepingTreeRebuildCount;
	}

	printf( "  world level: %d rebuilds, max height %d\n", rebuildCount, maxHeight );
	ENSURE( rebuildCount > 0 );
	ENSURE( b3HasTreeMoved( tree ) == false );

	b3DestroyWorld( worldId );
	return 0;
}

int SleepingTreeHeightTest( void )
{
	RUN_SUBTEST( SleepingTreeHeightTreeCase );
	RUN_SUBTEST( SleepingTreeHeightWorldCase );
	return 0;
}
