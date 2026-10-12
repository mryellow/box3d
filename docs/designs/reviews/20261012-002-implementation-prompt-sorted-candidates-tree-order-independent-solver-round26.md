---
title: Review round 26 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round25.md
mode: full pass
---

# Round 26

## Exact request sent to Codex

```
Review the document docs/designs/20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md: the instructions an implementing agent follows to land that design's three changes (A0 test helper, A1 CCD candidate order, A2 Explode order) as three commits, with the test mechanism, fixtures, test-to-requirement mapping, verification commands and PR/closure requirements. The design decides what changes and why; the Implementation Prompt decides how it lands and how it is shown to work. Test mechanism, fixtures and mapping are in scope here, not out of scope as they are for a design doc.

Scope: the Implementation Prompt's own content, the design it implements (read it in full; it is binding and its section 5 Provenance decisions are settled), and the source both cite (src/solver.c, src/physics_world.c, src/physics_world.h, src/broad_phase.c, src/sensor.c, src/dynamic_tree.c, src/qsort.h, src/body.c, src/solver_set.c, src/container.h, include/box3d/, test/, shared/, benchmark/, docs/simulation.md, CONTRIBUTING.md, CMakeLists.txt, CMakePresets.json). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the Implementation Prompt makes about it.

Pressure-test the prompt. Find real defects: claims about the code or build that are false; an instruction that contradicts the design or re-decides something the design settled; a fixture that cannot do what the prompt says (cannot be made to fail before the change, cannot pass after it, cannot observe what it asserts through the API named, or asserts something the design does not require); a witness or hash that does not measure what the prompt claims; a helper contract with a hazard the prompt does not state (world locking, recording, sleep, multi-worker, allocation, assertion builds); a command that does not work in this repository; a mapping row whose evidence does not establish its clause; a missing instruction without which the implementer must guess; contradictions between sections. Treat existing primitives (b3DynamicTree, b3ShapeTimeOfImpact, qsort.h, b3Hash, the test runner) as settled tools; judge what the prompt adds and the question each primitive is asked.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, other design documents, and the design's own decisions (cite them; do not re-open them).

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0,
2026-10-12T11:16:25+10:00 to 11:19:19+10:00. Same prompt as rounds 1 to 25 against revision 27. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The prompt largely matches the canonical design and live code, but its fuzz test has an evidence gap: it treats unwitnessed field queries as proof of tree order independence despite design §2 A0’s witness requirement. I also found a fail-before check that can stop before measuring velocity divergence, and one false source claim.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1; §6 | `ContinuousCandidateOrderTest` asserts field hashes at every step but does not witness the field queries. A nonzero aggregate fail-before count does not establish changed visit order for the queries and steps used as evidence. This conflicts with design §2 A0’s check for each query a test relies on. | Make the field queries witnessable by construction and require differing candidate sequences for the asserted steps; keep the fail-before count as additional evidence. |
| 2 | M | §4 A2; §3 | `ExplodeOrderTest` checks awake-set order first. In a fail-before run, that expected failure can return before the velocity comparison, so the instruction to move shapes *if the velocity check passes before the change* cannot be followed from that test output. `RUN_SUBTEST` and `ENSURE` return on the first failure (`test/test_macros.h`). | Compute and report both comparisons before the test returns failure, then preserve awake-order and velocity assertions. |
| 3 | L | §4 A1 | The prompt says only `src/broad_phase.c` currently includes `qsort.h`; `src/mesh_contact.c` includes it too. | Remove “only,” or name both users. |

## 3. Checked, no change

- The CCD callback gates filters, time of impact, sensors and `preSolve` as described; static, kinematic and dynamic queries run in the stated order (`src/solver.c`).
- The tree rebuild and query behavior supports the prompt’s corridor witness approach, including the need to observe dynamic and kinematic trees after the step (`src/dynamic_tree.c`, `src/broad_phase.c`, `src/solver.c`).
- Sensor hits can reach begin events after a pass-through step, and the explosion fixture can observe wake order and velocities through the named APIs and internals (`src/sensor.c`, `src/physics_world.c`, `src/body.c`, `src/solver_set.c`).
- Worker blocks, array lifecycle, test registration, CMake targets, benchmark output and the contribution requirements match the cited source.

## 4. Proposed edits

Revise the fuzz fixture and its §6 mapping so each field hash assertion has a visit-order witness under design §2 A0. Revise A2’s fail-before procedure to collect both divergence results before asserting. Correct the `qsort.h` inclusion claim.

## 5. Unresolved

The prompt still needs a concrete test-side construction for witnessing its fuzz-field queries. Fail-before counts and fixture behavior cannot be established by source inspection alone.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

All three findings checked against the live tree and the design (U-V2, U-V4, U-V7). One declined,
two adopted.

1. **M, field queries unwitnessed (declined; re-raised from round 24).** Re-verified against the
   design, which the request puts out of scope to re-open: design §2 A1 "Tests" prescribes the fuzz
   as hashes compared over many seeds under the A0 perturbation, with the PR recording how many
   seeds failed before the change, because the divergence is small and rare. The count is the
   design's chosen evidence for the fuzz; the witness requirement of design §2 A0 is met in this
   document by the corridors inside every seed, one per tree. The finding brings no source evidence
   that a field query can be witnessed exactly, and the reason it cannot stands (round 24). Folded as
   clarification: the fuzz paragraph now cites design §2 A1 for the count. If the finding recurs
   without fresh evidence, the prompt states this settled decision as scope (U-P13).
2. **M, velocity instruction unreachable in the fail-before run (adopted).** Confirmed: `ENSURE`
   returns at the first failed check (`test/test_macros.h`), so with the awake-set comparison first
   the velocity comparison is never reported before the change. The test now computes both
   comparisons, prints both, then asserts both; the shape-moving instruction reads the printed
   velocity result.
3. **L, "only `src/broad_phase.c`" is false (adopted).** Confirmed: `src/mesh_contact.c` includes
   `qsort.h` too. The word is removed.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 26: 3 findings (H0-M2-L1), 2 applied, 1 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2 -> 0/2/1. Series total 80 (H7-M55-L18). Not converged.

Tripwire. U-T1: no mechanism added or removed; a compute-then-assert order in one fixture. U-T2:
finding 1 is the second consecutive re-raise of the fuzz witness point (U-T5: the reason now cites
the design); finding 2 is a consequence of the round 18 fix (awake-set check first); finding 3 of the
round 25 fix. Each fix is a clause. U-T3: 476 lines before the fix; the after count is recorded in
the changelog. U-T6: over the split check; one decision, recorded in round 6.
