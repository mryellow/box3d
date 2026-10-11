---
title: Preflight - sorted-candidate tree-order-independent solver changes
status: Draft
date: 2026-10-12
---

# Preflight: sorted-candidate tree-order-independent solver changes

One PR to Box3D, carrying three changes (A0-A2). Together they make continuous collision and
`b3World_Explode` give the same answer for a given world state whatever the internal layout of the
broad-phase trees is, by evaluating tree candidates in ascending shape-id order. The PR is a
determinism hardening that upstream can accept on its own merits. It is an alternative to
20261011-001: the same guarantee with no additional time-of-impact calls and no second tree traversal;
the owner lands one of the two.

## 1. Why these changes, and only these

Tree layout depends on how a world's trees were built and maintained: proxy insertion order,
incremental rebuilds, `b3World_RebuildStaticTree`. Two worlds with identical bodies, shapes, ids and
state can therefore have different layouts. The guarantee is for fixed identities and state, with only
the layout varied, and for user callbacks whose decisions depend only on their arguments. An audit of what the step reads from the trees finds two places where a result
depends on the order a tree hands back candidates:

1. **Continuous collision.** `b3ContinuousQueryCallback` (`src/solver.c`) evaluates each candidate
   as the tree visits it. The running best fraction gates the next candidate's time-of-impact query,
   `preSolve` and custom-filter callbacks are called in visit order, and the first
   `B2_MAX_CONTINUOUS_SENSOR_HITS` (8) sensor candidates to pass the running gate fill the sensor
   buffer.
2. **`b3World_Explode`.** `ExplosionCallback` (`src/physics_world.c`) wakes bodies and accumulates
   impulses in the traversal order of the dynamic tree, the only tree it queries. Wake order fixes
   awake-set order, constraint colours and overflow order, and a body with several shapes in range sums
   floats in a layout-dependent order.

Everything else the step reads from a tree is already order-independent: pair discovery is sorted
by shape-pair key (`src/broad_phase.c`), end-of-step sensor hits are sorted and deduplicated
(`src/sensor.c`). Stateful callbacks stay layout-dependent: the custom filter is called during pair
discovery and sensor overlap before those sorts, and `preSolve` calls from different bodies interleave
across workers.

### Behaviour change, stated up front

Results stay physically comparable but are **not bit-identical to today's** in the affected cases.
Any recording (`.b3rec`) or golden hash captured before this PR may stop validating in them. The PR
description must say so in one line. The cases:

- **CCD sweeps with several candidates in one tree query.** Candidates are evaluated in ascending shape
  id instead of tree order. Where tree order differed, each candidate's time-of-impact query runs to
  a different running limit, so the impact fraction can differ, with no bound; the eight sensor hits
  kept can differ; custom-filter calls arrive in the new order; and `preSolve` is called for a
  different set of candidates, in the new order, with a point and normal from the new limit. Where
  tree order already matched, nothing changes.
- The public CCD iteration counters (`distanceIterations`, `pushBackIterations`, `rootIterations`) can
  change even where the collision result does not.
- **Explosions touching a multi-shape body, or waking more than one sleeping set.**

## 2. Commit sequence

```
A0 tree-layout test helper ── A1 CCD candidate order ── A2 Explode order
```

The PR holds A0 to A2 as one commit each, in that order, so each stays bisectable. A1 and A2 share no
code, but both tests use the A0 helper. A0 has no behaviour change and is the first commit. The PR
forks from `main`.

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

### A1. CCD: evaluate candidates in shape-id order

**File.** `src/solver.c`: `b3ContinuousQueryCallback`, `b3SolveContinuous`; `src/physics_world.h`:
`b3TaskContext`; `src/physics_world.c`: worker-context creation, destruction and the memory statistics.

**Change.**
- Each of `b3SolveContinuous`'s tree queries (static, then kinematic and dynamic for bullets) runs in
  two steps. The tree callback only appends `shapeId` to a per-worker array on the task context. After
  the query returns, the array is sorted ascending (`src/qsort.h`) and the current callback body runs
  once per id in that order, unchanged: its filters, `b3ShapeTimeOfImpact` against the running
  fraction, the sensor buffer and `preSolve`. The array is cleared before the next query.
- The array is a `b3Array` on `b3TaskContext`, reserved, destroyed and counted in the memory
  statistics with the other per-worker arrays (`pairKeys` is the precedent) and reused across bodies, so a step allocates only when a query
  returns more candidates than any before it.
- Nothing else changes: the running fraction, the accept tests, the sensor buffer and its end filter
  keep today's behaviour, so with ascending-id tree order the results equal today's.

**Costs and caveats.**
- One append per candidate and one sort of one query's candidates. No additional
  `b3ShapeTimeOfImpact` call, no added tree traversal, no change to the swept boxes or to
  `fastShape->aabb`. Report the benchmark delta (`benchmark/`, CCD-heavy scenes) in the PR, with the
  sort cost on a dense query and the iteration counters.
- The array keeps the capacity of the largest query it has seen, per worker; report the high-water
  mark.
- Alternative if reviewers prefer not to touch `b3TaskContext`: a small fixed stack buffer in
  `b3SolveContinuous` (a few dozen ids) that falls back to a heap `b3Array` only when a query
  overflows it. Same determinism, no struct change and no memory-statistics entry, at the price of
  one rare allocation on a dense query. Not chosen because the per-worker array is the existing idiom
  and keeps the hot path allocation-free; offer it if asked.
- Candidates arrive in id order instead of tree order, so the running fraction each call sees, and the
  work it does, can differ from today's. A solid candidate reaches `preSolve` only when its fraction
  is below the running fraction, so a candidate that was called today can be skipped, and one that
  was skipped can be called, with the point and normal its new limit produces. The custom filter runs
  before the time-of-impact call, so only its order changes.
- A sensor beyond the eventual solid hit can occupy one of the eight sensor slots before a later
  solid candidate shortens the fraction, as it can today; which sensors it displaces is now fixed by id.
- `preSolve` and the custom filter are called in id order within each query. The result is
  layout-independent for a callback that depends only on its arguments. A stateful one stays
  layout-dependent: fast bodies are processed on parallel workers, so calls from different bodies
  interleave as they do today, and the custom filter is also called during pair discovery and sensor
  overlap in tree order. State this in the PR and in `docs/simulation.md` next to the pre-solve
  description, replacing the sentence there that says pre-solve does not work with high-speed
  collisions.

**Tests.** Seeded fuzz: bullets and fast dynamic bodies against a field of static boxes with several
overlapping candidates, many seeds, A0 perturbation, compare hashes. The divergence is small and rare, so
the PR must record how many seeds failed before the change. Plus more than 8 sensors along a bullet's
path (deterministic repro, fails before) under A0 perturbation, asserting the same sensors are
reported.

### A2. Explode: deterministic wake and impulse order

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
- [ ] Benchmark delta quoted for A1.
- [ ] PR text states the not-bit-identical-to-before consequence in one line.

## 4. Out of scope

- Making each island's result independent of the rest of the world.
- Callbacks that keep state (§2 A1 caveats).
- Public query results (ray casts, shape casts, overlaps): their callback order and tied-hit results
  stay layout-dependent.

## 5. Provenance

- **Public ray and shape casts are outside the guarantee.** The step never casts, so their callback
  order and tied-hit results are not part of what this PR makes layout-independent.
- **Candidates are sorted rather than evaluated against a fixed initial fraction.** A fixed limit makes
  each time-of-impact call run to the full sweep and calls `preSolve` for every candidate nearer than
  it, and a deterministic sensor selection then needs a second tree traversal against the final solid
  fraction. Sorting keeps today's per-candidate logic and its cost, and fixes `preSolve` and
  custom-filter call order within a query too. The price is one per-worker array and a sort of a few ids.
- **Sensor selection is the first eight admitted, in body shape order, then tree order (static,
  kinematic, dynamic), then ascending id within each query.** It is deterministic and needs no second
  pass. It does not recover a sensor hit that a lower-id sensor beyond the final solid hit displaced;
  with eight or fewer sensor candidates nothing is lost.
- **Sensor reporting is unchanged.** A sensor is admitted at a fraction no greater than the running
  solid fraction and reported only strictly below the final one, including a sensor the shape starts
  inside (fraction 0) and the zero-initialised result a composite shape returns when no child hits.
- **The `docs/simulation.md` statement that sensors do not detect passage within a step is not
  changed.** This PR changes the order candidates are evaluated in, not whether CCD sensor detection
  exists.
