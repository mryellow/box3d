---
title: Review round 22 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round21.md
mode: full pass
---

# Round 22

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
2026-10-12T10:55:56+10:00 to 11:00:43+10:00. Same prompt as rounds 1 to 21 against revision 23. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

1. Summary

The implementation prompt has a blocking kinematic corridor fixture error and a false claim about rebuild timing. The canonical design’s ordering choices remain settled; these findings concern how the prompt tries to witness and prove them.

2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 A0 `CreateCorridor`; §4 A1 tests | The kinematic cluster is created before step one, while the sphere inserted afterward is **dynamic**. No kinematic proxy is inserted or moved between the first step and the corridor step. The specified perturbation therefore cannot provide the claimed changed kinematic candidate order by adjusting placement alone; the A0 witness and A1 per-tree fail-before check can halt on setup. See `src/solver.c`, `src/broad_phase.c`, `src/dynamic_tree.c`. | Create at least one kinematic corridor candidate in the first step’s after-step hook, before run two’s next perturbation. Require the kinematic witness to differ at the actual CCD query. |
| 2 | M | §2 rebuild reconciliation | The claim that a full rebuild “marks no internal node” and makes the step’s rebuild do nothing is false for newly inserted proxies. A full rebuild retains their moved marks (canonical design §2 A0); rebuilt internal nodes inherit those marks, so the next step can rebuild again. See `src/dynamic_tree.c`, `src/dynamic_tree.h`, `src/broad_phase.c`. | Correct the timing explanation and base the fixture’s order claim on its recorded pre-CCD witness, including the step’s possible rebuild. |

3. Checked, no change

- The three CCD tree queries, their order, callback gates, sensor limit and sensor-event path match `src/solver.c` and `src/sensor.c`.
- The Explode query box, nonzero-scale gate, wake behavior and velocity access support the A2 fixture in `src/physics_world.c`, `src/body.c` and `src/solver_set.c`.
- The worker-context array lifecycle, sort primitive, test entry-name filtering, CMake target names and benchmark CSV behavior match the cited source. The verification commands were checked against the repository configuration, not executed.
- The prompt respects the canonical design §5 sensor-selection and reporting decisions.

4. Proposed edits

Revise the corridor creation schedule so a kinematic proxy enters its tree after step one and before the second perturbation. Replace §2’s “build marks no internal node” explanation with the moved-mark behavior above, then update the A0 and A1 witness instructions to check the layout that CCD actually queries.

5. Unresolved

The revised fixture’s differing witness and fail-before result require the implementation prompt’s prescribed runs; this review did not execute an implementation.

6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Both adopted; both are consequences of the
revision 21 fix.

1. **H, kinematic corridor gets no insertion (adopted).** Confirmed: the kinematic corridor's sphere
   is a bullet on a dynamic body, so the revision 21 rule inserted a proxy into the dynamic tree and
   left the kinematic tree as the first step settled it. The corridor contract now creates each
   cluster in two parts in the cluster's own tree: a part with the scene, settled by the first step,
   and a part with the sphere in the after-step hook of the step before the corridor step. The
   sanity test and the fuzz say the same.
2. **M, "the build marks no internal node" is false (adopted).** Confirmed: `b3MakeInternalNode` ORs
   the children's moved marks into the parent, on insertion and on build (`src/dynamic_tree.c`), so
   after a full rebuild the ancestors of a marked leaf are marked and the next step's rebuild
   descends them. The round 19 verification note stated the opposite from an incomplete read; the
   record stands as written (U-R1) and this record corrects it. The §2 fact now states the mark
   inheritance and the condition that makes the two runs differ: the tree holds, at the
   perturbation, unmarked subtrees settled by an earlier step and proxies inserted since; the step's
   rebuild keeps the settled subtrees whole and the perturbation dissolved them. The witness
   instructions already record the layout after the corridor step, which is the layout CCD queried.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 22: 2 findings (H1-M1-L0), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0.
Series total 71 (H7-M49-L15). Not converged.

Tripwire. U-T1: no mechanism added or removed; a creation schedule stated for every corridor kind.
U-T2: both findings are consequences of the revision 21 fix to the corridor contract and the §2
rebuild fact; the corridor contract has produced findings in rounds 14, 15, 17, 19 and 22. U-S1: H
findings in rounds 19 and 22 are not three consecutive rounds. The fixes are a clause each plus a
restated §2 fact. U-T3: 465 lines before the fix; the after count is recorded in the changelog.
U-T6: over the split check; one decision, recorded in round 6.
