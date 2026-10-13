---
title: Review round 8 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round7.md
mode: full pass
---

# Round 8

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:51:09+10:00 to 2026-10-12T15:54:58+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

1. **Summary**  
The design’s layout perturbation, public witness, and three proposed comparisons are consistent with the live source and the `main` CCD path. I found no defect that would invalidate the proposed proof.

2. **Findings**  
None.

3. **Checked, no change**

- Move-away-and-back can change static tree layout while leaving the two worlds with matching ids, final bounds, and moved marks; the step clears those marks before CCD.
- The public overlap witness uses the same tree traversal order as CCD for the specified scenes, and the corridor and sensor candidates fit within their swept queries.
- On `main`, candidate evaluation, the running TOI fraction, pre-solve calls, iteration counters, and the eight sensor slots can depend on visit order. The branch sorts candidates per query.
- The proposed assertions require an impact, callback calls, or eight sensor reports as appropriate, so the reported comparisons are not empty.

4. **Proposed edits**  
None.

5. **Unresolved**  
The numerical counts in §5 were not independently rerun: its scratch `proof.c` is not among the available sources. The Debug, double-precision, Release, and PR transcript checks remain as listed in §7.

6. **Verdict: CONVERGED**

## Post-review verification (Claude)

No findings to verify.
The document is as after round 7's fix. A confirmation round follows on the unchanged document (U-P9).

## Status

8 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1 -> 0/0/0 -> 0/0/1 -> 0/0/0. Series total 16 (H4-M8-L4). Round 8: 0 findings. Document 250 lines, unchanged. Raw log shows no read of round records or the changelog. Clean full-scope round; confirmation round pending.
