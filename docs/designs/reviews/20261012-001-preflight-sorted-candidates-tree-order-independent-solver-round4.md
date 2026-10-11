---
title: Review round 4 - preflight sorted-candidates tree-order-independent solver changes
target: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-preflight-sorted-candidates-tree-order-independent-solver-round3.md
mode: full pass
---

# Round 4

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T06:32:55+10:00 to 06:35:38+10:00. Same prompt as round 3. The document was edited by the owner after round 3 (A1 caveats: stack-buffer alternative to the per-worker array), so this is a full pass, not the confirmation round.

## Review report (Codex final message)

## 1. Summary

Sorting candidates by shape ID addresses the two tree traversal order dependencies identified in the design, and the proposed per-worker storage and Explode lock scope fit the live call paths. The CCD behavior section understates what can change before versus after A1.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §1, §2 A1 | The design says CCD impact fractions can differ *within the TOI tolerance*. In `src/distance.c`, that tolerance is a separation **distance**, not a bound on fraction difference; TOI failure paths can also return a fraction. `src/solver.c` accepts returned fractions without a tolerance bound. | Remove the fraction-bound claim. State that changing candidate order and the running limit can change the returned impact fraction, with no documented bound. |
| 2 | M | §1, §2 A1 | The behavior section describes `preSolve` calls as arriving in a new order, but their **set and arguments can change**. In `src/solver.c`, a solid candidate reaches `preSolve` only when its TOI fraction is below the running fraction. Processing a nearer candidate first can therefore suppress a call that occurred under the old order, even when the final collision fraction is unchanged. | State the possible call-count and argument changes in the behavior notice and PR guidance, alongside the order change. |

## 3. Checked, no change

- `src/solver.c` queries static candidates first, then kinematic and dynamic candidates for bullets; its current callback evaluates them during traversal and caps continuous sensor hits at eight.
- `src/broad_phase.c` sorts accepted pair keys before contact creation. `src/sensor.c` sorts and deduplicates overlap results.
- `src/physics_world.c` queries only the dynamic tree for Explode, and keeps the world locked while waking bodies and applying impulses.
- `src/dynamic_tree.c` full rebuild preserves proxy identities and leaf data while rebuilding tree nodes. The A0 visit-order witness is necessary because the step rebuilds dynamic and kinematic trees before CCD.
- `src/qsort.h` handles empty and single-element sorts. The proposed per-worker array follows existing `b3TaskContext` storage and worker-index use.

## 4. Proposed edits

Revise §1’s CCD behavior list and §2 A1’s caveats to distinguish changed callback order from changed `preSolve` calls and arguments. Replace “within the TOI tolerance” with an unbounded qualitative statement about possible fraction changes.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (M) | Confirmed. In `src/distance.c` the tolerance is `0.25f * B3_LINEAR_SLOP` on separation distance, and `b3_toiStateFailed` returns the bracket fraction `t1`. `b3ContinuousQueryCallback` (`src/solver.c`) accepts any fraction in `(0, running)`. Nothing bounds the difference between two fractions found under different limits. | Applied in §1: the fraction can differ, with no bound, because each candidate's query runs to a different running limit. |
| 2 (M) | Confirmed. In `src/solver.c` `preSolve` is reached only when `0 < output.fraction < continuousContext->fraction` after the TOI call, so a nearer candidate processed first suppresses the call a farther candidate got today, and the point and normal come from the new limit. The custom filter is called before the TOI call for every candidate, so only its order changes. | Applied in §1 and the A1 caveats: `preSolve` call set and arguments can change; custom filter order only. |

The owner's stack-buffer bullet drew no finding; the reviewer's "Checked, no change" says the per-worker array follows existing `b3TaskContext` storage.

Note for 20261011-001: finding 1 applies there too. Its §1 says impact fractions "can differ within the TOI tolerance". That document converged and is not edited here (U-I7).

## Status

4 rounds: 1/2/1 -> 0/1/2 -> 0/0/0 -> 0/2/0. Series total 9 (H1-M5-L3). Document 186 -> 191 lines; the fixes added about 5 lines. No named mechanism added or removed. Both findings are in §1 and A1, where round 1 found findings; neither is a consequence of the round 1 or owner edits (both describe today's gating in `src/solver.c`, which the document had understated since revision 1). The raw log shows no read of round records or the changelog.
