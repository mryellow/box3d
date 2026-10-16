---
title: Review round 30 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round29.md
mode: full pass
---

# Round 30

Target at revision 30 (1672 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T12:56:32+10:00 to 13:05:01+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142a3-ca63-7300-b37f-06062c39d34d`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is largely consistent with the three binding designs and the live source. Three H4 instructions need correction before an implementing agent can follow them without guessing.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H4 | H4 defines its slot as image, queue copy and journal segment, omitting the borrowed geometry pointer words required in H1. H5 says those words leave only then. Following H4’s slot definition would leave restored mesh, height-field and compound shapes pointing into the registry that disable frees. | State explicitly that H4 retains the pointer words and H1’s repointing step through H4. |
| 2 | M | §4 H4 | The design §7.1 requires a shape ID in each hull-refcount entry, but H4 says the three hull-database functions emit those entries. Their live signatures in `src/physics_world.c` receive only the world and hull data; deduplicated hull data cannot identify its owning shape. | Specify how `src/shape.c` passes the owner ID into those functions, and how `src/world_snapshot.c` uses the entry-free restore route. |
| 3 | M | §3; §4 H4 | The fail-before rule requires running every extended test entry by name. Copying all of H4’s `test/test_history.c` into H3c cannot build: the new `HistoryJournalTest` calls undo functions absent there, and the extended `HistoryRingTest` calls the new `b3History_CountOwnedBlocks`. Thus the stated procedure cannot run the extended ring entry or obtain its named runtime failure. | Assign H4’s extensions compile-failure evidence explicitly, or provide a scratch test variant that compiles at H3c for the runtime checks. |

### 3. Checked, no change

- The §2 record, container, traversal, serializer and replay inventories agree with the cited live source on the write routes checked.
- H1’s interval-widening and window examples agree with the ring doc’s rules.
- The H2–H4 compile-boundary statements match the current types and planned boundary changes; the H5 and H6 size bounds should fail on their preceding image formats.

### 4. Proposed edits

Clarify H4’s retained slot layout, specify the hull journal’s owner-ID route, and make H4’s fail-before instructions executable for both new and extended tests.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** H1's slot carries the geometry pointer words and its step 3 repoints every
   restored borrowed-geometry shape at the recorded pointer; H5 says the words leave then; H4's
   slot sentence listed header, image, queue copy and segment only. Fix: H4's slot keeps the
   words, with the repoint standing until H5.
2. **Adopted (M).** The design §7.1's hull refcount entry holds a shape id, and the three
   database functions take `( world, hull )` only (`src/physics_world.c`). Their callers outside
   the serializer are `b3CreateShapeInternal`, `b3DestroyShapeAllocationForShapeChange` and
   `b3Shape_SetHull` in `src/shape.c`, each holding the shape id; `src/world_snapshot.c`'s add
   and remove are the restore routes H4 already lists. Fix: the three functions take the owning
   shape's id from H4, the callers named, the serializer's two calls assigned to the restore
   forms.
3. **Adopted (M).** §3's procedure copies the working tree's test files into the previous
   commit's worktree; at H3c `test/test_history.c` calls the walk halves and
   `b3History_CountOwnedBlocks`, which do not exist, so no entry in the file runs there. §6's row
   for the ring doc §9 test 1 already says build failure for the H4 extension; H4's extension
   paragraph said nothing. Fix: the paragraph states it fails before H4 by not building.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 30: 3 findings (H0-M3-L0), 3 applied, 0 declined. Series: 119 findings, 119 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none; finding 2 adds a parameter to three
  existing functions. Clustering: all three in §4 H4 (rounds 7, 17, 21, 27, 30 for the ring and
  ownership text); finding 1 is a consequence of round 23's pointer-words fix meeting H4's slot
  sentence, findings 2 and 3 are gaps present since revision 1. Each fix is a clause. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 1672 -> 1676 lines
  (finding 1 +1, finding 2 +3, finding 3 +0). Above the split-check length (U-T6): no new
  decision, no split.
- Second round of the series with no High finding in two rounds out of three (28 and 30); every
  finding this round was a wording gap inside H4's existing shape.
