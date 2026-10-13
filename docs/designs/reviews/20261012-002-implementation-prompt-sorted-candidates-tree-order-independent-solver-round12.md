---
title: Review round 12 - implementation prompt, sorted-candidate tree-order-independent solver changes
target: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-002-implementation-prompt-sorted-candidates-tree-order-independent-solver-round11.md
mode: full pass
---

# Round 12

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
2026-10-12T09:33:25+10:00 to 09:37:46+10:00. Same prompt as rounds 1 to 11 against revision 13. U-R9:
a read of the sections revision 13 changed found one mid-paragraph line break in the corridor
contract, reflowed (whitespace only) before the run.

## Review report (Codex final message)

## 1. Summary

The implementation prompt largely matches the canonical design (§2, §5) and the live code. Two gaps need correction: the Explode fixture omits a required radius setting, and the fuzz test claims witnessed coverage for steps where it records no witness.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 A2 | The Explode fixture says to set `impulsePerArea` and `falloff`, but does not say to set `radius`. `b3DefaultExplosionDef()` leaves radius at zero (`src/joint.c`), contradicting the fixture’s three shapes inside the radius and its intended unscaled impulse branch (`src/physics_world.c`). | Explicitly require a positive radius and place the shapes relative to that value. |
| 2 | M | §4 A1; §6 | `ContinuousCandidateOrderTest` witnesses each corridor only at the first step but attributes every step’s corridor hash to a witnessed query. Dynamic and kinematic tree contents and bounds can change across steps (`src/broad_phase.c`, `src/solver.c`, `src/dynamic_tree.c`), so later hash comparisons may have no differing visit order. This falls short of the design’s per-query witness check (§2 A0). | Record and check a witness for each step claimed as witnessed, or label later steps as unwitnessed fuzz evidence in §4 and §6. |

## 3. Checked, no change

- The CCD and Explode callback behavior, sort locations, worker arrays, locking, and sensor-hit path match the prompt (`src/solver.c`, `src/physics_world.c`, `src/sensor.c`).
- The one-worker and four-worker block thresholds, test runner filters, CMake targets, and benchmark `-t=8` option match the repository.
- The sensor fixture’s lowest-eight assertion follows the settled selection order for its single fast shape and static-tree query (design §5).
- The Explode awake-set and velocity assertions use observable state (`src/body.c`, `src/solver_set.c`, `src/physics_world.c`).

## 4. Proposed edits

Set `explosionDef.radius` explicitly in §4 A2. Revise §4 A1 and §6 so each hash attributed to a witnessed corridor has a corresponding differing visit-order witness.

## 5. Unresolved

The tuned fixtures’ fail-before divergence and benchmark deltas require execution; this was a read-only review.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Both findings checked against the live tree (U-V2). Both adopted.

1. **M, radius unset (adopted).** Confirmed: `b3DefaultExplosionDef` zero-initialises every field but
   `maskBits` (`src/joint.c`), and the fixture named only `impulsePerArea` and `falloff`. The fixture
   now sets a positive `radius` and places its bodies relative to it.
2. **M, corridor hash beyond the witnessed step (adopted).** Confirmed from the fixture: each corridor
   is witnessed at the first step, after which its sphere has hit a box and later corridor state comes
   from contacts, not from a witnessed query. The corridor hash is now compared after the corridor
   step only; from the second step the corridor bodies join the field hash as unwitnessed fuzz.

The raw log's hits on `designs/reviews` and `changelog` are a `git status` listing and the request's
own out-of-scope line; no read of round records or the changelog.

## Status

Round 12: 2 findings (H0-M2-L0), 2 applied, 0 declined. Findings:
1/4/1 -> 1/4/2 -> 1/3/1 -> 1/4/2 -> 1/3/2 -> 0/2/0 -> 0/3/0 -> 0/2/1 -> 0/1/0 -> 0/4/0 -> 0/3/1 ->
0/2/0. Series total 50 (H5-M35-L10). Not converged.

Tripwire. U-T1: no mechanism added or removed; one field set and one comparison narrowed. U-T2:
finding 1 is a consequence of the round 10 fix (starting from the default definition); finding 2 is a
consequence of the round 6 split hash. Both are in sections that produced findings last round. Each
fix is a clause. U-T3: 432 lines before the fixes; the after count is recorded in the changelog.
U-T6: over the split check; one decision, recorded in round 6.
