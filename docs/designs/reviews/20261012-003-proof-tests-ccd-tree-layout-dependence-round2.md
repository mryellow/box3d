---
title: Review round 2 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round1.md
mode: full pass
---

# Round 2

## Exact request sent to Codex

```
Review the design document docs/designs/20261012-003-proof-tests-ccd-tree-layout-dependence.md as an adversarial design reviewer for the Box3D physics engine. The document states a proposition (continuous collision on main gives different answers for equal worlds whose broad-phase trees are laid out differently), what a proof of it has to show, the mechanism, how a user reaches different layouts through the public API, a public-API reproduction, and three proposed public-API proof tests with a set of pre-finalisation checks.

Scope: the document's own content and the source it cites (src/solver.c, src/distance.c, src/dynamic_tree.c, src/id_pool.c, src/body.c, src/physics_world.c, test/test_tree_order.c, include/box3d/). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: claims about the code that are false, a proof that could pass vacuously or fail for a reason other than layout, perturbations that change more than layout (ids, bounds, state), a witness that covers a wider or narrower candidate set than the continuous query, mechanism claims that do not follow from the source, statements that contradict each other across sections, and proposed tests that cannot prove or falsify the proposition as stated. Treat existing primitives (b3DynamicTree, b3TimeOfImpact, b3ShapeTimeOfImpact, the id pool) as settled tools; judge what the design adds and the question each primitive is asked.

Decisions recorded in the document's section 8 (Decisions) are settled Provenance and binding on you unless you show fresh source evidence that their reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether a coverage table is complete, other design documents, and decisions the document assigns to a later Implementation Prompt.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:15:48+10:00 to 15:18:03+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

## 1. Summary

The cited source supports the proposed collision mechanism and the move-away-and-back route to a different static-tree layout. Two proof gaps remain: destroy-and-recreate does not satisfy the proposition’s identical-ID condition, and two proposed tests omit witness checks that §1 requires.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§1, 3, 6, 8 | Destroy-and-recreate reuses indices but increments body and shape generations (`src/body.c`, `src/shape.c`). Generations are part of public IDs. Those runs therefore do not prove Proposition P as written; P2 has no move-away-and-back test to supply a strict-ID proof. | Keep both perturbations as decided in §8. Use move-away-and-back for each strict-P proof, including P2, and label destroy-and-recreate as evidence for an index-equivalent variant. |
| 2 | M | §§1, 6 | `ContinuousLayoutPreSolveTest` does not require a differing, same-set visit witness. `ContinuousLayoutSensorTest` requires differing order but does not require the same twelve candidates in both worlds. Either test could fail for a candidate-set change, or pass without demonstrating the required order change. | Assert the same complete candidate set and a differing order in each proof test; retain the non-empty call and eight-event checks. |

## 3. Checked, no change

- On `main`, continuous candidates are evaluated in tree visit order; the PR evaluates collected candidates in shape-ID order (`src/solver.c`, `src/dynamic_tree.c`).
- The running fraction reaches the TOI root bracket, and accepted hits update the continuous iteration maxima (`src/solver.c`, `src/distance.c`).
- The pre-solve gate and eight-slot sensor limit support the stated order-dependent behavior (`src/solver.c`).
- Moving a body away and back reinserts its proxy; the static moved mark is cleared before continuous collision (`src/body.c`, `src/dynamic_tree.c`, `src/broad_phase.c`, `src/physics_world.c`).
- The described corridor and sensor placements put their stated static shapes within the continuous query’s swept region (`test/test_tree_order.c`, `src/solver.c`).

## 4. Proposed edits

Revise §§1 and 5–6 to distinguish full public IDs from reused indices, add the move-away-and-back P2 case, and state the candidate-set and order assertions explicitly. In the P1 test description, name the three CCD iteration fields being compared so “counters” cannot be read as the whole `b3Counters` value, which also contains tree height (`src/physics_world.c`).

## 5. Unresolved

The reported reproduction counts and the planned double-precision and Release checks remain empirical claims to verify when the proposed tests exist.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (H) | Confirmed in part. Generations advance on reuse (`src/body.c`, `src/shape.c`), as §3 already says after round 1. What remains true: §6 does not say which perturbation the P2 test runs, and §5 measured P2 only under destroy-and-recreate, so the strict-id P2 proof has no stated test and no fail-before count; the "106 of 106" remark is a destroy-and-recreate observation and §5 does not say so. The §8 decision (both perturbations) is not re-opened: the finding keeps it. | Applied. §1 states which perturbation keeps public ids; §6 runs every test under both, move-away-and-back as the strict proof and destroy-and-recreate as the index-equivalent variant; the P2 move-away-and-back fail-before is recorded at first run (§7) because it was not measured; the "106 of 106" remark says destroy-and-recreate. |
| 2 (M) | Confirmed. Only P1 states the same-set witness requirement; P2 says "the same corridor" and P3 requires the witness to differ but not to hold the same twelve. | Applied as one lead sentence in §6 covering all three tests; P1's duplicate clause trimmed. |
| Edit 4 | Confirmed. `b3Counters` (`include/box3d/types.h`) holds `distanceIterations`, `pushBackIterations` and `rootIterations` besides tree heights and `contactCount`; §1 and §6 say "counters" for both meanings, and round 1's fix added a second use. | Applied: the three continuous fields are named once in §1; the pre-step check says contact count. |

Raw log: no read of round records or the changelog. The two untracked files appear only in a `git status` listing the reviewer ran.

## Status

2 rounds: 1/3/0 -> 1/1/0. Series total 6 (H2-M4-L0). Round 2: 2 findings (H1-M1-L0), 2 applied, 0 declined;
one further proposed edit (name the counter fields) applied. Document 237 -> 244 lines; fixes added
about 7 net lines (32 added, 21 removed across the round's edits). No named mechanism added or removed.
Finding 1 is in the same area as round 1 finding 1 (perturbation id equality) and is a consequence of
round 1's fix: the fix scoped the claim in §3 and §8 but left §6's P2 test and §5's "106 of 106"
unscoped. Finding 2 is in §6, which round 1's finding 3 also touched. No re-raised declined finding.
The raw log shows no read of round records or the changelog. Below the 400-line split-check length.
Not converged: round 3 at the same scope.
