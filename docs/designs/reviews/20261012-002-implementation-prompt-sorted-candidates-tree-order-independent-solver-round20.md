---
title: Review round 20 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round19.md
mode: full pass
---

# Round 20

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
2026-10-12T10:45:25+10:00 to 10:48:37+10:00. Same prompt as rounds 1 to 19 against revision 21, the
revision that folds the owner's decisions on the round 19 pause (U-S5): scene preparation inside this
document for the vacuous witnesses, and withdrawal of the §7 shell functions in favour of stated
requirements. U-R9: the target is untracked so there is no diff; the changed sections were read whole;
no reflow needed.

## Review report (Codex final message)

## 1. Summary

The implementation instructions, fixtures, and source claims are largely consistent with the canonical design. One verification gap remains: the prescribed test build checks only one of the two golden-hash configurations in `test/test_determinism.c`.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5, §7 | `test/test_determinism.c` has separate expected hashes and sleep steps under `BOX3D_DOUBLE_PRECISION`, but §7 runs only the default build. An A1 change can therefore leave the double-precision goldens stale while the required suite passes. | Add a separate build and full test run with `-DBOX3D_DOUBLE_PRECISION=ON`; apply §5’s cause-tracing and golden-update policy to both configurations. |

## 3. Checked, no change

- The A0 rebuild and visit-order witness match `src/dynamic_tree.c` and `src/broad_phase.c`.
- The A1 query order, sensor limit and reporting path, callback gates, worker arrays, and iteration-counter behavior match the cited source.
- The A2 collection, wake, impulse, and lock requirements match `src/physics_world.c`.
- The test entry-name filter, build targets and output paths, and benchmark CSV behavior match the repository.

## 4. Proposed edits

Add double-precision configure, build, and full-suite commands to §7. State in §5 and §6 that golden verification covers both precision configurations.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The finding was checked against the live tree (U-V2) and adopted.

1. **M, double-precision goldens unchecked (adopted).** Confirmed: `test/test_determinism.c` holds a
   second set of expected hashes under `BOX3D_DOUBLE_PRECISION`, a `CMakeLists.txt` option that is off
   by default, so the §7 build exercised one set only. §7 now configures, builds and runs the suite a
   second time with the option on; §3, §5 and the §6 row state that both configurations are run and
   that the golden policy applies to each.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 20: 1 finding (H0-M1-L0), 1 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0 -> 1/1/0 -> 0/1/0. Series total 66
(H6-M47-L13). Not converged.

Tripwire. U-T1: no mechanism added or removed; a second build configuration in the command list.
U-T2: the finding cites §5 and §7; it is not a consequence of a previous fix and not about the
withdrawn shell block, which the round 19 pause cut (U-S4); it is the first finding on the golden
policy's coverage. Recorded against U-S4 as a §7 citation after the cut; proceeding because the
finding is a missing configuration, not a recurrence of the block's defects. The fix is three
command lines and three clauses. U-T3: 453 lines before the fix; the after count is recorded in the
changelog. U-T6: over the split check; one decision, recorded in round 6.
