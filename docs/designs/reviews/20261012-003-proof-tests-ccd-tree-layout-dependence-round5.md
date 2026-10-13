---
title: Review round 5 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round4.md
mode: full pass
---

# Round 5

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:41:47+10:00 to 15:45:06+10:00. The request is the round 1 request unchanged (U-P4); its clause that the decisions in section 8 are settled Provenance covers the owner's replaced decision.

## Review report (Codex final message)

## 1. Summary

The source supports the proposed CCD mechanism and the strict public API perturbation: it can change static tree visit order while leaving the two worlds with equal ids and bounds. The public proof tests are well directed, but the document overstates what one existing test proves and leaves part of its own evidence requirement unspecified.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 | `ContinuousCandidateOrderTest` does not prove P1, even statistically. It compares a hash containing transforms, velocities, awake state, and awake count; its perturbation rebuilds all three trees. A differing hash does not establish that the fast body’s transform or a CCD counter differed. | Describe it as a world-state divergence test. Attribute P1 specifically to `ContinuousLayoutImpactTest` and its separate transform and counter comparisons. |
| 2 | L | §§1, 6 | §1 requires a proof test to print both results before asserting equality. §6 specifies counts and one position pair for P1, but does not specify paired counter values, pre-solve logs, or sensor sets for the other comparisons. An implementation following only those test descriptions could fail without displaying the evidence the document requires. | Require each test to print the paired values for its first divergence in each observable, including the visit-order witness. |

## 3. Checked, no change

- `src/solver.c`, `src/distance.c`, and `src/shape.c` support the running-fraction, TOI-bracket, counter, pre-solve, and eight-sensor mechanisms described in §2.
- `src/body.c`, `src/shape.c`, `src/dynamic_tree.c`, and `src/broad_phase.c` support the strict move cycle’s equal final bounds and moved marks, with reinsertion order affecting layout.
- `src/physics_world.c` and `src/dynamic_tree.c` support the static-tree AABB visit-order witness for the specified corridor.
- `src/id_pool.c`, `src/body.c`, and `src/shape.c` support §8’s decision to exclude destroy-and-recreate: reused indices receive advanced generations.
- The public declarations in `include/box3d/` support the proposed public API route; `b3World_RebuildStaticTree` is internal.

## 4. Proposed edits

Correct the §4 table entry and make the paired diagnostic output explicit in each §6 test description. Keep the settled §8 decisions.

## 5. Unresolved

The reported scratch-program counts and the planned double-precision and Release results cannot be established from the cited source alone; the proposed fail-before runs remain the evidence for those numbers.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (M) | Confirmed. `ContinuousCandidateOrderTest` hashes transforms, velocities, awake flags and the awake count of the corridor bodies (`HashWorldState`, `test/test_tree_order.c`), and its perturbation rebuilds the static, kinematic and dynamic trees. §4 lists it as proving P1. It shows that world state diverges under a full rebuild, not that the sphere transform or a counter differs. | Applied: the §4 entry says what it measures. |
| 2 (L) | Confirmed. §1 requires both results printed; §6 states the printing for P1 only, and only as counts and one position pair. | Applied: one printing requirement in the §6 lead covering all three tests; P1's own print sentence removed. |

Raw log: no read of round records or the changelog; the files appear only in a `git status` listing.

## Status

5 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1. Series total 15 (H4-M8-L3). Round 5: 2 findings (H0-M1-L1), 2 applied, 0 declined. Document 249 -> 249 lines; fixes added about 3 lines and removed about 3. No named mechanism added or removed. Findings are in §4 and §6; §6 produced findings in each earlier round (§6 now five rounds running, with fixes of a clause each after the owner's rewrite, no new machinery). Neither finding is a consequence of the previous round's fix: the owner's edit was the previous revision. Raw log shows no read of round records or the changelog. Below the 400-line split-check length. Not converged: round 6 at the same scope.
