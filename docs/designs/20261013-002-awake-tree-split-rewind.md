---
title: What the awake tree split gives a bit-exact rewind
status: Draft
date: 2026-10-13
---

# What the awake tree split gives a bit-exact rewind

Written against the rewind-only history ring (`20260929-002`) for the engine change in the awake
tree split preflight (`20261013-001`), whose B0 to B4 and section 4 this note refers to by those
names. It records what that change removes from the rewind design and what it leaves, in the two
cases the preflight's section 1 distinguishes: with and without the tree-order changes
(`20261011-001`).


## 1. Awake tree: image it

- The awake tree is copied into the ring slot with the rest of the hot image: `nodes` and
  `parents` up to `nodeEnd`, `proxies` up to `proxyCapacity` (the free list threads through every
  free slot), and the scalars `nodeEnd`, `pairFreeList`, `proxyCount`, `proxyCapacity`,
  `proxyFreeList` and `dfsOrdered`. The rebuild scratch arrays are not state. About 100 B per
  proxy slot, against the 2 KB per awake body in the ring's measurements. The bound is the
  tree's high-water mark, not its live count: `proxyCapacity` never shrinks, and `nodeEnd` compacts
  only in a rebuild, which runs on a moved root or an unordered tree (a remove keeps the order) and
  returns at once for an empty tree (`dynamic_tree.c`). B2 clears `dfsOrdered` after a sleep
  migration, so `nodes` and `parents` compact on the next step; the `proxies` array keeps its
  high-water mark, 24 B per slot. A tree that held ten thousand awake proxies and now holds ten
  images about 960 KB in a capture taken before that step (20,000 nodes and parents at 36 B plus
  the slots) and about 240 KB from the capture after it. The awake tree starts at the 16-slot
  floor (B0), so the mark is set by what was awake, not by the capacity hint. This weakens the
  rewind design's requirement 2 from "proportional to awake state" to "proportional to the most
  that was awake at once" for that array.
- A capture is taken after `b3World_Step` returns, so it sees an empty migration queue (B3) and
  every proxy in the tree its body's set implies.
- Restore overwrites. Proxies that slept after T are in image T. Proxies that woke after T are not,
  and the overwrite removes them with their free-list pops.
- Proxy capacity is state, because the imaged free list covers slots below the imaged capacity
  only and `b3Free` hands the block size to a user free function (`core.c`); restore reallocates to
  the imaged capacity.
- For this tree the rewind design's tree-order dependence, restore-time move pass and first-step
  rebuild all go away.

## 2. Non-awake tree

**With the tree-order changes landed.** No tree's layout is simulation-affecting for the
engine's own reads (the call order a stateful `preSolve` or custom filter sees stays
layout-dependent, the exception both designs already carry). The non-awake tree gets exactly the
treatment the rewind design's section 7.4 gives every tree: the proxy-identity journal, the
restore-time pass that moves each restored shape's proxy to its restored fat AABB (a
`b3Body_SetTransform` on a sleeping body changes the leaf's AABB with no create or destroy), and
the moved-bit restore (clear every tree's moved bits, then mark image T's moved list with
ancestors; undoing a wake's removal is an ordinary insert, which marks the leaf, and the restore
clears that mark unless T's list carries it). The layout differs from T's, and nothing reads the
difference. What the split removes is that treatment for the awake tree. Removing a rolled-back
island from the non-awake tree is an ordinary remove.

**Without them.** The static, kinematic and non-awake trees are read in order by continuous
collision and `b3World_Explode`, so each must come back with its exact layout. An ordinary insert
does not give that: `b3FindBestSibling` and `b3RotateNodes` make the result depend on the path
taken, and `b3RemoveLeaf` does not undo a rotation. A physical journal does: one write accessor in
`dynamic_tree.c` is the only way to write `nodes`, `parents`, `proxies` and the scalars, and it
appends `(tree, array, index, old bytes)` before writing; undo writes the bytes back in reverse
order. For a tree it covers, the byte journal subsumes the rewind design's proxy create and destroy
entries, whose undo would otherwise pop or push the free list a second time; those entries are made
only for trees the byte journal does not cover. Growth of `proxies` is its own entry, undone by
reallocating to the old capacity; node capacity is not state, since growth changes no index and
`nodeEnd` is journaled. A rebuild, B4's height-triggered one, is one entry carrying the tree's
arrays and scalars, O(tree), paid on the write it fires. One path through the tree costs roughly its height in 36 B
writes, 0.5 to 1 KB at ten thousand proxies, plus the swaps an insert's rotations make on that path.
A migration is one path in this tree, since a sleep is only an insert here and a wake only a remove
(the awake side is imaged); a `b3Body_SetTransform` on a sleeping body is a remove and an insert,
two paths. Bytes per move are measured under the preflight's section 4 churn. The preflight's
section 4 gives the sleep and wake moves per tick, and the journal is also charged for every other
write in B2's list (`b3Body_SetTransform` and `b3ResetProxy` on a sleeping body, shape create and
destroy, enable and disable), which that table does not measure. B4's height trigger is what keeps
this journal bounded: a tree the step rebuilt whenever its order broke would cost its whole image
per sleep, and the triggered rebuild costs one image on the rare step the root height crosses
`B3_SLEEPING_TREE_MAX_HEIGHT`. The trigger reads the root height after an insert, which this journal
restores, so after a rewind it fires on the same write. Under the tree-order branch above the
restored layout can differ from T's, so the trigger can fire on a different write and the rebuild
needs no entry; nothing reads the difference. The kinematic tree is rebuilt
by the step, so in this branch it is imaged with the awake tree; it starts at 16 slots and is small
in every benchmark scene.

A cheaper variant logs operations rather than bytes: for a remove, the leaf index, its parent
index, the proxy slot's 24 bytes (user data, category bits, node, next), the fat AABB, the moved
bit and the old `dfsOrdered`; for an insert, the
leaf index, whether the pair came off the free list or moved the node end, the old `dfsOrdered`,
and each `b3SwapNodes` the rotation walk made, which is its own inverse. Ancestor AABBs are
recomputed as unions, which reproduces the originals only while every internal AABB of the tree is
the union of its children; that holds for a tree written by insert, remove and move only, which B4
guarantees. About 64 B per move plus 8 B per rotation. Measure before choosing it.

## 3. Journal coverage

Every writer in B2's list goes through `b3BroadPhase_CreateProxy`, `DestroyProxy` or
`MoveProxy`, and the moved-bit clear in B1 and B4 through the pair update, all in `broad_phase.c`;
the rewind design's proxy-identity entries sit inside the create and destroy functions, so the
migration makes them. The moved-bit setters
`b3BroadPhase_MarkProxyMovedSerial` and `b3BroadPhase_MarkProxyMoved` are inline in
`broad_phase.h`, called from `joint.c` and `solver.c`, and move out of line under the byte journal.
The direct tree writes outside the broad-phase, `b3World_RebuildStaticTree` (`physics_world.c`)
and the refit and enlarge calls in `solver.c`, are the ones the rewind design's section 7.4 moves
into `broad_phase.c`; B4's triggered rebuild is in there from the start. The growth
and rebuild entries of §2 hang off those. Moved bits are state,
since they decide which pairs are searched.

## 4. Changes to the rewind design

- Its sections §4 and 7.4 shrink for the awake tree; proxy create and destroy entries are not made
  for it, and its AABBs and moved bits ride in the image. The section 7.4 treatment stays for the
  non-awake tree. Phase 2 (derived trees) shrinks.
- The image grows by §1.
- Without the tree-order changes, the journal gains §2's entry kinds in place of the proxy
  create and destroy entries for the trees they cover, and its section 7.2 size estimate is
  remeasured with the preflight's section 4 churn.
- A proxy changes tree and id on every sleep and wake, so `shape->proxyKey` and the order in which
  each tree hands out ids must come back. The awake image gives it physically. For the non-awake
  tree under the proxy-identity journal, undo in reverse gives it: every slot is allocated or on the
  free list, growth runs only on an empty list and chains the new slots in ascending order, so after
  a rewind the list is T's list followed by the slots a discarded growth added, which the original
  timeline handed out in the same order when it grew (the rewind design's section 7.4). Under the
  byte journal the growth entry reallocates to the old capacity and the list is T's exactly.
- Its section 7.1 argument, "Leaving the awake set needs no entry of its own", no longer holds as
  written: sleep writes a cold structure and that write is journaled.
- Its test 8 (sleep and wake) checks both trees. The state hash covers the awake tree physically
  and, with the tree-order changes, the non-awake tree logically (membership, proxy ids, AABBs,
  moved bits), since its restored layout legitimately differs from T; under the byte journal it is
  physical for both.

## 5. Provenance

- **Proxy-pool growth needs no entry.** A slot is allocated or on the free list, undo in reverse
  keeps that, and growth happens only on an empty list, appending the new slots in ascending
  order. A rewound tree hands out the same ids as the original did, including the ones a discarded
  growth added, so capacity is not state for the journaled tree. It is state for the imaged tree,
  whose free list is copied rather than replayed (5.1).
- **No stack growth or height bound for the non-awake tree.** The traversal stacks and
  rotation-balanced inserts are settled primitives the static tree depends on today in the same
  way; the split adds a second tree with that reliance, not a new reliance. A branch is skipped
  only when a 512-entry stack fills, a tree of height over 500. The rewind design already derives
  the static tree's layout on restore and walks it with the same stacks, so a derived non-awake
  layout is the case it already carries. The height measurement under churn is the check.
