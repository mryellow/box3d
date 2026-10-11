---
title: Review round 2 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round1.md
mode: full pass
---

# Round 2

## Exact request sent to Codex

```
Review the design document docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md as an adversarial design reviewer for the Box3D physics engine. The document decides three changes, A0, A1 and A2, that make continuous collision and b3World_Explode independent of broad-phase tree layout by evaluating tree candidates in ascending shape-id order.

Scope: the document's own content and the source it cites (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, include/box3d/, test/, docs/simulation.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false, orderings or results that still depend on tree layout after A1 and A2, hazards in A0, A1 and A2 that the document does not state (threading, per-worker scratch reuse, re-entrancy, callbacks that touch the world, sort ties), wrong or inconsistent statements, contradictions between sections, and consequences of the changes (behaviour, threading, determinism, cost) that the document misses. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h) as settled tools; judge what the design adds and the question each primitive is asked.

Decisions recorded in the document's section 5 (Provenance) are settled and binding unless you show fresh source evidence that their reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, other design documents, and decisions the document assigns to a later Implementation Prompt.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:17:49+10:00 to 06:19:29+10:00.

## Review report (Codex final message)

## 1. Summary

Sorting candidates by shape id addresses the identified traversal-order dependence in CCD and explosions for the stated fixed-state, argument-dependent-callback guarantee. The design needs one implementation correction and two narrower clarifications before it is ready to implement.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2 A1; §3 | A1’s file list omits `src/physics_world.c`, where worker arrays are initialized, destroyed, and counted in memory statistics. The §3 file-scope check would exclude the code needed to avoid leaking the new array. | Add `src/physics_world.c` to A1’s scope and specify destruction and memory accounting. |
| 2 | L | §1; §2 A1 | §1 calls the stateful custom filter the exception to layout independence, while §2 correctly says stateful `preSolve` calls can also interleave across parallel workers. | Qualify §1 to cover both callbacks. |
| 3 | L | §2 A0 | The suggested static-tree equivalent, `b3World_RebuildStaticTree`, writes a recording command through `B3_REC` in `src/physics_world.c`. When recording is active, that conflicts with the helper’s claim to change no state beyond tree layout. | Have the test helper rebuild the static tree directly, or state that its unchanged-state comparison requires recording to be inactive. |

## 3. Checked, no change

- `src/solver.c` keeps one running CCD fraction across shapes and the static, kinematic, and dynamic queries. Sorting each query’s returned ids preserves that sequence while fixing candidate order within it.
- Each shape has one broad-phase proxy (`src/shape.c`), so sorting shape ids has no proxy tie to resolve.
- The eight-hit CCD sensor limit and final strict-fraction filter match §5.
- Pair keys are sorted before contact creation in `src/broad_phase.c`; sensor overlaps are sorted and deduplicated in `src/sensor.c`.
- `b3World_Explode` queries only the dynamic tree, and its wake and impulse work currently runs in query order (`src/physics_world.c`). Its world lock already spans that work.

## 4. Proposed edits

Add `src/physics_world.c` and its worker-array lifecycle and accounting functions to A1. Amend §1’s callback exception, and make A0’s static-tree perturbation instruction precise about recording.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (M) | Confirmed. `pairKeys` is reserved (line 179 area), destroyed (197) and counted in the memory statistics (2558) in `src/physics_world.c`; the new array needs the same three. | Applied: file added to A1's list; reserve, destroy and statistics named. |
| 2 (L) | Confirmed. §1 named only the custom filter. | Applied: §1 covers both stateful callbacks. |
| 3 (L) | Confirmed. `b3World_RebuildStaticTree` calls `B3_REC( world, WorldRebuildStaticTree, ... )`. | Applied: A0 rebuilds the trees directly and says why. |

Note for 20261011-001: findings 1 and 3 apply there as well (its A0 names `b3World_RebuildStaticTree`; its A1 lists no `src/physics_world.c` because it adds no array). Finding 3 is the only one that carries over; 001 is not edited here (U-I7).

## Status

2 rounds: 1/2/1 -> 0/1/2. Series total 7 (H1-M3-L3). Document 180 -> 181 lines; the fixes added about 1 net. No named mechanism added or removed. Findings are in A0 and A1 again, from different causes; no finding was a consequence of the round 1 fixes.
