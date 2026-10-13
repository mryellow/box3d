---
title: Review round 31 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round30.md
mode: full pass (confirmation, U-P9)
---

# Round 31

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
2026-10-12T11:37:28+10:00 to 11:40:20+10:00. Same request as rounds 28 to 30, against revision 29,
unchanged since round 30 (U-P9: confirmation at the same scope, no added framing). After its complete
final message the stream carried one Codex session-logging error line ("failed to record rollout
items"); the exit was 0 and the message is reproduced whole. U-R9: no change to the target since the
last read.

## Review report (Codex final message)

1. **Summary**

I found no material defect in the Implementation Prompt. Its source and build claims align with the live tree, and its fixtures and mapping provide a workable way to show the canonical design’s three changes.

2. **Findings**

None.

3. **Checked, no change**

- The A0 rebuild and visit-order witnesses match the tree query and rebuild paths in `src/dynamic_tree.c` and `src/broad_phase.c`.
- The A1 sort points, callback gates, sensor limit, and per-worker array lifecycle match `src/solver.c`, `src/physics_world.c`, and `src/physics_world.h`.
- The A2 fixture can observe both wake order and velocity through `src/body.c`, `src/solver_set.c`, and the named APIs.
- The test registration, precision options, benchmark output, and verification commands match the repository’s CMake files and runners.

4. **Proposed edits**

None.

5. **Unresolved**

The witness permutations, fail-before counts, and benchmark deltas require the proposed implementation and test runs; this was a source review.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

No findings. Nothing folded.

Gates before Canonical (U-G1, U-G4), checked by the author against revision 29:

- AC-1: the document adds no structure over an engine population. The per-worker candidate array
  and the explosion array hold one query's shape ids, are cleared or destroyed after the sort and
  evaluation pass, and follow the settled `pairKeys` shape; the design that decides them is Canonical.
- AC-2: no "the following must call" list. The sort-and-evaluate pass after each tree query is the
  one route; the test helper's hazards are stated as contract, not as a caller list.
- AC-3: nothing is reconciled with anything; witnesses and hashes are read from the live world.
- AC-4: no validator or guard changes control flow in `src/`; the document's checks are test
  assertions of constructed preconditions (leaf bounds disjoint, sphere flagged fast, distinct
  sleeping sets, bullets flagged fast) and test comparisons between runs.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 31: 0 findings (H0-M0-L0), 0 applied, 0 declined. Confirmation of round 30 at full scope.

Convergence record. 31 rounds:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2 -> 0/2/1 -> 0/1/0 -> 0/1/1 -> 0/0/0 (not counted) -> 0/0/0 -> SETTLED 0/0/0.
Series total 83 findings (H7-M57-L19), 78 applied, 5 declined (rounds 24, 26, 27 the same point;
round 19's two were carried to the owner and resolved by the owner's decisions in revision 21).
Final revision 29, 478 lines. Status set to Canonical as the next edit after this round (U-I5).

Tripwire. U-T1: no change to the doc. U-T2: n/a. U-T3: 478 lines, unchanged. U-T6: over the split
check; one decision, recorded in round 6.
