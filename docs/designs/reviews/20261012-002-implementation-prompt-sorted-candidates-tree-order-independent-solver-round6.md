---
title: Review round 6 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round5.md
mode: full pass
---

# Round 6

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
2026-10-12T08:43:48+10:00 to 08:48:00+10:00. Same prompt as rounds 1 to 5 against revision 6. U-R9: a
read of the sections revision 6 changed found no mangling.

## Review report (Codex final message)

## 1. Summary

The implementation prompt largely follows the canonical design, and its source and build descriptions are mostly accurate. Two A1 fixtures weaken the evidence it promises: the seeded fuzz count can reflect the witness corridor, and the specified four-worker scene does not dispatch CCD work to multiple workers.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1; §6 | `ContinuousCandidateOrderTest` hashes the witness corridor as part of every seeded scene. Its fast sphere hits a box, so a fail-before divergence can come from that corridor rather than the randomized field. The reported “diverging seed count” therefore does not establish the seeded-fuzz result required by design §2 A1. | Run the witness in a separate scene, or make its sweep miss the boxes and keep it out of the fuzz hash. Count divergences from the randomized field alone. |
| 2 | M | §4 A1; §6 | Eight launched bodies plus the corridor body yield at most nine awake dynamic bodies. `src/solver.c` dispatches finalization in blocks of 16 and bullet CCD in blocks of 8; four bullets also fit in one block. Thus this fixture cannot exercise candidate arrays concurrently across workers. Also, `b3World_SetWorkerCount` in `src/physics_world.c` recreates contexts but does not turn a serial world into a threaded one. | Create four-worker worlds through `b3WorldDef`, increase the fast-body workload beyond both dispatch thresholds, and verify that multiple worker contexts actually process CCD candidates; otherwise narrow the mapping claim. |

## 3. Checked, no change

- The A0 rebuild and witness mechanism matches design §2 A0 and `src/dynamic_tree.c`; the specified CCD witness uses the static tree, whose order remains available at that step.
- The A1 sort boundaries, sensor cap, callback gates, and iteration-counter description match `src/solver.c` and design §5.
- The A2 collection, lock scope, wake-order observation, and velocity checks are supported by `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- The test entry filter and build targets match `test/main.c`, `test/CMakeLists.txt`, and `CMakeLists.txt`; the benchmark output and `-s` description match `benchmark/main.c`.

## 4. Proposed edits

Revise the A1 fuzz fixture and its §6 mapping to attribute fail-before seed divergences to the randomized field. Revise the four-worker fixture and mapping to require observed CCD work on more than one worker.

## 5. Unresolved

The proposed fixtures were not built or run in this read-only review, so their actual witness permutations and fail-before counts remain to be measured.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Both adopted.

1. **M, corridor in the fuzz hash (adopted).** Confirmed from the prompt's own fixture: the corridor
   sphere hits a box and is hashed with the field, so a pre-change divergence may come from the
   corridor and the recorded count would not be the design's seeded-fuzz number. The fuzz now hashes
   the field's fast bodies and the corridor sphere separately, requires both to match, and reports the
   field's diverging seed count as the design's number with the corridor's beside it.
2. **M, four-worker run does not parallelise CCD (adopted).** Confirmed: `b3FinalizeBodiesTask` runs
   under `b3ParallelFor` with a minimum range of 16 awake bodies and `b3BulletBodyTask` with a minimum
   range of 8 bullets (`src/solver.c`, `src/parallel_for.c`), so nine awake bodies form one block on
   one worker. Also confirmed: `b3World_SetWorkerCount` recreates the worker contexts but creates no
   scheduler; a world gets the built-in scheduler only from `b3WorldDef.workerCount` above one at
   creation (`src/physics_world.c`). The four-worker rerun now creates its worlds with a worker count
   of four in the definition and uses forty fast bodies, half bullets, so both stages span several
   blocks; the mapping row is narrowed to that.

The raw log's five hits on `designs/reviews` and two on `changelog` are a `git status` listing and the
request's own out-of-scope line; no read of round records or the changelog.

## Status

Round 6: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0. Series total 33 (H5-M20-L8). Not converged.

Tripwire. U-T1: no mechanism added or removed; the fuzz computes two hashes instead of one and the
four-worker rerun's workload grows. U-T2: both findings are in §4 A1 tests, the sixth round running;
both are consequences of earlier fixes (the round 4 corridor and the round 5 four-worker rerun). Each
fix is a clause. The count fell from seven to two and the severity ceiling from H to M. U-T3: 392
lines before the fixes; the after count is recorded in the changelog. U-T6: the document crosses the
400-line split check with this revision. It decides one thing, how the design lands and is shown to
work; the length is instructions and fixtures, not a second decision. No split.
