---
title: Review round 30 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round29.md
mode: full pass
---

# Round 30

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
2026-10-12T11:34:34+10:00 to 11:37:03+10:00. Same request as rounds 28 and 29, against revision 29,
the revision that folds the round 28 findings. U-R9: the target is untracked so there is no diff;
the four edited passages were read whole after the fold; no reflow needed.

## Review report (Codex final message)

1. **Summary**  
The Implementation Prompt is consistent with the canonical design’s settled decisions and with the live source I checked. I found no defect in its fixtures, witnesses, requirement mapping, or verification instructions.

2. **Findings**

None.

3. **Checked, no change**

- The A0 rebuild and visit-order witness contract matches `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/solver.c`.
- The A1 sort passes, sensor-event fixture, callback-order fixture, and per-worker array lifecycle match `src/solver.c`, `src/sensor.c`, and `src/physics_world.c`.
- The A2 wake, impulse, and awake-set assertions match `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- The test registration, CMake targets and options, benchmark output, golden-value policy, and PR requirements match the cited repository files.

4. **Proposed edits**

None.

5. **Unresolved**

The fixtures and commands were reviewed against source; they were not executed. Their required fail-before results remain implementation-time checks.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

No findings. The "Unresolved" note restates what the document already says: fail-before results are
produced at implementation time (§3, §8). Nothing folded.

The raw log's hits on `changelog` are a line-count listing of the designs directory, not a read of
any changelog or round record; no hit on `designs/reviews` beyond the request's own out-of-scope
line.

## Status

Round 30: 0 findings (H0-M0-L0), 0 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2 -> 0/2/1 -> 0/1/0 -> 0/1/1 -> 0/0/0 (not counted) -> 0/0/0. Series total 83
(H7-M57-L19). First clean round at full scope against the current revision; a confirmation round
follows (U-P9).

Tripwire. U-T1: no change to the doc. U-T2: n/a. U-T3: 478 lines, unchanged. U-T6: over the split
check; one decision, recorded in round 6.
