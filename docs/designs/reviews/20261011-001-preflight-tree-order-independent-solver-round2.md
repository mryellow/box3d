---
title: Review round 2 - preflight tree-order-independent solver changes
target: 20261011-001-preflight-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261011-001-preflight-tree-order-independent-solver-round1.md
mode: full pass (confirmation)
---

# Round 2

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T05:59:58+10:00 to 06:04:10+10:00. Same prompt as round 1, document unchanged.

## Review report (Codex final message)

1. **Summary**  
I found no source-backed defect in A0–A3. The proposed ordering makes the covered results independent of world broad-phase tree layout under the document’s fixed-identity, fixed-state and callback assumptions; the material behavior and cost changes are stated.

2. **Findings**

None.

3. **Checked, no change**

- A0’s full rebuild preserves proxy identities, leaf bounds and pending moved marks; the visit-order witness accounts for the step’s dynamic and kinematic tree rebuilds (`src/dynamic_tree.c`, `src/broad_phase.c`).
- A1 uses a fixed TOI limit within each fast shape’s queries, while body shape-list order is independent of tree layout. Its callback and iteration-counter caveats match `src/solver.c`.
- A2’s final-fraction threshold, eight-pair selection, AABB update and subsequent sensor sorting and deduplication are consistent with `src/solver.c` and `src/sensor.c`.
- A3 queries only the dynamic tree. Sorting collected shape IDs fixes wake and impulse order while retaining the world lock (`src/physics_world.c`, `src/body.c`).
- Pair-key sorting, the pre-solve documentation correction, and the stated behavior and cost consequences match the cited source (§1–§5).

4. **Proposed edits**

None.

5. **Unresolved**

None within the review scope.

6. **Verdict:** `CONVERGED`

## Post-review verification (Claude)

No findings to verify. The document is unchanged since round 1.

## Status

2 rounds: 0/0/0 -> 0/0/0. Series total 0. Converged: a clean full-scope round followed by a clean confirmation round on an unchanged document (U-S8, U-P9). Before marking Canonical: re-run U-G1 (done for round 1, no edit since) and decide on the deferred U-C2/U-C10 move of test mechanism.
