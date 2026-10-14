// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

// Awake and sleeping dynamic tree tests. The broad-phase keeps the proxies of sleeping dynamic
// bodies in their own tree behind proxy code 3. These tests check the tree exists, is searched,
// is filled and emptied by sleep and wake, and is kept shallow.

#include "body.h"
#include "broad_phase.h"
#include "contact.h"
#include "physics_world.h"
#include "shape.h"
#include "test_macros.h"

#include "box3d/box3d.h"
#include "box3d/collision.h"
#include "box3d/math_functions.h"

#include <stdio.h>
#include <string.h>

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

// The proxy code the body's type and set imply, read from its first shape
static b3BodyType GetProxyType( b3World* world, b3BodyId bodyId )
{
	b3Body* body = b3GetBodyFullId( world, bodyId );
	b3Shape* shape = b3Array_Get( world->shapes, body->headShapeId );
	return B3_PROXY_TYPE( shape->proxyKey );
}

// Until sleep moves proxies on its own, place a sleeping body's proxies in the fourth tree from
// the old leaves' boxes, bits and marks. Called between world calls only.
static void PlaceInSleepingTree( b3World* world, b3BodyId bodyId )
{
	b3Body* body = b3GetBodyFullId( world, bodyId );
	int shapeId = body->headShapeId;
	while ( shapeId != B3_NULL_INDEX )
	{
		b3Shape* shape = b3Array_Get( world->shapes, shapeId );
		shapeId = shape->nextShapeId;

		int oldKey = shape->proxyKey;
		b3DynamicTree* tree = world->broadPhase.trees + B3_PROXY_TYPE( oldKey );
		int proxyId = B3_PROXY_ID( oldKey );
		b3AABB box = b3DynamicTree_GetAABB( tree, proxyId );
		uint64_t categoryBits = b3DynamicTree_GetCategoryBits( tree, proxyId );
		bool moved = b3IsNodeMoved( tree->nodes + tree->proxies[proxyId].node );

		b3BroadPhase_DestroyProxy( &world->broadPhase, oldKey );
		shape->proxyKey =
			b3BroadPhase_CreateProxy( &world->broadPhase, B3_SLEEPING_PROXY_TYPE, box, categoryBits, shape->id, moved );
	}
}

// A sleeping box moved by b3Body_SetTransform into a static box, another sleeping box and an awake
// kinematic box finds all three through the sleeping tree's passes, with the contacts in the sets
// the single tree gave: disabled for the two sleeping pairs, awake for the kinematic pair, whose
// begin touch wakes the box.
int SleepingTreePairTest( void )
{
	b3WorldDef worldDef = b3DefaultWorldDef();
	worldDef.workerCount = 1;
	worldDef.enableSleep = true;
	b3WorldId worldId = b3CreateWorld( &worldDef );
	b3World* world = b3GetWorldFromId( worldId );

	float extent = 0.5f;
	b3BodyId staticId = CreateBox( worldId, b3_staticBody, (b3Pos){ 0.0, 0.0, 0.0 }, extent, true );
	b3BodyId kinematicId = CreateBox( worldId, b3_kinematicBody, (b3Pos){ 20.0, 0.0, 0.0 }, extent, true );
	b3BodyId sleeper1 = CreateBox( worldId, b3_dynamicBody, (b3Pos){ -40.0, 0.0, 0.0 }, extent, false );
	b3BodyId sleeper2 = CreateBox( worldId, b3_dynamicBody, (b3Pos){ -60.0, 0.0, 0.0 }, extent, false );
	b3BodyId sleeper3 = CreateBox( worldId, b3_dynamicBody, (b3Pos){ -80.0, 0.0, 0.0 }, extent, false );
	b3BodyId sleeper4 = CreateBox( worldId, b3_dynamicBody, (b3Pos){ 40.0, 0.0, 0.0 }, extent, false );

	ENSURE( b3Body_IsAwake( kinematicId ) );
	ENSURE( b3Body_IsAwake( sleeper1 ) == false );
	ENSURE( b3Body_IsAwake( sleeper2 ) == false );
	ENSURE( b3Body_IsAwake( sleeper3 ) == false );
	ENSURE( b3Body_IsAwake( sleeper4 ) == false );

	PlaceInSleepingTree( world, sleeper1 );
	PlaceInSleepingTree( world, sleeper2 );
	PlaceInSleepingTree( world, sleeper3 );
	PlaceInSleepingTree( world, sleeper4 );
	ENSURE( GetProxyType( world, sleeper1 ) == B3_SLEEPING_PROXY_TYPE );
	ENSURE( GetProxyType( world, sleeper4 ) == B3_SLEEPING_PROXY_TYPE );

	float timeStep = 1.0f / 60.0f;
	b3World_Step( worldId, timeStep, 4 );
	b3ValidateBroadPhase( &world->broadPhase );
	ENSURE( b3World_GetAwakeBodyCount( worldId ) == 1 );

	// Overlap each target by a fifth of the extent along y
	float lift = 1.8f * extent;
	b3Body_SetTransform( sleeper1, (b3Pos){ 0.0, lift, 0.0 }, b3Quat_identity );
	b3Body_SetTransform( sleeper2, (b3Pos){ 40.0, lift, 0.0 }, b3Quat_identity );
	b3Body_SetTransform( sleeper3, (b3Pos){ 20.0, lift, 0.0 }, b3Quat_identity );

	b3World_Step( worldId, timeStep, 4 );
	b3ValidateBroadPhase( &world->broadPhase );

	b3Contact* contact1 = FindContact( world, sleeper1, staticId );
	ENSURE( contact1 != NULL );
	ENSURE( contact1->setIndex == b3_disabledSet );
	ENSURE( b3GetBodyFullId( world, sleeper1 )->contactCount == 1 );

	b3Contact* contact2 = FindContact( world, sleeper2, sleeper4 );
	ENSURE( contact2 != NULL );
	ENSURE( contact2->setIndex == b3_disabledSet );
	ENSURE( b3GetBodyFullId( world, sleeper2 )->contactCount == 1 );
	ENSURE( b3GetBodyFullId( world, sleeper4 )->contactCount == 1 );

	b3Contact* contact3 = FindContact( world, sleeper3, kinematicId );
	ENSURE( contact3 != NULL );
	ENSURE( contact3->setIndex == b3_awakeSet );
	ENSURE( b3GetBodyFullId( world, sleeper3 )->contactCount == 1 );

	ENSURE( b3Body_IsAwake( sleeper3 ) );
	ENSURE( b3Body_IsAwake( sleeper1 ) == false );
	ENSURE( b3Body_IsAwake( sleeper2 ) == false );
	ENSURE( b3Body_IsAwake( sleeper4 ) == false );

	b3DestroyWorld( worldId );
	return 0;
}

int SleepingProxyTreeTest( void )
{
	RUN_SUBTEST( SleepingProxyTreeCase );
	RUN_SUBTEST( SleepingTreeCountersCase );
	return 0;
}
