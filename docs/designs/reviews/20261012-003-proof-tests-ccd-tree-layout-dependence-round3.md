---
title: Review round 3 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round2.md
mode: full pass
---

# Round 3

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:21:13+10:00 to 15:25:05+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

1. **Summary**  
The source supports the core claim: on `main`, CCD evaluates candidates in tree visit order, and the branch evaluates them in shape id order. The proposed public tests need changes before they establish the strict layout-only proposition, chiefly because move-away-and-back changes static proxy bounds.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §3, §5–6, §8 | The claimed strict perturbation changes bounds. Static proxies are created with a `B3_SPECULATIVE_DISTANCE` margin, but [src/body.c](/home/user/box3d-solver/src/body.c) reinserts a proxy after `b3Body_SetTransform` using `shape->aabbMargin`; [src/shape.c](/home/user/box3d-solver/src/shape.c) shows these differ for the proposed boxes and sensors. This violates §1’s layout-only condition despite the final transform being restored. | Give both worlds the same initial move-away-and-back cycle, then apply an additional cycle to one world. Verify equal final bounds and candidate sets before calling it the strict proof. |
| 2 | M | §1, §6 | The 200-seed tests require a differing witness in only *one* seed, while any seed can make the equality assertion fail. A failure in a seed whose witness order did not differ would not prove layout caused it. | For every reported divergence, require and print a differing complete witness for that same seed; retain at least one such fail-before case. |
| 3 | L | §2 | “The eight kept are the first eight visited” is too broad. [src/distance.c](/home/user/box3d-solver/src/distance.c) returns `maxFraction` for a separated candidate; [src/solver.c](/home/user/box3d-solver/src/solver.c) can put it in a slot, then discard it when emitting hits. An off-path candidate can therefore consume a slot ahead of an on-path sensor. | Qualify the mechanism to the proposed scene, where all twelve candidates have hits before the sweep ends. |

3. **Checked, no change**

- The `main` callback evaluates solid candidates in tree order, passes the running fraction to TOI, conditionally calls `preSolve`, and records maxima for accepted hits. The TOI bracket and tolerance behavior described in §2 match the source.
- [src/dynamic_tree.c](/home/user/box3d-solver/src/dynamic_tree.c) supports the traversal, insertion, and move descriptions. The branch sorts each tree query’s candidates by shape id.
- The id pool is last in, first out; recreation advances shape and body generations. `b3World_RebuildStaticTree` is internal.
- For the specified corridor and sensor fixtures, the public overlap query can witness the static candidates’ order. Static moved marks are cleared during pair update before CCD; that does not resolve finding 1.

4. **Proposed edits**

Revise §§3, 5, 6, and 8 to distinguish restored transforms from restored proxy bounds, and use a bounds-equal move history for the strict tests. Tighten §6’s assertions so each fail-before divergence has its own differing witness. Qualify §2’s eight-slot explanation.

5. **Unresolved**

The recorded numeric counts were not independently reproduced: `proof.c` and the three proposed tests are not present. The document’s double-precision, Release, and move-away-and-back P2 checks remain pending.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (H) | Confirmed. `b3UpdateShapeAABBs` (`src/shape.c`) inflates a static proxy's box by `B3_SPECULATIVE_DISTANCE` at creation; `b3Body_SetTransform` (`src/body.c`) inflates the moved box by `shape->aabbMargin`. The margins differ, so one away-and-back cycle in world B alone changes B's fat bounds, and the §5 move-away counts (60 of 200 and the P3 sets) are not strict-layout counts. Destroy-and-recreate recreates the proxy with the creation margin in both worlds, so its bounds are equal (and its generations are not). The reviewer's remedy is sound: both worlds make the same first cycle (fat box = aabb + shape margin in both), world B makes a second (same final fat box, one more remove-and-reinsert without rotation). | Applied. §3 states the margin difference and the strict form; §5 states what the move-away rows measured; §6 and §7 stop quoting the move-away counts as strict-form fail-before and record them at first run. The §8 decision to run both perturbations stands. Its reason quoted the 60-of-200 rate, which is a count under the unequal bounds; that clause is removed and the "everyday operation" reason stays. |
| 2 (M) | Confirmed. §6 requires an order difference in some seed but attributes no divergent seed to one. | Applied: every seed whose results differ must have a differing witness order. |
| 3 (L) | Confirmed. `b3TimeOfImpact` returns `maxFraction` for a separated pair (`src/distance.c`), which passes the slot test `fraction <= running` (`src/solver.c`) and is dropped when hits are emitted; so a swept-box candidate that misses can take a slot ahead of an on-path sensor. §2's "the eight kept are the first eight visited" holds only when every candidate is a hit. | Applied: §2 qualified to the scene where all twelve are hits. |

Raw log: no read of round records or the changelog.

U-S1: three consecutive rounds each found an H. Judged not "a substantial structural problem": each was a false premise about what a perturbation leaves equal (generation, then bounds), found in the same proof-construction area; the design shape (public perturbation, order witness, assertions of equality, fail-before against `main`'s `src/`) was untouched. Reported in the final summary.
U-S4: findings in §1, §3 and §6 for the third round running. Per U-S4, the fix this round is a cut first: §6's test-mechanism sentence (entry functions, registration, print placement) is deleted, and the §6 lead requirement is restated in one sentence. If findings recur in these sections after the cut, pause and report.

Defect in the reviewed text: revision 3's edit left a duplicated line in §6 ("evidence before its
first assertion" twice). Rounds 2 and 3 reviewed text containing it and raised no finding on it. Found
while folding round 3 and removed in the same edit (U-R9, U-F8).

## Status

3 rounds: 1/3/0 -> 1/1/0 -> 1/1/1. Series total 9 (H3-M5-L1). Round 3: 3 findings (H1-M1-L1), 3 applied,
0 declined. Document 244 -> 244 lines; the round's fixes added about 7 lines and the cut removed
about 7. The cut removed a named mechanism (entry function and registration sentence in §6); the
fixes added one named construction (the strict move-away-and-back form, two cycles). Findings
clustered in §1, §3 and §6 for the third round running; finding 1 is a premise of the same
perturbation as rounds 1 and 2 (public ids, then bounds), not a consequence of round 2's fix. Part B
(AC-1..4) names solver populations and this document adds no structure over one, so re-applying it
to §3 and §6 yields nothing to rewrite. Raw log shows no read of round records or the changelog.
Below the 400-line split-check length. Not converged: round 4 at the same scope.
