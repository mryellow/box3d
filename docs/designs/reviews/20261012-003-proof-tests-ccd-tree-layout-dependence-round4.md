---
title: Review round 4 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round3.md
mode: full pass
---

# Round 4

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:26:46+10:00 to 15:30:07+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

## 1. Summary

The source supports the proposed mechanism: on `main`, continuous candidates are evaluated in tree visit order, and that order can affect TOI fractions, callbacks, counters, and sensor selection. The proposed tests still need stronger proof conditions to establish the stated layout-only proposition; the reported move-away results used a perturbation that also changed bounds.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §1, §5–7 | The only reported move-away run gives world B one cycle, which §3 correctly says changes proxy bounds. §7 asks to *record* strict two-cycle results, but §6 allows a test covering both perturbations to fail on `main` solely because destroy-and-recreate diverges. That would leave the layout-only proposition unproved. | Require a separate, nonzero fail-before divergence for the strict move-away case for each claimed observable, with its own differing witness and passing result on the PR. |
| 2 | M | §6 | The accepting `preSolve` pass compares call **sequences**. Different sequences can contain the same set, so its failure would not prove P2’s claimed set difference. | Compare and report normalized call sets separately; require a strict-layout fail-before set difference. |
| 3 | M | §6 | P1 groups transform and velocity divergence into one count. A failure from velocity or counters alone would not establish the claimed transform difference; a transform-only failure would not establish the counters claim. | Count and show transform, velocity, and each counter separately, and require fail-before evidence for the claimed transform and counters. |
| 4 | L | §1, §3, §8 | “Equal public ids” is literally false for two live worlds: body and shape handles contain `world0` (`include/box3d/id.h`). Destroy-and-recreate also advances generations, so equal indices do not settle the equal-world objection. | Define cross-world identity as matching indices and generations after ignoring the world selector. Describe destroy-and-recreate only as index-equivalent evidence. |

## 3. Checked, no change

- `main` evaluates CCD candidates inside the tree callback; the PR sorts candidate shape ids per tree query (`src/solver.c`, `src/dynamic_tree.c`).
- The running fraction bounds convex TOI’s root search, and accepted solids update the iteration maxima (`src/solver.c`, `src/distance.c`, `src/shape.c`).
- The described `preSolve` gating and eight-slot sensor behavior follow from `src/solver.c`.
- Creation and transform moves use different static proxy margins; moved static marks are cleared during pair update before CCD (`src/shape.c`, `src/body.c`, `src/broad_phase.c`).
- The public overlap query visits the static tree first and can witness its traversal order for the specified corridor (`src/physics_world.c`, `include/box3d/`).

## 4. Proposed edits

Make strict-layout fail-before results an explicit acceptance condition in §6–7. Split the P1 measurements and P2 sequence and set comparisons, and define cross-world ID normalization in §1.

## 5. Unresolved

The document does not yet report fail-before results for the strict two-cycle perturbation. The §5 numerical results come from a scratch program that is not among the cited source, so this review cannot independently verify those counts.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (H) | Confirmed. §6 has each test run both perturbations and assert equality in both, so a fail-before on `main` can come from destroy-and-recreate alone; no clause requires a divergence under the strict move-away form, and §5 holds no strict-form count. The strict form is the one the proposition P is stated for. | Carried forward, not folded: see the pause below. |
| 2 (M) | Confirmed. §6 asserts equal call sequences for the accepting pass and quotes 70 of 200, which §5 counts as differing call sets; a sequence failure does not show a set difference. | Carried forward. |
| 3 (M) | Confirmed in part. §6 groups transform and velocities into one count and gives counters their own; P1 names the transform and the counters as the claimed observables, so transform and each counter need their own counts. | Carried forward. |
| 4 (L) | Confirmed. `include/box3d/id.h` puts `world0` in body and shape ids, so "equal public ids" across two worlds needs a stated normalisation (ignore the world selector). | Carried forward. |

Raw log: no read of round records or the changelog; the three files appear only in a `git status` listing.

## Pause (U-S4, U-S1)

U-S4: round 3 cut §6 first; round 4 again has findings in §1 and §6 (findings 1 to 3), the fourth round running in these sections. The rule says pause and report if it recurs after a cut. U-S1: four consecutive rounds each found an H. No fix is folded and no round 5 is sent until the owner decides.

What the findings share: the proposition P, the strict form of the perturbation and the assertions of §6 are not yet one consistent statement. The open choice is U-S3-shaped. Option A: P is stated and proved only for the strict move-away-and-back form (two worlds with equal bounds and public ids apart from the world selector); destroy-and-recreate becomes a reachability remark, not an asserted proof; each observable needs its own strict fail-before, measured before the document is final. Option B: P is stated for "equal indices and bounds" and both perturbations stay as proofs; each observable needs a fail-before under each perturbation. A changes §1, §6 and §8 (a settled decision); B keeps §8 and weakens P.

## Owner decision and resolution

Owner: Option A. The §8 "both perturbations" decision is reversed (U-S5); the reason is restated in the
revision's §8 (U-C4).

Before folding, the strict form was measured (`proof.c`, `main` at `5d83df8` and the PR head at
`f66ee7f`, Debug float, 200 seeds). The two-cycle form of revision 4 proves P1 (transform 37,
counters 19/19/20) and P2 (order 114, set 75) but not P3: two cycles on one body reach the same
layout (witness identical), and on twelve bodies the layout differs but the eight reported are the
same set. A second strict form, one cycle per body in both worlds with world B in descending body
order, proves all five: witness differs 200 of 200, transform 107, each counter 67, rejecting order
200, accepting set 188 and sequence 188, sensors (one body per sensor) `3 5 6 7 8 9 10 11` against
`3 4 6 7 8 9 10 11`; velocities 0 of 200; no divergence in a seed without a differing witness. PR
head: every count zero, the witness counts unchanged. (A first PR-head run reproduced `main`'s counts
exactly; its library had been built with `main`'s `src/` during the recipe check of §6 and not
rebuilt. Rebuilt and re-run before anything was recorded.) The revision adopts the descending-order
form as the strict form.

| # | Resolution |
|---|---|
| 1 (H) | Applied. §1 states P for the strict form only; §3 defines it (one cycle per body in both worlds, world B in descending order) and moves destroy-and-recreate to a reachability row; §5 reports the strict counts for every observable; §6 uses one perturbation; §8 records the reversal and its reason. |
| 2 (M) | Applied. §5 and §6 count and require the accepting pass's call sets and call sequences separately. |
| 3 (M) | Applied. §1 names the transform and each of the three counters as separate observables and drops velocities; §5 and §6 count each separately. |
| 4 (L) | Applied. §1 defines cross-world id equality as equal index and generation with the world selector ignored; §3 says destroy-and-recreate is not a proof perturbation for that reason. |

## Status

4 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1. Series total 13 (H4-M7-L2). Round 4: 4 findings (H1-M2-L1),
4 applied after the owner's decision, 0 declined. Document 244 -> 249 lines; §5 to §8 rewritten for
the single strict perturbation, §1 and §3 corrected. Named mechanism removed: destroy-and-recreate as
a proof perturbation and the two-cycle strict form; added: the descending-order strict form and the
one-body-per-sensor P3 scene. Findings clustered in §1, §6 (fourth round) and §3; findings 1 and 3
are consequences of earlier fixes (round 3's strict form; round 2's per-test requirement). Raw log
shows no read of round records or the changelog. Not converged: round 5 at the same scope, to be sent
by the owner.
