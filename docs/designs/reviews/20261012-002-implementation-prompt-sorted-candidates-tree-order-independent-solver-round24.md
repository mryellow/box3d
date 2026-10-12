---
title: Review round 24 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round23.md
mode: full pass
---

# Round 24

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
2026-10-12T11:06:26+10:00 to 11:11:39+10:00. Same prompt as rounds 1 to 23 against revision 25. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The implementation instructions largely match the canonical design and live source. One evidence gap remains: the fuzz test asserts field hashes without witnessing the field CCD queries those hashes depend on.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §§4 A1, 6 | `ContinuousCandidateOrderTest` compares the field’s fast-body hash at every step, but witnesses only separate corridor queries. The prompt acknowledges that the field queries are unwitnessed, while §6 claims the design’s §2 A0 witness requirement is covered for queries the test relies on. A corridor order change cannot establish that a field hash comparison exercised a changed candidate order (`src/solver.c`, `src/dynamic_tree.c`). | Add an exact visit-order witness for field CCD queries used as evidence, requiring the same candidate set and a different order, and correct the mapping row. |

## 3. Checked, no change

- The A1 collection, sort, and evaluation instructions match the CCD query and worker-context paths (`src/solver.c`, `src/physics_world.c`, `src/physics_world.h`).
- The sensor event fixture can observe CCD hits through `b3World_GetSensorEvents` (`src/solver.c`, `src/sensor.c`).
- The A2 fixture can observe wake order and velocities through the named internals and APIs (`src/body.c`, `src/solver_set.c`, `src/physics_world.c`).
- The stated CMake targets, test-name filtering, benchmark CSV output, and counter semantics match the repository (`CMakeLists.txt`, `test/main.c`, `benchmark/main.c`, `src/physics_world.c`).

## 4. Proposed edits

Revise §§4 A1 and 6 so each field hash comparison offered as witness-backed evidence has a corresponding exact field query witness. Keep the corridor witnesses for their three tree-specific checks.

## 5. Unresolved

The proposed fixtures have not been implemented or run, so their runtime outcomes remain unverified.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The finding was checked against the live tree and the document (U-V2, U-V4, U-V7) and declined.

1. **M, field CCD queries unwitnessed (declined; clarification folded).** The document decides this in
   §4: the corridor hashes are the assertions tied to witnessed queries, one per tree, and the field
   hash is the design's broad fuzz whose queries are not witnessed. The decision was taken when the
   fuzz mapping was corrected from "any layout" to the witnessed layouts, and the reason holds: a
   field query's box is the fast body's start box united with its end box at the integrated
   transform, computed inside the solver (`src/solver.c`), so a test-side witness over the field is
   approximate, not exact, and §2 makes exactness the condition for a witness. The field assertion's
   non-vacuity is shown another way the document already requires: the fail-before run must record a
   nonzero count of seeds whose field hash diverges, and the implementer raises density or body count
   until it does, which is the design's own §2 A1 instruction. The finding engages neither point and
   brings no fresh source evidence. Folded as clarification only: §4 states that the nonzero
   fail-before count is the field's evidence of a changed order, and the §6 A0 row says the field's
   queries are not witnessed and names that count.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 24: 1 finding (H0-M1-L0), 0 applied, 1 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0 -> 0/1/0. Series total 73 (H7-M51-L15). Not converged (U-V9).

Tripwire. U-T1: no mechanism added or removed; two clauses of stated reason. U-T2: the finding is a
re-raise of the round 14 mapping finding in the fuzz subsection, not a consequence of the previous
round's fix; U-T5: the reason was in §4 but not beside the §6 row, now it is. U-T3: 470 lines before
the fold; the after count is recorded in the changelog. U-T6: over the split check; one decision,
recorded in round 6.
