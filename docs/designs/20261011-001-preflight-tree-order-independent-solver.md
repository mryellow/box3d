---
title: Preflight - tree-order-independent solver changes
status: Draft
date: 2026-10-11
parent: 20260929-002-rewind-only-history-ring.md (§7.4)
---

# Preflight: tree-order-independent solver changes

Four small PRs to Box3D. Together they make the step, and `b3World_Explode`, give the same answer
whatever the internal layout of the broad-phase trees is. Each PR stands alone: it is a determinism
hardening that upstream can accept on its own merits, with no mention of rewind, history rings or
journals.

## 1. Why these changes, and only these

Rewind-only design §7.4 needs the answer not to depend on tree layout, because restore rebuilds
the trees in its own way and never images the static tree. The audit in that section finds three
places where a result depends on the order a tree hands back candidates:

1. **CCD solid minimum.** `b3ContinuousQueryCallback` (`src/solver.c`) passes the running best
   fraction into the next candidate's time-of-impact query.
2. **CCD sensor hits.** The same callback collects up to `B2_MAX_CONTINUOUS_SENSOR_HITS` (8) sensor
   candidates, gated on the running solid fraction. Which 8 fill the cap depends on visit order.
3. **`b3World_Explode`.** `ExplosionCallback` (`src/physics_world.c`) wakes bodies and accumulates
   impulses in tree traversal order. A body with several shapes in range sums floats in a
   layout-dependent order.

Everything else the step reads from a tree is already order-independent: pair discovery is sorted
by shape-pair key (`src/broad_phase.c`), end-of-step sensor hits are sorted and deduplicated
(`src/sensor.c`), ray and shape casts take a min.

### What is not needed

- **No solver change is needed to rewind from a full world image.** `b3SerializeWorld` writes all
  three trees verbatim (`src/world_snapshot.c`), so a restored world has the original layout. The
  second preflight (`20261011-002-preflight-public-world-snapshot.md`) therefore has no dependency
  on this one.
- These PRs are needed once restore stops imaging trees: the static tree first (it is world-sized),
  the kinematic and dynamic trees later. They are the prerequisite for that work, not for the
  public snapshot API.

### Behaviour change, stated up front

Results stay physically equivalent but are **not bit-identical to today's** in the affected cases:
rare CCD sweeps with several competing candidates (ulp-level differences), CCD sweeps with more
than 8 sensor candidates, and explosions touching a multi-shape body. Any recording (`.b3rec`) or
golden hash captured before these PRs may stop validating in those cases. Each PR description must
say so.

## 2. PR sequence

```
A0 tree-layout test helper ──┬── A1 CCD solid min ── A2 CCD sensor hits
                             └── A3 Explode order
```

A1 -> A2 is a real dependency (A2 evaluates sensors against A1's final solid fraction). A3 is
independent of both. A0 has no behaviour change and merges first.

### A0. Test helper: perturb tree layout

**Scope.** `test/` only. No `src/` change.

**Change.** A helper that runs the same scene twice and compares per-step state hashes, where
the second run perturbs tree layout before each step without changing proxy ids, fat AABBs or any
other state. Perturbation is a full `b3DynamicTree_Rebuild( tree, true )` on each world tree
(`b3World_RebuildStaticTree` is the public equivalent for the static tree). Tests may include
internal headers (`test_broad_phase.c` already does).

**Check before merging.** The helper must demonstrably change layout (compare `nodes` order or a
tree-walk trace between the two runs), otherwise later tests pass vacuously.

**Done when.** A sanity test shows the helper changes layout and leaves the hash of a scene with
no CCD, sensor or explosion unchanged.

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
- Iteration statistics (`distanceIterations`, etc.) take a max over accepted candidates. The
  accepted set is now order-independent, so these are too.

**Costs and caveats.**
- A few extra TOI evaluations on a multi-candidate sweep, because the running clamp no longer
  prunes. Report the benchmark delta (`benchmark/`, CCD-heavy scenes) in the PR.
- `preSolve` is now called for every candidate nearer than `limit`, not only those nearer than the
  running best. Its return value still affects only that candidate, so the result is
  order-independent, but a *stateful* callback sees more calls, in tree order. State this in the
  PR and in `docs/simulation.md` next to the pre-solve description.
- Alternative if reviewers dislike the extra calls: collect candidates, sort by shape id, evaluate
  in that order with the running clamp. Same determinism, no extra TOIs, deterministic `preSolve`
  call order, at the price of a per-worker scratch array. Not chosen because it adds allocation to
  a hot, parallel path; offer it if asked.

**Tests.** Seeded fuzz: bullets and fast dynamic bodies against a field of static boxes with several
overlapping candidates, many seeds, A0 perturbation, compare hashes. The divergence is ulp-level, so
the PR must record how many seeds failed before the change. If none fail, say so; the change is
still justified by the argument above.

### A2. CCD: order-independent sensor hits

**File.** `src/solver.c`.

**Change.**
- Pass 1 (A1's) skips sensor candidates entirely.
- Pass 2 runs after every fast shape of the body has finished pass 1, so the solid fraction is
  final. It re-queries the same trees with the same swept boxes, evaluates only sensor candidates
  with `b3ShapeTimeOfImpact` against the final solid fraction, and keeps the eight smallest
  `(sensorId, visitorId)` pairs among those with fraction strictly below it. Storage is the existing
  fixed eight-slot buffer, replace-largest on insert. The kept set, the eight smallest of a set,
  does not depend on visit order.
- The existing filter at the end of `b3SolveContinuous` (`sensorFractions[i] < context.fraction`)
  becomes redundant; keep it as an assert.

**Implementation hazard.** The first loop overwrites `fastShape->aabb` with `box2`, losing `box1`,
which pass 2 needs for the swept box. Defer the overwrite until after pass 2 (the callback never
reads the fast shape's own `aabb`), or recompute `box1`; the former is smaller. The no-hit branch
relies on `shape->aabb` holding `box2`, so the write must still precede it.

**Cost.** A second tree traversal per sweeping body, only for bodies whose shapes can overlap any
sensor. Skip pass 2 when `world->sensors.count == 0`. Report the benchmark delta.

**Tests.** More than 8 sensors along a bullet's path (deterministic repro, fails before), plus a
sensor and a solid hit at exactly equal fractions, under A0 perturbation. Assert the same eight
sensors are reported and that none lies beyond the solid hit.

### A3. Explode: deterministic wake and impulse order

**File.** `src/physics_world.c`: `ExplosionCallback`, `b3World_Explode`.

**Change.**
- The callback only collects `shapeId` for shapes with `explosionScale != 0` into a temporary array
  (world allocator, freed before return; the query is serial).
- After `b3DynamicTree_Query`, sort the ids ascending (`src/qsort.h`) and run the current callback
  body for each in that order, unchanged: distance test, `b3WakeBody`, impulse. Wake order fixes
  colour and overflow-colour order of woken constraints, and sorted order fixes the float addition
  order for a body hit through several shapes.
- `world->locked` stays set across both the query and the loop.

**Tests.** A body with three shapes inside the radius, plus several single-shape bodies sleeping in
range, exploded under A0 perturbation of the dynamic tree; assert equal body velocities bit for
bit and equal awake-set order. Fails before the change.

## 3. Review checklist for each PR

- [ ] Diff touches only the files listed for that PR, plus a test and the docs sentence.
- [ ] New test fails on `main` and passes on the branch (or the PR says why it cannot fail).
- [ ] Existing `test` target passes, including `test_determinism.c`.
- [ ] Benchmark delta quoted for the hot-path PRs (A1, A2).
- [ ] PR text states the not-bit-identical-to-before consequence in one line.
- [ ] No mention of rewind, ring, journal or history in code, comments or PR text.

## 4. Out of scope

- The six larger solver changes that would make each island's result independent of the rest of
  the world (incremental replay, rewind-only design §11.2 item 4). Not needed for rewind.
- Proxy-identity journaling, moving tree writers into `broad_phase.c`, the restore-time tree pass
  (rewind-only §7.4). Those belong to the history-ring work, not to the solver.

## 5. Open decisions for the owner

1. A1 variant: evaluate against the initial fraction (recommended, smaller diff) or sort by shape
   id (cheaper, needs scratch).
2. Whether to land A2 and A3 at all before a consumer exists. Each is justified by determinism alone
   and is small; the risk is review appetite for changes with no visible feature.
