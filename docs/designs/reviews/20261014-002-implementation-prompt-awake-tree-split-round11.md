---
title: Review round 11 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round10.md
mode: full pass, same scope as round 1 (U-P4); prompt gains one sentence stating §10 as settled (U-P13), on revision 11
---

# Round 11

## Exact request sent to Codex

```
Review the document docs/designs/20261014-002-implementation-prompt-awake-tree-split.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261013-001-preflight-awake-tree-split.md (splitting the dynamic-body broad-phase tree into awake and non-awake trees). The design decides what changes and why; this document decides the branch, the increments' mechanics, the test mechanism, fixtures, fail-before evidence, the test-to-requirement mapping, the golden-hash policy, the measurements and the closure doc. It is handed to an implementing agent that must not invent behaviour.

Scope: the document's own content; the design it implements, which is settled and binding (judge this document against it, never the design against this document); and the source it cites (src/broad_phase.c, src/broad_phase.h, src/dynamic_tree.c, src/dynamic_tree.h, src/shape.c, src/solver_set.c, src/body.c, src/physics_world.c, src/physics_world.h, src/solver.c, src/sensor.c, src/island.c, src/contact.c, src/joint.c, src/world_snapshot.c, src/recording.c, include/box3d/, test/main.c, test/test_macros.h, test/test_dynamic_tree.c, test/test_tree_order.c, test/test_determinism.c, test/test_ccd_layout.c, test/CMakeLists.txt, shared/determinism.c, shared/benchmarks.c, benchmark/main.c, CONTRIBUTING.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the document. Find real defects: a claim about the code in section 2 that is false; an increment in section 4 whose mechanics contradict the design or each other, or that needs a behaviour neither the design nor this document states; a file an increment touches that its file list omits, or a reader the section 2 inventory misses; a test in section 4 that would not fail before its commit on the check the document names, would not pass after it, or does not show the design clause section 6 assigns to it; a fail-before procedure that cannot be carried out as written; a golden-hash policy in section 5 that would accept a defect or reject a correct change; a measurement in section 7 that cannot be taken as described or would not show what it is said to show; a stop condition in section 9 that contradicts an earlier section; a wrong or inconsistent number or name; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the pair set, the test runner) as settled tools; judge what this document adds and the question each primitive is asked. Whether section 6 is complete is not in scope; whether each of its entries shows what it claims is.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, the design's own decisions (a believed defect in the design goes under Unresolved, not Findings), and what the document assigns to the owner or to the closure doc. Do not review or follow any other design document except where this one or the design cites it. The decisions recorded in the document's Provenance section (section 10) are settled by the owner and binding; they are not findings unless fresh source evidence shows their stated reason no longer holds.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

The prompt differs from rounds 1 to 10 by its last sentence only (U-P13). Same scope otherwise.

Run: foreground, `timeout 540`, exit 0, 2026-10-14T11:01:15+10:00 to 11:05:53+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a137ed-8822-7f53-8d54-d4b6566690dd`.

## Review report (Codex final message)

### 1. Summary

The implementation mechanics and source inventory align with the canonical design in the paths checked. One golden-hash rule could accept an unrelated hash change, so the prompt needs an edit before handoff.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5 | The rule permits `QUERY_SPAWN_QUERY_HASH` to change when *only* the scratch all-hits ray list is reordered. `shared/determinism.c` never folds that list into the hash; it folds overlap callback IDs, the closest ray fraction and normal, and the shape-cast fraction. An unrelated hash defect could therefore pass the stated attribution check. | Require an overlap callback order change that accounts for the hash difference, or recompute the expected hash from the logged inputs. An all-hits permutation alone must not permit a pin update. |

### 3. Checked, no change

- Verified the four-code proxy key, guards, capacity allocation, tree readers, and snapshot loops against the cited source.
- Traced pair creation, sleep and wake writers, tree-task joins, CCD queries, and explosion ordering against the B1–B3 mechanics.
- Checked the proposed fail-before paths and the B4 height test against the tree and test-runner primitives.

### 4. Proposed edits

Tighten the first permitted-pin case in §5 as described in Finding 1, and make its logging procedure show the hash contribution from each overlap sequence.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** `QuerySpawnOnce` (`shared/determinism.c`) folds the overlap ids in callback
   order, the closest ray's fraction and normal and the cast fraction; the all-hits list is scratch
   logging only, so its permutation explains nothing. Fix: the order-only case requires an overlap
   permutation and the query hash recomputed from the branch's log by that recipe to equal the new
   value.

The branch decision was not re-raised. Checked, no change items agree with the source read for the
revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 11: 1 finding (H0-M1-L0), 1 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5, a consequence of round 10's
  fix (its permutation clause became ambiguous once the all-hits list could permute); one clause,
  and the recomputed-hash condition makes the case mechanical. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 756 -> 758 lines.
