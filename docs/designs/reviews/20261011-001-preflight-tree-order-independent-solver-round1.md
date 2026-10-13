---
title: Review round 1 - preflight tree-order-independent solver changes
target: 20261011-001-preflight-tree-order-independent-solver.md
date: 2026-10-11
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

```
Review the design document docs/designs/20261011-001-preflight-tree-order-independent-solver.md as an adversarial design reviewer for the Box3D physics engine.

Scope: the document's own content and the source it cites (src/solver.c, src/physics_world.c, src/broad_phase.c, src/sensor.c, src/world_snapshot.c, src/qsort.h, include/box3d/, test/, docs/simulation.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false, orderings or results that still depend on tree layout after the proposed change, hazards in the changes (A0-A3) the document does not state, wrong or inconsistent statements, contradictions between sections, and consequences of the changes (behaviour, threading, determinism, cost) that the document misses. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h) as settled tools; judge what the design adds and the question each primitive is asked.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, and decisions the document assigns to a later Implementation Prompt. Do not review or follow any other design document except where this one cites it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-11T10:13:27+10:00 to 10:15:21+10:00.

## Review report (Codex final message)

## 1. Summary

The fixed TOI limit and sorted explosion shape IDs address the tree traversal order described in the design. The claimed tree layout independence still fails for permitted callbacks whose decisions depend on call order, and the document overstates both its query coverage and the equivalence of sensor results.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §1; §2 A1 | A1 still calls `preSolve` in tree order. A thread-safe callback that accepts its first candidate and rejects later ones can select different solid fractions under different layouts. Custom filtering also runs before pair sorting and before sensor overlap sorting (`src/solver.c`, `src/broad_phase.c`, `src/sensor.c`). The documented callback contract requires thread safety, not call-order independence (`include/box3d/types.h`). | Define a narrower guarantee that requires call-order-independent callbacks, or evaluate callbacks in a stable candidate order. Remove the claim that a stateful `preSolve` callback cannot affect the result. |
| 2 | M | §1 | “Ray and shape casts take a min” does not describe their public results. Cast callbacks receive hits in traversal order, and `b3World_CastRayClosest` can return a layout-dependent shape on tied hits; returned visit counts also reflect tree layout (`src/physics_world.c`). | Bound the guarantee explicitly to internal step results and `b3World_Explode`, and state that public query results and callback order remain layout-dependent. |
| 3 | M | §2 A2 | The stated cost applies the second traversal “only” to bodies whose shapes can overlap a sensor. The specified skip checks only `world->sensors.count == 0`; with any sensor anywhere in the world, eligible fast shapes still re-query trees (`src/solver.c`). | Describe the cost as an additional traversal per eligible fast shape whenever the world has sensors, or specify a sound local skip. |
| 4 | L | §1 | “Physically equivalent” obscures a user-visible change: choosing the eight lowest-ID sensor pairs can replace earlier path hits with later ones when the cap is exceeded. Those hits feed sensor overlap events (`src/solver.c`, `src/sensor.c`). | Say that trajectories remain physically comparable while the reported sensor events can change. |

## 3. Checked, no change

- `src/world_snapshot.c` stores and restores the layout of all three broad-phase trees.
- `src/broad_phase.c` sorts accepted pair keys before creating contacts.
- `src/sensor.c` sorts and deduplicates end-of-step overlaps.
- `src/solver.c` confirms the running solid fraction, eight-hit cap, final strict sensor filter, and the `fastShape->aabb` overwrite hazard described in A1 and A2.
- `src/physics_world.c` confirms that explosion traversal currently orders waking and impulse accumulation, and that the world remains locked during the query.

## 4. Proposed edits

Revise the guarantee and callback caveat in §1 and A1; correct A2’s cost statement; distinguish sensor event changes from physical trajectory changes. Keep the A3 ordering proposal.

## 5. Unresolved

Whether the intended guarantee includes call-order-dependent user callbacks and public query results. That boundary determines whether the design needs stable callback ordering or a narrower claim.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Gates first (U-G1): AC-1..AC-4 checked against the doc. The design adds no shadow structure, side table,
reconciliation or validator over an engine population; the A2 eight-slot buffer already exists and
A3's id array is a transient local. No AC failure. Author-side content check (U-C2/U-C10): the
per-change **Tests** paragraphs, **Done when**, and §3 checklist carry test mechanism (seeds, fuzz,
assertions); the Implementation Prompt owns these. Not sent as a finding; noted for the revision.

Reviewer's raw log shows no read of round records or the changelog (none exist yet).

| # | Verdict | Verification |
|---|---|---|
| 1 | Adopt (narrow claim); fork carried to owner | `b3ContinuousQueryCallback` calls `preSolve` and the custom filter inline in tree order, and `include/box3d/types.h` documents only the "do not modify the world" and multithreading constraints, not call-order independence. A1's sentence "so the result is order-independent" holds only for a callback that is a pure function of its arguments. The false claim is corrected in the doc. The fork: the sort-by-shape-id variant (A1 alternative) is the only one that also fixes call order for stateful callbacks, which changes the trade-off in §5 decision 1. U-S3: owner decides. |
| 2 | Decline as stated; resolved by deletion | The step never casts: grep of `src/` shows ray and shape casts only in the public query API (`src/physics_world.c`), `src/body.c` and geometry primitives, none reachable from the solver, contact or sensor paths. Public query results are outside "what the step reads from a tree". The clause "ray and shape casts take a min" names something the design does not use (U-C7, U-C8) and is deleted. |
| 3 | Adopt | §2 A2 says pass 2 runs "only for bodies whose shapes can overlap any sensor" yet specifies only `world->sensors.count == 0` as the skip (`b3World.sensors` is a plain `b3Array`). Cost sentence is wrong as written; restate as one extra traversal per eligible fast shape whenever the world has any sensor. |
| 4 | Adopt | Pre-change code keeps the first eight sensor candidates in visit order; A2 keeps the eight smallest `(sensorId, visitorId)` pairs, not the eight nearest. Reported sensor events can change when more than eight candidates exist. §1 "physically equivalent" is amended with one clause to say so. |

Author-found (not in the report), read from `b3SolveContinuous`:

- A2's deferred `fastShape->aabb = box2` write: in the current loop the write precedes the `continue`
  for mesh, height and sensor fast shapes. Moving it after pass 2 must still cover those shapes, or the
  no-hit branch reads a stale `aabb` for them. Adopt: one clause in the A2 implementation hazard.

Proposed edits to the doc (not yet applied; U-R3 keeps resolution and revision separate):

1. §1 delete "ray and shape casts take a min".
2. §1 behaviour-change paragraph: replace "physically equivalent" with a clause that sensor events
   reported past the cap can change.
3. A1 caveat: state that order-independence holds for a `preSolve` and custom filter that depend only on
   their arguments.
4. A2 cost: correct the skip condition description.
5. A2 hazard: deferred write covers the skipped shape types.
6. §5 decision 1: add that sort-by-shape-id is the variant that also fixes callback call order.
7. U-C2/U-C10: move the **Tests** paragraphs, **Done when** and §3 test items to the Implementation Prompt.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 1: 4 findings (H1-M2-L1), 3 applied, 1 declined (finding 2: the step does not cast; the clause is deleted instead, and the reason goes in the revision's Provenance per U-C4/U-V10). Finding 1's fork between narrowing the claim and the sort variant is carried to the owner (U-S3).
- Findings: 1/2/1 (round 1) -> pending
- Tripwire: fixes added or removed a named mechanism: no (clause-level only). Clustering: none (first round). Reviewer read of round records or changelog: none. Doc length: 175 lines before; edits above are one clause each, except moving test text out (net shorter).
