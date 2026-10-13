---
title: Review round 4 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round3.md
mode: full pass
---

# Round 4

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
2026-10-12T07:56:46+10:00 to 07:59:01+10:00. Same prompt as rounds 1 to 3 against revision 4. U-R9: a
read of the sections revision 4 changed found no mangling.

## Review report (Codex final message)

## 1. Summary

The Implementation Prompt is close to the canonical design, and its main code and build descriptions match the live repository. Its A1 evidence can still pass without witnessing reordered collision candidates, and several instructions overstate coverage or leave the implementer with conflicting release steps.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 A1; §6 | The fuzz witness observes a separate body whose `maskBits` are zero. Its reordered CCD query cannot show that a collision query affecting the compared hashes was reordered. The §6 row therefore does not establish the design’s §2 A0 witness condition for the A1 result. | Require a differing visit order for at least one collision producing CCD query in the fuzz fixture, with its candidate set verified against that query. |
| 2 | M | §4 A1; §6 | Neither A1 test installs a pre-solve or custom-filter callback. Hash and sensor-event equality cannot establish the design’s §2 A1 callback order and selection claims. | Add argument-only callbacks that log the candidate shape ids and compare their within-query sequences across layouts. |
| 3 | M | §4 A1 Docs | The requested replacement text mentions worker interleaving but omits the design’s §2 A1 caveat that custom filtering also runs during pair discovery and sensor overlap in tree order. | Require that caveat in `docs/simulation.md` as well as the PR text. |
| 4 | M | §1; §8 | §1 permits a labelled discussion alone, while §8 requires `Closes #<issue>`. `CONTRIBUTING.md` says the required PR check expects a linked, approved issue. A discussion-only start cannot satisfy the stated PR instruction. | Require an approved issue number before the PR, or specify the repository-accepted way to link an approved discussion. |
| 5 | L | §2; §7 | The iteration counters are described as high-water marks “since world creation.” `b3World_SetWorkerCount` destroys and recreates the task contexts in `src/physics_world.c`, resetting them. | Say “since the current worker contexts were created,” and keep the measurement harness’s worker count fixed. |
| 6 | M | §6 | The “nothing else changes” row claims `test_determinism.c` runs at all worker counts. Its CCD-focused mesh-drop case runs at worker counts 1 and 4 only; other cases also use selected ranges. | State the actual coverage, or add the worker counts the row claims. |
| 7 | L | §3; §8 | The closure doc is written after the three commits, but its inclusion in the PR is left to the owner. Committing it then would require an unspecified amend or a fourth commit, conflicting with the exact three-commit instruction. | Specify that it stays outside the PR unless the owner directs an amendment to A2. |

## 3. Checked, no change

- The proposed A1 and A2 collection, sort, and evaluation sites match `src/solver.c`, `src/physics_world.c`, `src/physics_world.h`, `src/qsort.h`, and `src/container.h`.
- The A0, sensor, and Explode fixtures can witness their stated tree queries; `src/dynamic_tree.c` and `src/sensor.c` support the described visit-order and same-step sensor-event mechanisms.
- The test entry-name filtering, CMake target paths, and benchmark output descriptions match `test/`, `CMakeLists.txt`, and `benchmark/`.

## 4. Proposed edits

Revise the A1 fixtures and mapping for findings 1, 2, and 6. Correct the docs, approval prerequisite, counter wording, and closure-doc instruction as shown in findings 3–5 and 7.

## 5. Unresolved

The required differing orders and fail-before counts remain empirical checks for the implementing agent; this was a read-only review.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live tree (U-V2). No fix is folded in this round: U-S4 is triggered
(below), so the resolution waits for the owner.

1. **H, fuzz witness body (confirmed).** The witness body's filter mask is zero, so its query never
   produces a collision; a reordering among its candidates does not show that any collision-producing
   query was reordered. The design's §2 A0 check is for the query a test relies on. Confirmed by
   reading the fixture against `src/solver.c`. Two resolutions exist and they fork (U-S3):
   (a) read the design's A1 fuzz paragraph as naming its own evidence, the pre-change failing seed
   count, and record that reading as a settled decision in the prompt; or (b) give each seed a
   constructed witness corridor: one bullet whose swept AABB contains a cluster of solid boxes and
   nothing from the random field, so the candidate set is known by construction, the witness is exact,
   and the query produces the collision the hash depends on. (b) stays within `test/` and within the
   design's words ("a field of static boxes with several overlapping candidates"); it replaces the
   filtered witness body.
2. **M, no callback test (confirmed).** The design's §2 A1 states that `preSolve` and the custom filter
   are called in id order within each query; no fixture observes either. An argument-only logging
   callback in a constructed single-step scene (bullet clear of every fat AABB at step start, so the
   step's custom-filter calls are CCD's alone) would map that clause. Call sites confirmed in
   `src/solver.c`, `src/broad_phase.c`, `src/sensor.c`.
3. **M, docs caveat (confirmed).** The design's §2 A1 asks that the stateful-callback caveat, including
   the custom filter running in pair discovery and sensor overlap in tree order, go into
   `docs/simulation.md`; the prompt's replacement text names only worker interleaving.
4. **M, issue versus discussion (confirmed).** `CONTRIBUTING.md` lets the discussion start with an
   issue or a Discussion, but the PR check requires a linked, approved issue and `Closes #N`. §1 must
   require an approved issue number.
5. **L, counters wording (confirmed).** `b3World_SetWorkerCount` destroys and recreates the worker
   contexts (`src/physics_world.c`), resetting the counters. "Since world creation" is wrong.
6. **M, worker counts (confirmed).** `test/test_determinism.c` runs its scenes at one to five, one to
   four and one to four workers; "all worker counts" overstates it.
7. **L, closure doc and the three commits (confirmed).** Committing the closure doc after the three
   commits would need a fourth commit or an amend, which §4 does not allow. It stays outside the PR.

Findings 3 to 7 have one fix each and would be adopted as written. Findings 1 and 2 are the fourth
round running in §4 A1 tests.

## Status

Round 4: 7 findings (H1-M4-L2), 0 applied, 0 declined, 7 pending the owner. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2. Series total 25 (H4-M15-L6). Not converged.

Tripwire. U-T2: findings in §4 A1 tests for the fourth round running, after the round 3 cut; finding 1
is a consequence of the round 3 fix (the filtered witness body). U-S4: paused; reported to the owner
with the fork of finding 1 and a recommendation for (b). No doc edit this round. The raw log shows no
read of round records or the changelog. U-T3: 362 lines, unchanged this round. U-T6: under the
400-line split check; one decision.

Resolution of this round is written as revision 5 once the owner decides; this record then closes.

## Resolution (owner decision, same day)

Owner chose (b). All seven findings adopted in revision 5: finding 1 by a constructed witness corridor
per fuzz seed, replacing the filtered witness body and the after-step hook that existed only for it;
finding 2 by a new deterministic test, `ContinuousCallbackOrderTest`, on a single corridor with
argument-only logging callbacks; findings 3 to 7 as written. Round 4: 7 applied, 0 declined. This
record is closed.
