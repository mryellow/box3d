---
title: Review round 6 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round5.md
mode: full pass
---

# Round 6

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:42:25+10:00 to 06:44:58+10:00. Same prompt as round 5, on the document revised after round 5.

## Review report (Codex final message)

1. **Summary**  
The source supports A1’s per-query shape-id ordering and A2’s sorted explosion processing. I found one low-severity error in A0’s description of full rebuilds; it affects the helper’s stated pair-update behavior, though I found no case where it defeats the proposed collision or explosion result guarantee.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §2 A0 | A full `b3DynamicTree_Rebuild(tree, true)` does **not** always set the tree ordered. In `src/dynamic_tree.c` it returns immediately for an empty tree, leaving `dfsOrdered` unchanged. An empty dynamic or kinematic tree can therefore retain an unordered flag, which `src/dynamic_tree.h` and `src/broad_phase.c` use to enter pair-update work despite no moved proxies. | Qualify the claim to nonempty trees. Specify how the helper handles empty-tree flags if it needs the two runs to take the same pair-update path. |

3. **Checked, no change**

- In `src/solver.c`, CCD retains its fraction and eight sensor slots across shape and tree queries; sorting each query’s unique shape ids preserves the stated static, kinematic, dynamic order without adding TOI calls.
- The finalize and bullet passes use per-worker task contexts in separate parallel stages. The proposed scratch reuse has no evident cross-worker or same-world re-entrancy conflict under the callback rules in `include/box3d/`.
- `src/broad_phase.c` sorts pair keys after discovery; `src/sensor.c` sorts and deduplicates overlaps. Stateful callback limitations are stated.
- `src/physics_world.c` keeps the explosion query serial and the world locked; sorted unique shape ids fix its wake and per-body impulse addition order. I found no sort-tie issue.
- For nonempty trees, `src/dynamic_tree.c` preserves proxy identities, leaf bounds, and moved marks through a full rebuild. The document correctly requires a visit-order witness after the step’s tree rebuild.

4. **Proposed edits**  
Revise §2 A0’s “sets the tree ordered” and subsequent ordered-flag explanation to account for the empty-tree early return.

5. **Unresolved**  
None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (L) | Confirmed. `b3DynamicTree_Rebuild` (`src/dynamic_tree.c`) returns before any write when `proxyCount` is zero, and only an insert or a rotation clears `dfsOrdered`, so an emptied tree can keep the unordered flag. The perturbation is then a no-op on that tree, so both runs take the same pair-update path; the helper needs no handling. | Applied in A0: the rebuild claim is stated for a nonempty tree, and the empty-tree early return is stated. The proposed helper handling is not adopted; the runs already agree. |

Note for 20261011-001: not applicable; that document does not make the rebuild claim.

## Status

6 rounds: 1/2/1 -> 0/1/2 -> 0/0/0 -> 0/2/0 -> 0/1/1 -> 0/0/1. Series total 12 (H1-M6-L5). Document 196 -> 197 lines; the fix added about 1 line. No named mechanism added or removed. The finding is in A0 for the second round running and is a consequence of the round 5 fix, which stated the rebuild's effect without the empty-tree case (U-T2). The raw log shows no read of round records or the changelog.
