---
title: Review round 55 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round54.md
mode: full pass
session: 01a14390-49ce-78f3-abfd-8ce3d24038a5
---

# Round 55

Target at revision 52 (1843 lines).

## Exact request sent to Codex

Identical to round 1 (U-P4):

```
Review the document docs/designs/20261016-001-implementation-prompt-history-ring.md as an adversarial reviewer for the Box3D physics engine. It is the Implementation Prompt for three Canonical design documents: docs/designs/20260929-002-rewind-only-history-ring.md (the design: per-tick image, undo journal, restore, caller contract, verification, public API), docs/designs/20261015-003-history-ring-budget-and-eviction.md (the ring doc: slots, ends, interval, budget, window, enable and disable) and docs/designs/20261015-004-history-ring-broad-phase-trees.md (the trees doc: which trees are imaged or derived, proxy identity, the restore pass). The designs decide what changes and why; this document decides the branch, the increments' mechanics, the test mechanism, fixtures, fail-before evidence, the test-to-requirement mapping, the golden-hash policy, the measurements and the closure doc. It is handed to an implementing agent that must not invent behaviour.

Scope: the document's own content; the three designs it implements, which are settled and binding (judge this document against them, never a design against this document); and the source it cites (src/physics_world.h, src/physics_world.c, src/body.h, src/body.c, src/shape.h, src/shape.c, src/contact.h, src/contact.c, src/mesh_contact.c, src/contact_solver.c, src/joint.h, src/joint.c, the eight joint files, src/island.h, src/island.c, src/solver_set.h, src/solver_set.c, src/constraint_graph.h, src/constraint_graph.c, src/sensor.h, src/sensor.c, src/broad_phase.h, src/broad_phase.c, src/dynamic_tree.h, src/dynamic_tree.c, src/id_pool.h, src/id_pool.c, src/container.h, src/bitset.h, src/table.h, src/block_allocator.h, src/core.h, src/core.c, src/world_snapshot.h, src/world_snapshot.c, src/recording.h, src/recording.c, src/replay.h, src/replay.c, src/solver.c, src/hull.h, include/box3d/, test/main.c, test/test_macros.h, test/test_recording.c, test/test_determinism.c, test/test_awake_tree.c, test/test_tree_order.c, test/test_name_cache.c, test/CMakeLists.txt, src/CMakeLists.txt, CMakeLists.txt, shared/benchmarks.h, shared/benchmarks.c, benchmark/main.c, CONTRIBUTING.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the document. Find real defects: a claim about the code in section 2 that is false; an increment in section 4 whose mechanics contradict a design or another increment, or that needs a behaviour neither a design nor this document states; a file an increment touches that its file list omits, or a writer or reader the section 2 inventory misses; a test in section 4 that would not fail before its commit on the check the document names, would not pass after it, or does not show the clause section 6 assigns to it; a fail-before or compile-boundary procedure that cannot be carried out as written; a golden policy in section 5 that would accept a defect or reject a correct change; a measurement in section 7 that cannot be taken as described or would not show what it is said to show; a stop condition in section 9 that contradicts an earlier section; a wrong or inconsistent number or name; contradictions between sections. Treat existing primitives (b3DynamicTree, the solver sets, the constraint graph, the id pools, the serializer, the recording and replay, the test runner) as settled tools; judge what this document adds and the question each primitive is asked. Whether section 6 is complete is not in scope; whether each of its entries shows what it claims is. Decisions recorded in section 10 are settled unless fresh source evidence shows their reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, the designs' own decisions (a believed defect in a design goes under Unresolved, not Findings), and what the document assigns to the owner or to the closure doc. Do not review or follow any other design document except where this one or the three designs cite it.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```
Run: foreground, `timeout 540`, exit 0 after 39 source reads, 2026-10-16T17:14:51+10:00 to
17:20:31+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14390-49ce-78f3-abfd-8ce3d24038a5`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the three binding designs and the cited source. Two defects need edits before handoff: a contradictory rule for freeing detached blocks, and a missing kinematic tree layout test required by the trees design.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §3, §4 H4, §6 | §4 says every journal call first returns when history is disabled, but also requires `b3Journal_TakeBlocks` to free detached blocks in that case. A literal early return leaks blocks; §6's first-statement inspection would endorse it. This conflicts with the ownership rule in the design §7.1. | Exempt ownership-taking calls from the early-return rule. Require them to free each detached handle before returning when history is disabled, and make §6 inspect that path. |
| 2 | M | §4 H6, §6, §10 | The trees doc §10 requires replay on a **different kinematic tree layout**. H6 checks kinematic ids, boxes, moved bits and replay, but takes no layout witness. §10's reason for omitting one is incomplete: `src/broad_phase.c` calls a partial rebuild each step, but `src/dynamic_tree.c` returns without rebuilding when the root is unmarked and the tree is DFS ordered. | Add a staged kinematic fixture whose visit order is verified to differ after rewind and perturbation, then compare replay hashes and events as the trees doc §10 requires. |

### 3. Checked, no change

- §2's filter recreation behavior, step capture point, serializer omissions and broad-phase traversal count match the cited source.
- H1's stated interval-widening and window tick sequences match the ring doc §§5–7.
- H5 and H6's 256 KB cases distinguish the serialized and raw kinematic images from the intended partial image.

### 4. Proposed edits

Clarify the disabled-history ownership path in §§3–4 and its evidence in §6. Add the kinematic layout witness to H6 and update the trees doc §10 mapping and §10 rationale.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Finding 2 reverses the
position rounds 50 to 52 took and closes round 52's owner item.

1. **Adopted (H).** The H4 bullet said every journal call is the bare null return first and, in
   the next sentence, that `b3Journal_TakeBlocks`'s disabled branch frees the handles; the §6 row
   for the design §7.3 inspects "every journal function's first statement", which a literal
   reading satisfies with a leak. The design §7.3 asks for one null check and nothing else with
   history disabled, which the free-then-return branch keeps. Fix: the bullet names the ownership
   transfers as the exception with the branch's form, and the §6 row inspects that branch.
2. **Adopted (M).** `b3DynamicTree_Rebuild( tree, false )` returns at once when the root is
   unmarked and the tree is ordered, and otherwise descends marked internal nodes only, keeping
   unmarked subtrees as build leaves (`src/dynamic_tree.c:2199` and the gather below it), so a
   kinematic tree whose bodies are at rest keeps a staged layout through the steps.
   `test/test_tree_order.c` already plans its witness corridor on a kinematic cluster
   (`trees[3] = { static, kinematic, dynamic }`, line 770) and `ENSURE`s `WitnessDiffers` for it,
   which rounds 50 to 52 did not consult when they recorded the clause as unprovable. Fix: H6's
   sweep case has its kinematic sensor bodies at rest with shapes created in two parts around a
   step and takes the visit-order witness over the kinematic tree as the H5 cases do; the three
   moving cases take none; the H5 bullet, the §6 row and the §10 item say so, the owner item
   closed.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 55: 2 findings (H1-M1-L0), 2 applied, 0 declined. Series: 173 findings, 172 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> pending
- Tripwire: fix 2 adds a witness to an existing case and removes an owner item; fix 1 adds no
  mechanism. Clustering: the kinematic layout witness (rounds 24, 31, 42, 50, 51, 52, 55), the
  seventh round, a recurrence reported under U-S4: the series' position changed from "no
  witness" to "witness with bodies at rest" on evidence in the repo's own test that earlier
  rounds did not read; the H4 ownership bullet (rounds 52, 55). Finding 2 is against round 52's
  §10 item. Each fix is a clause or a paragraph. Reviewer read of round records or changelog:
  none in the raw log. Doc length: 1843 -> 1847 lines. Above the split-check length (U-T6): no
  new decision, no split.
- First round since 50 to finish inside the timeout. Owner items back to three: rounds 15 and
  20 (design wording) and 35 (the trees doc's pointer-to-const wording).
