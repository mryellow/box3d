---
title: Review round 16 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round15.md
mode: full pass
---

# Round 16

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
2026-10-12T10:04:58+10:00 to 10:09:28+10:00. Same prompt as rounds 1 to 15 against revision 17. U-R9:
the target is untracked so there is no diff; a read of the whole doc found one overlong line and one
short-line break in the paragraphs revision 17 touched, reflowed (whitespace only) before the run.

## Review report (Codex final message)

## 1. Summary

The implementation instructions largely match the canonical design and the live solver paths. Two verification instructions need revision: the fail-before command does not establish that every required test failed, and the benchmark commands do not preserve results for comparison across revisions.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §3, §7 | `fail_before` returns nonzero both when a test fails as expected and when the build fails. For the three A1 tests, it also returns nonzero if only one fails. Its status therefore cannot establish the required fail-before result for **each** test. | Give build failure a distinct status; record and check every named test’s exit code separately; require each to fail before the change and pass afterward. |
| 2 | L | §7 | `benchmark/main.c` writes each scene’s CSV into the current directory using the same filename on both revisions. Running the shown command twice from the same `build-rel` directory overwrites the first measurements. | Specify separate output directories or an explicit copy of all CSVs before the second run. |

## 3. Checked, no change

- The A0 query witness matches `b3DynamicTree_Query` traversal, and the prompt accounts for the step’s dynamic and kinematic tree rebuilds.
- The CCD fixture paths, sensor hit limit and begin-event path, callback gates, and per-worker array lifecycle match the cited source.
- The Explode query, falloff branch, wake behavior, and awake-set observation match the cited source.
- The test entry-name filter and CMake target names support the stated test commands.
- The benchmark profile and counter descriptions match their APIs.

## 4. Proposed edits

Revise §7 so the fail-before procedure reports a separate build result and an exit result for every test, failing verification if any expected failure is absent. Add revision-specific benchmark output paths and use those saved CSVs for the §7 comparison.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Both adopted.

1. **M, fail-before status conflates outcomes (adopted).** Confirmed: the function returned the same
   nonzero status for a failed build, for one failing test and for all failing tests, and its trailing
   comment called a failing test the expected result, so the status could not show that every named
   test failed. The function now prints a build line, then one exit line per test, and returns 0
   only when the build succeeded and every named test failed. The worktree is still removed on every
   path.
2. **L, benchmark csv overwritten (adopted).** Confirmed: `benchmark/main.c` writes `<scene>.csv`
   into its working directory with a fixed name per scene and reads no data file, so a second run
   from the same directory overwrites the first. §7 now runs each revision in its own scratch
   directory; the §2 benchmark fact names the output location.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 16: 2 findings (H0-M1-L1), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1. Series total 58 (H5-M40-L13). Not converged.

Tripwire. U-T1: no mechanism added or removed; a shell function restructured and a directory per
benchmark run. U-T2: both findings are in §7, which last produced a finding in round 14, not 15;
finding 1 is a consequence of the round 14 fix (the function's single failure status). Each fix is a
few lines of the command block. U-T3: 448 -> 456 lines; fix 1 added five lines, fix 2 three. U-T6:
over the split check; one decision, recorded in round 6.
