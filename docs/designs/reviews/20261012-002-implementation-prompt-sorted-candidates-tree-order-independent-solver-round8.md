---
title: Review round 8 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round7.md
mode: full pass
---

# Round 8

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
2026-10-12T08:57:43+10:00 to 09:00:00+10:00. Same prompt as rounds 1 to 7 against revision 8. U-R9: a
read of the sections revision 8 changed found two mid-paragraph line breaks, reflowed (whitespace
only) before the run.

## Review report (Codex final message)

## 1. Summary

The prompt’s implementation mechanics and repository commands largely match the live source and the canonical design. Three verification claims need tightening: the CCD tests do not establish ordering in every queried tree, the four-worker fixture does not prove parallel use of the candidate arrays, and the explosion test can pass if the new implementation applies no explosion.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1; §6 | The mapping claims layout-independent CCD results, but every exact CCD witness covers the static tree. There is no kinematic candidate fixture, and the fuzz field does not witness bullet queries of the dynamic tree. A defect in either added sort pass could pass these tests (`src/solver.c`; design §2 A0–A1). | Add witnessed bullet fixtures with multiple kinematic and dynamic candidates, or narrow the mapping’s evidence claim. Account for the tree rebuild before bullet CCD when recording those witnesses. |
| 2 | M | §4 A2; §6 | `ExplodeOrderTest` checks equality between runs without checking that an explosion woke or accelerated any body. An implementation that skips every collected shape would satisfy both comparisons, so the test does not establish the wake and impulse behavior claimed by its mapping row (`src/physics_world.c`; design §2 A2). | Assert the expected bodies are awake and that chosen bodies receive nonzero impulses, alongside the existing order and bitwise velocity comparisons. |
| 3 | L | §4 A1 | Forty fast bodies and twenty bullets create several work blocks, but do not prove that CCD ran on multiple workers. The scheduler may let one worker drain them, while the prompt says the per-worker candidate array is exercised in parallel (`src/solver.c`, `src/parallel_for.c`). | Record which worker contexts processed candidates, or describe this fixture as *eligible* for parallel execution rather than verified parallel coverage. |

## 3. Checked, no change

- The design’s §5 provenance decisions are carried through, including sorted evaluation and first-eight sensor selection.
- The CCD filters, running fraction, sensor-hit path, and callback gates match `src/solver.c` and `src/sensor.c`.
- The explosion query, lock interval, wake path, and awake-set inspection path match `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- The stated test entry filtering, CMake targets, benchmark CSV behavior, and contribution requirements match the repository files reviewed.

## 4. Proposed edits

Strengthen the A1 and A2 fixtures as described above, then update their §6 mapping rows to state exactly what each test observes. Qualify the parallel-coverage claim unless worker use is measured.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All three findings checked against the live tree (U-V2). All adopted.

1. **M, only the static tree witnessed (adopted).** Confirmed: every exact CCD witness was on the
   static tree; the fuzz had no kinematic body and did not witness bullet queries of the dynamic tree,
   so a defect in either of A1's other two sort passes could pass. The fuzz now adds two bullet
   corridors per seed, one against resting kinematic boxes and one against resting dynamic boxes. The
   step order fixes where their witnesses are taken: `b3UpdateBroadPhasePairs` enqueues the dynamic
   and kinematic rebuild, `b3Solve` waits for it before finalize, CCD and the bullet stage follow,
   and afterwards only `b3DynamicTree_EnlargeProxy` touches those trees, enlarging bounds and marking
   moved nodes without restructuring (`src/physics_world.c`, `src/solver.c`, `src/broad_phase.c`,
   `src/dynamic_tree.c`). With sleep off, the structure after the step is the structure CCD queried,
   and the structure before the step is not. The kinematic and dynamic witnesses are recorded after
   the step, with the bullet's own shape removed from the sequence.
2. **M, Explode test blind to a no-op (adopted).** Confirmed: both comparisons hold if the loop
   applies nothing. The test now requires every body in range awake with nonzero linear velocity after
   the explosion, in both runs, before the bitwise comparison.
3. **L, parallel coverage overclaimed (adopted).** Confirmed: several blocks are dispatched, and
   which worker drains which is the scheduler's choice (`src/parallel_for.c`). Wording narrowed to
   eligibility.

The raw log's hits on `designs/reviews` and `changelog` are a `git status` listing and a `wc -l`
listing; no read of round records or the changelog.

## Status

Round 8: 3 findings (H0-M2-L1), 3 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1. Series total 39 (H5-M25-L9).
Not converged.

Tripwire. U-T1: the fixes add two fixture pieces (bullet corridors against kinematic and dynamic
clusters) and one assertion; nothing removed. U-T2: findings 1 and 3 are in §4 A1 tests, the eighth
round running; finding 3 is a consequence of the round 6 fix, finding 1 is new ground (the two other
trees), finding 2 is the first in §4 A2 since round 2. U-T3: 405 lines before the fixes; the after
count is recorded in the changelog. U-T6: over the split check; one decision, recorded in round 6.
