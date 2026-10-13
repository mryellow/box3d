---
title: Preflight - sweep-volume cull of continuous candidates
status: Closed
date: 2026-10-13
follows: 20261012-005 (sweep-ordered candidates), 20261013-004 (measurement)
---

# Preflight: sweep-volume cull of continuous candidates

A proposed change to Box3D that the measurement in 20261013-004 shows removes nothing on any project
scenario. It is written out so that the reason is on record and so that the sound form of a cull is
stated once, against the unsound form that 20261012-005 dropped. §5 asks the owner to Close it.

## 1. Why a cull, and why this one

The tree query that collects a fast shape's candidates uses the union of its start and end boxes.
For a diagonal sweep that union is far larger than the volume the shape passes through, so it can
collect candidates the shape never approaches, each costing a time-of-impact call that ends in one
GJK query and one separation evaluation.

The volume the shape does pass through has an exact, cheap bound. `b3GetSweepTransform`
(`src/distance.c`) moves the centre of mass along the straight segment from `c1` to `c2` and
rotates the shape about it, so every point of the shape stays within a fixed distance `R` of a point
on that segment, where `R` is the farthest shape point from the centre of mass plus the shape's
radius. The swept shape lies inside the capsule of radius `R` about the segment. A candidate whose
box the segment misses after inflation by `R` plus the acceptance distance of `b3TimeOfImpact`
(1.25 linear slops, the distance at which it reports a hit for shapes without radius) cannot be
reported hit through the GJK distance path, whatever the rotation. This bound is on the geometry of
the sweep, which the stage owns, not on what the primitive reports, which is where the dropped cull
failed.

### What the bound does not cover

`b3TimeOfImpact` can report a hit from its separation-axis path at a fraction where the GJK distance
is larger than the acceptance distance; the source notes the case (diagonal edge contact on faces).
A culled candidate that would have been reported hit only through that path is a result change
against the sweep-ordered PR. The dropped cull had the same exception and more; this one has only
this one, and it is why the change could not be called result-preserving.

### Measured

On the project scenarios the test culls no candidate (20261013-004 §2: zero of 8,757 convex
candidates across convex_pile, junkyard and washer; the six other scenarios with continuous work
present mesh, height-field and compound candidates, whose boxes enclose the whole terrain or object
and are always reached). The one candidate a real query holds is the thing the shape is about to
land on. The cull has no work to do.

## 2. The change, had it a case

In `b3ContinuousQueryCallback` (`src/solver.c`), after the argument-only filters: for a sphere,
capsule or hull candidate, inflate its box relative to the base by `R + 1.25 * B3_LINEAR_SLOP` and
ray-cast the fast body's centre-of-mass segment against it with `b3RayCastAABB`; a miss drops the
candidate. `R` is computed once per fast shape from its proxy points and the body's local centre.
Compound, mesh and height-field candidates are collected as today: their own time-of-impact paths
query their children or triangles against the swept box and are the subject of 20261013-007.
`b3RayCastAABB` treats a direction component below `FLT_EPSILON` as parallel, which can report a miss
for a segment that grazes a slab over a sweep longer than about a hundred kilometres; the cull
would have to state that limit or test the slab directly.

## 3. Review checklist

- [ ] The counting build shows a non-zero cull count on a project scenario before any further work.

## 4. Out of scope

Everything in §2 until §3 is met.

## 5. Open decisions for the owner

None. Closed by the owner on the measurement in §1. A user scene with convex candidates on diagonal
sweeps that the cull removes would reopen it as a new document.

## 6. Provenance

- **The capsule is about the centre of mass, not the shape centroid.** The centre of mass moves on
  the straight segment the sweep interpolates; the centroid of a shape offset from it follows a
  curve. The sweep key of 20261012-005 uses the centroid segment and is an estimate for that reason;
  the cull needs the bound.
- **The acceptance distance is added to the radius.** `b3TimeOfImpact` reports a hit before the
  surfaces touch, at `target + tolerance`; a cull at the geometric radius alone would drop
  candidates it reports hit.
- **Mesh, height-field and compound candidates are never culled here.** Their boxes are the whole
  object; the cull belongs inside their time-of-impact paths, per triangle or child.
