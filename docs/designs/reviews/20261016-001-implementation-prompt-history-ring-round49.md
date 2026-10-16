---
title: Review round 49 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round48.md
mode: full pass (confirmation round after round 48, U-P9)
session: 01a14348-c440-7d70-a954-86677d4be387 (resumed to finalize)
---

# Round 49

Target at revision 46 (1783 lines), unchanged since round 48.

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 68 source reads and no final
message), 2026-10-16T15:56:44+10:00 to 16:05:44+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a14348-c440-7d70-a954-86677d4be387`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 16:05:55 to 16:06:25,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt is close, but three implementation defects need correction before handoff. One source claim is false, the material-block restore route is underspecified in a way that would journal a rewind, and two proposed read APIs expose mutable container storage.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2; §4 H3a | §2 says every single-material shape has `materials == NULL`. A compound with one distinct material still gets an allocated block in `src/shape.c`. Treating count 1 as proof of inline storage would miss that block during writes or ownership transfer. | State the compound exception and make the H3a/H4 routes distinguish inline storage from a block by `materials`, not `materialCount`. |
| 2 | H | §4 H3a–H4 | H3a makes `b3Shape_SetMaterialBlock` the writer used by the serializer’s reader. H4 says block mutators journal, while the restore mark may be read only by assertions (§3, §10). On an H4 phase-0 rewind, the reader therefore has no specified no-entry route for loading or freeing material blocks; journaling those operations would leave pending entries after rewind. | Specify separate forward mutators and no-entry restore load/free functions, as H3b does for contact blocks, and route `b3DeserializeIntoShell` through the latter. |
| 3 | H | §4 H4 | `b3PairSet_Raw` returns `const b3HashSet*`, but `b3HashSet.items` is a mutable pointer (`src/table.h`). Likewise, `b3SlotPool_GetIdPool` returns `const b3IdPool*`, whose `freeArray.data` remains mutable (`src/id_pool.h`, `src/container.h`). Callers can change membership or free-list order without a journal call, defeating the binding design §5.2’s compile-enforced boundary. | Replace both raw-header getters with value or const-pointee read APIs for serialization and hashing. |

### 3. Checked, no change

- The ring test’s stated widening ticks, 32-tick window, and over-budget two-image cases follow the ring doc §§5–7.
- The phase-0 capture point follows the step’s final migration drain, and the serializer’s omitted queue and event state is accounted for.
- H5 images the awake tree and H6 derives the kinematic tree in the order required by the trees doc §9.
- The named test runner, benchmark scene count, compiler-boundary examples, and principal source write routes match the live source.

### 4. Proposed edits

Correct §2’s material-storage claim; add explicit no-entry shape material restore routes to H3a/H4; replace H4’s raw pair-set and id-pool getters with read APIs that do not expose mutable storage.

### 5. Unresolved

The trees doc §7 itself relies on `const b3DynamicTree*` to prevent outside writes, but `b3DynamicTree` contains mutable `nodes`, `parents`, and `proxies` pointers (`include/box3d/types.h`). This is a concern with the binding design’s stated boundary, rather than a finding against this prompt.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `b3CreateShapeInternal` (`src/shape.c`) allocates a compound's block from
   `compound->materialCount` whatever that count is, allocates for a def with more than one
   material, and stores one def material inline with `materials` NULL. §2's sentence read as
   "count 1 means inline". Fix: §2 states the three cases and that inline storage is told by the
   pointer; H3a's material writer says the same.
2. **Adopted (H).** H3a gave `b3Shape_SetMaterialBlock` to creation, to H4's detach and attach
   and to the serializer's reader, and said it runs under the restore mark with no entry; H4
   says block mutators journal; the mark is read by assertions only (§3), so one function could
   not do both. Fix: the H3b pattern, a forward writer for creation (whose entry is the slot-pool
   push) and restore-only `b3Shape_LoadBlocks` and `b3Shape_FreeBlocks` for the reader; H4's
   journaling sentence names the two block mutators that journal and excludes the loads.
3. **Adopted (H).** `b3HashSet.items` (`src/table.h`) and `b3IdPool.freeArray.data`
   (`src/id_pool.h`, `src/container.h`) are mutable pointers inside the structs the two getters
   returned by const pointer, so a caller could write membership or the free order past the
   journal. `b3SerHashSet` and `b3SerIdPool` (`src/world_snapshot.c`) read capacity, count,
   items and `nextIndex`, free ids. Fix: two value views with const element pointers replace the
   getters; nothing else referenced them.

Checked, no change items agree with the source read for the revision. The Unresolved item
repeats round 35's trees doc §7 concern, recorded there for the owner; no change here.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round of round 48's clean result found three
  findings, so the series is not converged (U-P9, U-S8); a fresh clean round and its
  confirmation are required.
- Round 49: 3 findings (H2-M1-L0), 3 applied, 0 declined. Series: 162 findings, 161 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  pending
- Tripwire: fixes add or remove a named mechanism: `b3Shape_LoadBlocks` (the shape's restore
  load, the sibling H3b already had) and two value views in place of two raw getters.
  Clustering: §4 H3a's block bullet (rounds 12, 15, 19, 29, 34, 49; finding 2 is a gap left when
  round 27 made detach and attach transfers of their own and the reader's route stayed on the
  forward writer), §4 H4's container sentence (rounds 7, 17, 21, 49; finding 3 is a consequence
  of round 21's raw getters), §2's shape inventory (rounds 2, 49; finding 1 present since
  revision 1). Two of three findings are consequences of earlier fixes. Each fix is a clause or
  a short sentence. Reviewer read of round records or changelog: none in the raw log. Doc
  length: 1783 -> 1790 lines. Above the split-check length (U-T6): no new decision, no split.
- Third clean round followed by a non-clean confirmation (35/36, 41/42, 48/49); this one holds
  two Highs, both boundary facts (a route, a getter type), none against a design clause.
