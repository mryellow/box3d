---
title: Preflight - tree-order-independent solver changes
status: Draft
date: 2026-10-12
---

# Preflight: tree-order-independent solver changes

One PR to Box3D, carrying four changes (A0-A3). Together they make continuous collision and
`b3World_Explode` give the same answer for a given world state whatever the internal layout of the
broad-phase trees is. The PR is a determinism hardening that upstream can accept on its own merits.

## 1. Why these changes, and only these

Tree layout depends on how a world's trees were built and maintained: proxy insertion order,
incremental rebuilds, `b3World_RebuildStaticTree`. Two worlds with identical bodies, shapes, ids and
state can therefore have different layouts. The guarantee is for fixed identities and state, with only
the layout varied. An audit of what the step reads from the trees finds three places where a result
depends on the order a tree hands back candidates:

1. **CCD solid minimum.** `b3ContinuousQueryCallback` (`src/solver.c`) passes the running best
   fraction into the next candidate's time-of-impact query.
2. **CCD sensor hits.** The same callback collects up to `B2_MAX_CONTINUOUS_SENSOR_HITS` (8) sensor
   candidates, gated on the running solid fraction. Which 8 fill the cap depends on visit order.
3. **`b3World_Explode`.** `ExplosionCallback` (`src/physics_world.c`) wakes bodies and accumulates
   impulses in the traversal order of the dynamic tree, the only tree it queries. Wake order fixes
   awake-set order, constraint colours and overflow order, and a body with several shapes in range sums
   floats in a layout-dependent order.

Everything else the step reads from a tree is already order-independent: pair discovery is sorted
by shape-pair key (`src/broad_phase.c`), end-of-step sensor hits are sorted and deduplicated
(`src/sensor.c`).

### Behaviour change, stated up front

Results stay physically comparable but are **not bit-identical to today's** in the affected cases.
Any recording (`.b3rec`) or golden hash captured before this PR may stop validating in them. The PR
description must say so in one line. The cases:

- **CCD sweeps with several competing candidates.** Impact fractions can differ, with no bound,
  because each candidate's time-of-impact query runs to a different limit. That also changes the
  point and normal a `preSolve` callback receives, so its decision can change even for a callback
  that depends only on its arguments. Because `b3TimeOfImpact` judges separation at its limit, a
  sweep that enters and leaves an obstacle within the step can be missed where today's shorter
  running limit caught it.
- **CCD sweeps with more than 8 sensor candidates.** The eight kept are the eight lowest ids, not the
  eight nearest, and a hit at exactly the solid fraction no longer takes a slot. Because a sensor's
  TOI limit is now the final solid fraction, never above the running fraction it sees today, a sensor
  event can appear or disappear even below the cap.
- **Mesh or compound sensors beyond a solid hit.** Today they can be reported at fraction 0 when
  visited after the solid hit. After A2 they can be reported whatever the visit order, subject to the
  eight-pair cap, and can displace genuine sensor hits with higher ids.
- **Explosions touching a multi-shape body, or waking more than one sleeping set.**
- The public CCD iteration counters (`distanceIterations`, `pushBackIterations`, `rootIterations`) can
  change even where the collision result does not.

## 2. Commit sequence

```
A0 tree-layout test helper ── A1 CCD solid min ── A2 CCD sensor hits ── A3 Explode order
```

The PR holds A0 to A3 as one commit each, in that order, so each stays bisectable. A1 -> A2 is a real
dependency (A2 evaluates sensors against A1's final solid fraction). A3 shares no code with A1 and A2
but its test uses the A0 helper. A0 has no behaviour change and is the first commit. The PR forks from
`main`.

### A0. Test helper: perturb tree layout

**Scope.** `test/` only. No `src/` change.

**Change.** A helper that runs the same scene twice and compares per-step state hashes, where the
second run perturbs tree layout. Perturbation is a full `b3DynamicTree_Rebuild( tree, true )` on
each world tree, called directly (`b3World_RebuildStaticTree` also writes a recording command, which
is state). A full rebuild of a nonempty tree keeps proxy ids, leaf bounds and moved marks and sets
the tree ordered; nothing else it writes is simulation state. On an empty tree it returns before
writing anything. The ordered flag gates the step's pair update together with the moved marks
(`src/broad_phase.c`), but pair discovery starts only from moved marks, so entering on the flag
alone finds no pairs and only rebuilds the dynamic and kinematic trees, which is layout. The helper
can perturb before each step or immediately before a named call such as `b3World_Explode`. It also
provides a visit-order witness: a way for a test to record, for the query under test, the sequence
of candidates the query hands back. Tests may include internal headers (`test_broad_phase.c` already
does).

**Check before merging.** For the query a test relies on (the CCD queries, or the Explode query), the
witness must show a different candidate visit order between the two runs, otherwise the test passes
vacuously. `nodes` order is not enough: a query follows child links, and the step rebuilds the dynamic
and kinematic trees before CCD.

**Done when.** A sanity test shows the helper changes the visit order at the CCD queries and at the
Explode query, and leaves the hash of a scene with no CCD, sensor or explosion unchanged.

### A1. CCD: order-independent solid minimum

**File.** `src/solver.c`: `b3ContinuousContext`, `b3ContinuousQueryCallback`, `b3SolveContinuous`.

**Change.**
- Record `limit = context.fraction` once per fast shape, before the first of its tree queries
  (static, then kinematic and dynamic for bullets). Every candidate's `b3ShapeTimeOfImpact` call in
  that shape's queries uses `limit`, not the running value.
- Track the accepted minimum in a separate field. After the queries, `context.fraction` becomes
  that minimum. Fast shapes of one body are visited in the body's shape list, which is not
  tree-dependent, so carrying the result into the next shape is fine.
- The accept test stays `0 < output.fraction && output.fraction < limit`, then `preSolve`.
- Sensor candidates are otherwise untouched: their TOI call and accept test also use `limit`, and the
  end filter against the accepted minimum still applies. Their selection stays visit-order-dependent
  until A2.
- Iteration statistics (`distanceIterations`, etc.) take a max over accepted candidates. The
  accepted set is now order-independent, so these are too.

**Costs and caveats.**
- The number of `b3ShapeTimeOfImpact` calls is unchanged, but each now runs to `limit` rather than the
  running best, so a call can do more work on a multi-candidate sweep. Report the benchmark delta
  (`benchmark/`, CCD-heavy scenes) in the PR.
- `preSolve` is now called for every candidate nearer than `limit`, not only those nearer than the
  running best. Its return value still affects only that candidate, so the result is
  order-independent for a `preSolve` or custom filter that depends only on its arguments, though its
  inputs (the point and normal from a different TOI bracket) can differ from today's. A
  *stateful* `preSolve` sees more calls, in tree order, and a stateful custom filter, called once per
  visited candidate, already sees tree order; both stay layout-dependent. State
  this in the PR and in `docs/simulation.md` next to the pre-solve description, replacing the sentence
  there that says pre-solve does not work with high-speed collisions.
- Alternative if reviewers dislike the extra calls: collect candidates, sort by shape id, evaluate
  in that order with the running clamp. Same determinism, deterministic `preSolve`
  call order, at the price of a per-worker scratch array. Not chosen because it adds allocation to
  a hot, parallel path; offer it if asked.

**Tests.** Seeded fuzz: bullets and fast dynamic bodies against a field of static boxes with several
overlapping candidates, many seeds, A0 perturbation, compare hashes. The divergence is small and rare, so
the PR must record how many seeds failed before the change. If none fail, say so; the change is
still justified by the argument above.

### A2. CCD: order-independent sensor hits

**File.** `src/solver.c`.

**Change.**
- Pass 1 (A1's) skips sensor candidates and no longer writes `fastShape->aabb`, so `box1` survives it.
- Pass 2 is a second loop over the body's fast shapes, after pass 1 has made the solid fraction
  final. It exists only when `world->sensors.count > 0`; otherwise pass 1 writes `box2` as today. For
  each shape it recomputes `box2` from the same transform. For the shapes pass 1 queried (not mesh,
  height or sensor fast shapes) that have `b3_enableSensorEvents` set, it re-queries the same trees
  with `union(box1, box2)` and evaluates only sensor candidates with
  `b3ShapeTimeOfImpact` against the final solid fraction. Then it writes `box2` to `shape->aabb` for
  every shape of the body, including the ones pass 1 skips, because the no-hit branch reads
  `shape->aabb` as `box2`. The callback never reads the fast shape's own `aabb`.
- It keeps the eight smallest `(sensorId, visitorId)` pairs among those with fraction strictly below
  the solid fraction. Storage is the existing fixed eight-slot buffer, replace-largest on insert. The
  kept set, the eight smallest of a set, does not depend on visit order.
- The existing filter at the end of `b3SolveContinuous` (`sensorFractions[i] < context.fraction`)
  becomes redundant; keep it as an assert.

**Cost.** None when the world has no sensors. Otherwise one extra traversal per queried tree for each
fast shape pass 2 queries (one tree, three for a bullet), plus the `box2` recomputation. Report the
benchmark delta.

**Tests.** More than 8 sensors along a bullet's path (deterministic repro, fails before), plus a
sensor and a solid hit at exactly equal fractions, under A0 perturbation. Assert the same eight
sensors are reported and that no convex sensor lies beyond the solid hit.

### A3. Explode: deterministic wake and impulse order

**File.** `src/physics_world.c`: `ExplosionCallback`, `b3World_Explode`.

**Change.**
- The callback only collects `shapeId` for shapes with `explosionScale != 0` into a temporary array
  (`b3Array` on the project allocator, freed before return; the query is serial). It holds every
  candidate the broad phase returns with a nonzero `explosionScale`, including shapes outside the
  blast distance, so scratch memory and the sort are proportional to that count; allocation failure
  asserts as elsewhere.
- After `b3DynamicTree_Query`, sort the ids ascending (`src/qsort.h`) and run the current callback
  body for each in that order, unchanged: distance test, `b3WakeBody`, impulse. Wake order fixes
  colour and overflow-colour order of woken constraints, and sorted order fixes the float addition
  order for a body hit through several shapes.
- `world->locked` stays set across both the query and the loop.

**Tests.** A body with three shapes inside the radius, plus several single-shape bodies sleeping in
range, exploded under A0 perturbation of the dynamic tree applied immediately before the explosion.
The test first uses the A0 witness to require a different `ExplosionCallback` visit order between the
runs, then asserts equal body velocities bit for bit and equal awake-set order. Fails before the
change.

## 3. Review checklist

- [ ] Diff touches only the files listed for its changes, plus a test and the docs sentence.
- [ ] New tests fail on `main` and pass on the branch (or the PR says why one cannot fail).
- [ ] Existing `test` target passes, including `test_determinism.c`.
- [ ] Benchmark delta quoted for A1 and A2 separately, one per commit.
- [ ] PR text states the not-bit-identical-to-before consequence in one line.

## 4. Out of scope

- Making each island's result independent of the rest of the world.
- Public query results (ray casts, shape casts, overlaps): their callback order and tied-hit results
  stay layout-dependent.
- Callbacks that depend on call order (A1 caveats).

## 5. Provenance

- **Public ray and shape casts are outside the guarantee.** The step never casts, so their callback
  order and tied-hit results are not part of what this PR makes layout-independent.
- **Initial-fraction evaluation is chosen over sorting.** Sorting would also fix `preSolve` call
  order, but it adds a per-worker scratch array to a hot, parallel path. The cost is that the
  guarantee holds only for callbacks that depend on their arguments alone (A1 caveats).
- **A2 and A3 are part of the PR.** Each is a separate commit, so review can drop either without
  touching A0 or A1 (A2 depends only on A1; A3 only on A0). The guarantee in §1 then narrows to what
  remains.
- **Sensor reporting threshold is unchanged.** A2 reports a TOI output exactly as today: any output
  with a fraction strictly below the solid fraction (today's buffer admits fractions equal to the
  running solid fraction and only the final filter drops them, so an equal-fraction hit no longer
  occupies one of the eight slots), including a sensor the shape starts inside
  (fraction 0) and the zero-initialised result a composite shape returns when no child hits. Requiring
  a hit state would drop the overlapping sensors that are reported today.
- **The `docs/simulation.md` statement that sensors do not detect passage within a step is not
  changed.** A2 changes which CCD sensor hits are reported, not whether CCD sensor detection exists;
  the statement predates this PR.
- **Sensor TOI runs against the final solid fraction.** A limit of the full sweep judges separation at
  its end, so a sensor a bullet is still inside when it hits a solid can be missed. The cost
  is that a mesh or compound sensor beyond the solid hit can return the zero-initialised result and
  be reported at fraction 0, as it can today when visited after a solid hit; the reporting threshold
  above is unchanged.
