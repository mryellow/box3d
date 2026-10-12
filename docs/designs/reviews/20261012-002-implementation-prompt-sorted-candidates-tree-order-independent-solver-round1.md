---
title: Review round 1 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

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
2026-10-12T07:29:49+10:00 to 07:32:09+10:00. The target was untracked at the time, so U-R9 was a full
read of the file rather than a diff; no mangling.

## Review report (Codex final message)

## 1. Summary

The Implementation Prompt follows the canonical design’s A0–A2 sequence, but two evidence requirements cannot be met as written: the fuzz witness can differ without witnessing an actual CCD query, and the prescribed test command cannot show both A1 tests failing before the change. The benchmark instructions also request measurements the existing executable does not produce.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 A1; §6 | `ContinuousCandidateOrderFuzz` witnesses a query box covering the entire static field. A changed order among those 40 shapes does not establish a changed order among candidates in any actual swept CCD query (`src/solver.c`, `src/dynamic_tree.c`). The §6 “query a test relies on” mapping can therefore pass vacuously. | Require a witness over a proven common candidate set for a named CCD query, and compare that set and its order between runs. |
| 2 | M | §3; §4 A1; §7–§8 | Both A1 tests live under `TreeOrderTest`, but the runner filters only by entry function and `RUN_SUBTEST` returns on the first failure (`test/main.c`, `test/test_macros.h`). The given command cannot capture separate fail-before outputs for both tests when the first fails. | Specify separate runnable entries or an explicit temporary isolation procedure for each pre-change run. |
| 3 | M | §7 | The benchmark executable writes total `threads,ms` results; it does not print the requested `step`, `transforms`, and `bullets` columns, and its profile reduction does not retain `bullets` (`benchmark/main.c`). | Specify temporary profile instrumentation or a separate measurement harness and the exact invocation. |
| 4 | M | §7 | Timing the “sort-and-evaluate loop against the unsorted loop” measures collection and evaluation overhead as well as sorting. It cannot support a reported *sort cost*. | Time `QSORT` separately, or label the result as total candidate-ordering overhead. |
| 5 | M | §7 | `cmake --preset linux-debug && cmake --build build` leaves samples enabled. A fresh test build consequently enters the samples’ external downloads and system-library requirements (`CMakeLists.txt`, `samples/CMakeLists.txt`), which are unrelated to suite verification. | Configure the test build with `-DBOX3D_SAMPLES=OFF` and build the `test` target explicitly. |
| 6 | L | §2; §4 A1 | The prompt calls the `RandomState` idiom in `test/test_broad_phase.c` an LCG; that implementation is xorshift32. | Name it accurately, or specify a new local LCG without attributing it to that file. |

## 3. Checked, no change

- The A0 direct-rebuild mechanism, tree access, query callback data, and Explode query box agree with `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/physics_world.c`.
- CCD’s eight-hit limit, candidate filters, running-fraction behavior, sensor-event path, and per-worker array sites agree with `src/solver.c`, `src/sensor.c`, and `src/physics_world.c`.
- The Explode fixture can observe velocity and awake-set order through the named APIs and internal structures (`src/body.c`, `src/solver_set.c`, `src/physics_world.h`).
- The sensor fixture’s end position avoids discrete final-overlap events; its eight-event assertion matches the intended CCD hit path (`src/solver.c`, `src/sensor.c`).
- The A2 pre-change stash procedure preserves A1 once A1 is committed, as §3 requires.

## 4. Proposed edits

Revise the fuzz witness and its §6 mapping to identify an actual CCD candidate set. Add a way to run each fail-before test independently. Give executable instructions for collecting profile stages and isolating sort time, use a test-only CMake configuration, and correct the PRNG description.

## 5. Unresolved

The required witness frequency and pre-change failure counts need measurement once the fixtures are implemented. No canonical design decision needs reopening.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live tree (U-V2).

1. **H, fuzz witness (adopted, different fix).** Confirmed: a covering-box query gives the tree's
   leaf order restricted to the box (`src/dynamic_tree.c`), so a different order among all 40 field
   shapes does not show a different order among the candidates of any actual swept query. The fix is
   not the reviewer's "witness over a proven common candidate set": the fuzz's swept boxes are not
   reproducible from outside the step without re-deriving solver arithmetic. Instead the fuzz's
   non-vacuity evidence is the count of seeds that diverge before the change, which the design's §2 A1
   already requires the PR to record, and the fuzz is changed to count diverging seeds rather than
   stop at the first. A seed that diverges before the change is a CCD query whose candidate order the
   perturbation changed. The sanity and sensor fixtures keep a direct witness, and the prompt now
   states why theirs is exact: every static shape in those scenes lies inside the swept AABB by
   construction, so the covering box's candidate set is the query's candidate set.
2. **M, one entry function (adopted).** Confirmed: `RUN_SUBTEST` returns at the first failing subtest
   (`test/test_macros.h`) and the runner filters by entry name only (`test/main.c`). The new file now
   exposes four entry functions, one per test, registered in `test/main.c` in the commit that adds
   each test; `test/main.c` joins the A1 and A2 file lists for that reason.
3. **M, benchmark output (adopted).** Confirmed: the executable prints per-run milliseconds and writes
   a `threads,ms` csv per scene; with `-s` it writes per-step rows of step, pairs, collide,
   constraints, transforms, refit and sleepIslands, and never the `bullets` stage
   (`benchmark/main.c`). The measurement section now names `-s` and the `transforms` column for the
   suite, and a scratch harness printing `b3World_GetProfile` for `bullets`.
4. **M, sort cost (adopted).** The timed region is now the `QSORT` call alone.
5. **M, build command (adopted).** Confirmed: `BOX3D_SAMPLES` defaults on at top level
   (`CMakeLists.txt`), and the samples fetch imgui, implot and nfd and require OpenGL and X11
   (`samples/CMakeLists.txt`). Verified live in a scratch build directory:
   `cmake -S . -B <dir> -DCMAKE_BUILD_TYPE=Debug -DBOX3D_SAMPLES=OFF`, then
   `cmake --build <dir> --target test`, then `<dir>/bin/test TableTest` configured, built and passed.
   The benchmark target is `benchmark` (`benchmark/CMakeLists.txt`).
6. **L, PRNG name (adopted).** Confirmed: `NextRandom` in `test/test_broad_phase.c` is xorshift32.
   Wording corrected.

Unresolved item (witness frequency and pre-change failure counts): measurement at implementation time,
as the reviewer says; not a change to the document.

## Status

Round 1: 6 findings (H1-M4-L1), 6 applied, 0 declined. Findings: 1/4/1. Series total 6 (H1-M4-L1).
Not converged.

Tripwire. U-T1: the fixes remove one mechanism (the fuzz's whole-field witness) and add three
test-side entry functions. U-T2: first round, no clustering to compare; the raw log shows no read
of round records or the changelog (the only "changelog" hits are the request's own out-of-scope line
and a `git status` listing). U-T3: 284 lines before the fixes; the after count is recorded in the
changelog. U-T6: under the 400-line split check; the document decides one thing.
