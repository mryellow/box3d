---
title: Review round 10 - proof tests for continuous collision tree-layout dependence
target: 20261012-003-proof-tests-ccd-tree-layout-dependence.md
date: 2026-10-12
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-003-proof-tests-ccd-tree-layout-dependence-round9.md
mode: full pass
---

# Round 10

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

Run in the `solver-tree-order-independent` worktree, foreground, `timeout 540`, exit 0, 2026-10-12T15:57:56+10:00 to 2026-10-12T16:01:31+10:00. The request is the round 1 request unchanged (U-P4).

## Review report (Codex final message)

1. **Summary**

The proposed public API scenes appear to isolate static tree layout and expose the claimed CCD behavior on `main`. I found no source contradiction or proof defect in the design.

2. **Findings**

None.

3. **Checked, no change**

- The move-away-and-back perturbation preserves public IDs and final bounds across the two worlds; moved marks are cleared before CCD. ([src/body.c](/home/user/box3d-solver/src/body.c), [src/dynamic_tree.c](/home/user/box3d-solver/src/dynamic_tree.c), [src/broad_phase.c](/home/user/box3d-solver/src/broad_phase.c))
- The corridor and sensor witnesses cover the static candidates used by their continuous queries. ([src/solver.c](/home/user/box3d-solver/src/solver.c), [src/physics_world.c](/home/user/box3d-solver/src/physics_world.c))
- `main` evaluates candidates in tree visit order. The running TOI fraction, accepted-candidate counters, pre-solve calls, and eight sensor slots can therefore depend on that order. The PR source sorts candidates by shape ID per query. ([src/solver.c](/home/user/box3d-solver/src/solver.c), [src/distance.c](/home/user/box3d-solver/src/distance.c))
- Destroy-and-recreate advances ID generations, so §8 correctly excludes it as a proof perturbation. ([src/id_pool.c](/home/user/box3d-solver/src/id_pool.c), [src/body.c](/home/user/box3d-solver/src/body.c))

4. **Proposed edits**

None.

5. **Unresolved**

The exact reproduction counts in §5 require the scratch `proof.c` run to verify independently; that program is outside the scoped source. The configuration checks in §7 remain pending.

6. **Verdict: CONVERGED**

## Post-review verification (Claude)

No findings to verify.
The document is as after round 9's fix. A confirmation round follows on the unchanged document (U-P9).

## Status

10 rounds: 1/3/0 -> 1/1/0 -> 1/1/1 -> 1/2/1 -> 0/1/1 -> 0/0/0 -> 0/0/1 -> 0/0/0 -> 0/1/0 -> 0/0/0. Series total 17 (H4-M9-L4). Round 10: 0 findings. Document 250 lines, unchanged. Raw log shows no read of round records or the changelog. Clean full-scope round; confirmation round pending.
