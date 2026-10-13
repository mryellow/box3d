---
title: Review round 11 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round10.md
mode: full pass
---

# Round 11

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
2026-10-12T09:27:01+10:00 to 09:30:06+10:00. Same prompt as rounds 1 to 10 against revision 12, the
owner's cut of fixture detail to contracts after the round 10 threshold report. U-R9: the splice
boundaries of the rewritten sections were checked before the run; no mangling.

## Review report (Codex final message)

## 1. Summary

The implementation mechanics largely match the canonical design and live source. The prompt needs changes to its A0 fixture instructions and witness checks before its test-to-requirement mapping can support the claims it makes.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A0 | The sanity test is specified as “zero gravity, no sleep,” but case 3 requires gravity and stepping until the pile sleeps. Those instructions cannot both apply to that case. | State the world settings separately for cases 1–2 and case 3. |
| 2 | M | §§4 A0–A1, 6 | A test-side visit-order query can differ even if the intended CCD pass never runs. The sanity and fuzz corridor checks do not assert that their sphere was classified fast; the negative-control pile likewise does not establish its claimed absence of CCD. `src/solver.c` invokes CCD conditionally during finalize. | Assert CCD eligibility for each witnessed corridor and the absence of fast bodies in the negative control, then adjust fixtures if either check fails. |
| 3 | M | §§4 A2, 6 | The Explode witness requires only a different *shape* sequence. Reordering the three shapes of one body can satisfy it while distinct bodies still wake in the same order, leaving the wake-order part of design §2 A2 unwitnessed. `src/body.c` and `src/solver_set.c` wake and append whole solver sets. | Also require a different sequence of first visits to distinct in-range bodies before the change; retain the shape-order and velocity checks. |
| 4 | L | §7 | `benchmark -t=8` does not guarantee the requested CSV rows for thread counts 1–8: `benchmark/main.c` caps `-t` at the detected core count. | Require a machine with at least eight detected cores, or report and compare the actual common thread-count range. |

## 3. Checked, no change

- A1’s collect, sort, evaluate, and clear sequence preserves the three separate CCD tree-query passes in `src/solver.c` and follows the design’s §5 decisions.
- The sensor fixture can observe admitted CCD hits through `b3World_GetSensorEvents`; `src/solver.c` and `src/sensor.c` support the stated path.
- The Explode query box, default explosion mask, worker-context lifecycle, counter high-water behavior, test entry-name filtering, and principal CMake commands match the cited source.
- Direct tree rebuilding between calls avoids a recording command; the prompt states the relevant lock and recording constraints.

## 4. Proposed edits

Resolve finding 1 by making the sanity test’s settings case-specific. Add explicit fixture assertions for finding 2 and a distinct-body wake-order witness for finding 3. Qualify the benchmark thread range as described in finding 4.

## 5. Unresolved

None.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

All four findings checked against the live tree (U-V2). All adopted.

1. **M, sanity settings contradict case 3 (adopted).** Confirmed from the document: revision 12 moved
   "zero gravity, no sleep" into the sanity test's header, and case 3 needs gravity and sleep. The
   settings are now per case.
2. **M, CCD eligibility unasserted (adopted).** Confirmed: finalize marks a body fast and runs CCD
   only when its motion exceeds the safety factor times its minimum extent (`src/solver.c`), so a
   witness can differ while no CCD ran. No public accessor exposes the classification
   (`include/box3d/box3d.h`); the flag is `b3_isFast` on the body sim, cleared and re-set by each
   step's finalize, reachable through `b3GetBodyFullId` and `b3GetBodySim` (`src/body.h`). Each
   corridor now asserts its sphere's flag set after the corridor step, and the negative control
   asserts it clear on every body after every step.
3. **M, Explode wake order unwitnessed (adopted).** Confirmed: `b3WakeBody` wakes whole solver sets
   (`src/body.c`, `src/solver_set.c`), so a reordering among one body's three shapes leaves the wake
   order unchanged. The witness now also requires a different sequence of first visits to distinct
   bodies, mapping shape ids to bodies through the world's shape array.
4. **L, thread range capped by core count (adopted).** Confirmed: `benchmark/main.c` clamps `-t` to
   the detected core count. §7 now requires at least eight detected cores or reports the common range.

The raw log's one "changelog" hit is the request's own out-of-scope line; no read of round records or
the changelog.

## Status

Round 11: 4 findings (H0-M3-L1), 4 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1.
Series total 48 (H5-M33-L10). Not converged.

Tripwire. U-T1: no mechanism added or removed; three assertions and a settings split. U-T2: finding 1
is a consequence of the revision 12 cut; findings 2 and 3 are on fixture surface the cut exposed as
contracts; finding 4 is in §7. Each fix is a clause. U-T3: 420 lines before the fixes (434 before the
cut); the after count is recorded in the changelog. U-T6: over the split check; one decision,
recorded in round 6. B8: the owner chose to cut detail rather than reconsider Part B; recorded in the
round 10 record and the changelog.
