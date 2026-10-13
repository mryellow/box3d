---
title: Review round 9 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round8.md
mode: full pass
---

# Round 9

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
2026-10-12T09:07:08+10:00 to 09:10:20+10:00. Same prompt as rounds 1 to 8 against revision 9. U-R9: a
read of the sections revision 9 changed found no mangling.

## Review report (Codex final message)

## 1. Summary

The implementation prompt’s fixtures, witnesses, and test-to-requirement mapping are consistent with the binding design and the inspected source. I found one verification command that is unsafe on hosts with more than 32 online CPUs.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §7, benchmark command | `./bin/benchmark` defaults to the host’s online CPU count (`shared/utils.c`). In the requested Release build, a count above `B3_MAX_WORKERS` (32) can index past `minTime[B3_MAX_WORKERS]` because the guard in `benchmark/main.c` is an `assert` disabled in Release. | Run the benchmark with an explicit cap, such as `./bin/benchmark -t=8`, on both `main` and the branch. |

## 3. Checked, no change

- §4 A0’s direct tree rebuild and visit-order witness match the tree query and step ordering in `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/solver.c`.
- §4 A1’s candidate collection, per-query sorting, sensor limit, callback gates, and worker-context lifecycle match the cited source and design §2 A1 and §5.
- §4 A2’s query, wake, impulse, and lock requirements match `src/physics_world.c` and design §2 A2.
- The test registration, filtering, CMake targets, sensor events, and awake-set inspection paths exist as described.

## 4. Proposed edits

In §7, add `-t=8` to the benchmark invocation and specify the same thread cap for both measurements.

## 5. Unresolved

The fail-before counts and fixture outcomes require the proposed tests to be implemented and run.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The finding was checked against the live tree (U-V2) and adopted.

1. **M, benchmark thread count (adopted).** Confirmed: `benchmark/main.c` takes its default thread
   count from `GetNumberOfCores` (`shared/utils.c`), guards it only with `assert`, which `NDEBUG`
   removes in the Release build §7 asks for, and indexes a `B3_MAX_WORKERS` (32) array with it. A host
   with more than 32 online CPUs would write past the array. The §7 invocation now caps threads at
   eight on both `main` and the branch.

The raw log's hits on `designs/reviews` and `changelog` are a `git status` listing and the request's
own out-of-scope line; no read of round records or the changelog.

## Status

Round 9: 1 finding (H0-M1-L0), 1 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0. Series total 40
(H5-M26-L9). Not converged.

Tripwire. U-T1: no mechanism added or removed; one command flag. U-T2: the finding is in §7, which
last produced a finding in round 7; §4 A1 tests produced none for the first time in the series. The
fix is a flag. U-T3: 419 lines before the fix; the after count is recorded in the changelog. U-T6:
over the split check; one decision, recorded in round 6. Criteria review threshold (ten rounds
without converging): round 10 is next; if it does not converge, Part B is reconsidered per B8.
