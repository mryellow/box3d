---
title: Review round 1 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none
mode: full pass
---

# Round 1

## Exact request sent to Codex

```
Review the document docs/designs/20261014-002-implementation-prompt-awake-tree-split.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for the Canonical design docs/designs/20261013-001-preflight-awake-tree-split.md (splitting the dynamic-body broad-phase tree into awake and non-awake trees). The design decides what changes and why; this document decides the branch, the increments' mechanics, the test mechanism, fixtures, fail-before evidence, the test-to-requirement mapping, the golden-hash policy, the measurements and the closure doc. It is handed to an implementing agent that must not invent behaviour.

Scope: the document's own content; the design it implements, which is settled and binding (judge this document against it, never the design against this document); and the source it cites (src/broad_phase.c, src/broad_phase.h, src/dynamic_tree.c, src/dynamic_tree.h, src/shape.c, src/solver_set.c, src/body.c, src/physics_world.c, src/physics_world.h, src/solver.c, src/sensor.c, src/island.c, src/contact.c, src/joint.c, src/world_snapshot.c, src/recording.c, include/box3d/, test/main.c, test/test_macros.h, test/test_dynamic_tree.c, test/test_tree_order.c, test/test_determinism.c, test/test_ccd_layout.c, test/CMakeLists.txt, shared/determinism.c, shared/benchmarks.c, benchmark/main.c, CONTRIBUTING.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the document. Find real defects: a claim about the code in section 2 that is false; an increment in section 4 whose mechanics contradict the design or each other, or that needs a behaviour neither the design nor this document states; a file an increment touches that its file list omits, or a reader the section 2 inventory misses; a test in section 4 that would not fail before its commit on the check the document names, would not pass after it, or does not show the design clause section 6 assigns to it; a fail-before procedure that cannot be carried out as written; a golden-hash policy in section 5 that would accept a defect or reject a correct change; a measurement in section 7 that cannot be taken as described or would not show what it is said to show; a stop condition in section 9 that contradicts an earlier section; a wrong or inconsistent number or name; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the pair set, the test runner) as settled tools; judge what this document adds and the question each primitive is asked. Whether section 6 is complete is not in scope; whether each of its entries shows what it claims is.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, the design's own decisions (a believed defect in the design goes under Unresolved, not Findings), and what the document assigns to the owner or to the closure doc. Do not review or follow any other design document except where this one or the design cites it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-14T08:27:45+10:00 to 08:30:38+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13760-ff9f-79f0-8870-9f8aa5c29cb4`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The implementation prompt is not ready to hand off unchanged. B1’s file restrictions conflict with its CCD instructions, its fail-before procedure cannot produce the stated failure, and several verification checks would leave the claimed behavior unproven.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 B1, §3 | B1 instructs the implementer to add the sleeping-tree bullet query in `b3SolveContinuous`, but omits `src/solver.c` from B1’s permitted files. B2 says that query is already present. | Add `src/solver.c` to B1’s file list. |
| 2 | H | §4 B1, §3 | `SleepingTreePairTest` manually creates marked proxies in the fourth tree before its first step. Before B1, pair discovery never clears those marks, while B0’s `b3ValidateNoMoved` checks the fourth tree. The Debug fail-before run therefore asserts before the named contact checks. | Specify a fail-before fixture that reaches the contact assertions, or record the validation assertion as the expected failure and revise §3. |
| 3 | M | §4 B2/B3, §3 | `SleepingExplodeTest` is said to compile and run on the base commit, but §3 copies the entire B2 test file into that worktree. That file also uses B0’s fourth-tree constants and counter fields, which the base lacks. | Specify an isolated base-compatible explosion harness or a precise extraction procedure. |
| 4 | M | §4 B2/B3 | A third awake box “moving elsewhere” does not guarantee a pending rebuild on the step when begin touch wakes the sleeper. Proxy movement marks the tree only when the shape leaves its fat AABB (`src/body.c`, `src/solver.c`). The test could pass without exercising the stated race. | Require a displacement beyond the fat AABB on the relevant step and verify that a rebuild was enqueued. |
| 5 | M | §5 | The permitted `QUERY_SPAWN_QUERY_HASH` update is justified by finding *one* reordered overlap query. The hash also contains ray fraction and normal and shape-cast fraction (`shared/determinism.c`); that evidence could accept a simultaneous defect in either cast. | Compare every query’s inputs and outputs, and show that replaying only the changed overlap order reproduces the new hash. |
| 6 | M | §7 | The proposed scene hash covers transforms and awake flags, while the cited `HashWorldState` recipe also hashes linear velocity, angular velocity, and awake count (`test/test_tree_order.c`). Equal hashes as specified would not establish equal state. | Use the complete recipe for every benchmark step. |
| 7 | M | §7 | The “cross-pass node visits” counter excludes `b3GatherCrossSeeds`, which traverses and tests node pairs before `b3VisitPair` runs (`src/broad_phase.c`). A pass can report zero visits despite doing seed-gather work. | Count seed-gather visits per pass as well, and distinguish them from task visits if useful. |
| 8 | L | §6 | The B1 reader mapping marks Explode as “Fails before? Yes,” but §4 explicitly says `SleepingExplodeTest` fails before on no assertion. | Mark it as a regression comparison, with no fail-before assertion. |

### 3. Checked, no change

- The four-code proxy-key format, current guards, tree capacity setup, and snapshot tree loops match §2’s account.
- The sleep and wake set writes, contact-set selection, and step join and validation order support the proposed migration timing.
- The cited tree behavior supports carrying leaf boxes and moved bits, clearing moved marks after pair discovery, and using a full rebuild for the height trigger.
- The test runner’s entry-name filter and the benchmark command options match the prompt.

### 4. Proposed edits

Resolve findings 1–4 in §4 and the fail-before instructions before implementation. Tighten the golden-hash proof and both measurement recipes, then correct the §6 Explode mapping.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2). The target is an Implementation Prompt, so
test mechanism and fail-before evidence are its content (U-C10) and in scope.

1. **Adopted (H).** B1's code bullet changes `b3SolveContinuous` but its file list named
   `src/broad_phase.c`, `src/sensor.c`, `src/physics_world.c` and the test files only. Fix: B1's
   file list names `src/solver.c`.
2. **Adopted (H).** `b3ValidateNoMoved` (`src/broad_phase.c`) asserts no node in any tree carries
   the moved bit, and B0 widens it to four trees. At B0 nothing clears the fourth tree's bits: the
   dynamic and kinematic trees are cleared by the rebuild in `b3UpdateTreesTask`, the static tree
   by `b3DynamicTree_ClearMoved`, and the fourth tree by neither until B1. `b3BroadPhase_CreateProxy`
   marks every non-static create, so the test's placed proxies are marked and the first step
   asserts before any contact check. Fix: the fail-before check is the `b3ValidateNoMoved`
   assertion on the first step; §3 admits a named `B3_ASSERT` message as the quoted evidence; the
   §6 row says so.
3. **Adopted (M).** §3 copies the whole working-tree test file into the fail-before worktree, and
   at the base commit that file names B0's constant and counter fields. Fix: the base run copies
   only the test's entry function and its helpers, which use the public API and the awake set's
   body list, into an otherwise empty test file in a worktree at `c6c6ead`.
4. **Adopted (M).** Finalize marks a proxy only when its fat AABB no longer contains the shape's
   AABB (`src/solver.c`), so a box moving freely need not mark every step. `b3Body_SetTransform`
   always marks (`b3BroadPhase_MoveProxy`, `src/body.c`), `b3InsertLeaf` refits to the root with
   `b3MakeInternalNode`, which carries the moved bit up, and `b3NeedsRebuild` reads the root's bit
   (`src/dynamic_tree.h`). Fix: the third box is moved with `b3Body_SetTransform` before every
   step and the test asserts the awake tree's root carries the moved bit before each step.
5. **Adopted (M).** `shared/determinism.c` folds the ray hit's fraction and normal and the shape
   cast's fraction into `queryHash` beside the overlap shape ids. Fix: the cause proof logs all
   three per query and requires every ray and cast value equal, every overlap set equal, and one
   overlap order differing with a sleeping body; the existing stop condition covers anything else.
6. **Adopted (M).** `HashWorldState` (`test/test_tree_order.c`) hashes transform, linear and
   angular velocity and awake flag per body, then the awake count. Fix: §7's scene hash names
   that recipe in full.
7. **Adopted (M).** `b3GatherCrossSeeds` (`src/broad_phase.c`) tests node pairs with `b3TestPair`
   breadth-first before any seed reaches `b3VisitPair`. Fix: the counter also counts node pairs
   tested in the gather. The pass counts are stated exactly: five cross and two self passes on
   the branch, two cross and one self on the base.
8. **Adopted (L).** §4 says `SleepingExplodeTest` fails before on no assertion; two §6 rows said
   "Explode: yes". Fix: both rows say regression comparison with the base.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Gate (U-G1) run by the author before this round: the prompt adds no structure beyond the
  Canonical design's queue, depth and choke point; AC-1 to AC-4 hold.
- Round 1: 8 findings (H2-M5-L1), 8 applied, 0 declined.
- Findings: 2/5/1 (round 1) -> pending
- Tripwire: fixes add or remove a named mechanism: no; one test fixture changes its mover and
  gains an assertion, one scratch-only log widens. Clustering: none (first round). Reviewer read
  of round records or changelog: none in the raw log. Doc length: 665 -> 676 lines; the fixes add
  one line (1), four (2), two (3), two (4), two (5), one (6), two (7), none (8), with one line
  removed by rewording. Above the split-check length (U-T6): the doc decides one thing, how one
  design lands; its sections are facets of that, so no split.
