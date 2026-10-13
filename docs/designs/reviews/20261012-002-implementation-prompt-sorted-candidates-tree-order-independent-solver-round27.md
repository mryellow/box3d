---
title: Review round 27 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round26.md
mode: full pass
---

# Round 27

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
2026-10-12T11:20:33+10:00 to 11:25:00+10:00. Same prompt as rounds 1 to 26 against revision 28. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The prompt’s implementation mechanics, fixture APIs, and verification commands are consistent with the live source. One evidence gap remains: it treats a changed fuzz hash as proof of changed CCD candidate visit order, although the canonical design requires a visit-order witness for a query the test relies on.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1; §6 | The field portion of `ContinuousCandidateOrderTest` compares hashes without witnessing its CCD queries. A nonzero fail-before hash count establishes a result difference, not the candidate visit-order difference required by design §2 A0. The mapping explicitly substitutes that count for a witness. | Add an exact visit-order witness for field queries used as evidence, requiring the same candidate set and a different sequence; update the §6 mapping. If the fixture cannot provide one, follow the §9 stop condition rather than crediting the hash count as a witness. |

## 3. Checked, no change

- Confirmed the CCD callback, three tree queries, sensor limit and reporting path in `src/solver.c` and `src/sensor.c`.
- Confirmed the rebuild, query traversal, worker contexts, counters, wake order and explosion behavior in the cited `src/` files.
- Confirmed the test registration, golden configurations, CMake targets, benchmark output and contribution requirements in `test/`, `shared/`, `benchmark/`, `CMakeLists.txt` and `CONTRIBUTING.md`.
- The prompt preserves the settled sensor-selection and reporting decisions in design §5.

## 4. Proposed edits

Revise §4 A1 to require the field witness and revise the corresponding §6 evidence row. Keep the fail-before seed count as result evidence, separate from visit-order evidence.

## 5. Unresolved

The proposed fixtures’ actual fail-before and pass-after results require implementation and execution; this was a source review, with no builds run.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

The finding was re-verified (U-V7) and declined; the document is unchanged.

1. **M, field queries unwitnessed (declined; third re-raise, rounds 24, 26, 27).** The finding's
   proposed action is the one declined twice with the reason beside the decision in §4 and §6: no
   exact test-side witness over a field query exists, because the query's box is computed inside the
   solver (§2), and the fuzz's evidence is the one design §2 A1 prescribes, hashes compared over many
   seeds with the fail-before count recorded. The finding adds no source evidence; its new element,
   that a hash difference is a result difference and not a visit-order difference, does not hold for
   this fixture: with the perturbation the only difference between the runs, a diverged field hash
   has no cause but a changed candidate order in a field query. The §9 stop condition it invokes
   covers a witness a test prescribes and cannot make differ, not a query the document says is not
   witnessed. Per U-P13 the next request states the settled decision as scope: the fuzz's evidence
   form is the design's, and the absence of a field witness is not a finding. The scope wording
   changes nothing else in the request (U-P3, U-P4).

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 27: 1 finding (H0-M1-L0), 0 applied, 1 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0 -> 0/2/2 -> 0/2/1 -> 0/1/0. Series total 81 (H7-M56-L18). Not converged (U-V9).

Tripwire. U-T1: no change to the doc. U-T2: the one finding is the third consecutive re-raise of
the fuzz witness point; U-T5 applied twice (reason stated in §4, then beside the §6 row, then citing
the design); the remaining step is U-P13. U-T3: 477 lines, unchanged. U-T6: over the split check;
one decision, recorded in round 6.
