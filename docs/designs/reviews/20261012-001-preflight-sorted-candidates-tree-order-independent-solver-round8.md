---
title: Review round 8 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round7.md
mode: full pass (confirmation)
---

# Round 8

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:47:41+10:00 to 06:50:08+10:00. Same prompt as round 7, document unchanged.

## Review report (Codex final message)

1. **Summary**  
I found no source-backed defect in A0–A2. For fixed shape identities and state, the proposed ordering removes the tree visit order dependence identified in CCD and Explode, within the document’s stated callback limits.

2. **Findings**

None.

3. **Checked, no change**

- Full rebuild preserves proxy identities, leaf bounds, and moved marks; the A0 visit-order witness addresses rebuilds that leave query order unchanged (`src/dynamic_tree.c`, `src/broad_phase.c`).
- CCD queries retain their static, then kinematic, then dynamic order. Their current callbacks continue traversal, so sorting collected shape IDs adds no tree traversal or time-of-impact calls (`src/solver.c`).
- Each world-tree shape has one proxy, so shape-ID sort ties do not arise (`src/shape.c`, `src/broad_phase.c`).
- Worker contexts are reused across bodies, and the fast-body and bullet phases run separately (`src/physics_world.c`, `src/solver.c`).
- Sensor hits are filtered against the final solid fraction, then sorted and deduplicated for reporting (`src/solver.c`, `src/sensor.c`).
- Explode queries only the dynamic tree; waking and impulse accumulation currently occur in its callback. Keeping the world locked through the proposed sorted loop matches that operation’s existing lock scope (`src/physics_world.c`, `src/body.c`).

4. **Proposed edits**

None.

5. **Unresolved**

None within the review scope.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

No findings to verify. The document is unchanged since round 7.

## Status

8 rounds: 1/2/1 -> 0/1/2 -> 0/0/0 -> 0/2/0 -> 0/1/1 -> 0/0/1 -> 0/0/0 -> SETTLED 0/0/0. Series total 12 (H1-M6-L5). Converged: a clean full-scope round followed by a clean confirmation round on an unchanged document (U-S8, U-P9). Document 197 lines. Before marking Canonical: re-run U-G1 against the final text, and decide on the U-C2/U-C10 move of the test paragraphs, as for 20261011-001. The raw log shows no read of round records or the changelog.
