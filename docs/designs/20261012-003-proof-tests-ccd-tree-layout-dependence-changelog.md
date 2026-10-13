# Changelog: 20261012-003 proof tests, continuous collision tree-layout dependence

## Revision 1: first draft

- Committed at 157c90a. Public-API proof tests for the claim that continuous collision on `main`
  depends on broad-phase tree layout, with the reproduction run and the decisions in §8.
- Not yet reviewed.

## Revision 2 (after round 1)

- §1: a passing case on `main` voids the case, not P; the void list gains public ids, the tree's
  moved marks, and a step with no impact, call or report.
- §3: destroy-and-recreate keeps indices and advances generations; move-away-and-back keeps public
  ids, and its moved mark is cleared by the step's pair update before the continuous stage.
- §5: "indices" for the equality check.
- §6: witness is the sixteen boxes, equal contact counters before the step and an impact in both
  worlds (P1); non-empty rejecting-callback sequences (P2); eight reported sensors (P3).
- §8: both perturbations kept; the reason now says indices, and names move-away-and-back as the
  equal-public-id case.

## Revision 3 (after round 2)

- §1: the three continuous counter fields named; move-away-and-back keeps public ids,
  destroy-and-recreate is the index-equivalent variant.
- §5: the 106-of-106 remark is scoped to destroy and recreate.
- §6: every test runs both perturbations; one lead requirement for all three (same complete witness
  set, order differing in at least one seed); P1 clause trimmed to the checks only it has; P2
  fail-before counts scoped to destroy and recreate.
- §7: record the P2 fail-before under move away and back (not measured in §5).

## Revision 4 (after round 3)

- §3: a single move-away-and-back cycle changes the static bounds (shape margin against speculative
  distance); the strict form is the same first cycle in both worlds and a second in world B.
- §5: the move-away rows are not strict-layout counts.
- §6: every divergent seed must have a differing witness order; move-away fail-before counts dropped
  until recorded in the strict form; entry-function and registration sentence cut; the lead
  requirement restated in one sentence. A duplicated line left by revision 3's edit removed.
- §7: strict-form fail-before counts to be recorded for all three tests.
- §2: the eight-slot sentence is qualified to every candidate being a hit.
- §8: the 60-of-200 clause removed from the both-perturbations decision; decision unchanged.

## Revision 5 (after round 4, owner decision: Option A)

- §8: the "both perturbations" decision reversed by the owner; proofs use one perturbation, the
  strict move-away-and-back form. Reason recorded in §8: destroy-and-recreate advances id
  generations, the strict form proves every observable at a higher rate with equal public ids and
  bounds.
- §3: the strict form is one cycle per static body in both worlds, world B in descending body order
  (the two-cycle form of revision 4 proved no P3 case); destroy-and-recreate moved to a reachability
  row; the P3 scene puts each sensor on its own static body.
- §1: P stated for public ids and bounds; cross-world id equality defined (index and generation,
  world selector ignored); P1 names the transform and each of the three counters, velocities
  dropped (0 of 200 in every run).
- §5: table and excerpts replaced by the strict-form run (transform 107, counters 67/67/67,
  rejecting order 200, accepting set and sequence 188, sensors differ; PR head all zero); sets and
  sequences reported separately.
- §6: one perturbation; P1 counts the transform and each counter separately; P2's accepting pass
  requires equal sets and equal sequences separately; fail-before counts from §5.
- §7: the strict-count item dropped (recorded).
- Measurement note: the first PR-head run of the strict form was invalid (library built from `main`'s
  `src/` during the recipe check); rebuilt and re-run. Recorded in the round 4 record.

## Revision 5 (owner edit, before round 5)

- Option A of the round 4 pause taken. §1: public ids compared with the world selector ignored; P
  states proxy bounds; P1 lists each counter as its own observable, velocities not claimed; the
  proofs use the strict perturbation only.
- §3: the strict form is one cycle per static body in both worlds, world A ascending and world B
  descending; the P3 scene puts each sensor on its own static body; destroy and recreate is a
  reachability remark, not a proof perturbation.
- §5: reproduction re-run in the strict form; counts replaced (transform 107, each counter 67, P2
  order 200, P2 accepting set 188); no divergence without a differing witness order.
- §6: tests use the strict form only; P1 counts transform and each counter separately; P2 counts
  equal call sets and equal call sequences separately; fail-before counts replaced.
- §8: the both-perturbations decision replaced by the one-perturbation decision.
- Round 4's findings are resolved by this edit; round 5 is a full pass on the edited document.
  Document 244 -> 249 lines.

## Revision 6 (after round 5)

- §4: `ContinuousCandidateOrderTest` described as world-state divergence under a full rebuild of all
  three trees; P1 is attributed to the new impact test only.
- §6: one printing requirement for all three tests (counts, and for each observable's first
  diverging seed both values and both witness orders); P1's own print sentence removed.

## Revision 7 (after round 6)

- No edits. Round 6 clean; confirmation round pending.

## Revision 8 (after round 7)

- §6: the fail-before recipe runs each of the three tests by name.

## Revision 9 (after round 8)

- No edits. Round 8 clean; confirmation round pending.

## Revision 10 (after round 9)

- §6: the accepting pre-solve pass requires non-empty call sets.

## Revision 11 (after round 10)

- No edits. Round 10 clean; confirmation round pending.

## Revision 12 (after round 11)

- No edits. Round 11 confirmation clean on the same document. Converged; status left Draft.

## Revision 13: Canonical

- Status set to Canonical after the confirming clean round 11. U-G1 re-checked against the final text: the document adds tests and no structure over a solver set, the constraint graph, an island or an ID pool, so AC-1 to AC-4 have nothing to fail.
