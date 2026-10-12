---
title: Review round 11 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round10.md
mode: full pass
---

# Round 11

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T16:01:37+10:00 to 2026-10-12T16:03:57+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

1. **Summary**  
The design’s causal path is supported by the live source: the strict move cycles can leave equal public state and proxy bounds while changing static-tree visit order, and `main` evaluates continuous candidates in that order. I found no defect in the proposed proof criteria or test assertions.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| — | — | — | None. | — |

3. **Checked, no change**

- `src/body.c`, `src/dynamic_tree.c`, and `src/broad_phase.c`: move and reinsert behavior, final bounds, and moved-mark clearing support §3.
- `src/physics_world.c` and `src/solver.c`: the public overlap witness covers the stated static candidates; the static tree is not rebuilt before continuous collision.
- `main`’s `src/solver.c`, `src/distance.c`, and the live candidate-sorting code: the TOI, callback, counter, and sensor mechanisms support §2.
- `src/id_pool.c`, `src/body.c`, and `src/shape.c`: recreation reuses indices while advancing generations, as §8 records.

4. **Proposed edits**  
None.

5. **Unresolved**  
The §5 scratch program is absent from the checkout, so I could not independently reproduce its numerical counts. The proposed fail-before and pass-after runs remain the stated validation gate.

6. **Verdict:** `CONVERGED`

## Post-review verification (Claude)

No findings to verify.
The document is unchanged since round 9's fix.

## Status

11 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1 -> 0/0/0 -> 0/0/1 -> 0/0/0 -> 0/1/0 -> 0/0/0 -> SETTLED 0/0/0. Series total 17 (H4-M9-L4). Converged: a clean full-scope round followed by a clean confirmation round on an unchanged document (U-S8, U-P9). Document 250 lines. Raw log shows no read of round records or the changelog. Status stays Draft: before Canonical, re-run U-G1 against the final text (U-C2/U-C10 apply to §6's test descriptions, as for the earlier documents), then the owner's go-ahead.
