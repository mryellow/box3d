---
title: Review round 7 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round6.md
mode: full pass
---

# Round 7

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
hook: SessionStart
hook: SessionStart Completed
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:45:37+10:00 to 06:47:16+10:00. Same prompt as round 6, on the document revised after round 6.

## Review report (Codex final message)

## 1. Summary

I found no design defect within the stated scope. The live code supports the document’s ordering claims, and A1 and A2 would process the relevant tree candidates in a fixed order under the document’s fixed-state and argument-dependent-callback assumptions.

## 2. Findings

None.

## 3. Checked, no change

- In `src/solver.c`, CCD currently evaluates candidates during tree traversal. The proposed per-query sort preserves the existing shape, tree-type, fraction, sensor-buffer, and callback rules.
- In `src/physics_world.c`, explosion wake and impulse effects currently follow dynamic-tree visit order. Sorting shape ids before those effects removes that ordering input.
- In `src/dynamic_tree.c` and `src/broad_phase.c`, a full rebuild preserves leaf moved marks; the ordered flag can trigger a tree update, while pair discovery still requires moved marks.
- In `src/broad_phase.c` and `src/sensor.c`, pair keys and sensor overlaps are sorted before their order affects reported results. The document correctly limits its guarantee for stateful callbacks.
- In `src/physics_world.h`, `src/physics_world.c`, and `src/solver.c`, worker task contexts provide the stated scratch-storage precedent and the two CCD stages use them sequentially.

## 4. Proposed edits

None.

## 5. Unresolved

None.

## 6. Verdict

CONVERGED

## Post-review verification (Claude)

No findings to verify.

## Status

7 rounds: 1/2/1 -> 0/1/2 -> 0/0/0 -> 0/2/0 -> 0/1/1 -> 0/0/1 -> 0/0/0. Series total 12 (H1-M6-L5). Document unchanged at 197 lines. Clean round; the confirmation round on the unchanged document is next (U-P9, U-S8). The raw log shows no read of round records or the changelog.
