// SPDX-FileCopyrightText: 2025 Erin Catto
// SPDX-License-Identifier: MIT

#pragma once

#include "container.h"
#include "dynamic_tree.h"
#include "table.h"

#include "box3d/collision.h"
#include "box3d/types.h"

typedef struct b3Body b3Body;
typedef struct b3Shape b3Shape;
typedef struct b3Stack b3Stack;
typedef struct b3World b3World;

// Store the proxy type in the lower 2 bits of the proxy key. This leaves 30 bits for the id.
#define B3_PROXY_TYPE( KEY ) ( (b3BodyType)( ( KEY ) & 3 ) )
#define B3_PROXY_ID( KEY ) ( ( KEY ) >> 2 )
#define B3_PROXY_KEY( ID, TYPE ) ( ( ( ID ) << 2 ) | ( TYPE ) )

// The three body types take the proxy codes 0 to 2 and name the tree of each. The fourth code is
// not a body type: it names the tree of sleeping dynamic proxies. The key format holds four codes.
#define B3_SLEEPING_PROXY_TYPE ( (b3BodyType)3 )
#define B3_PROXY_TREE_COUNT 4

/// The broad-phase is used for computing pairs and performing volume queries and ray casts.
/// This broad-phase does not persist pairs. Instead, this reports potentially new pairs.
/// It is up to the client to consume the new pairs and to track subsequent overlap.
typedef struct b3BroadPhase
{
	// One tree for each body type, then the tree of sleeping dynamic proxies.
	b3DynamicTree trees[B3_PROXY_TREE_COUNT];

	// The moved siblings gathered from the dynamic body tree.
	int* movedSiblings;

	// Number of full rebuilds of the sleeping dynamic tree, reported by b3World_GetCounters.
	int sleepingTreeRebuildCount;

	// Count of the open tree traversals: the step, and every public call that walks the trees and
	// runs host callbacks. A proxy migration asked for while this is nonzero is queued, so no walk
	// sees a tree change under it. It is not the world lock, which the API a callback can call
	// checks or toggles itself.
	int traversalDepth;

	// Bodies whose proxy migration waits for the drain, by id. The drain reads each body's live set,
	// so a body replaced, destroyed or reused meanwhile is placed by what it is then.
	b3Array( int ) migrationQueue;

	// Tracks shape pairs that have a b3Contact
	// todo pairSet can grow quite large on the first time step and remain large
	b3HashSet pairSet;
} b3BroadPhase;

void b3CreateBroadPhase( b3BroadPhase* bp, const b3Capacity* capacity );
void b3DestroyBroadPhase( b3BroadPhase* bp );

int b3BroadPhase_CreateProxy( b3BroadPhase* bp, b3BodyType proxyType, b3AABB aabb, uint64_t categoryBits, int shapeIndex,
							  bool markMoved );
void b3BroadPhase_DestroyProxy( b3BroadPhase* bp, int proxyKey );

// The proxy code a body's type and solver set imply. A sleeping dynamic body's proxies are in the
// sleeping tree; every other enabled body's are in its type's tree. One exception: a marked leaf
// of a sleeping dynamic body waits in the awake tree until the next pair update has searched it,
// since the sleeping tree is never searched for its own marks (b3PlaceBodyProxies).
b3BodyType b3BroadPhase_GetBodyProxyType( const b3Body* body );

// Sleep and wake call this beside their set index write, and every writer that marks a proxy of a
// body that may be asleep calls it after the write. The body's proxies are placed in the tree its
// set, type and marks imply at once when no traversal is open, and at the next drain otherwise; a
// body with a marked leaf bound for the sleeping tree is queued until the drain after the rebuild
// that clears the mark.
void b3BroadPhase_MigrateBody( b3World* world, int bodyId );

// Place every queued body's proxies by its live set, type and marks, keep the bodies still waiting
// and test the sleeping tree's height
void b3BroadPhase_DrainMigrations( b3World* world );

// One entry and exit pair wraps every call that walks the trees and can wake a body inside the
// walk, around all of the call's walks. The outermost exit drains the queued migrations.
void b3BroadPhase_BeginTraversal( b3BroadPhase* bp );
void b3BroadPhase_EndTraversal( b3World* world );

void b3BroadPhase_MoveProxy( b3BroadPhase* bp, int proxyKey, b3AABB aabb );

int b3BroadPhase_GetShapeIndex( b3BroadPhase* bp, int proxyKey );

void b3UpdateBroadPhasePairs( b3World* world );
bool b3BroadPhase_TestOverlap( const b3BroadPhase* bp, int proxyKeyA, int proxyKeyB );

void b3ValidateBroadPhase( const b3BroadPhase* bp );
void b3ValidateNoMoved( const b3BroadPhase* bp );

static inline void b3BroadPhase_MarkProxyMovedSerial( b3BroadPhase* bp, int proxyKey )
{
	b3BodyType proxyType = B3_PROXY_TYPE( proxyKey );
	int proxyId = B3_PROXY_ID( proxyKey );
	b3DynamicTree_MarkProxyMovedSerial( bp->trees + proxyType, proxyId );
}

static inline void b3BroadPhase_MarkProxyMoved( b3BroadPhase* bp, int proxyKey, b3AABB aabb )
{
	b3BodyType proxyType = B3_PROXY_TYPE( proxyKey );
	int proxyId = B3_PROXY_ID( proxyKey );
	b3DynamicTree_MarkProxyMoved( bp->trees + proxyType, proxyId, aabb );
}
