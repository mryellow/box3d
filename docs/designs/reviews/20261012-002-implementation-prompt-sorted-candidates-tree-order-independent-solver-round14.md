---
title: Review round 14 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round13.md
mode: full pass
---

# Round 14

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
2026-10-12T09:44:56+10:00 to 09:47:13+10:00. Same prompt as rounds 1 to 13 against revision 15. U-R9:
a read of the paragraph revision 15 changed found one mid-paragraph line break, reflowed (whitespace
only) before the run.

## Review report (Codex final message)

## 1. Summary

The prompt’s A0–A2 code instructions largely match the canonical design and live source. Three defects remain in how it requires and reports verification: the CCD corridor’s fail-before claim is not guaranteed, the mapping overstates what two layouts prove, and the fail-before shell function hides test failures.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A0, §4 A1 | The prompt says two on-path boxes make a corridor’s **state hash** order-sensitive and requires a pre-change divergence for every tree kind. A changed TOI limit can change work without changing the earliest hit or the hashed body state (`src/solver.c`, `src/distance.c`). The specified geometry therefore does not establish the required fail-before result; the instruction to keep changing placement may block an otherwise valid implementation. | Require witnessed order and equal post-change hashes. Use an observable callback-order check for each tree kind when a guaranteed pre-change failure is needed; record state-hash divergences when found, without making them mandatory for every corridor. |
| 2 | M | §6 | The mapping says `ContinuousCandidateOrderTest` establishes equal results for **any layout**, but it compares two layouts per seed. Those runs provide regression evidence, not the general claim. | Map the tests to the witnessed layouts, and add code inspection of the sort-and-evaluate pass after each tree query as evidence for the layout-independent ordering rule. |
| 3 | L | §7 | `fail_before()` prints each test’s exit code, then returns the status of `git worktree remove`. It can report shell success after a test failure. A build failure also returns before removing the worktree, preventing a clean retry at the same path. | Collect test statuses, return failure when any test fails, and arrange cleanup on both build and test failure. |

## 3. Checked, no change

- The design’s §5 decisions are preserved: separate ascending-id passes for the three CCD queries, current sensor admission and reporting, and no public-query guarantee.
- The proposed per-worker array lifecycle matches `src/physics_world.h`, `src/physics_world.c`, `src/container.h`, and the `src/broad_phase.c` sort idiom.
- The witness timing accounts for the dynamic and kinematic tree rebuild before CCD; the Explode query box and lock instructions match the source.
- The sensor-event and awake-set APIs can observe the outcomes named by their fixtures. The CMake targets, test-name filter, and benchmark command match the repository.

## 4. Proposed edits

Revise the corridor fail-before requirement and its §6 row to distinguish a changed visit order from a changed body state. Narrow the “any layout” evidence row and state the code invariant it relies on. Replace the §7 function with one that preserves failing statuses and always removes its scratch worktree.

## 5. Unresolved

The proposed corridor placements and benchmark measurements require execution; this was a source and document review.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All three findings checked (U-V2). All adopted.

1. **M, corridor hash divergence not guaranteed (adopted).** Confirmed by reasoning the design already
   records (§1: the fraction can differ when the running limit differs; it is not required to). A
   second on-path box makes divergence possible, not certain, so a mandatory nonzero pre-change count
   per corridor kind could send the implementer into an unbounded placement loop. The guaranteed
   fail-before evidence per tree kind is the callback order: the custom filter is called for every
   CCD candidate of every tree query in visit order before the change and in id order after it. The
   callback test now runs one corridor of each kind. Corridor hash divergence counts are recorded,
   not required; the field's count keeps the design's own rule.
2. **M, "any layout" overstated (adopted).** Confirmed: each seed compares two layouts. The mapping
   row now names the witnessed layouts as the evidence and adds inspection of the sort-and-evaluate
   pass after each tree query as the evidence for the general rule.
3. **L, shell function status (adopted).** Confirmed: the function returned `git worktree remove`'s
   status and skipped removal after a build failure. It now records every status, removes the
   worktree on every path, and returns failure when any step or test failed.

The raw log's one "changelog" hit is the request's own out-of-scope line; no read of round records or
the changelog.

## Status

Round 14: 3 findings (H0-M2-L1), 3 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1. Series total 55 (H5-M39-L11). Not converged.

Tripwire. U-T1: the fixes remove a requirement (mandatory per-corridor divergence) and widen one test
(callback test over three corridor kinds); no mechanism added. U-T2: finding 1 is a consequence of the
round 7 fix that made the corridor hash the mandatory assertion; finding 3 of the round 13 fix; finding
2 is in §6, its first finding. Each fix is a clause or a command edit. U-T3: 438 lines before the
fixes; the after count is recorded in the changelog. U-T6: over the split check; one decision,
recorded in round 6.
