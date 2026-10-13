---
title: Preflight - the mesh and height-field time-of-impact path
status: Closed
date: 2026-10-13
follows: 20261013-004 (measurement)
---

# Preflight: the mesh and height-field time-of-impact path

A proposed change to Box3D's per-triangle time-of-impact path that the measurement in 20261013-004
shows cannot earn its cost: a sound geometric reject before each triangle's time-of-impact call
removes at most 3 percent of the calls on the terrain scenes. The document records the measured
ceiling, states where the path's cost actually sits, and asks the owner to Close it.

## 1. Why the path, and what the measurement says

The continuous stage's time on the project scenarios is in the mesh path. Six of the nine scenarios
with continuous work present mesh or height-field candidates and none convex; on mesh_drop and
spinner the stage is 19 and 17 percent of the step (20261013-003 §3). Each query's single candidate
is the terrain, so the work per query is `b3ShapeTimeOfImpact`'s mesh branch (`src/shape.c`): a
mesh query with the swept box, then per visited triangle two plane tests on the centroid path and,
for triangles that survive them, a time-of-impact call against the triangle, with a second call
about a small sphere when the first reports fraction 0.

The proposal was a cheaper reject for the triangles that survive the plane tests: the distance from
the centre-of-mass segment to the triangle against the shape's radius about the centre of mass plus
the acceptance distance, which is a bound on the swept shape under any rotation. 20261013-004 §1
measures what it would remove: 3.0 percent of mesh_drop's calls, 2.0 percent of rain's, under half a
percent of the three trees scenes'. The triangles the plane tests pass on are within the swept
shape's bounding capsule; the call rejects them because the shape inside the capsule does not reach
the triangle, and no test cheaper than the call can tell. Spinner's 36 percent is of 9,816 calls
under 1.17 million queries and does not move its stage.

Where the cost sits instead (20261013-004 §2):

- **The query count.** Spinner runs 1.17 million continuous queries for 3,053 triangle hits and
  mesh_drop 284 thousand for 18 thousand. Each query pays a static-tree query with the swept box, a
  mesh descent, and one or two plane tests, before any call. That cost is proportional to the number
  of shapes `b3IsShapeFast` (`src/solver.c`) classifies as fast, which for a tumbling body is set by
  the rotation term of its motion bound against its minimum extent times the body's safety factor.
  Lowering it changes which bodies get continuous collision, which is the engine's tunnelling
  contract, not an optimisation.
- **The fallback call.** A fifth of mesh_drop's calls are the second, small-sphere call after a
  first call at fraction 0: bodies resting on the terrain and still classified fast. The fallback is
  what keeps a touching body from passing through thin terrain; skipping it changes results.
- **Triangles per query where it is high.** trees25 makes 21 calls per query and 1.3 percent hit.
  Those calls are needed by the bound above. The order of triangles within a query cannot help
  either: hit triangles per query are 0.27, and an order pays only from the second hit.

## 2. The change, had it a case

In `b3MeshTimeOfImpactFcn`, after the plane tests and before the time-of-impact call: the
segment-to-triangle distance of the centre-of-mass segment, in mesh space, against the radius plus
1.25 linear slops; a larger distance skips the call. The radius is computed once per
`b3ShapeTimeOfImpact` call from the fast shape's proxy and the body's local centre. The same
exception as 20261013-006 §1 applies: a hit reported only through the separation-axis path of
`b3TimeOfImpact` could be lost, so the change is not result-preserving.

## 3. Review checklist

- [ ] A scene from a Box3D user on which the counting build's reject share exceeds the cost of the
      test, before any further work.

## 4. Out of scope

- The fast-body classification and the safety factor.
- The fallback call and the hit path of `b3TimeOfImpact`.
- Ordering triangles within a query.

## 5. Open decisions for the owner

None. Closed by the owner on the measurement. A user scene where the per-triangle reject removes a
material share of the time-of-impact calls would reopen it as a new document.

## 6. Provenance

- **The reject was measured before it was designed.** The dropped static cull of 20261012-005 cost
  eight review rounds before its bound was found wanting; the counting build answers the same
  question for a few minutes of machine time.
- **The query count is named as the cost but not proposed as the change.** It is set by a documented
  contract (`safetyFactor`), and a change to it is a behaviour decision for the owner, not a
  preflight.
