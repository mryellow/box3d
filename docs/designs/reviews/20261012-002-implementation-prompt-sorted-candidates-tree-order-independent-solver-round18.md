---
title: Review round 18 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round17.md
mode: full pass
---

# Round 18

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
2026-10-12T10:16:36+10:00 to 10:20:07+10:00. Same prompt as rounds 1 to 17 against revision 19. U-R9:
the target is untracked so there is no diff; the sections revision 19 changed were read whole and the
§7 block was parsed with `bash -n`; no reflow needed.

## Review report (Codex final message)

## 1. Summary

The implementation prompt largely matches the binding design and the live code. Three evidence gaps remain: the fail-before script can count a crash as a successful regression test, and the multi-worker and explosion fixtures do not verify the conditions their mapping claims.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §7 | `fail_before` accepts **any nonzero exit**. An assertion failure or crash could therefore be recorded as proof that a test detected the old behavior. `test/main.c` returns 1 for a reported test failure. | Capture output and require exit 1, the named `test failed` line, and the expected failed assertion or count. |
| 2 | M | §§4 A1, 6 | “25 bullets” does not establish four bullet CCD blocks. `src/solver.c` queues only bullets marked `b3_isFast`; `src/parallel_for.c` splits the resulting queue into blocks of eight. The fixture checks fast status for corridor spheres, not for 25 queued bullets. | At the measured step, verify at least 25 bullets are fast and eligible for CCD; qualify the mapping as four available blocks, since scheduling can still assign them to fewer workers. |
| 3 | M | §§4 A2, 6 | The Explode test witnesses a changed **first body visit** order, but does not verify that those bodies occupy distinct sleeping solver sets. “None touching” does not establish that condition when speculative contacts may link islands (`src/physics_world.c`, `src/island.c`). Velocity divergence alone could make the test fail before A2 without exercising wake-order divergence. | Assert distinct sleeping set indices before the explosion and record a differing awake-set order in the fail-before run. |

## 3. Checked, no change

- The A0 direct rebuild, witness query, and post-step tree-structure reasoning match `src/dynamic_tree.c`, `src/broad_phase.c`, and `src/solver.c`.
- The three CCD query passes, eight-hit sensor limit, callback gates, and sensor event path match `src/solver.c` and `src/sensor.c`.
- The Explode query box, lock lifetime, wake path, and impulse accumulation match `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- The listed CMake targets, test entry filtering, benchmark CSV behavior, and counter semantics match the cited repository files.

## 4. Proposed edits

Tighten the §7 fail-before success condition, add an eligible-bullet check to the §4 A1 fixture and §6 mapping, and add sleeping-set and pre-change awake-order checks to §4 A2.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

All three findings checked against the live tree (U-V2). All adopted; the third in part.

1. **M, any nonzero exit counted as fail-before (adopted).** Confirmed: `RUN_TEST` prints
   `test failed:` and returns 1 for a reported failure (`test/test_macros.h`, `test/main.c`); a crash
   or assertion abort exits with another code. The function now counts an exit of exactly 1 as the
   expected result. The closure doc already quotes each run's output, so no further pattern check is
   added.
2. **M, 25 bullets do not establish four bullet blocks (adopted).** Confirmed: finalize queues a bullet
   for the bullet stage only when it flags it fast (`src/solver.c`), and the stage splits that queue in
   blocks of eight (`src/parallel_for.c`). The four-worker runs now require `b3_isFast` on at least 25
   bullets at the first step, in the after-step hook; the §6 row claims at least four blocks available
   per stage, which worker drains which being the scheduler's choice as §4 already says.
3. **M, "none touching" does not establish separate sleeping sets (adopted in part).** Confirmed:
   a contact links islands when it begins touching (`src/physics_world.c`, `src/island.c`), and
   touching means the manifold has a point, which for convex shapes other than sphere pairs includes
   separations up to `B3_SPECULATIVE_DISTANCE` (`src/convex_manifold.c`). The fixture now separates
   every two bodies by more than that distance and asserts, before the explosion, that each body's
   `setIndex` is a distinct sleeping set. The awake-set sequence check now runs before the velocity
   checks, so the fail-before run fails on wake order; the awake-set order is thereby what the closure
   doc's quoted output shows. No separate "record a differing order" step is added: the test's first
   failing check is the record.

The raw log has no hit on `designs/reviews` or `changelog` beyond the request's own out-of-scope
line; no read of round records or the changelog.

## Status

Round 18: 3 findings (H0-M3-L0), 3 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0 -> 0/2/0 -> 0/2/1 -> 0/0/1 -> 0/1/1 -> 0/2/0 -> 0/3/0. Series total 63 (H5-M45-L13). Not
converged.

Tripwire. U-T1: no mechanism added or removed; two assertions of constructed preconditions and one
exit-code comparison. U-T2: finding 1 is in §7 and is a consequence of the round 16 fix (its
any-nonzero rule); §7 has now drawn a finding in rounds 16, 17 and 18, three rounds running, so
U-S4 applies: the §7 fix is cut to one token (`-eq 0` becomes `-ne 1`), and a §7 finding in round
19 is a pause-and-report, not another patch. Findings 2 and 3 are in §4 subsections (the fuzz
four-worker run, the Explode fixture) that had not produced findings before; each fix is a clause.
U-T3: 466 lines before the fix; the after count is recorded in the changelog. U-T6: over the split
check; one decision, recorded in round 6.
