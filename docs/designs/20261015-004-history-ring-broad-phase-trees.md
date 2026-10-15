---

title: The broad-phase trees under a rewind-only world history
status: Draft
  Split from docs/designs/20260929-002-rewind-only-history-ring.md, which decides the per-tick image, the undo journal, the restore and the
  public API, and cites this document for the four broad-phase trees: which is imaged, which are
  derived, how proxy identity and leaf boxes come back exactly, and the restore pass. The document
  is self-contained except where it cites that one.
  Code references are against source as of 55a40d1, with the awake and sleeping dynamic trees
  (`20261013-001`, `20261015-001`) and the bulk migration in.
date: 2026-10-15

---

# The broad-phase trees under a rewind-only world history

## 1. What this decides

`docs/designs/20260929-002-rewind-only-history-ring.md` restores a world to a retained tick by undoing a journal back to an image (its §9). The
broad phase's four trees are the one structure that is neither plainly hot nor plainly cold: one
is rewritten every step by the awake population, two are world-sized and never change on their
own, and one is both. This document decides how each comes back after `Rewind(T)` so that the
first replayed step finds the pairs the original did.

## 2. Requirements

1. **Bit-exact where the step reads.** After `Rewind(T)` every live leaf's box, every shape's
   proxy id and tree, every moved bit and the migration queue equal their values at the end of
   step T. Tree layout (which node holds which proxy) is not state: nothing the step reads depends
   on it (`docs/designs/20260929-002-rewind-only-history-ring.md` §3), and a stateful `preSolve`, custom filter or public query callback that sees
   layout order is that document's named exception.
2. **No copy of a world-sized tree per tick** (`docs/designs/20260929-002-rewind-only-history-ring.md` requirement 2).
3. **By construction.** Every write to a world tree is a function of `broad_phase.c`, and the
   journal call lives inside the proxy create and destroy that every caller reaches (§7).

## 3. The trees today

The broad phase holds four trees (`broad_phase.h`): static, kinematic, awake dynamic and sleeping
dynamic (`b3DynamicTree`, 32 B nodes + 4 B parents + 24 B proxies). The static and sleeping trees
hold the proxies of static and sleeping bodies, so imaging either breaks `docs/designs/20260929-002-rewind-only-history-ring.md` requirement 2; the
kinematic tree holds every kinematic proxy, awake or asleep. The step's rebuild (`broad_phase.c`,
`dynamic_tree.c`) covers the awake and kinematic trees: it copies every retained subtree into a
fresh DFS-ordered array whenever the tree's root is marked moved or the tree is not DFS-ordered,
so the engine's own per-step cost for those two trees is O(their proxies) while they are moving.
The static tree is rebuilt only by an explicit `b3World_RebuildStaticTree` call, and the sleeping
tree only in full when an insert takes its root height past `B3_SLEEPING_TREE_MAX_HEIGHT`
(`dynamic_tree.h`). The ring must not add a copy of a world-sized tree per tick. §5 images the
awake tree, which is proportional to what is awake, and makes the layout of the other three
reconstructible from fat AABBs plus the moved list. Which proxy id each shape holds is not derived
the same way; §7 journals it alongside the id pools it is structurally identical to.


A proxy's tree follows its body (`b3BroadPhase_GetBodyProxyType`, `broad_phase.c`): the static
and kinematic trees hold every proxy of their body type, awake or asleep; a dynamic body's proxies
are in the awake tree while it is awake and in the sleeping tree while it sleeps, except that a
marked leaf of a sleeping body waits in the awake tree until the next pair update has searched it,
and its body waits in the migration queue for the drain that follows that pair update
(`b3PlaceBodies`). Sleep and wake move a set's proxies between the two dynamic trees through the
broad phase's migration, as one built subtree for a large set; the sleeping tree is never searched
for its own marks and holds none at a step boundary. The drain writes the parked flag of each
body it places, a sleeping body's record being cold, through `b3Body_Write`'s structural entry;
its moving flag is set and cleared inside the same drain, so it is clear at every boundary.

## 4. Layout, leaf boxes and proxy identity

Two different things depend on the trees: layout (which internal nodes hold which proxies, node
rotations) and proxy identity (which integer id a given shape's proxy has in its tree, stored in
`shape->proxyKey`). Layout is a performance property for the engine's own reads (`docs/designs/20260929-002-rewind-only-history-ring.md` §3); what
remains order-dependent is the call order a stateful `preSolve` or custom filter sees and the
order of public query callbacks and tied closest-hit casts (`docs/designs/20260929-002-rewind-only-history-ring.md` §10). A leaf's box is not layout: the
pair passes hand every overlapping pair of leaf boxes they reach to contact creation, filtered on
the pair set and never re-tested on the shapes' boxes (`broad_phase.c`), so each live leaf's box is
state and comes back exactly. Proxy identity is state because `shape->proxyKey` is part of the
journaled or imaged `shapes[id]` record (`docs/designs/20260929-002-rewind-only-history-ring.md` §5), and a live tree that assigned a shape a different proxy
id than it had at T would disagree with that record.

## 5. The awake tree is imaged

It holds the proxies of awake dynamic bodies and the parked leaves
of sleeping ones, and the step rewrites it on every tick its root moved or its order broke, so it
is hot state of the awake population. Its image is the fields the serializer's tree writer
(`world_snapshot.c`) writes, less the fixed compatibility `version` (`types.h`): the scalars (`nodeEnd`, `pairFreeList`, `proxyCount`,
`proxyCapacity`, `proxyFreeList`, `dfsOrdered`), `nodes` and `parents` up to `nodeEnd`, and
`proxies` up to `proxyCapacity`, since the free list threads through every free slot; the spare
node array and the rebuild scratch arrays hold no state. About 100 B per proxy slot. The bound is
the tree's high-water mark, not its live count: `proxyCapacity` never shrinks, and `nodeEnd`
compacts only in a rebuild, which the step runs on the tick after a remove or a bulk join left the
tree unordered and which returns at once for an empty tree. The tree starts at the 16-slot floor
and grows with what the tree holds, awake proxies and parked leaves, so the mark is set by the most
held at once, not by the capacity hint; that document's requirement 2 reads "proportional to the most awake proxies
and parked leaves held at once" for this array. The
migration queue is imaged with it, by its stored count and order: after a step's drain it holds
exactly the sleeping bodies whose marked leaves are parked in the awake tree, each once, in the
order the next drain places them, and that order decides which sleeping-tree ids they receive; the
tick-0 image can also hold a body once per shape created on it since the last drain, and the id of a
destroyed body, which the drain drops (`broad_phase.c`). Restore frees the live tree's arrays,
its spare array and its rebuild buffers, allocates the image's sizes and copies, and copies the
queue; proxy capacity is state for this tree, because the imaged free list covers the slots below
the imaged capacity only and `b3Free` hands the block size to a user free function (`core.c`).
Proxies that slept after T are in image T; proxies that woke after T are not, and the overwrite
removes them with their free-list pops. No proxy entry is made for this tree, its moved bits and
leaf boxes ride in the image, and no restore pass runs on it.

## 6. The derived trees and the restore pass

Restore may rebuild their layout any way
it likes:

- every restore-time write of a shape record, whether a journal undo write or the image
  scatter, goes through one function that appends the shape id to a restore list and touches no
  tree;
- after the journal walk and the image scatter, every proxy alive at T exists, and one
  pass calls `b3DynamicTree_MoveProxy` with the restored fat AABB on the proxy of each listed shape
  whose restored record holds one in the static or kinematic tree (`proxyKey` not `B3_NULL_INDEX`
  and one of those two tree codes; disabled shapes and free slots hold none, and a listed id at or
  beyond the restored shape array's length names a slot an undone bump allocation truncated away
  and is skipped without a read), and a shape listed more than once is moved once per listing,
  which is idempotent;
- clear each derived tree's moved bits (`b3DynamicTree_ClearMoved`, which descends only moved
  nodes), then mark each proxy of image T's moved list moved together with its ancestors
  (`b3DynamicTree_MarkProxyMovedSerial`), since `ClearMoved`, the rebuild and pair update all
  descend by the internal nodes' moved bits. The sleeping tree holds no mark at a step boundary.
  The static tree's are set only by API calls between steps and cleared by the next pair update,
  so its list is empty after any step and non-empty only in tick 0's image, which
  `b3World_EnableHistory` takes after such calls (`docs/designs/20260929-002-rewind-only-history-ring.md` §8);
- the next step's rebuild puts the kinematic tree back into DFS order as usual. The static tree
  is not rebuilt by the step, the sleeping tree only on its height trigger, and nothing reads
  their order.

In the static and kinematic trees every write of a leaf's box writes the shape's `fatAABBs`
entry to it: a create and a move from the API (`shape.c`, `body.c`) and the finalize mark in the
step (`solver.c`, through `b3BroadPhase_MarkProxyMoved`, which replaces the leaf's box). So at a
step boundary each live leaf of those two trees equals its `fatAABBs` entry, and moving a proxy
to its restored fat AABB reproduces the leaf. A dynamic leaf need not: the bullet enlarge
(`solver.c`, `b3DynamicTree_EnlargeProxy`) leaves the leaf alone when it already contains the
new entry, so a bullet's leaf can hold a box larger than its entry, and a migration carries the
box the leaf held into the sleeping tree. The sleeping tree's leaves are therefore restored from
entries alone and the pass does not touch that tree: an undone migration re-inserts the box the
destroy entry recorded, and a leaf written in place, which happens only inside an API call that
then parks or wakes the body (`b3ResetProxy` keeping the proxy), is moved back by its move entry
after the undone migration has put it back.

Cost O((restored shape records + moved proxies) · log n). Ring cost for the three layouts: zero.
A kinematic tree the pass moved a proxy in can be left unordered (a move clears `dfsOrdered` when it
breaks the order, `dynamic_tree.c`) or with its root marked moved, and the next step then rebuilds it
even if nothing moves, at O(that tree's nodes), which is the engine's
own per-step cost while its proxies move (§3). That rebuild is part of the first replayed step,
not of `Rewind`. An undone wake inserts into the sleeping tree through the ordinary create, whose
height test can fire that tree's full rebuild inside `Rewind`, as the same insert can in a step.

## 7. Proxy identity

Each tree allocates proxy ids from its own LIFO free list
(`tree->proxyFreeList`, `dynamic_tree.c`), independent of the six id pools `docs/designs/20260929-002-rewind-only-history-ring.md` §5.2 already
journals. The choke points are the proxy create and destroy functions of `broad_phase.c`: the
single create that `b3BroadPhase_CreateProxy` and the drain's one-by-one placement share, the
drain's bulk placement of sixteen or more proxies into one tree (`b3DynamicTree_CreateProxies`,
which allocates one id per leaf in leaf order), and `b3BroadPhase_DestroyProxy`. They make an
id-pool-shaped entry, one per proxy and shared by the trees since the entry records which tree,
for the static and sleeping trees, and for the kinematic tree once it is derived (`docs/designs/20260929-002-rewind-only-history-ring.md` §13). Outside
`broad_phase.c` the world's four trees are reachable only as a pointer-to-const, which the
`b3DynamicTree` write functions do not accept, and every function that writes one (proxy create
and destroy, single and bulk, move, enlarge, refit, rebuild, moved bits, the migration's placement)
is a function in that file, so `solver.c`'s enlarge and refit calls and
`b3World_RebuildStaticTree`'s rebuild (`physics_world.c`) move there, and the serializer's tree
load (`b3DesTree`, `world_snapshot.c`), which replaces a tree's arrays and scalars in place, becomes
a load function of that file, the one route by which a deserialize writes a tree. A compound shape's own
child tree, built in `compound.c`, is borrowed geometry (`docs/designs/20260929-002-rewind-only-history-ring.md` §10), not world state, and stays a plain
tree. The journal call lives inside the choke points, so every caller is covered: shape create and
destroy, `b3ResetProxy` (`shape.c`), which destroys and recreates a shape's proxy, possibly in a
different tree, on a filter or body-type change, and the migration's placement, which destroys
every moved leaf and then creates it in its destination tree; the same document's §5.2 table lists `b3ResetProxy` as
a `shapes[id]` write-accessor caller for the same reason. Undoing a proxy's destruction is a real
tree insert, not a bare id reservation, and in every caller the `proxyKey` write follows the proxy
operation, assigned from the create's return value or the bulk ids (`b3ResetProxy` writes the
shape's other fields and its fat AABB before the proxy operation, `shape.c`), so a destroy entry carries its own payload: the tree, the proxy id, the leaf's box, the category bits
and the shape id. A create entry carries the tree and the id. The undo insert is unmarked, and its
height test is the one every insert into the sleeping tree makes.

Entries are undone in strict reverse journal order, so when an entry is undone the free list is in
the state the recorded operation left it in. A destroy pushed its id onto the head of the free
list, so undoing it with the tree's ordinary create pops that id; the undo asserts the id matches.
A create popped its id from the head, so undoing it with the tree's ordinary destroy pushes the id
back where it came from; a bulk create popped its ids in leaf order and its entries are undone in
reverse, so the list is as it was. Undoing a shape's re-create then destroy through the journal
therefore leaves it with the same proxy id, in the same tree, that it had at T, and
`shape->proxyKey` never needs a post-restore correction. For a journaled tree proxy capacity is
not state: growth appends fresh ids in ascending order only when the free list is exhausted, so a
tree left with extra capacity by a discarded timeline hands out the same id sequence as the
original. A sleep's creates and a wake's destroys in the sleeping tree are these entries, so a
wake after T is undone by re-inserting the set's proxies in the sleeping tree at their recorded
ids and a sleep after T by destroying them there; the awake side of either comes back with the
image. This is why proxy identity is not in §3's derived class: it is cold and journaled, like
the pools it structurally matches.

## 8. Height-triggered rebuilds

The sleeping tree is rebuilt in full when an insert
takes its root height past `B3_SLEEPING_TREE_MAX_HEIGHT`, and the awake tree when a bulk join
does (`broad_phase.c`). A rebuild changes no leaf box, no proxy id and no moved bit, so neither
makes an entry; after a rewind the sleeping tree's layout differs from T's and the trigger can
fire on a different insert, which nothing reads.

## 9. Phases

Until the restore pass covers the kinematic tree (phase 2), it is imaged raw as the awake tree is:
O(kinematic proxies) ring memory, which fails requirement 2 for a world with many sleeping
kinematic bodies and is fine for everything in `benchmark/`, where the kinematic tree starts at 16
slots and stays small. The static and sleeping trees are world-sized and never imaged by the ring, so their
proxy entries and the restore pass are required in every phase from phase 1; phase 0 restores all
four trees from the serialized whole-world image (`docs/designs/20260929-002-rewind-only-history-ring.md` §13).

## 10. Verification

1. **Tree-order independence.** Restore-and-replay cases built to differ in the layout of the
   static, kinematic and sleeping trees after a rewind from the original run's: a bullet sweep
   with several competing candidates, some awake and some asleep, one of them a nearer hit that
   `preSolve` rejects, a sweep with more than eight sensor candidates, an explosion hitting several
   shapes of one body, and an explosion waking bodies in several sleeping sets, some connected by
   constraints. The full state hash and the events of each replayed step match the original run.
2. **Restore exactness for the trees.** The tree cases of `docs/designs/20260929-002-rewind-only-history-ring.md` §12 test 2 (a filter change that
   recreates the proxy, a body-type change that recreates it in a different tree, a step that
   grows a tree's proxy capacity, a body put to sleep while marked, a shape created on a sleeping
   body, a bulk placement, an undone wake past the sleeping tree's height trigger, and a slept
   bullet whose leaf outgrew its entry) are decided here and run there.

## 11. Open questions

1. Should the bullet enlarge replace the leaf whenever it differs from the fat AABB entry, as the
   finalize mark does for every other body (`solver.c`)? Today it leaves a leaf that already
   contains the new entry, which is why a bullet's leaf can be larger than its entry and the
   sleeping tree is restored from entries alone (§6). Replacing it would make every tree's leaf
   equal its entry at a boundary, let the restore pass cover the sleeping tree, and retire the
   proxy move entry; it also changes which pairs a slowing bullet's leaf finds, so it is an engine
   change for its own preflight, not a decision of this design. Deferred until phase 1 shows
   whether the entry is worth removing.

## 12. Provenance

- **The awake tree is imaged, not derived.** Deriving it would put every awake proxy through the
  restore pass and the first replayed step's rebuild, work proportional to the awake population
  that the image's flat copy pays once, and the parked leaves and the migration queue would then
  be rebuilt from the sleeping bodies' marks, a second route to what the image holds.
- **The awake tree's image is not compacted.** Its size is the most proxies and parked leaves it held
  at once. A compaction would renumber the proxies, and `shape->proxyKey` and the free list's id
  sequence (which a rewound timeline must hand out again) are keyed by those ids; the exception is
  stated in `docs/designs/20260929-002-rewind-only-history-ring.md` requirement 2 beside the other non-awake-proportional term.
- **Tick 0 images the migration queue as stored, without a drain first.** A queued body whose leaf
  is marked cannot be placed before the next pair update has searched the leaf, so a drain at enable
  would leave the same bodies queued and add a write to the world outside a step.
- **The sleeping tree is derived, not imaged and not byte-journaled.** It is proportional to the
  sleeping population, which requirement 2 forbids imaging. A byte journal of its writes would
  restore its exact layout, which is needed only where a tree's layout is read, and nothing reads
  this tree's layout (`docs/designs/20260929-002-rewind-only-history-ring.md` §3).
- **Proxy-pool growth needs no entry for a journaled tree.** A slot is allocated or on the free
  list, undo in reverse keeps that, and growth happens only on an empty list, appending the new
  slots in ascending order, so a rewound tree hands out the same ids as the original did,
  including the ones a discarded growth added. Capacity is state for the imaged tree, whose free
  list is copied rather than replayed.
- **No height bound is added for the derived trees.** The traversal stacks and rotation-balanced
  inserts are settled primitives the static tree depends on today, and the sleeping tree's height
  trigger is the engine's own; a restored layout is walked with the same stacks, and an undone
  insert is tested by the same trigger.
- **The migration queue is imaged, not rebuilt from the parked flags.** After a step's drain the
  queue is exactly the parked bodies in placement order, and that order decides the sleeping-tree
  ids the next drain hands out; a scan of the body records for the flag would be O(world) and would
  give id order, not placement order.
