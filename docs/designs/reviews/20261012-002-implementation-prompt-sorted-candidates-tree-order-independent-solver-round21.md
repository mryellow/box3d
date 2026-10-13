---
title: Review round 21 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round20.md
mode: full pass
---

# Round 21

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
2026-10-12T10:49:30+10:00 to 10:55:01+10:00. Same prompt as rounds 1 to 20 against revision 22. U-R9:
the target is untracked so there is no diff; the changed sections were read whole; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The prompt’s A0–A2 implementation sequence is consistent with the Canonical design, and the main witnesses match the relevant tree queries. Three prompt defects remain: an incomplete callback fixture, an incorrect AABB claim, and a benchmark reporting contradiction.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A1, `ContinuousCallbackOrderTest` | The fixture enables pre-solve events but never instructs the implementer to register its callback. A new world has a null `preSolveFcn`, and CCD calls it unconditionally when an enabled solid reaches the pre-solve gate (`src/physics_world.c`, `src/solver.c`). Followed literally, the test can crash before its required divergence check. | Explicitly register the pre-solve and custom-filter callbacks in both worlds before stepping, with separate logs for each run. |
| 2 | L | §2 | The claim that `b3Shape_GetAABB` returns a tight box is false. It returns `shape->aabb`, which is normally inflated by the speculative distance (`src/shape.c`). | Call it the speculative shape AABB; retain the specified `world->fatAABBs` check for proxy overlap. |
| 3 | L | §7 | The text permits reporting fewer than eight thread counts on a machine with fewer than eight cores, but the measurement list still requires counts “one to eight.” The benchmark clamps `-t=8` to the detected core count (`benchmark/main.c`). | Require counts from one through the shared maximum actually run, capped at eight. |

## 3. Checked, no change

- The A0 direct rebuild, tree selection, and visit-order witness agree with design §2 A0 and `src/dynamic_tree.c`.
- The A1 three separate candidate passes, per-worker array lifecycle, sensor limit, and callback gates agree with design §2 A1 and the live source.
- The sensor-event and Explode fixtures use observable results from the paths specified in design §2.
- The CMake targets and test-runner entry-name filter support the listed verification commands on a single-configuration build.

## 4. Proposed edits

Apply the three actions in the findings table. Keep the design’s §5 decisions unchanged.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All three findings checked against the live tree (U-V2). All adopted.

1. **M, pre-solve callback never registered (adopted).** Confirmed: CCD calls `world->preSolveFcn`
   with no null check when the flag is on either shape (`src/solver.c`), while pair discovery checks
   the custom filter for null (`src/broad_phase.c`). The callback test now registers both callbacks
   in both worlds before stepping, through `b3World_SetPreSolveCallback` and
   `b3World_SetCustomFilterCallback`, with a per-run log as context; §2 states the hazard.
2. **L, "tight box" is false (adopted).** Confirmed: `b3Shape_GetAABB` returns `shape->aabb`, which
   `b3UpdateShapeAABBs` computes with the speculative margin (`src/shape.c`). §2 now says so.
3. **L, "one to eight" contradicts the core-count sentence (adopted).** The measurement bullet now
   asks for counts from one to the maximum both runs share, at most eight.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 21: 3 findings (H0-M1-L2), 3 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0 -> 0/1/2. Series total
69 (H6-M48-L15). Not converged.

Tripwire. U-T1: no mechanism added or removed; a registration step the fixture already assumed.
U-T2: finding 2 is a consequence of the round 17 fix (the `b3Shape_GetAABB` aside); finding 3 is a
consequence of the round 13 fix (the core-count sentence) meeting an older bullet; finding 1 is new,
in the callback fixture, which produced findings in rounds 14, 15 and 17. §7 is cited for the third
round since the cut, each time for a different sentence; each fix is a clause. U-T3: 460 lines
before the fix; the after count is recorded in the changelog. U-T6: over the split check; one
decision, recorded in round 6.
