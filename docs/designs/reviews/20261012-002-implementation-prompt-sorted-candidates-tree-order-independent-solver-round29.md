---
title: Review round 29 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round28.md
mode: full pass
---

# Round 29

## Exact request sent to Codex

```
Review the document docs/designs/20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md: the instructions an implementing agent follows to land that design's three changes (A0 test helper, A1 CCD candidate order, A2 Explode order) as three commits, with the test mechanism, fixtures, test-to-requirement mapping, verification commands and PR/closure requirements. The design decides what changes and why; the Implementation Prompt decides how it lands and how it is shown to work. Test mechanism, fixtures and mapping are in scope here, not out of scope as they are for a design doc.

Scope: the Implementation Prompt's own content, the design it implements (read it in full; it is binding and its section 5 Provenance decisions are settled), and the source both cite (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, src/solver_set.c, src/container.h, include/box3d/, test/, shared/, benchmark/, docs/simulation.md, CONTRIBUTING.md, CMakeLists.txt, CMakePresets.json). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the Implementation Prompt makes about it.

Pressure-test the prompt. Find real defects: claims about the code or build that are false; an instruction that contradicts the design or re-decides something the design settled; a fixture that cannot do what the prompt says (cannot be made to fail before the change, cannot pass after it, cannot observe what it asserts through the API named, or asserts something the design does not require); a witness or hash that does not measure what the prompt claims; a helper contract with a hazard the prompt does not state (world locking, recording, sleep, multi-worker, allocation, assertion builds); a command that does not work in this repository; a mapping row whose evidence does not establish its clause; a missing instruction without which the implementer must guess; contradictions between sections. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h, b3Hash, the test runner) as settled tools; judge what the prompt adds and the question each primitive is asked.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, other design documents, and the design's own decisions (cite them; do not re-open them). One such decision, stated here because it governs a fixture: the design's section 2 A1 prescribes the seeded fuzz's evidence as hashes compared over many seeds under the A0 perturbation with the count of seeds that failed before the change recorded; the Implementation Prompt accordingly witnesses the corridors inside each seed and not the field's own CCD queries, and the absence of a field witness is settled and not a finding.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0,
2026-10-12T11:31:33+10:00 to 11:33:46+10:00. Same request as round 28. Provenance (U-R8 spirit):
the revision 29 edits the round 28 record describes failed to apply before this run, through an
author-side shell error that aborted the edit script, so this round reviewed revision 28 unchanged,
with the two round 28 findings still unfolded. The edits were applied after this run, as revision 29,
and the changelog entry written before the run now carries the correct line count. This round's
result is recorded and not counted toward convergence; the next round is the full pass against
revision 29.

## Review report (Codex final message)

## 1. Summary

The implementation prompt is consistent with the Canonical design, including its settled §5 decisions. I found no defect in its source claims, fixtures, test mapping, or verification instructions.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| — | — | — | None. | — |

## 3. Checked, no change

- A0 witness timing and tree perturbation against `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/solver.c`.
- A1 candidate gates, sensor reporting, callback order, worker contexts, and counters against `src/solver.c`, `src/sensor.c`, and `src/physics_world.c`.
- A2 wake order and velocity checks against `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- Test registration, golden policy, build targets, benchmark output, and contribution requirements against `test/`, `CMakeLists.txt`, `benchmark/`, and `CONTRIBUTING.md`.

## 4. Proposed edits

None.

## 5. Unresolved

None.

## 6. Verdict

CONVERGED

## Post-review verification (Claude)

No findings to verify. The reviewer reported none against revision 28, which still carried the two
round 28 findings; that is reviewer variance, not evidence that they were not defects (U-V2 applies
to findings, not to their absence). Nothing is folded from this round.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 29: 0 findings (H0-M0-L0), 0 applied, 0 declined; not counted (reviewed revision 28 before
the revision 29 fold). Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2 -> 0/2/1 -> 0/1/0 -> 0/1/1 -> 0/0/0 (not counted). Series total 83
(H7-M57-L19). Not converged.

Tripwire. U-T1: no change to the doc by this round. U-T2: n/a. U-T3: 477 lines at the run; the
revision 29 fold after it is recorded in the changelog. U-T6: over the split check; one decision,
recorded in round 6.
