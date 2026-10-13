---
title: Review round 3 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round2.md
mode: full pass
---

# Round 3

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
2026-10-12T07:46:39+10:00 to 07:51:07+10:00. Same prompt as rounds 1 and 2 against revision 3. U-R9: a
read of the sections revision 3 changed found one mid-paragraph line break in the Explode test,
reflowed (whitespace only) before the run.

## Review report (Codex final message)

## 1. Summary

The prompt identifies the right implementation sites, and its sensor and explosion assertions are observable through the APIs it names. I found five defects in the required evidence and landing workflow; the most consequential are the missing direct witness for the CCD fuzz test and an unsafe fail-before command.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 A1; §6 | The fuzz test treats a pre-change hash divergence as proof that a CCD query visited the same candidates in a different order. That is an inference, not the visit-order witness required by design §2 A0 for the query a test relies on. | Require a test-side witness for a specified CCD query, checking equal candidate sets and different sequences. Keep the divergence count as separate fail-before evidence. |
| 2 | M | §3; §7 | The `git stash` command chain can run a test against a stale binary if the build fails, hides a failing test behind the final `git stash pop` exit status, and can pop an unrelated stash if no new stash was made. Its output is unreliable fail-before evidence. | Specify an isolated worktree or a script that checks stash creation, records each build and test status, and restores only the stash it created. |
| 3 | M | §5 | The policy permits changing an affected scene’s hash but forbids “any other expected value.” `test/test_determinism.c` also pins sleep steps and query values. A legitimate CCD result change can change a sleep step or a later query result, leaving the implementer unable to satisfy both the design’s behaviour allowance and this policy. | Permit causally explained expected-value changes in affected CCD scenes, with old and new values reported; stop on unexplained changes. |
| 4 | M | §3; §8 | §3 forbids touching any file outside its list except the golden file, while §8 requires writing a new closure document. Writing it is therefore simultaneously required and a stop-worthy deviation. | Explicitly exempt the §8 closure document from the file limit. |
| 5 | L | §8 | The PR template asks whether the contributor **ran all samples** (`.github/pull_request_template.md`); §8 treats a build with `BOX3D_SAMPLES` enabled as sufficient for that box. A successful build does not establish that claim. | Require the sample run if the box is checked, or report build-only verification accurately and leave the box unchecked. |

## 3. Checked, no change

- The candidate collection and sorting sites match the current CCD queries in `src/solver.c`, and the per-worker array lifecycle and memory-statistics sites exist in `src/physics_world.h` and `src/physics_world.c`.
- The single-step sensor test can read CCD hits as begin events: `src/solver.c` transfers hits before `src/physics_world.c` runs the sensor overlap pass in `src/sensor.c`.
- The explosion query box, default explosion values, wake behaviour, test entry-name filtering, and CMake test and benchmark targets match the prompt’s source claims.

## 4. Proposed edits

Revise §§4 and 6 to require the fuzz witness; replace the §7 stash line with a status-preserving procedure; broaden §5’s causal golden policy; exempt the closure file in §3; and align §8’s samples instruction with the PR template.

## 5. Unresolved

The specified fixture placements and fail-before seed count remain to be measured when the tests are implemented. This review was source-based; it did not run an implementation branch.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live tree (U-V2). All adopted.

1. **H, fuzz witness (adopted; re-raised from round 1, re-verified per U-V7).** The design's §2 A0
   "Check before merging" requires a visit-order witness for the query a test relies on, and the
   design is binding on this document. Round 1 replaced the fuzz's proxy witness with an inference
   from pre-change divergence; the reviewer is right that an inference is not the required witness.
   A direct, exact witness is possible, and the source supports it: `b3FinalizeBodiesTask` leaves a
   fast body's shape AABB untouched and calls `b3SolveContinuous`, which reads that stored AABB as the
   start box, computes the end box at the integrated transform, queries the union, and when nothing
   is hit stores the end box back into the shape (`src/solver.c`); `b3Shape_GetAABB` returns the
   stored AABB (`src/shape.c`). So for a fast body that never impacts, the union of its shape AABB
   read before and after a step is bit for bit the swept box its static-tree query used. A body
   whose shape filter has `maskBits` zero never impacts: `b3ShouldShapesCollide` rejects it in CCD,
   in pair discovery and in sensor overlap (`src/shape.h`, `src/solver.c`, `src/broad_phase.c`,
   `src/sensor.c`), while the tree query still returns candidates against `B3_DEFAULT_MASK_BITS`.
   Each fuzz seed gets such a witness body, and the test requires equal candidate sets every step and
   a differing sequence in at least one step per seed. The divergence count stays as separate
   fail-before evidence.
2. **M, stash chain (adopted).** Confirmed: a failed build would leave the previous binary in place,
   the chain's exit status is `git stash pop`'s, and `git stash pop` with nothing stashed pops an
   unrelated entry. Replaced by a scratch `git worktree` at the last commit with the working tree's
   three test files copied in, each build and test status printed, the worktree removed after.
3. **M, golden policy (adopted).** Confirmed: `test/test_determinism.c` pins sleep steps, a hit count
   and a query hash as well as hashes. The policy now covers every expected value in a scene with CCD
   over several candidates, each change reported with its cause; an untraceable change stops.
4. **M, closure doc versus file limit (adopted).** The closure doc is exempted from §3's file limit.
5. **L, samples box (adopted).** The box is ticked only after running the samples app; otherwise left
   unticked with a sentence saying so.

The raw log's one "changelog" hit is the request's own out-of-scope line; no read of round records or
the changelog.

## Status

Round 3: 5 findings (H1-M3-L1), 5 applied, 0 declined. Findings: 1/4/1 -> 1/4/2 -> 1/3/1. Series
total 18 (H3-M11-L4). Not converged.

Tripwire. U-T1: the fixes add one test-side mechanism (the fuzz's filtered witness body and the
after-step hook it needs) and remove one procedure (the stash chain). U-T2: findings landed in §4 A1
tests for the third round running, and finding 1 is a consequence of the round 1 fix that replaced the
whole-field witness with an inference. U-S4 is therefore triggered: this round's fix is the cut (the
inference is deleted and the design's witness requirement is met directly); if round 4 lands in §4 A1
tests again, pause and report to the owner before fixing. The raw log shows no read of round records
or the changelog. U-T3: 331 lines before the fixes; the after count is recorded in the changelog.
U-T6: under the 400-line split check; one decision.
