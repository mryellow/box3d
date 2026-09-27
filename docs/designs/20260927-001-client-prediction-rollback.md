---

title: Client-prediction rollback for box3d
status: Draft, converged (round 9) — the round 9 Codex review in docs/designs/reviews/20260927-
  001-client-prediction-rollback-round9.md closed both of round 8's findings and stated the
  design "has converged," with only two Low-severity, prototype-level implementation details left
  (a still-world-sized bitset inside `b3Collide`'s internals, and an id-reuse guard for the
  deferred split-candidate slot — both folded into §9.2 as noted prototype work, not further
  design changes). §9.2 (scoped resimulation's implementation) went through five review rounds
  (4-9): round 3 sketched a "make the awake set a variable" approach that Codex found unsound;
  rounds 4-5 tried giving the scope its own solver set and constraint graph and found new
  ownership/validator conflicts each round; round 6 replaced that with in-place scoped iteration
  (nothing physically moves; a scope id set plus compact iteration lists; the engine's existing
  zero-mass dummy-endpoint mechanism reused for boundary partners), which eliminated the
  ownership-conflict class entirely; rounds 7-9 fixed the remaining mechanical gaps (mass-override
  timing, mixed-island sleep, step-ordering of the compact-list rebuilds, split-island bookkeeping,
  tick-stamp semantics) until no design-level findings remained. Full review history in
  docs/designs/reviews/: this file's round2-round9 suffixes (round2 file has no suffix) and the
  performance review that drove round 3's rewrite, at
  docs/designs/reviews/20260927-001-client-prediction-rollback-perf.md. Next step: a narrow
  prototype covering §9.2's explicitly-flagged implementation work (the joint-type/contact-solver
  audit, pass-local parallel-solver buffers, and the two Low items above), not another design round.
date: 2026-09-27

---

# Client-prediction rollback for box3d

## 1. Problem

Games that use client-side prediction with server reconciliation (the Overwatch GDC model:
`~/mygame/docs/2865_ResearchOverwatchGameplayArchitectureNetcode.md`) keep a history of recent
simulation state plus the inputs that produced it. When an authoritative correction arrives for
an older tick T, the client restores T, applies the correction, and resimulates forward through
its buffered inputs to "now".

`docs/faq.md` §Determinism says box3d cannot do this today ("There is no mechanism to set a world
back to a prior state and then resume simulation"). This doc designs that mechanism as a general
box3d feature, not a `mygame`-specific one. It must hold up on the stress scenes in `benchmark/`,
with worlds well past 500 awake bodies.

## 2. Requirements

1. **Per-tick cost is proportional to what is predicted, not to the world.** Capture runs every
   step, so it must be O(predicted state), allocation-free in steady state, with no hashing.
2. **Correction cost is proportional to what is predicted, not to the world, for collide and
   solve.** Resimulating N ticks must not step unrelated bodies. Two step stages this design does
   not scope — broad-phase pair discovery and sensor overlap — remain proportional to world size
   until the follow-on work in §12 lands; §9.3 states this precisely and §13 measures it
   separately so it can't hide inside a passing total.
3. **Robust in a live world.** Restore must not be rejected because unrelated parts of the world
   changed (contacts created elsewhere, ids allocated, islands merging or sleeping), and must never
   leave the world inconsistent.
4. **Graceful when the predicted set is huge.** A predicted body resting in a 5,000-body pile is
   coupled to all 5,000. That cost is inherent. The API must let the caller see it and fall back,
   not stall.
5. **General API.** Any body can be a root, any number of roots. Nothing is tied to a character
   controller or one game's netcode.

## 3. Measured constraints that drive the design

From the perf review addendum (12 benchmark scenes, 8 workers, 60 Hz / 4 substeps, 9-tick
correction ≈ 150 ms RTT):

- **Resimulating the whole world is the dominant cost.** A 9-tick correction fits in a 16.7 ms
  frame only in the ~50-body `trees` scenes. At 2–2.6k bodies (`sleep`, `rain`) it costs
  14–32 ms. At ~10k (`many_pyramids`, `junkyard`) it costs 106–228 ms. Any design that resims
  the whole world fails requirement 2.
- **Whole-world capture is 20–35% of a step every tick** (≈ 2 KB per awake body, mostly contacts
  and manifolds). A 10-tick ring needs 50–500 MB, and `large_world` (1M bodies, 10 awake) needs
  650 MB/tick. This fails requirement 1.
- **Islands vary from 1 to 9,900 bodies.** In `many_pyramids` the largest island is 55 bodies
  out of 10,780, so scoping to islands cuts resim ~200×. In `large_pyramid` there is one
  5,050-body island, so scoping cannot help, and requirement 4 applies.

## 4. Determinism contract

**v1 is consistent, not bit-exact.** Restore + resim produces a physically valid continuation from
the restored state. It is not guaranteed to reproduce the client's own earlier timeline bit-for-bit.

Why this is the right contract for a general prediction feature:

- **Prediction doesn't need it.** On a correction, the predicted body's state is overwritten
  with the server's value and the future is resimulated from there. Client-vs-server comparison
  is always tolerance-based, because the server's world differs from the client's regardless.
- **Bit-exact rollback of a subset is not achievable in a live world without solver changes.**
  Contacts that begin touching are processed in contact-id order (bitset walk,
  `physics_world.c:945`). That order drives island linking and constraint-graph colour assignment
  (first free colour, `constraint_graph.c:216-274`), and colour order changes results. Contact
  ids come from a world-global pool that unrelated contacts allocate from every tick. Round 2 of
  this doc tried to restore the pools and reject on outside churn. In a scene with other moving
  bodies that rejects almost every restore.
- **Bit-exact rollback of the whole world is achievable but fails requirements 1–2** above
  ~500 awake bodies (§3).

Bit-exact whole-world save/restore remains useful for small-world lockstep/GGPO-style games. It
is a separate future feature (§12) and can reuse the `world_snapshot.c` checklist. It is not this
design.

## 5. Overview

Three mechanisms, each O(scope):

1. **Scope.** The set of bodies being predicted. Recomputed every tick from the islands
   containing the caller's root bodies (plus touching kinematics) to decide what to record; at
   restore/resim time it is exactly whatever was actually recorded, not recomputed from current
   topology (§6).
2. **History.** An engine-owned ring of compact per-tick records of the scope's dynamic state and
   warm-start impulses, captured at the end of every `b3World_Step` (§7).
3. **Scoped resimulation.** Restore writes the recorded state back into the live world (§8). Then
   a resim mode makes `b3World_Step` simulate only the scope, holding every other awake body
   frozen (§9).

What round 2 needed and this design drops entirely: sleep exemption, id-pool restore,
constraint-graph/solver-set slot reconstruction, `pairSet` reconciliation, component-stability
preflight, broad-phase tree rebuild, and engine-side capacity reservation. Contact *topology* is
never rolled back. The live world keeps its contacts, islands, graph and pools, and the next step
reconciles them against the restored poses through the normal pair-update and narrow-phase paths.

Round 3 also sketched scoped resimulation as "make the awake set/graph a variable, point it at a
scope subset." Round 3's Codex review found that sketch unsound: bodies can't be moved between
solver sets while they have live contacts, non-awake set indices are treated as "sleeping" by wake
and linking code, and constraint prep for a body outside the scope needs its own state rather than
its real live mass (§9.2). Round 4 replaces the sketch with a real mechanism, §9.2.

## 6. Scope

- **Roots** are registered per body (`b3Body_SetRollbackRoot`, §10). There can be any number.
- At each capture, **capture scope = every island containing a root**, found via `b3Body.islandId`.
  Each island's contiguous `bodies` / `contacts` / `joints` link arrays (`island.h:61-67`) give the
  members without walking linked lists. Enabled kinematic bodies get islands and are pulled into a
  root's island by the normal touching-contact link (`island.c:218`, `body.c:317`), so a kinematic
  platform under a root needs no special-case lookup: it is already a member and is captured
  (transform + velocity) like any other scope body. The caller may re-drive its velocity per
  replayed tick like any other input.
- **Static bodies are never in scope.** Their live state is used during resim. A caller that
  moves static geometry during the window must replay that itself (§11).
- **Sleeping roots:** if every scope island is asleep at capture, the record is a one-word
  "unchanged since slot k" marker (§7). A sleeping island's state cannot change until it wakes.
- **Capture scope is not the same set as the resim scope, and resim scope is defined by history,
  not by current topology.** Capture scope (above) is recomputed every tick purely to decide what
  to record. **The resim scope is exactly the set of bodies with a valid record at T** — the same
  set restore writes (§8), no more and no less. Round 3 and round 4 both tried to define resim
  scope from the roots' *current* islands instead, and round 4's review found the resulting hole:
  a body that merged into a root's island after T has no T-record, so recomputing scope from
  current topology and then stepping it through `[T, now]` steps a body with no baseline — its
  "state at T" would just be whatever it happened to be at Begin, which is not T's state.
  Defining resim scope as "has a T-record" removes the hole by construction: every body actually
  stepped in replay was actually restored, so it has a real baseline. A live neighbor that is
  currently in the same island but has no T-record (because it wasn't part of any root's capture
  scope at T) is never added to resim scope; it is a **boundary partner** exactly like any other
  out-of-scope body (§9.1), regardless of whether it currently shares an island with a root.
  Between T and now, islands can merge (`island.c:218`) or split; that changes which bodies are
  currently coupled to a root, but restore already handles this correctly (it only ever writes
  bodies that have a T-record, §8) and resim scope inherits that same, already-correct set instead
  of recomputing something new and getting it wrong.

No sleep exemption. Bodies sleep and islands split exactly as they do without rollback. (Round 2's
exemption prevented splitting entirely, because splits are only triggered by sleepy islands,
`solver.c:824-835`, so the scope would have grown for the whole session.)

## 7. History (capture)

### 7.1 Records

Each ring slot holds structure-of-arrays records for one tick:

| Record | Fields | Size |
|---|---|---|
| Body | body id + generation, transform, `center`, `center0`, `rotation0`, linear/angular velocity, `sleepTime`, flags | ≈ 100 B |
| Contact warm-start | shape-pair key (`b3ShapePairKey`), shape A/B generation, manifold count; per manifold: `pointCount`, per point `{featureId, normalImpulse}` (≤ 4), `twistImpulse`, `frictionImpulse`, `rollingImpulse` | ≈ 84 B per manifold |
| Joint | joint id + generation, the `b3JointSim` type union's impulse/limit state | type-dependent, ≤ 444 B |
| Header | tick, counts, "unchanged since k" marker | fixed |

This is sufficient because the narrow phase already carries normal impulses across frames by
matching `featureId` against the previous manifold (`contact.c:590-612`). Manifold-level friction,
twist and rolling impulses persist with the manifold. Everything else in `b3Contact`
(SAT/simplex cache, recycling cache, anchors, separation) is recomputed by the next narrow phase
from the restored poses (§8). Compared with copying `b3Contact` + `b3Manifold` (216 + 268 B), a
warm-start record is ~6× smaller.

Only **touching** contacts (the scope islands' `contacts` arrays) are recorded. Non-touching
contacts carry no impulses and are re-derived by the next pair update.

**Warm-start records are convex-only.** Mesh and height-field narrow phase matches old manifolds
first by normal, then matches points by `featureId` **and** `triangleIndex`
(`mesh_contact.c:1017`, `mesh_contact.c:1069`); a compact record without those fields cannot
reproduce that matching. Rather than widen every record to carry mesh-specific fields for a case
that mostly matters for large static terrain (usually not a rollback root), a contact where either
shape is a mesh or height field is simply not recorded and is always recreated cold on restore
(§8.4). This is a caller-visible limitation (§11), not a correctness gap: a cold mesh contact
under-predicts friction/rolling state for one tick after a correction, the same as any other
never-before-seen contact.

### 7.2 When and how

- Capture runs at the end of `b3World_Step`, after `b3StoreImpulses` and body finalisation, so
  it records the state the next step starts from. The record for tick T is "state after step T".
- It is a gather over the scope islands' link arrays into the next ring slot. For large scopes
  it is split across workers with the world's existing task system (same pattern as other
  per-step parallel-fors). For small scopes it runs inline, so there's no task overhead.
- **Optional fused path** if benchmarks show the gather exceeds budget on large scopes: tag scope
  bodies/contacts with a flag bit and write records from inside `b3FinalizeBodies` /
  store-impulses, where the data is already in cache and the loop is already parallel. Record
  order doesn't matter because records carry ids. This is an implementation optimisation, not
  an API change.

### 7.3 Memory

- One contiguous arena per world, `historyLength` slots. Slot capacity grows when a capture
  exceeds it (realloc + repack, amortised; rare after warm-up). A configurable byte budget caps
  growth. A capture that would exceed it stores an **overflow** header, and restoring to that
  tick returns `b3_rollbackScopeOverflow` (§10).
- Estimated per-tick bytes: ~100 B/body + ~84 B/touching manifold (convex only; mesh/height-field
  contacts are not recorded, §7.1). A character-sized scope
  (tens of bodies) is a few KB per slot. The worst measured island (`joint_grid`, 9,900 bodies)
  is ~1 MB of body records plus joint records.

## 8. Restore

`b3World_RestoreRollback(worldId, tick)` writes slot T back into the live world. It is O(scope):

1. **Validate.** The slot exists (within history, not overflowed). Per body record, check the
   body id generation; a body destroyed since T is skipped, not an error. Per contact record,
   check both shapes' generations (a shape-pair key encodes shape index and child index, not
   generation, `table.h:47`; a shape can be destroyed and its index reused, `shape.c:166`) — a
   stale generation means the record no longer describes a real pair and it is dropped, not
   applied. Per joint record, check the joint's generation the same way (`joint.c:220`); a stale
   generation drops that joint's record.
2. **Wake** any scope body currently in a sleeping set, with the ordinary, unmodified
   `b3WakeSolverSet` — whole set, all its islands, sleep timers reset (`solver_set.c:45`,
   `solver_set.c:55`, `solver_set.c:157`). Round 4 flagged whole-set wake as more than a cost side
   effect, worried it could let unrelated bodies retroactively satisfy §9.1's join check; round 6
   removes that worry by construction, not by adding an island-only wake primitive. §9.2 no
   longer uses live-world island/set membership to decide what resim steps — it uses an explicit
   scope id set built fresh at Begin and a `validSinceTick` stamp (§9.2) that only advances on an
   actual state-defining event (sleep entry or a live mutation), never on waking. A body woken as
   a side effect of restore is simply not in the scope id set unless it independently qualifies,
   so it is stepped as an ordinary frozen non-scope body afterward — extra wake-up cost (documented
   in §11), not a correctness or eligibility problem.
3. **Bodies.** Write transform, centers, `center0`/`rotation0`, velocities, `sleepTime`. Recompute
   world inertia. Update shape AABBs/fat AABBs; a proxy is moved via `b3BroadPhase_MoveProxy` only
   when the new AABB escapes the shape's stored fat AABB (`body.c:1143`), the same conditional
   path `b3Body_SetTransform` uses. This is O(scope shapes · log n) with no tree rebuild.
4. **Contacts.** Round 3 tried invalidating narrow-phase caches in place (clearing
   `b3_relativeTransformValid`, resetting SAT/simplex state); the round 4 review found this
   insufficient for mesh/height-field contacts specifically — clearing `b3_relativeTransformValid`
   only disables recycling (`physics_world.c:653`), but the mesh triangle query still matches and
   copies old per-triangle caches when it re-runs (`mesh_contact.c:75`, `mesh_contact.c:135`), and
   an existing touching manifold still carries its old impulses into feature matching regardless
   (`mesh_contact.c:1007`, `mesh_contact.c:1047`) — so a contact that was already touching before
   restore never goes through the invalidation path at all and keeps its pre-restore state and
   caches untouched. In-place invalidation cannot reliably guarantee "cold" for every contact type
   through every code path, so restore does not attempt it: **every live contact incident to a
   scope body is destroyed and recreated** through the standard creation/destruction paths,
   exactly as if the pair had separated and re-formed. This guarantees a true cold start (no
   manifold, no cache, no old impulses, whatever the shape types) using only already-correct,
   already-validated engine code, at the cost of O(scope contacts) destroy+create work instead of
   O(scope contacts) cache clears — still O(scope), not a different complexity class.

   Recorded warm-start impulses are staged in a **restore-scoped side table** keyed by shape-pair
   key (freed at the end of the restore/resim bracket, not persisted), because a contact that is
   not touching must have no manifold (`physics_world.c:4048`, `physics_world.c:4068`) — the
   recreated contact starts non-touching and cannot carry a manifold immediately. The side table
   is consulted, and its entry consumed, on **every** post-restore narrow-phase update of a
   matching pair — including the very first one, whether that update finds the pair touching or
   not — not only on a touch *transition*, since after a destroy+recreate every contact's first
   post-restore update is its first update, full stop; there is no "already touching, so skip the
   table" case left to get wrong. When the first update finds the pair touching, the side-table
   record seeds its impulses outright — there is no prior manifold to prefer it over, since the
   contact is brand new. Only convex-vs-convex records exist (§7.1); a mesh/height-field pair's first
   post-restore update always starts from nothing, matching §7.1's "always recreated cold"
   contract exactly, because there is no side-table entry for it to consult. A pair with no
   record (created during the window, not at T) simply finds no side-table entry and starts cold
   like any brand-new contact. A pair recorded at T with no live contact at restore time (it
   separated during the window) has nothing to destroy+recreate — restore leaves it absent, same
   as §11's out-of-scope-mutation contract — but its side-table entry is retained through the
   bracket and picked up by the standard new-pair path if/when broad-phase rediscovers it and it
   touches again; if it never does, the entry expires unused at the end of the bracket.

   **Destroy+recreate is observable, and that is accepted, not hidden.** Destroying a contact
   frees its id and emits an end-touch event if it was touching; creating its replacement
   allocates a new id and generation and begins it non-touching (`contact.c:189`, `contact.c:292`,
   `contact.c:340`). A pair that was touching immediately before restore and still would be
   touching at T therefore reports a spurious end-touch followed by a begin-touch, and any contact
   handle/id a caller held across the restore is stale, exactly as if the pair had actually
   separated and reformed. Restore does not attempt to suppress or filter these events or preserve
   the old id: `b3World_IsResimulating` is false during restore itself, so a caller cannot use it
   to distinguish a genuine separation from a restore artifact — this is a caller-visible
   consequence of the destroy+recreate approach and is documented as one in §11, not fixed, because
   suppressing it would mean re-adding exactly the kind of special-cased event filtering this
   design has otherwise avoided. **Side-table consumption also rechecks shape generations at
   match time, not only at the initial restore-time validate (step 1):** a shape referenced by a
   staged record can be destroyed and its index reused for a different shape between restore and
   whenever the pair actually touches again during a multi-tick resim window; the side table
   carries the recorded generations and a lookup whose current shape generations don't match is
   dropped, the same as a stale record at restore time, rather than seeding impulses onto a pair
   between different shapes than the ones recorded.
5. **Joints.** Write recorded impulse/limit state into each joint's live `b3JointSim`, wherever
   it lives (graph colour or solver set), after the generation check in step 1. Joint *settings*
   (limits, ratios, motor targets — as opposed to solver impulse/limit state) are not recorded and
   are not restored; if a caller mutates joint settings inside the prediction window and needs the
   old values back, it must replay that itself (§11), the same as any other out-of-scope mutation.

Islands, graph colours, solver-set membership, id pools and the broad-phase tree are untouched by
restore itself. They are live-world structures and the next step updates them normally — and
resim (§9.2) never touches them either, since it iterates the existing structures in place rather
than restructuring them. That is why restore cannot leave the world inconsistent: every mutation
goes through an existing engine path or an explicitly invalidated cache.

## 9. Scoped resimulation

### 9.1 Semantics

Between `b3World_BeginResimulation` and `b3World_EndResimulation`, `b3World_Step` simulates
only the scope. **Scope = the bodies with a valid record at T (§6, §8)** — restore's write-set,
exactly, never recomputed from current topology. Nothing about the live world's structure
changes to express this (§9.2): every body stays in whatever solver set and island it is already
in, every contact and joint stays in whatever graph colour it is already in. "Scope" is purely a
question of which bodies/contacts/joints get processed by this particular `b3World_Step` call and
which get treated as immovable inputs to it.

- **Scope bodies** are integrated and solved normally, including CCD against static geometry.
- **Non-scope awake bodies are frozen**: never integrated (so their position/velocity is
  unchanged by anything that happens during the bracket), never woken, and their islands are
  otherwise undisturbed. A scope body's contact or joint with one is a **boundary constraint**,
  solved directly against that body's real live `b3BodySim` — not a copy — but with its effective
  inverse mass and inverse inertia forced to zero and its velocity treated as zero for that
  constraint's solve, by reusing the exact mechanism the engine already applies to a constraint
  endpoint that isn't in the awake set (§9.2). This is exact for the frozen side (it doesn't move,
  by construction: nothing ever integrates it) and an approximation for the scope side (it is
  pushed against a partner that in reality would also react and might genuinely be moving) — the
  same approximation round 3 already documented, now backed by a real, already-tested engine code
  path instead of a new one.
- **A sleeping, non-scope neighbor may be added as a real (non-boundary) scope participant**,
  instead of treated as a boundary partner, only if it is provably unchanged since at or before T:
  its `validSinceTick` (§9.2) is `≤ T`. A sleeping body has no T-record either (it wasn't part of
  any root's capture scope at T), but if nothing about it has changed since ≤ T, its live state
  *is* T's state, so no record was ever needed. If eligible, it is woken (ordinary
  `b3WakeSolverSet`, whole sleeping set and all — see §9.2 for why that's no longer a problem) and
  added to scope; otherwise it is a frozen boundary partner like any awake non-scope body, until
  the caller's window moves past its last mutation.
- **History keeps capturing**, addressed by one persistent history-clock counter (§9.2) instead
  of `world->stepIndex`, so each resim step overwrites ring slots T+1…now with the corrected
  timeline and a later correction can roll back into it.
- **Events** (contact begin/end/hit, body move, sensor) are generated for scope bodies as usual,
  and `b3World_IsResimulating` is true while they are. The caller decides whether to act on
  resim events. Sensor overlap state is not rolled back. After resim it reflects the corrected
  timeline's final state.

### 9.2 Implementation: scoped iteration, not a parallel world

Rounds 3–5 tried to give the scope its own solver set, and then its own solver set *and*
constraint graph, so that stepping "just" the scope could reuse the normal step functions
unmodified. Three review rounds found the same class of problem each time it was patched: the
engine assumes one awake set and one constraint graph everywhere — validation, island linking,
joint creation, wake/sleep — and every attempt to stand up a second one next to it found another
place that assumption was load-bearing (moving a body with live contacts, counting graph
constraints, linking islands on joint creation, and so on). After three rounds of finding new
conflicts instead of converging, that is a sign the shape of the mechanism was wrong, not that it
needed one more patch. Round 6 replaces it with something that never creates a second copy of
anything:

- **Nothing physically moves.** Scope bodies stay in the awake set, at their existing
  `setIndex`/`localIndex`. Scope contacts and joints stay in whatever constraint-graph colour they
  are already in. There is one awake set and one constraint graph, exactly as always, so every
  existing validator, island-linking rule, and joint-creation path is completely unmodified and
  needs no auditing at all — there is nothing new for them to see.
- **A scope id set** (a bitset or hash set over body ids, sized to the scope, not the world) is
  built at Begin from the bodies restore just wrote (§6, §8) plus any sleeping neighbors added per
  §9.1. This is the only new persistent-for-the-bracket state. It answers one question, "is body
  X active in the current resim pass," in O(1).
- **Compact iteration lists are rebuilt fresh at two points in every replayed step, not once and
  not incrementally.** Round 6 originally proposed building them once at Begin and appending to
  them as new contacts/joints appear. The round 6 review found that unsound over a multi-step
  bracket: a contact moves between the awake set's non-touching array and a graph colour, can be
  destroyed with its id later reused, and a joint can be removed and its id reused too
  (`physics_world.c:968`, `contact.c:468`, `joint.c:802`) — an append-only list has no way to
  un-list a destroyed entry or notice an id was recycled for something unrelated. Round 7 fixed
  that by rebuilding once per replayed step, after collide, before `b3Solve`. The round 8 review
  found that still incomplete: it fixes solve's input but leaves collide's own input unscoped.
  `b3World_Step` runs broad-phase pair discovery, then `b3Collide` (`physics_world.c:830`) —
  which *itself* first gathers every touching contact from all graph colours plus every
  non-touching contact from the awake set's `contactIndices` into one flat array
  (`physics_world.c:830`-`870`) before processing any of them — and only after `b3Collide`
  returns does `b3Solve` run (`physics_world.c:893`, `physics_world.c:938`,
  `physics_world.c:1134`). A list rebuilt only after collide never scopes collide's own gather,
  which stays O(world contacts) regardless. Two rebuild points are needed, both using the same
  underlying walk — each scope body's current edge list, `body->headContactKey` and its joint
  equivalent, the same traversal §7's capture already does, keyed by **ids**, not cached local
  indices (see the sleep bullet below for why that matters) — at different points in the pipeline
  because collide and solve need different shapes of list:
  1. **Right after pair discovery, before `b3Collide` runs:** a flat list of every contact
     incident to a scope body, touching or not. This replaces `b3Collide`'s own gather step for
     the resimulating case, so collide itself processes O(scope + boundary) contacts instead of
     gathering the whole world's touching-plus-non-touching population.
  2. **Right after `b3Collide` returns, before `b3Solve` runs:** the solve-facing lists — touching
     contacts and joints bucketed by their *current* graph colour, plus a flat non-touching/
     integration list — rebuilt from the same scope-body edge walk, now reflecting this tick's
     just-settled touch/untouch decisions. This is also the correct point to evaluate the
     sleeping-neighbor join rule (§9.1): a new contact discovered by the scoped collide pass that
     touches an eligible sleeping neighbor is exactly what should trigger admitting that neighbor
     to scope, and it only exists once collide has run.

  Both rebuilds are O(scope + boundary): the walk is bounded by scope bodies' own incident edges
  regardless of which point in the step it runs at. One piece of bookkeeping inside `b3Collide`
  itself is not yet scoped by this: it clears and scans a per-worker contact-state bitset sized to
  the *world's* contact-id capacity, not the scoped subset (`physics_world.c:881`, `bitset.c:26`).
  The round 9 review flagged this as narrower than the gather itself and appropriate to resolve
  during the prototype (a sparse, scoped changed-contact collection instead of a world-sized
  bitset), not a reason to revise this design further.
- **Boundary treatment reuses an existing engine mechanism instead of inventing one, but needs an
  explicit mass override, not just a widened index check.** Contact prepare (`contact_solver.c`)
  already has exactly the rule a frozen partner needs: when a constraint endpoint's local index is
  `B3_NULL_INDEX` (today, that means the body isn't in the awake set at all — static, sleeping, or
  disabled), prepare forces its effective inverse mass and inertia to zero and its sampled
  velocity to zero for restitution/hit-event purposes, instead of reading a live `b3BodySim`. The
  round 6 review found that joint prepare does **not** follow the same shape: revolute, distance,
  and prismatic prepare copy both endpoints' real inverse mass and inertia into the joint's solve
  state *before* deciding whether either gets a nullable solver index (`revolute_joint.c:279`,
  `distance_joint.c:268`, `prismatic_joint.c:320`), so widening the index check alone still leaves
  a frozen partner's real, nonzero mass in the constraint math — the mass override has to happen,
  not just the index. The fix is the same shape but applied earlier in each function: **before**
  a joint type computes effective mass/inertia for either endpoint, check whether that endpoint's
  body id is in the scope id set (true unconditionally outside resim) and substitute zero for its
  inverse mass and inertia if not, then proceed with the existing math unchanged. For contacts,
  the fix is narrower: contact prepare already zeros mass correctly for a non-participating index,
  but its stored awake-index encoding is itself validated elsewhere (`contact_solver.c:86`), so
  resim must **not** rewrite the contact's own stored index to express "frozen" — it must derive a
  separate, pass-local index/mass decision used only within that prepare call, leaving the
  contact's persistent encoding and its validation invariant untouched. This is still a real,
  mechanical audit across every joint type file plus the contact solver (there is no way around
  touching each one, and rounds 4/5 were right that this is non-optional cost), but each site's
  change is a small, localized override, not a new kind of solver participant or storage.
- **Ordinary sleep processing may only run on all-scope islands during resim, not on any island
  that merely contains a scope body.** The round 6 review found a real correctness bug in the
  original proposal, not just a cost one: island-sleep finalization marks an island's bit awake
  only when a *visited* body in it reports a short enough sleep timer, then tries to sleep every
  island whose bit didn't get set that step (`solver.c:816`, `solver.c:2221`-`2240`). Round 7's
  fix — skip any island with no scope-body member — stops unrelated islands from being swept, but
  the round 7 review found it still wrong for a **mixed** island (a live island containing both
  scope and non-scope bodies, e.g. a scope body resting on a pile that also touches unrelated
  bodies not in any root's history): finalization only visits the scope member, so only it gets a
  chance to hold the island's bit awake; an unvisited non-scope member that would, if visited,
  still be moving too fast to sleep never gets that chance. If the scope body's own timer says
  sleepy, "has a scope member" lets `b3TrySleepIsland` run on the *whole* island — which puts
  every member to sleep, scope and non-scope alike, via the ordinary sleep-transition code
  (`solver_set.c:216`) — even though the non-scope members were never actually evaluated. That
  breaks the frozen-body contract for exactly the bodies §9.1 says must be left untouched. The
  correct gate is narrower: **only an island whose entire membership is already in the scope id
  set may run ordinary sleep evaluation during resim; a mixed island's sleep decision is deferred
  entirely until `EndResimulation`**, left exactly as it was (awake, timers whatever they were),
  and re-evaluated normally on the first ordinary step after End, when every one of its members is
  visited again as usual. A **fully-scope island legitimately falling asleep mid-bracket is
  allowed and correct** — physically, it can happen, and it must go through the engine's ordinary
  sleep-transition code (swap-compacting the awake set, moving the body/contacts/joints to a new
  sleeping set, `solver_set.c:216`) exactly as it would outside resim. That transition changes local indices for
  other, unrelated bodies via swap-compaction, which is why the scope id set and the compact lists
  (above) are keyed by stable ids and rebuilt fresh every step rather than caching local indices
  across steps: a mid-bracket sleep transition invalidates cached positions, never cached ids.
- **A pending island split needs its own slot to defer through a bracket, not "leave it alone."**
  `world->splitIslandId` is a single-candidate relay, not a queue: at the start of each step's
  sleep processing, whatever candidate the *previous* step left there is executed and the slot is
  unconditionally cleared to `B3_NULL_INDEX`, then a fresh candidate for the *next* step is
  collected into that same now-empty slot, asserting it was empty first (`solver.c:1861`,
  `solver.c:2201`). The round 8 review found that "just leave it alone" doesn't work with a
  single-slot relay: if resim leaves a non-scope candidate sitting in `world->splitIslandId` at
  Begin, the very first replayed step's ordinary logic executes it unconditionally before resim's
  own code gets a chance to object — there is no way to "skip" a value already sitting in the slot
  the mechanism always acts on. And separately, collide can merge away the very island a deferred
  candidate names, invalidating its id (`island.c:166`, `island.c:49`), so simply holding onto the
  id isn't safe either. The fix needs a second, resim-only field: at Begin, if `world->
  splitIslandId` currently names a candidate that is not an all-scope island, move its value into
  `world->pendingSplitIslandId` and reset `world->splitIslandId` to `B3_NULL_INDEX` — so the
  ordinary per-step relay finds nothing to execute and is free to collect its own, scope-gated
  candidate (per the all-scope rule above) each replayed step, exactly as it always does, without
  colliding with the deferred one. "Still names a live island" needs an identity guard, not just
  an existence check: destroying an island frees its id, and a new island can later be allocated
  the same id (`island.c:49`, `id_pool.c:19`), so a naive liveness check could match the pending
  candidate to an unrelated, newly-created island of the same id. The simplest guard is to clear
  `world->pendingSplitIslandId` immediately and unconditionally whenever an island is destroyed
  (one added line at that existing call site) rather than re-deriving liveness later from the id
  alone. At `EndResimulation`, if `world->pendingSplitIslandId` is still valid, execute its split
  immediately and synchronously, right there, then clear it — before resuming ordinary stepping —
  rather than trying to hand it back into `world->splitIslandId`, which may by then legitimately
  hold a fresh candidate resim's own last replayed step collected; this avoids ever needing two
  live candidates in one slot.
- **The existing parallel solver's batching still needs pass-local variants, which this doc does
  not yet specify.** The round 6 review found that per-colour ids are not, by themselves, enough
  to feed the solver's internal parallel machinery: its stages size buffers from a colour's full
  count, pack convex contacts into SIMD groups, assign mesh-manifold offsets, and use a separate
  serial overflow path for whatever doesn't fit the packed layout (`solver.c:1471`,
  `solver.c:1528`, `contact_solver.c:2270`). The compact per-colour lists give a conflict-free
  *subset* of a colour's ids (the graph's colouring guarantee still holds for any subset), but
  actually processing only that subset in parallel requires pass-local counts, spans, and packed
  buffers sized to the subset, plus a scoped variant of the serial-overflow path — new
  implementation work, not implied by "iterate a smaller list" alone. This is scoped to the
  solver's existing internals (`solver.c`, `contact_solver.c`) and does not change graph colouring
  or ownership, but it is real, not free, and belongs in the prototype's first pass.
- **`validSinceTick`** is a per-body stamp (new persistent state, one integer per body) meaning
  "the first history tick for which this body's current live state is the correct historical
  value" — used by §9.1's sleeping-neighbor join rule. The round 6 review found the original rule
  (stamp with the current `historyTick`) off by one in exactly the case that matters: a mutation
  or sleep-transition that happens *while tick T+1 is being produced* — before that step's
  end-of-step increment (below) has run — would read the still-current value `T`, understating
  when the change actually took effect and letting a body that changed strictly after T pass a
  `≤ T` check. The fix is to stamp with **`historyTick + 1`**, not `historyTick`, at the moment of
  the event: this is "the tick this event's result will belong to once the in-progress step
  finishes," which is correct both for an API call made between two `b3World_Step` calls (where it
  equals the tick about to be produced) and for an internal event like sleep-entry that happens
  mid-step before the increment (`solver.c:2192`) — either way, the stamped value is the first
  tick for which the new state is authoritative. This is stamped at exactly two kinds of event:
  (1) sleep-transition, when a body's island goes to sleep (`b3TrySleepIsland`); (2) any API call
  that can mutate a sleeping body's state without waking it — `b3Body_SetTransform`,
  `b3Body_SetLinearVelocity`/`SetAngularVelocity`, `b3Body_SetMassData` (`body.c:1856`), force/
  impulse application, and any future one, which is a real, necessary audit surface, not a detail
  to defer. Waking a body does *not* update its stamp — only a state-defining event does — so a
  body that restore's step-2 wake incidentally touches gains no eligibility it didn't already
  have. A sleeping neighbor joins scope only if `validSinceTick ≤ T`.

  **Why the comparison stays sound even though `validSinceTick` is persistent across many
  brackets and `historyTick` is deliberately rewound inside each one** — the round 7 review
  correctly rejected the round 6 draft's "brackets never coexist" argument, since a stamp set once
  is compared against every *later* bracket's T, not just the one it was set during. The real
  reason it stays sound: **tick numbers are never reused for different states.** Slot contents at
  a given tick number can be overwritten by a later resim (that's the whole point of restore), but
  the *number* itself always refers to the same point in the timeline's logical order, and
  `historyTick` only ever takes a value ≤ the highest tick number the session has produced so far
  — a rewind moves it backward to re-produce ticks that were already going to exist, never forward
  past "now." A stamp is always the earliest tick number for which its body's mutated state is
  claimed authoritative — that is what makes it a **conservative** bound: if a later correction's
  T is at or after that number, the comparison `validSinceTick ≤ T` correctly reflects "this body's
  state hasn't changed since T," and if some *even later* event (from a different, subsequent
  bracket or from ordinary stepping) has since moved the body again, that event overwrites the
  stamp with a new value reflecting *when that event took effect* — the stamp is a single field
  per body, always holding its most recent state-defining event's tick, not necessarily a
  numerically larger one (a later event during a rewound bracket can legitimately write a smaller
  tick number than a stamp from a still-later ordinary-time mutation that hasn't happened yet at
  that point). What never happens is the stamp *understating* how recently the body's live state
  actually changed relative to the timeline being reasoned about: it can only ever be conservative
  (making a body ineligible when it happens to still be fine), never wrong in the direction that
  would admit a body whose state doesn't actually match T.
- **Begin:** build the scope id set and the first replayed step's compact lists as above. Cost:
  O(scope + boundary). Also establish the history-clock guard below.
- **End:** discard the scope id set and compact lists. There is nothing to move back, because
  nothing moved. Cost: O(scope + boundary). No solver-set or graph validation is affected by
  Begin/End at all, since the world's structure — which sets and graph colours own which bodies,
  contacts, and joints — is identical before Begin and after End. (Ordinary validation still runs
  wherever it always does; this design simply gives it nothing new to check.)
- **One history-clock counter addresses ring slots, with an explicit guard instead of a prose
  promise.** `world->stepIndex` (incremented once per `b3Solve`, `solver.c:1444`) is owned by the
  solver for its own purposes and is never touched by rollback. Capture instead uses
  `world->historyTick`, which names **the tick most recently completed and written**: at the end
  of every `b3World_Step` — ordinary or resimulated — `historyTick` is incremented first, then
  that tick's record is written to slot `historyTick % historyLength`. The round 6 review found
  that naming a "last restored tick" isn't itself a guard against an ordinary step sneaking in
  between restore and Begin (`physics_world.c:1038` runs the same step path regardless): the
  design needs actual state, not a promise in prose. `b3World_RestoreRollback(worldId, T)` sets
  `world->pendingRestoreTick = T` and `world->hasPendingRestore = true`; any ordinary
  `b3World_Step` call (outside a resim bracket) clears `hasPendingRestore` to `false` as one of
  its first actions, unconditionally. `b3World_BeginResimulation` takes no tick argument (§10) and
  instead asserts `hasPendingRestore` is still `true` (a caller-contract violation otherwise: an
  ordinary step ran between restore and Begin), consumes it (sets it back to `false`), saves the
  current `historyTick` into `world->resimTargetTick` (this is "now" — the tick to return to), and
  only then sets `world->historyTick = world->pendingRestoreTick` — "we are positioned as though T
  was the last completed tick." The first resimulated step then increments to `T + 1` and writes
  to slot `(T + 1) % historyLength`, exactly where that tick's original data lives, and so on.
  `b3World_EndResimulation` asserts `historyTick == world->resimTargetTick`: the replay loop must
  call `b3World_Step` exactly once per tick from `T + 1` to `now` (§10). Ordinary stepping after
  End simply keeps incrementing the same counter from `now` — there is no separate "what tick does
  capture resume at" question, because there is only ever one counter and one write/increment
  rule, used identically whether resimulating or not.

Rejected alternative (rounds 3–5): a dedicated scope solver set and constraint graph that bodies/
contacts/joints physically transfer into and out of. It would let solve/collide reuse existing
per-set/per-graph entry points unmodified, but three rounds of review found it repeatedly conflicts
with the engine's one-set/one-graph assumptions (validator ownership counts, island linking on
joint creation, wake transferring whole sleeping sets, a moved body's live contacts) faster than it
converged. Reusing the engine's *existing* zero-mass dummy-endpoint mechanism in place, instead of
building a second world to move things into, avoids that entire class of problem by never changing
what the live world's structure looks like.

### 9.3 A cost gap this design does not close: broad-phase and sensors

Collide + solve above are O(scope + boundary), but two step stages are not scoped by this design
and remain O(world):

- **Broad-phase pair discovery is a full scan, not a query of what moved.** When any tree needs
  an update, `b3UpdateBroadPhasePairs` calls `b3GatherMovedSiblings`, which walks every sibling
  pair in the *entire* dynamic tree checking a moved flag (`broad_phase.c:83`) — O(total dynamic
  proxies in the world), not O(moved proxies), regardless of how few of them are in the scope.
- **Sensor overlap is a flat scan of every sensor in the world.** `b3OverlapSensors` runs a
  parallel-for over `world->sensors.count` whenever it is nonzero (`sensor.c:281`), with no
  filter for whether a sensor could plausibly touch the scope.

Making either of these scope-aware (an incrementally-maintained moved-proxy list instead of a
full-tree scan; a sensor-to-body/AABB index to skip sensors nowhere near the scope) is its own
engine change, deferred to §12. Until then, this design's O(scope) claim covers collide + solve
only; total resim cost is `O(scope + boundary) + O(dynamic proxies in the world, when something
moved) + O(sensor count)`. For most scenes the first term dominates and the other two are cheap
relative to a full narrow-phase + solve pass over the same population, but that must be measured,
not assumed: §13 adds broad-phase and sensor time as their own reported columns, separate from
collide/solve time, so a scene with many sensors or a very large dynamic tree shows up as a
distinct cost rather than being hidden inside a passing "resim time" number.

### 9.4 Cost

Resim collide+solve cost ≈ N × (collide+solve cost of the scope); §9.3's broad-phase/sensor terms
add on top. Scaling the measured whole-world step times by scope fraction gives these estimates
for the collide+solve term, to be confirmed by the benchmark in §13:

| scene | awake | largest island | 9-tick resim, whole world (measured) | 9-tick resim, largest-island scope, collide+solve (est.) |
|---|---|---|---|---|
| many_pyramids | 10,780 | 55 | 106 ms | < 1 ms |
| rain | 2,520 | 36 | 32 ms | < 1 ms |
| sleep | 2,100 | 210 | 14 ms | ~1.5 ms |
| convex_pile | 5,120 | 2,652 | 49 ms | ~25 ms |
| large_pyramid | 5,050 | 5,050 | 46 ms | ~46 ms (inherent) |
| junkyard | 10,585 | 9,726 | 228 ms | ~210 ms (inherent) |

When the scope *is* the world, no scoping scheme helps. That is requirement 4's case, and it is
why the API exposes scope size before restore and lets the caller cap it (§10).

## 10. API sketch

```c
typedef struct b3RollbackDef
{
	// Number of ticks retained. Restore is possible for any tick in (now - historyLength, now].
	int historyLength;

	// Byte cap for the history arena. A capture that would exceed it records an overflow
	// slot instead of growing. 0 = unbounded.
	int maxHistoryBytes;

	// Scope size above which a capture records an overflow slot (restore to it fails fast).
	// Lets callers bound worst-case resim cost. 0 = unbounded.
	int maxScopeBodies;
} b3RollbackDef;

B3_API b3RollbackDef b3DefaultRollbackDef( void );

// Enable/disable history capture for this world. Capture then runs at the end of every
// b3World_Step. Disabling frees the history.
B3_API void b3World_EnableRollback( b3WorldId worldId, const b3RollbackDef* def );
B3_API void b3World_DisableRollback( b3WorldId worldId );

// Mark a body as a rollback root. Capture scope = islands containing any root (+ touching
// kinematics, §6); the resim/restore scope at any given tick is whatever was actually recorded.
B3_API void b3Body_SetRollbackRoot( b3BodyId bodyId, bool flag );
B3_API bool b3Body_IsRollbackRoot( b3BodyId bodyId );

typedef enum b3RollbackResult
{
	b3_rollbackOk,
	b3_rollbackTickUnavailable,  // outside the retained window
	b3_rollbackScopeOverflow,    // the capture at that tick exceeded maxScopeBodies / maxHistoryBytes
} b3RollbackResult;

typedef struct b3RollbackInfo
{
	b3RollbackResult result;
	int bodyCount, contactCount, jointCount; // scope size recorded at that tick
} b3RollbackInfo;

// O(1). Lets the caller decide between resim and a cheaper fallback (snap/smooth) up front.
B3_API b3RollbackInfo b3World_GetRollbackInfo( b3WorldId worldId, uint64_t tick );

// Restore the scope to its state at the end of `tick` (§8). Call at a step boundary.
B3_API b3RollbackResult b3World_RestoreRollback( b3WorldId worldId, uint64_t tick );

// Scoped resimulation (§9). BeginResimulation takes no tick: it reads the tick RestoreRollback
// most recently established (§9.2) and asserts no ordinary step happened in between. Typical use:
//   if ( b3World_RestoreRollback( w, T ) == b3_rollbackOk ) {
//       apply server correction to predicted bodies;
//       b3World_BeginResimulation( w );
//       for ( t = T + 1; t <= now; ++t ) { apply buffered inputs for t; b3World_Step( w, dt, sub ); }
//       b3World_EndResimulation( w );
//   }
B3_API void b3World_BeginResimulation( b3WorldId worldId );
B3_API void b3World_EndResimulation( b3WorldId worldId );
B3_API bool b3World_IsResimulating( b3WorldId worldId );
```

A tick is the ring-slot index assigned by `world->historyTick` (§9.2), numbered one-to-one with
steps at all times — ordinary or resimulating — so the caller's own tick counter can just count
steps; `world->stepIndex` is a separate, solver-owned counter the caller never needs.
`Begin/EndResimulation` must bracket steps at the same `timeStep`/`subStepCount` as the original
steps. This is documented rather than enforced. The record could carry both values and assert in
debug builds.

## 11. Caller contract and limitations

- **Inputs include every mutation of scope bodies**: forces, impulses, velocity sets, teleports,
  kinematic target velocities. The caller replays them per tick during resim, exactly as for
  the predicted body itself.
- **Out-of-scope mutations are not rolled back**: static body transforms, shape
  geometry/material/filter/sensor settings, world settings. Resim uses their live values. A
  caller that changes these inside the window and needs the old values must replay them itself.
  This is the same class of limitation as any partial-rollback design, and box3d can't infer
  historical values it didn't record.
- **Bodies created during the window** have no record at T. They stay in the world at their
  current state and are treated like any other non-scope body during resim: frozen if awake,
  joinable if asleep. Callers that want to un-spawn a mispredicted projectile do so in their
  own logic, as the Overwatch talk describes.
- **Bodies destroyed during the window** are skipped by restore (generation check). A destroyed
  root simply drops out of the scope.
- **Frozen bodies are an approximation.** A scope body that hits an awake non-scope body during
  resim treats it as immovable at its current pose, via the same zero-mass, zero-velocity
  treatment the engine already gives a static or sleeping constraint partner (§9.2). Callers can
  widen the scope by adding roots. This trade-off is what keeps resim O(scope + boundary).
- **A body not provably unchanged since T is treated as frozen, not joined** (§9.1, §9.2), until
  the caller's replay moves past its last state-defining mutation. Its live state is otherwise
  indistinguishable from an awake frozen body from the scope's point of view.
- **Mesh and height-field contacts are never warm-started across a restore** (§7.1): they are
  always recreated cold, the same as a brand-new contact. Only convex-vs-convex contacts carry
  impulses through a restore.
- **Restore destroys and recreates every contact incident to a scope body** (§8, step 4): a pair
  touching both before and after restore reports a spurious end-touch/begin-touch event pair and
  gets a new contact id/generation. A caller holding a contact handle across a restore, or relying
  on contact-event continuity through one, sees a separation-and-reformation that didn't really
  happen. This is a deliberate trade for guaranteed-cold narrow-phase state, not an oversight.
- **Joint settings are not restored**, only solver impulse/limit state (§8, step 5). A caller
  that mutates a joint's limits, ratio, or motor target inside the window and needs the old value
  back must replay that mutation itself.
- **Broad-phase pair discovery and sensor overlap are not scoped** (§9.3): their cost during
  resim is proportional to the whole world's dynamic-proxy and sensor counts, not to the scope,
  until the follow-on work in §12 lands.
- **Worker count** must stay fixed across capture and resim, as for the Recording system.

## 12. Future work (explicitly not v1)

- **Bit-exact whole-world save/restore** for small-world lockstep/GGPO games: flat memcpy of
  the awake state, dirty-tracked sleeping sets and static tree, reusing the
  `world_snapshot.c` checklist. The measurements say it is viable up to ~500 awake bodies.
- **Bit-exact scoped rollback** would first require removing global-id ordering from the solver.
  For example, process begin-touch in shape-pair-key order instead of contact-id order. That is
  a solver change with its own cost and review.
- **Rolling back sensor overlap state** if callers need sensor events to be exactly
  timeline-consistent after a resim.
- **Scope-aware broad-phase and sensor processing** (§9.3): an incrementally-maintained
  moved-proxy list in place of `b3GatherMovedSiblings`'s full-tree scan, and a sensor index that
  skips sensors nowhere near the scope, so total resim cost is O(scope + boundary) rather than
  O(scope + boundary) plus O(world) for these two stages.

## 13. Validation plan

Add a `rollback` mode to `benchmark/main.c`, built with `B3_ENABLE_VALIDATION` so
`b3ValidateSolverSets`, `b3ValidateContacts` and `b3ValidateConnectivity` are active
(`physics_world.c:3534`). For each stress scene at mid-run, pick roots in the smallest, median
and largest island, then report:

- capture time per tick, and as a percentage of step time;
- history bytes per slot;
- restore time;
- 9-tick scoped resim time vs 9 whole-world steps, broken out into collide+solve time,
  broad-phase pair-discovery time, and sensor-overlap time (§9.3), not one combined number.

Acceptance criteria:

- **Capture:** median-island scope costs < 1% of step time; largest-island scope stays below the
  whole-world memcpy measured in the review (~20–35% of step).
- **Resim:** collide+solve time scales with scope (scope ÷ world tracks scope body count ÷ awake
  body count, plus a constant overhead to be measured); broad-phase and sensor time are reported
  separately and are expected to track world size, not scope, per §9.3.
- **Robustness:** 1,000 random restore/resim cycles per scene with unrelated bodies moving,
  sleeping and colliding, in a validation build (`B3_ENABLE_VALIDATION`). Validation passes after
  each cycle, and no restore is rejected except for documented reasons (tick unavailable,
  overflow).
- **Quality:** in a scripted scene, restore T → resim to now → compare the predicted body's
  final position and orientation against an un-rolled-back run, measured once at the end of the
  replayed window (not compounded per tick). Position divergence stays within 1% of the body's
  characteristic size (its AABB extent) — for a 1 m box, under 1 cm. Orientation divergence,
  measured as the quaternion geodesic angle between the two final orientations (reducing to the
  angle difference in 2D), stays under 2 degrees. Both after a 9-tick correction; a scene that
  exceeds either bound is a design bug, not expected behaviour, since v1's whole point is
  "consistent," not "unconstrained."

## 14. Open questions

1. Should resim events be generated (current proposal, flagged by `b3World_IsResimulating`)
   or suppressed by default?
2. Is `maxScopeBodies` the right budget knob, or should it be `maxScopeContacts` (contacts
   dominate cost in dense scenes such as `junkyard`: 140k contacts vs 10.5k bodies)?
3. Should the scope-set/scope-graph machinery (§9.2) be exposed on its own, e.g. as
   `b3World_StepBodies(bodies)`? It has uses beyond rollback, such as stepping a single
   subsystem, but it widens the public surface.
4. Per-body `validSinceTick` (§9.1) is new persistent state, updated by every mutating body API.
   Is tracking it unconditionally worth the bookkeeping and the audit surface, or should it be
   opt-in (only maintained for worlds with rollback enabled)?
