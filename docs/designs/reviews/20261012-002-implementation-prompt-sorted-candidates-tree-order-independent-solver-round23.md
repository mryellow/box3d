---
title: Review round 23 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round22.md
mode: full pass
---

# Round 23

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
2026-10-12T11:02:25+10:00 to 11:05:25+10:00. Same prompt as rounds 1 to 22 against revision 24. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The implementation instructions largely match the canonical design and the live source. One test fixture overstates what its witness proves, so its claimed fail-before evidence for `preSolve` order is not guaranteed.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1; §6 | `ContinuousCallbackOrderTest` requires a changed visit sequence among “on-path” boxes, then claims the `preSolve` log must differ before A1. In `src/solver.c`, `preSolve` runs only when TOI is strictly between zero and the running fraction. The reordered boxes could include a zero-fraction or non-hit candidate, leaving the accepted `preSolve` sequence unchanged even while every stated witness check passes. | Require the reordered subsequence to contain at least two candidates that actually reach `preSolve`, and check that their pre-A1 log order differs for each tree kind. Update the §6 mapping to cite that check. |

## 3. Checked, no change

- A0’s direct rebuild, lock and recording restrictions match `src/dynamic_tree.c` and `src/physics_world.c`.
- The CCD query order, candidate filters, sensor limit, worker context lifecycle, and explosion wake path match the cited source.
- The sensor event and awake-set APIs can observe the outcomes assigned to them.
- The CMake targets, executable paths, test-name filter, and benchmark CSV behavior match the repository. I checked the commands against configuration and source; I did not run builds.

## 4. Proposed edits

In §4 A1, define the `preSolve` witness using candidates confirmed to produce `0 < TOI < 1` with the callback returning false. Require their visit order and pre-A1 callback order to differ in each corridor. Reflect that evidence in §6.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The finding was checked against the live tree (U-V2) and adopted.

1. **M, "on the sphere's path" is not the pre-solve condition (adopted).** Confirmed: CCD issues the
   pre-solve call only for a fraction strictly between zero and the running fraction, and a false
   return leaves the running fraction where it was (`src/solver.c`), so with the fixture's callback
   the condition is a fraction strictly between zero and one. The corridor contract now defines an
   on-path box as one whose time-of-impact call against the full sweep returns such a fraction; the
   callback test requires the pre-solve log to hold exactly the on-path boxes' indices, so the
   witness restricted to the on-path boxes is the pre-solve order before the change; the §6 row
   says so.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 23: 1 finding (H0-M1-L0), 1 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2 -> 1/1/0 ->
0/1/0. Series total 72 (H7-M50-L15). Not converged.

Tripwire. U-T1: no mechanism added or removed; a definition made precise. U-T2: the finding is in the
callback fixture, which produced findings in rounds 14, 15, 17 and 21; it is not a consequence of
the previous round's fix. The fix is a clause in two places. U-T3: 469 lines before the fix; the
after count is recorded in the changelog. U-T6: over the split check; one decision, recorded in
round 6.
