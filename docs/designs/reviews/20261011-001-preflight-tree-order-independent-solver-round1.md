---
title: Review round 1 - preflight tree-order-independent solver changes
target: 20261011-001-preflight-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

```
Review the design document docs/designs/20261011-001-preflight-tree-order-independent-solver.md as an adversarial design reviewer for the Box3D physics engine. The document decides four changes, A0, A1, A2 and A3, that make continuous collision and b3World_Explode independent of broad-phase tree layout.

Scope: the document's own content and the source it cites (src/solver.c, src/physics_world.c, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, include/box3d/, test/, docs/simulation.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false, orderings or results that still depend on tree layout after A1, A2 and A3, hazards in A0, A1, A2 and A3 that the document does not state, wrong or inconsistent statements, contradictions between sections, and consequences of the changes (behaviour, threading, determinism, cost) that the document misses. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h) as settled tools; judge what the design adds and the question each primitive is asked.

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T05:52:57+10:00 to 05:55:19+10:00.

## Review report (Codex final message)

## 1. Summary

I found no design defect within the stated scope. With callbacks that depend only on their arguments, A1 removes the running TOI limit from tree visit order, A2 selects a stable set of sensor hits, and A3 processes explosion candidates in shape-id order.

## 2. Findings

None.

## 3. Checked, no change

- A0’s full tree rebuild preserves proxy identities and leaf bounds; the required visit-order witness guards against a vacuous test.
- The CCD query, fraction update, sensor cap, shape AABB handling, and iteration counters in `src/solver.c` support A1 and A2’s stated behavior.
- `src/sensor.c` sorts and deduplicates overlaps before publishing events, so A2’s buffer slot order does not determine event order.
- Broad-phase contact creation sorts pair keys in `src/broad_phase.c`.
- `b3World_Explode` queries only the dynamic tree, and its current callback wakes and applies impulses in visit order. A3’s sorted processing addresses both effects.
- The planned pre-solve documentation correction matches the live callback path.

## 4. Proposed edits

None.

## 5. Unresolved

None.

## 6. Verdict

CONVERGED

## Post-review verification (Claude)

No findings to verify. Gates before the round (U-G1): AC-1 to AC-4 checked against the document by the author. A2 re-queries the live trees and recomputes `box1`/`box2` rather than keeping a copy (AC-1); A3's id array is transient scratch inside one call, not a copy of an engine population (AC-1, AC-3); no reconciliation (AC-3); the only assertion is the A2 end filter, which is not control flow (AC-4); no enumerated completeness (AC-2). Document length 190 lines, under the 400-line split check. Test mechanism in A0 and the Tests paragraphs remains in the document, deferred to the Implementation Prompt (U-C2, U-C10).

The raw log ends with an unrelated Codex rollout-recording error line; the exit code was 0 and the report is complete.

## Status

1 round: 0/0/0. Clean first round; the confirmation round (U-P9, U-S8) on the unchanged document is next. No fixes, so no tripwire entries.
