---
title: Measurement - what the continuous stage does per query on the project scenarios
status: Draft
date: 2026-10-13
supports: 20261013-005, 20261013-006, 20261013-007 (preflights)
---

# Measurement: the continuous stage's paths on the project scenarios

Counts, not times. A counting build of the sweep-ordered PR head answers, for every project scenario,
what each continuous query does: how many candidates it collects, which time-of-impact path they
take, how many triangles or children that path visits, how many it rejects early, how many reach a
time-of-impact call, and how many of those calls two sound geometric culls would have removed. The
counts are deterministic and were taken in one run per scenario. The regime figures (one candidate
per query everywhere) are in 20261013-003 §3; this document carries the path figures the three
preflights cite.

## 1. The two culls, measured before they were designed

**Sweep-volume cull of convex candidates (20261013-006).** For each sphere, capsule or hull candidate,
the fast body's centre-of-mass segment is ray-cast against the candidate's box inflated by the fast
shape's radius about the centre of mass plus 1.25 linear slops. A miss means no time-of-impact hit is
possible through the GJK distance path.

| Scenario | Convex candidates | Culled |
|---|---|---|
| convex_pile | 520 | 0 |
| junkyard | 503 | 0 |
| washer | 7,734 | 0 |

The other scenarios with continuous work present no convex candidate (§2). The one candidate a real
query holds is the thing the shape is about to land on; the sweep always reaches its box.

**Sound triangle reject inside the mesh time-of-impact path (20261013-007).** For each triangle that
reaches a time-of-impact call in `b3MeshTimeOfImpactFcn` (`src/shape.c`), the distance from the
centre-of-mass segment to the triangle is compared with the same radius; a larger distance means the
call cannot report a hit through the GJK distance path.

| Scenario | Triangle TOI calls | Would be rejected | Share | Of which hit anyway |
|---|---|---|---|---|
| mesh_drop | 141,395 | 4,202 | 3.0% | 0 |
| rain | 21,976 | 449 | 2.0% | 0 |
| spinner | 9,816 | 3,583 | 36.5% | 0 |
| trees100 | 5,750 | 8 | 0.1% | 0 |
| trees50 | 12,166 | 11 | 0.1% | 0 |
| trees25 | 36,779 | 129 | 0.4% | 0 |

No rejected call was a hit, as the bound requires. The triangles that reach a call are within the
bounding capsule of the swept shape; the call rejects most of them because the shape itself, inside
that capsule, does not reach the triangle, and only the query can tell. Spinner's share is large on
a small count: its cost is 1.17 million queries, not 9,816 calls (§2).

## 2. The mesh and compound paths per scenario

| Scenario | Queries | Triangles visited | Behind plane | Finished in front | TOI calls | Of which fallback | Hits |
|---|---|---|---|---|---|---|---|
| mesh_drop | 283,785 | 433,586 | 6,885 | 313,254 | 141,395 | 27,948 | 18,468 |
| rain | 123,871 | 174,322 | 3,523 | 149,665 | 21,976 | 842 | 2,197 |
| spinner | 1,170,613 | 1,689,296 | 0 | 1,680,026 | 9,816 | 546 | 3,053 |
| trees100 | 1,342 | 6,525 | 0 | 775 | 5,750 | 0 | 216 |
| trees50 | 1,537 | 14,525 | 0 | 2,359 | 12,166 | 0 | 328 |
| trees25 | 1,757 | 53,659 | 0 | 16,880 | 36,779 | 0 | 474 |

Hits here count triangle calls that improved the running fraction; the stage's accepted hits per
query are in 20261013-003 §3. Per query: mesh_drop visits 1.5 triangles and makes 0.5 calls;
spinner visits 1.4 and makes 0.008; trees25 visits 30.5 and makes 21, of which 1.3 percent hit.
The plane early-outs remove 72 percent of visited triangles on mesh_drop, 86 percent on rain, 99.4
percent on spinner and 31 percent on trees25. A fifth of mesh_drop's calls are the fallback
(a second call with a small sphere about the centroid after a first call at fraction 0).

**Village** is the one compound scenario: 32,296 compound queries visit 10,072 children (0.31 per
query; the compound's own tree rejects the rest), make 9,908 calls and improve the fraction 3,871
times (39 percent of calls). Its 1,316 triangle visits come from mesh children of the compound,
where the counting build's triangle-reject fields were not filled; its triangle-reject figures are
therefore invalid and omitted from §1.

## 3. What follows

- Ordering: one candidate per query (20261013-003), so no order has work. 20261013-005 makes the
  sweep-ordered PR's keying free on that path.
- Sweep-volume cull: zero on every scenario. 20261013-006 records the sound form and stops.
- Mesh path: the calls that remain after the plane early-outs are within the swept shape's capsule
  and need the query; a sound per-triangle reject removes at most 3 percent on the terrain scenes.
  The stage's cost on mesh_drop (19 percent of the step) and spinner (17 percent) is the number of
  queries, 284 thousand and 1.17 million, each paying a tree query, a mesh descent and one or two
  plane tests before any call. 20261013-007 records this and stops.

## 4. Setup

- Counting build: Release, AVX2, `BOX3D_VALIDATE` off, static library, GCC 14.2, built with
  `-DB3_CCD_COUNT=1` from `solver-ccd-counters` (three commits on the sweep-ordered PR head `854bd28`:
  stage counters, path counters, triangle-reject counter). Counters are process globals; single
  worker only. The `bullets` profile field times bullet bodies only and is zero on every project
  scenario, which is why the counters exist.
- Harness: each scenario of `benchmark/main.c` created and stepped for its project step count at a
  single worker, counters read at the end.
- The capsule radius is the farthest proxy point of the fast shape from the body's local centre plus
  the shape radius, which bounds the swept shape under any rotation because the sweep moves the
  centre of mass on a straight segment and rotates about it (`b3GetSweepTransform`,
  `src/distance.c`). The segment-to-triangle distance is the minimum over the two endpoints to the
  triangle, the segment to the three edges, and zero when the segment pierces the triangle.
