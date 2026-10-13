---
title: Review round 7 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round6.md
mode: full pass
---

# Round 7

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:48:17+10:00 to 15:50:51+10:00. The request is the round 1 request unchanged (U-P4); this was the confirmation round after round 6.

## Review report (Codex final message)

1. **Summary**

The source supports the proposed layout mechanism and the strict move-away-and-back perturbation: both worlds finish with equal static proxy bounds and state, while tree traversal can differ. I found one defect in how the document tells a reviewer to reproduce the three fail-before results.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §6–§7 | The stated fail-before recipe runs only `ContinuousLayoutImpactTest`. It cannot produce the promised fail-before output for the pre-solve and sensor proofs. | Give commands that run all three named tests, and specify the expected failure and restored-branch pass for each. |

3. **Checked, no change**

- `src/solver.c` and `src/distance.c` support the running-fraction, counter, pre-solve, and eight-slot sensor mechanisms.
- `src/body.c`, `src/shape.c`, `src/dynamic_tree.c`, and `src/broad_phase.c` support equal final bounds and cleared moved marks under the strict perturbation.
- `src/physics_world.c` supports the public overlap-query witness for the stated scenes. The public API and `test/test_tree_order.c` support the other cited API and test claims.
- The id-pool behavior supports §8’s decision to exclude destroy-and-recreate from a proof requiring equal public ids.

4. **Proposed edits**

Replace §6’s single test invocation with invocations for all three proof tests; align §7’s instruction to quote that output.

5. **Unresolved**

The reported 200-seed counts in §5 were not independently reproduced. The double-precision and Release runs remain the pre-finalisation checks already listed in §7.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (L) | Confirmed. §6's recipe passes one test name; `test/main.c` filters by exact name, one per invocation, so the pre-solve and sensor proofs are not run by it, while §7 asks for the output of all three. | Applied: the recipe loops over the three names. |

Raw log: no read of round records or the changelog; the files appear only in a `git status` listing.

## Status

7 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1 -> 0/0/0 -> 0/0/1. Series total 16 (H4-M8-L4). Round 7: 1 finding (H0-M0-L1), 1 applied, 0 declined. Document 249 -> 250 lines; the fix added 1 line. No named mechanism added or removed. The finding is in §6, which has produced findings in most rounds; it is a different clause from round 5's (the recipe, not the printing). It is not a consequence of the previous round's fix. Raw log shows no read of round records or the changelog. Below the 400-line split-check length. Not converged: the clean round 6 is not confirmed; a clean full-scope round and a confirmation round on the same document are needed.
