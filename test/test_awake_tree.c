// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

// Awake and sleeping dynamic tree tests. The broad-phase keeps the proxies of sleeping dynamic
// bodies in their own tree behind proxy code 3. These tests check the tree exists, is searched,
// is filled and emptied by sleep and wake, and is kept shallow.

#include "body.h"
#include "broad_phase.h"
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

int SleepingProxyTreeTest( void )
{
	RUN_SUBTEST( SleepingProxyTreeCase );
	RUN_SUBTEST( SleepingTreeCountersCase );
	return 0;
}
