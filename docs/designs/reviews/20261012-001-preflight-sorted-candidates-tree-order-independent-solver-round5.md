---
title: Review round 5 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round4.md
mode: full pass
---

# Round 5

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:37:03+10:00 to 06:39:06+10:00. Same prompt as round 4, on the document revised after round 4.

## Review report (Codex final message)

1. **Summary**  
Sorting the collected shape IDs should remove tree visit order from CCD candidate evaluation and Explode’s wake and impulse order under the document’s stated callback assumptions. Two claims need correction: A0’s rebuild changes metadata used by the next step, and A2 overstates which candidates it stores.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2 A0 | A full rebuild does more than change layout: it sets `dfsOrdered` and resets tree allocation metadata. `b3NeedsRebuild` reads `dfsOrdered` when deciding whether broad-phase work runs, so the helper’s claim that it changes no other state is false (`src/dynamic_tree.c`, `src/dynamic_tree.h`, `src/broad_phase.c`). | Require the compared runs to have equivalent moved flags and broad-phase rebuild eligibility at each perturbation point; narrow the helper’s state claim. |
| 2 | L | §2 A2 | The array does **not** hold every candidate returned by the query: the proposed callback excludes shapes whose `explosionScale` is zero. The stated scratch and sort cost is therefore overstated (`src/physics_world.c`). | Say it stores every returned candidate with nonzero `explosionScale`, including those outside blast distance. |

3. **Checked, no change**

- CCD keeps its fraction across shapes and queries; static, kinematic, and dynamic query order is fixed (`src/solver.c`).
- Each shape has one proxy, so sorting shape IDs has no tie to resolve (`src/shape.c`).
- Pair keys are sorted before contact creation, and sensor overlaps are sorted and deduplicated (`src/broad_phase.c`, `src/sensor.c`).
- Explode queries only the dynamic tree, and waking a sleeping set makes its body and constraint order depend on which set wakes first (`src/physics_world.c`, `src/body.c`).
- The document correctly identifies the conflicting pre-solve statements in `docs/simulation.md`.

4. **Proposed edits**  
Revise A0’s helper contract and validation condition as in finding 1. Correct A2’s collection and cost wording as in finding 2.

5. **Unresolved**  
Whether A0’s planned perturbation points actually produce different rebuild eligibility needs to be established by the helper.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (M) | Confirmed in part. `b3DynamicTree_Rebuild( tree, true )` (`src/dynamic_tree.c`) strips moved marks only on an incremental build, rebuilds internal marks from the children, and sets `dfsOrdered`. `b3NeedsRebuild` (`src/dynamic_tree.h`) reads that flag and gates `b3UpdateBroadPhasePairs` (`src/broad_phase.c`) for the dynamic and kinematic trees, so "any other state" was false. The consequence is narrower than the finding says: `b3TestPair` and `b3GatherMovedSiblings` start only from moved marks, so a pair update entered on the flag alone finds no pairs and only enqueues the tree rebuild. World proxies in those trees are always created and moved with a mark (`b3BroadPhase_CreateProxy`, `b3BroadPhase_MoveProxy`). Scratch capacity is allocation, not simulation state. | Applied as a fact in A0: what a full rebuild keeps and sets, and why the ordered flag changes only layout. The proposed run-equivalence requirement is not adopted: it is a check (AC-4), and the construction already gives the property. The reviewer's Unresolved item is answered by the same fact. |
| 2 (L) | Confirmed. The A2 bullet first restricted collection to shapes with nonzero `explosionScale` and then said every returned candidate is held. | Applied: every returned candidate with a nonzero `explosionScale`, including those outside the blast distance. |

Note for 20261011-001: both findings apply there too (its A0 states the same "any other state" claim; its A3 bullet has the same contradiction). That document converged and is not edited here (U-I7).

## Status

5 rounds: 1/2/1 -> 0/1/2 -> 0/0/0 -> 0/2/0 -> 0/1/1. Series total 11 (H1-M6-L4). Document 191 -> 196 lines; the fixes added about 5 lines. No named mechanism added or removed; a proposed check was declined. Finding 1 is in A0, where round 2 found one; finding 2 is the first in A2. Neither is a consequence of the round 4 fixes. The raw log shows a `find` listing that names the round records and the changelog, and no read of them.
