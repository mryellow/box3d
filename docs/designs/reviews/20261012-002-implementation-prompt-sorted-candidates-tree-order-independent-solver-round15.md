---
title: Review round 15 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round14.md
mode: full pass
---

# Round 15

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
2026-10-12T09:55:43+10:00 to 09:58:41+10:00. Same prompt as rounds 1 to 14 against revision 16. U-R9:
a read of the sections revision 16 changed found one overlong mid-paragraph line, reflowed (whitespace
only) before the run.

## Review report (Codex final message)

## 1. Summary

The implementation prompt is largely consistent with the canonical design, and its build commands and main test mechanisms match the live source. One callback fixture claim needs tightening before implementation.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §4 A1, `ContinuousCallbackOrderTest` | The prompt says the step’s custom-filter calls come from CCD alone because each sphere starts clear of other fat AABBs. That does not exclude pair-discovery calls between dynamic cluster boxes whose fat AABBs overlap (`src/broad_phase.c`). | Require dynamic cluster boxes’ fat AABBs to be mutually separate, or explicitly restrict each callback log to calls involving its corridor sphere. Correct the CCD-only claim. |

## 3. Checked, no change

- The design’s §5 provenance decisions are preserved.
- The CCD query order, sensor-hit path, explosion query and wake behavior match `src/solver.c`, `src/sensor.c`, `src/physics_world.c`, and `src/body.c`.
- The tree witness approach matches traversal and rebuild behavior in `src/dynamic_tree.c`.
- Test registration, CMake options, benchmark output, and the listed commands match the repository.

## 4. Proposed edits

Tighten the callback fixture and its CCD-only explanation as described in finding 1.

## 5. Unresolved

The specified random placements, fail-before counts, and benchmark deltas require the implementation to exist.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The finding was checked against the live tree (U-V2) and adopted.

1. **L, pair-discovery calls among dynamic cluster boxes (adopted).** Confirmed: `b3PairQueryCallback`
   calls the custom filter for any overlapping pair where either shape has the flag
   (`src/broad_phase.c`), and the dynamic corridor's cluster boxes carry it. Each callback log now
   keeps only calls that involve its corridor sphere, and the CCD-only claim is stated for those calls.

The raw log's hits on `designs/reviews` and `changelog` are a `git status` listing and the request's
own out-of-scope line; no read of round records or the changelog.

## Status

Round 15: 1 finding (H0-M0-L1), 1 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1. Series total 56 (H5-M39-L12). Not converged.

Tripwire. U-T1: no mechanism added or removed; a log filter. U-T2: the finding is a consequence of the
round 14 fix (the callback test widened to a dynamic corridor). The fix is a clause. U-T3: 446 lines
before the fix; the after count is recorded in the changelog. U-T6: over the split check; one
decision, recorded in round 6.
