---
title: Review round 1 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

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

Run in the `solver-tree-order-independent` worktree at 157c90a, foreground, `timeout 540`, exit 0, 2026-10-12T15:10:19+10:00 to 15:13:55+10:00.

## Review report (Codex final message)

1. **Summary**  
The source supports the proposed CCD mechanism: on `main`, the static tree’s visit order determines candidate evaluation order, and the PR sorts candidates before evaluation. The design still needs stronger equality and witness checks before its public API tests can establish the proposition as stated.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§1, 3, 5, 8 | Destroy and recreate preserves numeric ID slots, but changes public body and shape ID generations. `src/body.c` increments a reused body’s generation, and `include/box3d/id.h` makes generation part of ID equality. This perturbation therefore does not meet P’s “identical ids; only layout differs” premise; it can also change arguments passed to an ID-dependent pre-solve callback. | Keep this perturbation as supporting evidence of user reachability, as §8 decides, but state its limit. Use the move-away-and-back case for the strict equal-ID proof. |
| 2 | M | §§3, 5, 6 | Move-away-and-back can change a proxy’s moved state as well as its layout. `src/body.c` moves the proxy, `src/dynamic_tree.c` stores moved flags, and `b3World_Step` processes broad-phase changes before CCD (`src/physics_world.c`). The proposed checks do not establish that this extra state cannot affect the compared step. | Add a public API setup step that clears pending movement before the measured step, then recapture the witness; or demonstrate that both worlds enter CCD with equivalent contacts and other relevant state. |
| 3 | M | §§1, 3, 6 | The proposed tests require only equal witness sets and some differing witness order. They do not require the covering-box witness to equal the CCD query’s candidate set or prove CCD produced a hit. Equality could pass with empty pre-solve logs or sensor events; a covering-box order difference could involve shapes outside the CCD query. | Assert the intended candidates are in the actual swept query in both worlds, check a CCD hit for P1, nonempty expected calls for P2, and eight expected sensor events for P3. |
| 4 | M | §1 | P is existential: one equal-world pair *can* differ. A test passing on `main` with a differing witness rules out that test case, not P. | Replace the stated falsification criterion with “the chosen case fails to demonstrate P”; retain a demonstrated fail-before case as the proof requirement. |

3. **Checked, no change**

- `src/solver.c` confirms that `main` evaluates candidates in tree callback order, passes the running fraction to TOI, and updates the cited counters only for accepted solid hits. The PR evaluates collected candidates in ascending shape ID within each tree query.
- `src/distance.c` confirms `maxFraction` supplies the root search endpoint and that the search alternates bisection and false position with a separation tolerance.
- `src/dynamic_tree.c` confirms layout-dependent query traversal, rotating insertion, and remove/reinsert without rotation on proxy movement. `src/id_pool.c` confirms last-in-first-out reuse of freed numeric slots.
- `src/physics_world.c` and `include/box3d/` confirm the public overlap witness, pre-solve callback, counters, and sensor-event APIs. The eight-hit CCD sensor limit and later fraction check are present in `src/solver.c`.

4. **Proposed edits**  
Revise §§1, 3, 5, and 8 to distinguish numeric slot reuse from full public ID equality. In §6, specify a pre-step state check and require each test’s witness and nonempty outcome to correspond to the CCD query being tested. Correct §1’s falsification language.

5. **Unresolved**  
The double-precision, Release, and three-test fail-before runs listed in §7 remain to be performed; this review did not reproduce the scratch program’s counts.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (H) | Confirmed. `src/body.c` does `body->generation += 1` when a body is created, and `src/shape.c` does the same for a shape, including on a reused slot; `include/box3d/id.h` includes `generation` in id equality. Destroy-and-recreate returns the same numeric slots (`src/id_pool.c`, unchanged) with advanced generations. The `main` and PR-head counts are unaffected: the §5 program compares the internal index and the PR sorts by `shape->id`. | Applied. Facts corrected in §1, §3 and §5: the indices are equal, the generations are not; move-away-and-back is the case with equal public ids. The §8 decision to run both perturbations stands; its stated reason ("the same ids") is corrected to indices, and the limit is stated. |
| 2 (M) | Confirmed. `b3BroadPhase_CreateProxy` does not mark a static proxy moved (unless pair creation is forced); `b3BroadPhase_MoveProxy` always marks it; `b3UpdateBroadPhasePairs` (run at the start of `b3World_Step`, before the continuous stage) consumes the static moved marks and clears them. So world B enters the step with state A does not have. Whether the pairs and contacts found differ was not shown. | Applied. §1 names moved marks as state; §3 states the mark and where it is cleared; §6 requires equal contact counters before the step in both worlds. |
| 3 (M) | Confirmed. §3 claims the witness is exact for the corridor construction, but §6 does not require the witness set to be that construction's candidate set, nor that the step produced an impact, a call or eight reports. A pass on the PR head could be vacuous. | Applied as the proof's non-vacuity requirement in §1 and one clause per test in §6. |
| 4 (M) | Confirmed. P is existential ("can give different results"); a passing case on `main` does not refute it. The sentence that said it would falsify P is wrong. | Applied: the sentence now says what a passing case voids. |

Raw log: no read of round records or the changelog.

## Status

1 round: 1/3/0. Series total 4 (H1-M3-L0). Round 1: 4 findings (H1-M3-L0), 4 applied, 0 declined.
Document 233 -> 237 lines; fixes added about 4 net lines (19 added, 15 replaced). No named mechanism
added or removed. Findings are in §1, §3, §5, §6 and §8; none repeats a section from a prior round
(first round). One finding (the generation field) was a premise of a settled §8 decision; the decision
stands and its stated reason is corrected. No finding was a consequence of a previous fix. Doc is
below the 400-line split-check length. The raw log shows no read of round records or the changelog.
Not converged: round 2 at the same scope.
