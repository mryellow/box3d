---
title: Review round 13 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round12.md
mode: full pass
---

# Round 13

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
2026-10-12T09:39:51+10:00 to 09:43:04+10:00. Same prompt as rounds 1 to 12 against revision 14. U-R9:
a read of the sections revision 14 changed found two mid-paragraph line breaks, reflowed (whitespace
only) before the run.

## Review report (Codex final message)

1. **Summary**

The implementation instructions largely match the canonical design and live source. Two defects in the verification plan need edits: the reported fuzz field count can include corridor failures, and the fail-before command block does not run the stated checks as written.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1, §6, §7 | The field hash includes corridor bodies from step two onward. Because each corridor is required to diverge before A1, a later field-hash divergence can come entirely from a corridor. The reported “field” failed-seed count therefore does not establish the broad fuzz result required by design §2 A1. | Keep a field-only hash at every step and report its failed-seed count. Track any combined-world hash separately. |
| 2 | M | §7 | `W=<scratch directory>/fail-before` is invalid shell syntax as written. The active `TESTS` assignment also selects the A1 tests for the A2 run; `ExplodeOrderTest` is only in a comment. Thus the supplied block cannot produce the required fail-before evidence without editing it. | Use a valid, quoted scratch path and give explicit A1 and A2 invocations, or pass the test list as an argument. |

3. **Checked, no change**

- The A1 collection, per-query sorting, worker-array lifecycle, and iteration-counter descriptions match `src/solver.c`, `src/physics_world.h`, `src/physics_world.c`, `src/container.h`, and `src/qsort.h`.
- The witness approach matches query traversal and the step’s tree rebuild/refit sequence in `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/solver.c`, subject to the prompt’s stated fixture conditions.
- The sensor-event path and eight-hit assertion match `src/solver.c` and `src/sensor.c`.
- The Explode fixture observes the wake and impulse paths in `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- The test target, entry-name filtering, benchmark flags, CSV output, documentation edit, and PR requirements match the cited repository files.

4. **Proposed edits**

Revise §4 A1 and the corresponding §6–7 mapping so the field count always uses field bodies alone. Replace §7’s fail-before block with executable commands for each increment.

5. **Unresolved**

The required pre-change divergence counts and witness sequences still need to be demonstrated when the fixtures are implemented.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Both findings checked (U-V2). Both adopted.

1. **M, corridor bodies in the field count (adopted).** Confirmed from the fixture text: revision 14
   folded the corridor bodies into the field hash from the second step, and each corridor is required
   to diverge before the change, so the field's failed-seed count could include corridor divergence.
   The field hash is now over field bodies alone at every step; corridor bodies are hashed only at
   their corridor step.
2. **M, fail-before block not executable (adopted).** Confirmed: the scratch path was an
   angle-bracket placeholder, and the A2 test list lived in a comment. The block is now a shell
   function taking the test list, called once for A1 and once for A2, with a quoted scratch variable.

The raw log's hits on `designs/reviews` and `changelog` are a `git status` listing and the request's
own out-of-scope line; no read of round records or the changelog.

## Status

Round 13: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0. Series total 52 (H5-M37-L10). Not converged.

Tripwire. U-T1: no mechanism added or removed. U-T2: finding 1 is a consequence of the round 12 fix;
finding 2 is in §7, last touched in round 11, and the placeholder dates from revision 4. Each fix is
a clause or a command edit. U-T3: 435 lines before the fixes; the after count is recorded in the
changelog. U-T6: over the split check; one decision, recorded in round 6.
