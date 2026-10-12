---
title: Review round 5 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round4.md
mode: full pass
---

# Round 5

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
2026-10-12T08:34:02+10:00 to 08:36:37+10:00. Same prompt as rounds 1 to 4 against revision 5 (the
owner's choice (b) from round 4). U-R9: a read of the sections revision 5 changed found one
mid-paragraph line break in the Docs paragraph, reflowed (whitespace only) before the run.

## Review report (Codex final message)

## 1. Summary

The prompt follows the canonical design’s implementation choices, and most of its source and build claims match the live repository. It still has gaps in how the tests prove callback order and parallel behavior, plus an A2 fail-before command that never runs the A2 test.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §7, §8 | The fail-before script runs only the three A1 tests. At A2, it never invokes `ExplodeOrderTest`, despite §4 requiring that test to fail before the change and §8 requiring both results. | Add an A2-specific invocation, preserve its exit status, and record its before and after output. |
| 2 | M | §4 A1, §6 | `ContinuousCallbackOrderTest` requires identical `preSolve` logs, but never requires ascending shape ids. The running-fraction gate in `src/solver.c` can leave that log with just one entry, so the mapping does not establish the claimed order. | Make at least two candidates reach `preSolve`, then assert their candidate ids are ascending. A callback that returns false can keep the running fraction open for this fixture. |
| 3 | M | §4 A1, §6 | Every layout-perturbed A1 test uses one worker. `test/test_determinism.c` runs other worker counts without layout perturbation, leaving the new per-worker candidate array and parallel layout guarantee untested together. | Run a callback-free paired CCD fixture with multiple workers and perturbation, alongside the one-worker runs. |
| 4 | M | §4 A1 | “Spaced along the x axis” does not require the twelve sensors to be mutually disjoint. `src/sensor.c` queries the static tree for each sensor, so sensor-to-sensor overlaps can add begin events and invalidate the asserted count of eight. | Require pairwise non-overlapping sensors and place the bullet’s start and end outside every sensor. |
| 5 | L | §4 A0 | `PerturbTreeLayout` has no unlocked-world or recording precondition. `b3GetWorldFromId` permits access while locked, while a direct rebuild during recording has no recorded command for replay. | State and enforce that perturbation occurs between world calls, on an unlocked world with recording inactive. Keep the design’s direct rebuild. |
| 6 | L | §2 | The claim that no fat-AABB overlap at step start makes custom-filter calls CCD-only is too broad. `src/sensor.c` can call the filter during end-of-step sensor overlap. The specified callback corridor has no sensors, so its intended isolation remains valid. | Qualify the claim to the sensor-free callback fixture. |

## 3. Checked, no change

- The A1 separate static, kinematic, and dynamic passes and A2 locked query-and-apply loop follow design §2 and §5.
- The CCD sensor limit, running-fraction gates, event path, explosion falloff, wake order, worker array lifecycle, and iteration-counter behavior match the cited source.
- The test runner’s entry-name filter, CMake targets and options, benchmark `-s` output, golden-hash tests, and linked-issue requirement match the repository.
- The test-side visit-order witness uses the same tree traversal and mask as the specified fixtures when their candidate sets are kept exact.

## 4. Proposed edits

Apply the six actions in the findings table, then update §6’s callback-order evidence and §8’s fail-before records to match the strengthened tests.

## 5. Unresolved

Fixture permutations, pre-change divergence counts, and benchmark measurements still require execution during implementation. This review inspected the live source but did not run builds or tests.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live tree (U-V2). All adopted.

1. **H, A2 fail-before never run (adopted).** Confirmed: the §7 script names only the three A1 tests;
   the A2 case lived in a comment. The script now runs a test list, with the A2 list given explicitly.
2. **M, `preSolve` order unproven (adopted).** Confirmed: `preSolve` runs only for a candidate whose
   fraction is strictly below the running fraction (`src/solver.c`), and a separated candidate returns
   the running fraction (`src/distance.c`), so in ascending order the lowest-id hit can close the
   sweep and leave one entry. The fixture now puts at least two boxes on the sphere's path, returns
   false from `preSolve` so the running fraction stays open, and asserts the log ascending and equal
   across runs.
3. **M, no multi-worker layout test (adopted).** Confirmed: every A1 fixture ran one worker. The fuzz
   now reruns its first sixteen seeds at four workers with the same assertions.
4. **M, sensor-to-sensor events (adopted).** Confirmed: `b3SensorQueryCallback` (`src/sensor.c`) skips
   only the sensor's own shape and shapes on its own body; two sensors with events enabled on
   different bodies report each other. The twelve sensors now sit on one static body.
5. **L, perturbation preconditions (adopted).** Confirmed: `b3GetWorldFromId` does not check the lock
   (`src/physics_world.c`), and a direct rebuild writes no recording command. Stated as a precondition
   of the helper.
6. **L, CCD-only custom-filter claim (adopted).** Confirmed: sensor overlap also calls the custom
   filter (`src/sensor.c`). The §2 claim is qualified to a scene without sensors.

## Status

Round 5: 6 findings (H1-M3-L2), 6 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2. Series total 31 (H5-M18-L8). Not converged.

Tripwire. U-T1: no mechanism added or removed; the fixes change the script, fixture placements and
assertions, and add a precondition and a qualifier. U-T2: findings 2 to 4 are in §4 A1 tests, the
fifth round running; finding 2 is a consequence of the round 4 fix (the new callback test). Each
fix is a clause, not a paragraph. The owner saw the U-S4 state at round 4 and chose to continue;
this round is reported to the owner with the cluster named again. The raw log shows no read of round
records or the changelog. U-T3: 384 lines before the fixes; the after count is recorded in the
changelog. U-T6: under the 400-line split check; one decision.
