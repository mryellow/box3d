---
title: Review round 3 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round2.md
mode: full pass
---

# Round 3

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:20:30+10:00 to 06:23:49+10:00.

## Review report (Codex final message)

## 1. Summary

I found no source-backed defect in A0, A1, or A2 within the stated scope. Sorting each query’s shape IDs before evaluation removes the identified broad-phase visit-order dependence for CCD and explosions under the document’s fixed-state and callback assumptions.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| — | — | — | None. | — |

## 3. Checked, no change

- `src/solver.c` confirms the running TOI fraction, pre-solve calls, and eight-slot sensor buffer currently depend on candidate visit order. Its body-shape and tree-query order match the design.
- `src/physics_world.c` confirms explosions query only the dynamic tree, wake bodies, and add impulses during traversal. The world lock covers the proposed collection and evaluation phases.
- `src/broad_phase.c` and `src/sensor.c` confirm pair keys and sensor overlaps are sorted after discovery; the stated caveat about earlier custom-filter calls is accurate.
- `src/dynamic_tree.c`, `src/qsort.h`, and the shape proxy use sites support the A0 witness approach and ascending shape-ID sorts. One proxy per shape per query leaves no shape-ID tie to resolve.
- The worker-context lifecycle and CCD task scheduling support per-worker scratch reuse. The public callback contract also supports the document’s warning about stateful callbacks and world access.
- The cited statements in `docs/simulation.md` and the section 5 provenance decisions are consistent with the reviewed source.

## 4. Proposed edits

None.

## 5. Unresolved

None.

## 6. Verdict

CONVERGED

## Post-review verification (Claude)

No findings to verify. The findings table holds a placeholder row reading "None."; it counts as zero.

## Status

3 rounds: 1/2/1 -> 0/1/2 -> 0/0/0. Series total 7 (H1-M3-L3). First clean round; the confirmation round on the unchanged document is next (U-P9, U-S8).
