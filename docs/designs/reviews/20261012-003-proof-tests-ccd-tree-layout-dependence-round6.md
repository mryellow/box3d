---
title: Review round 6 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round5.md
mode: full pass
---

# Round 6

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:45:35+10:00 to 2026-10-12T15:48:07+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

1. Summary

The design’s mechanism and strict public-API perturbation agree with the cited source. I found no defect that would let the proposed tests prove a difference caused by something other than tree layout, or pass without the required witness.

2. Findings

None.

3. Checked, no change

- `src/body.c`, `src/shape.c`, `src/dynamic_tree.c`, and `src/broad_phase.c`: the move-away-and-back cycles restore equal transforms and proxy bounds; moved marks are cleared before CCD, while reinsertion can change traversal order.
- `src/solver.c` and `src/distance.c`: candidate order can affect the running TOI fraction, the three counters, pre-solve calls, and the eight-slot sensor selection.
- `src/physics_world.c` and `include/box3d/`: the public overlap query can witness the static-tree order for the specified scenes. The proposed candidate-set and non-vacuity requirements address the relevant proof gaps.
- `src/id_pool.c` and `src/shape.c`: destroy-and-recreate advances generations, so §8 correctly excludes it as a proof perturbation.

4. Proposed edits

None.

5. Unresolved

The reported 200-seed counts were not rerun in this review. The double-precision, Release, and fail-before checks in §7 remain to be performed when the tests are implemented.

6. Verdict: CONVERGED

## Post-review verification (Claude)

No findings to verify. The document is as after round 5's fixes. A confirmation round follows on the unchanged document (U-P9).

## Status

6 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1 -> 0/0/0. Series total 15 (H4-M8-L3). Round 6: 0 findings. Document 249 lines, unchanged. Raw log shows no read of round records or the changelog. Clean full-scope round; confirmation round pending.
