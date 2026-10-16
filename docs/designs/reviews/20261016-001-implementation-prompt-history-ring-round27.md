---
title: Review round 27 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round26.md
mode: full pass
---

# Round 27

Target at revision 27 (1644 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T12:17:55+10:00 to 12:23:54+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14280-6e99-7393-8210-c0a464cb4436`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The ring behavior and most fail-before checks match the three binding designs and the live source. I found three implementation gaps: one prevents H3c from compiling as specified, and two leave H4’s ownership path unable to handle ordinary destruction safely.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H3c | The graph becomes inaccessible outside `src/constraint_graph.c`, but H3c’s scratch setter covers only five fields. `src/solver.c` also assigns `manifoldConstraints` and `contactConstraints`; those assignments cannot compile through the stated boundary. | Include both overflow pointers in the graph’s scratch writer and name their call sites in H3c. |
| 2 | H | §4 H4 | Pool free is told to detach contact blocks through `b3Contact_SetBlocks`’ restore form. H3b defines that form as requiring the restore mark, while an ordinary contact destroy runs outside restore. The specified route would assert. | Specify a non-journaling detach and attach route in `src/contact.c` for ownership transfer; keep the restore-only load for deserialization and undo. |
| 3 | H | §§3, 4 H4 | H4 moves blocks into ownership entries on destroy, while every journal append returns immediately when history is disabled. It gives no disposal path for blocks detached with no entry. This affects contact and shape blocks and the other owning destroy entries. | State the disabled-history branch for every owning destroy: free its blocks as the current engine does; transfer ownership only when an entry is retained. |

### 3. Checked, no change

- §2’s snapshot version, filter behavior, step capture point, tree write routes, and principal record and container inventories match the cited source.
- `HistoryRingTest`’s interval-widening and window sequences match the ring doc §§5–7.
- The H0, H1 and H4 build-failure procedures, and the H5 and H6 cost-bound fail-before checks, target behavior absent before their commits.
- The hash’s exclusions and the H5/H6 tree image versus derived-tree split match the design §12 and trees doc §§5–9.

### 4. Proposed edits

Add the two overflow scratch fields to H3c’s graph writer. In H4, distinguish normal detach and attach from restore-only loading, and spell out how each owning destroy frees blocks when history is disabled.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `src/solver.c` assigns the overflow colour's `contactConstraints` and
   `manifoldConstraints` (`src/constraint_graph.h`) beside the five wide fields round 25's setter
   covered. Fix: seven fields.
2. **Adopted (H).** Round 17's detach went "through `b3Contact_SetBlocks`' restore form", which
   H3b puts under the restore mark; a contact destroyed between steps is no restore, so the
   validation build would assert on every ordinary destroy. Fix: detach and attach are ownership
   transfers of their own with no mark assertion and no entry.
3. **Adopted (H).** The journal's null check returns without doing anything, and every owning
   destroy had been written as handing its blocks to an entry; with history disabled nothing
   recorded the handles and nothing freed them. The design §7.3 fixes the cost (one null check)
   and the source frees these blocks today. Fix: one journal function takes every detached block,
   freeing it in the disabled branch and recording the owning entry otherwise, so no caller
   branches (AC-2's one choke point).

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 27: 3 findings (H2-M1-L0), 3 applied, 0 declined. Series: 110 findings, 110 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: `b3Journal_TakeBlocks`, the single route by
  which an owning destroy hands over or frees a block (one choke point, AC-2; the design's
  ownership entries are what it records); the detach and attach pair of round 17 renamed as
  ownership transfers. Clustering: §4 H3c's colour bullet (rounds 3, 16, 25, 27: finding 1 is a
  consequence of round 25's fix 1), §4 H4's slot pool (rounds 7, 17, 27: finding 2 of round 17's
  fix 2), §4 H4's null-check rule (round 27, first time; finding 3 of revision 1, a gap the
  design's cost rule left to the prompt). Each fix is a clause or a short paragraph. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 1644 -> 1655 lines. Above
  the split-check length (U-T6): no new decision, no split.
- Finding 3 is the first finding since round 11 to add a route the design implies but does not
  name; it was reachable from the design's §7.3 cost rule alone and would have leaked every owned
  block on a world that never enables history.
