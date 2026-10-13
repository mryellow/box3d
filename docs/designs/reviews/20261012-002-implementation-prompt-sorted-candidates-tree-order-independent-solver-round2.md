---
title: Review round 2 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round1.md
mode: full pass
---

# Round 2

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
2026-10-12T07:39:47+10:00 to 07:42:44+10:00. Same prompt as round 1 against revision 2. The target is
untracked, so U-R9 was a full read of the revised sections; no mangling.

## Review report (Codex final message)

## 1. Summary

The prompt follows the canonical design’s A0–A2 sequence, and most of its source and build claims match the repository. Several test instructions need correction before an implementer can rely on the promised fail-before evidence, especially the sensor and explosion fixtures.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 A1; §6 | A different visit sequence does not prove the sensor test fails before A1. A swap among the first eight sensors, or among the last four, leaves the *set* of eight reported sensors unchanged. | Require the pre-change runs to retain different eight-sensor sets. After A1, assert the expected eight lowest shape IDs, as required by design §5. |
| 2 | M | §4 A1 | The test is told to use `B2_MAX_CONTINUOUS_SENSOR_HITS`, but that macro exists only in `src/solver.c`; `test/test_tree_order.c` cannot see it. | Use a test-local constant of 8, with a comment identifying the limit, or assert the literal value. |
| 3 | M | §4 A1 | `b3World_GetSensorEvents` returns public `b3ShapeId` handles containing a world index and generation (`include/box3d/id.h`). Comparing full handles from two fresh worlds can reject equivalent sensor selections. | Map event IDs to fixture ordinals, or compare their shape indices after verifying each belongs to its expected world. |
| 4 | M | §4 A2 | The fixture says nonzero `falloff` exercises the gradient branch, but `src/physics_world.c` enters it only when shape distance is **greater than** `radius`; the specified shapes are inside the radius. The fixture also omits an explicit nonzero `impulsePerArea`, which its velocity assertion needs. | Set a nonzero impulse explicitly. Remove the gradient-branch claim, or add a separate shape in the falloff region. |
| 5 | M | §4 A1 docs | The proposed wording says `preSolve` runs once per candidate below the running fraction. `src/solver.c` calls it only for an enabled, accepted solid candidate with a fraction strictly between zero and the running fraction. | State those conditions in the replacement sentence. |
| 6 | L | §7 | The scratch harness describes `b3World_GetCounters` iteration values as per-step measurements. `src/solver.c` retains worker maxima across steps, and `src/physics_world.c` returns their maxima; later readings are high-water marks. | Label them as high-water marks, or use temporary instrumentation if per-step iteration values are needed. |
| 7 | L | §1; §8 | The prompt starts implementation before requiring the approved issue. `CONTRIBUTING.md` requires an issue or discussion labeled `ready-for-pr` **before writing code**, while §8 leaves the issue number for the owner to supply. | Put the approved issue prerequisite at the start of the task and require the PR to satisfy the repository’s PR template. |

## 3. Checked, no change

- The three tree queries, running fraction, sensor limit, and sensor-hit path match `src/solver.c` and `src/sensor.c`.
- Direct full rebuilds, query traversal, and the pre-CCD dynamic and kinematic rebuild match `src/dynamic_tree.c` and `src/broad_phase.c`. The specified helper call sites are between world operations, when direct tree access is safe.
- Explosion locking, wake order, and awake-set storage match `src/physics_world.c`, `src/body.c`, and `src/solver_set.c`.
- The test target, entry-name filter, benchmark output columns, and `-s` files match the CMake files, `test/`, and `benchmark/`. These commands were inspected, not executed.

## 4. Proposed edits

Revise the sensor fixture to specify a pre-change difference in the *retained* eight, normalize event IDs across worlds, and use a test-local limit. Revise the explosion fixture to set a nonzero impulse and correct its falloff claim. Tighten the pre-solve wording, label iteration readings as high-water marks, and add the approved-issue prerequisite.

## 5. Unresolved

The revised sensor placement still needs a fail-before run to establish that the retained sets differ. The approved issue number must come from the owner.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live tree (U-V2). All adopted.

1. **H, sensor fixture's fail-before evidence (adopted).** Confirmed by reasoning over `src/solver.c`:
   with twelve admitted sensors the retained eight are the first eight visited, so a visit-order
   change confined to the first eight, or to the last four, leaves the reported set unchanged. The
   fixture now requires the pre-change runs to report different sets, and after the change asserts
   the eight lowest shape ids, which is what the design's §5 sensor-selection decision gives with no
   solid in the sweep.
2. **M, macro visibility (adopted).** Confirmed: `B2_MAX_CONTINUOUS_SENSOR_HITS` is defined in
   `src/solver.c` only; no header carries it. The test uses a local constant of 8.
3. **M, shape ids across worlds (adopted).** Confirmed: `b3ShapeId` carries `world0`
   (`include/box3d/id.h`), and the two runs use two worlds. The test compares `index1`.
4. **M, explosion fixture (adopted).** Confirmed: `ExplosionCallback` scales the impulse only when
   the distance exceeds `radius` with nonzero `falloff` (`src/physics_world.c`), and
   `b3DefaultExplosionDef` zero-initialises `impulsePerArea` (`src/joint.c`). The fixture sets a
   nonzero impulse and places one single-shape body in the falloff band so both branches run.
5. **M, pre-solve wording (adopted).** Confirmed from `src/solver.c`: the call needs pre-solve events
   enabled on either shape, a solid candidate, and a fraction strictly between zero and the running
   fraction. The replacement wording states those conditions.
6. **L, iteration counters (adopted).** Confirmed: `b3TaskContext` iteration fields are only ever
   raised (`src/solver.c`) and `b3World_GetCounters` takes their maximum (`src/physics_world.c`);
   nothing resets them. Labelled high-water marks.
7. **L, approved issue first (adopted).** Confirmed: `CONTRIBUTING.md` requires the `ready-for-pr`
   label before code is written, and `.github/pull_request_template.md` exists. The prerequisite now
   opens §1 and §8 requires the template be filled.

The one raw-log hit on `designs/reviews` is a `git status` listing, not a read.

## Status

Round 2: 7 findings (H1-M4-L2), 7 applied, 0 declined. Findings: 1/4/1 -> 1/4/2. Series total 13
(H2-M8-L3). Not converged.

Tripwire. U-T1: no mechanism added or removed; the fixes change fixture contents, assertions and
wording. U-T2: findings cluster in §4 A1 tests, which produced findings last round; none is a
consequence of a round 1 fix (round 1 changed the fuzz and the entry functions; this round's findings
are in the sensor and Explode fixtures and the docs sentence, untouched last round). The raw log
shows no read of round records or the changelog. U-T3: 308 lines before the fixes; the after count is
recorded in the changelog. U-T6: under the 400-line split check.
