---
title: Review round 9 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round8.md
mode: full pass
---

# Round 9

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:55:07+10:00 to 2026-10-12T15:57:41+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

## 1. Summary

The live source supports the design’s CCD mechanism, public API perturbation, and witness for the proposed scenes. I found one gap in the test specification: the accepting pre-solve pass can satisfy its PR-side assertions without making a call.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §§1, 6 | §1 says a proof is void when there is no call to compare. The rejecting pre-solve pass requires non-empty logs, but the accepting pass requires only equal sets and sequences. It could pass on the PR branch with both logs empty. | Require a non-empty accepting log in each world for each tested seed, alongside the equality checks. |

## 3. Checked, no change

- On `main`, candidates are evaluated in tree visit order. On the PR branch, they are collected and evaluated by shape id within each query.
- The TOI upper bracket uses the running fraction; accepted solid hits update it and the three reported iteration maxima.
- Sensor hits take eight slots in visit order. The stated move cycle retains ids and gives both worlds the same final bounds; the step clears static-tree moved marks before CCD.
- The public overlap query uses the static tree’s visit order. For the specified scenes, its static candidate set matches the continuous query’s set.
- The existing tests and the public API declarations support their descriptions in the document.

## 4. Proposed edits

Add the accepting-pass non-empty requirement to `ContinuousLayoutPreSolveTest` in §6.

## 5. Unresolved

The exact §5 reproduction counts could not be independently checked: the scratch `proof.c` is not present in the workspace. The proposed tests and their fail-before runs would provide that check.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

| # | Verdict | Resolution |
|---|---|---|
| 1 (M) | Confirmed. §6 requires non-empty call sequences for the rejecting pass and only equal sets and sequences for the accepting pass, so the accepting pass can pass on the PR branch with empty logs, which §1 voids. | Applied: the accepting pass requires non-empty sets. |

Raw log: no read of round records or the changelog; the files appear only in a `git status` listing.

## Status

9 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1 -> 0/0/0 -> 0/0/1 -> 0/0/0 -> 0/1/0. Series total 17 (H4-M9-L4). Round 9: 1 finding (H0-M1-L0), 1 applied, 0 declined. Document 250 -> 250 lines; the fix replaced a clause of the same length. No named mechanism added or removed. The finding is in §6 and is a gap in the same pass that round 7's finding did not touch; not a consequence of the previous round's fix. Raw log shows no read of round records or the changelog. Below the 400-line split-check length. Not converged: the confirmation round found a defect, so a clean round and a confirmation round on one document are still needed.
