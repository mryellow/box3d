---
title: Review round 62 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round61.md
mode: full pass
session: 01a1445c-7a6c-7aa2-bed0-4cc7ccae4068
---

# Round 62

Target at revision 59 (1935 lines).

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
Run: foreground, `timeout 540`, exit 0 after 65 source reads, 2026-10-16T20:57:52+10:00 to
21:06:26+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1445c-7a6c-7aa2-bed0-4cc7ccae4068`. The prompt file was rebuilt verbatim from round 61's
record before the run (the session scratchpad had been cleared); the request is byte-identical to
round 1's text as recorded. The stream emitted the final message twice; the last copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt’s ring and restore sequence is consistent with the three binding designs in the paths checked. I found two documentation defects at the H3c graph boundary; neither changes the designs’ behavior.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §4 H3c | The proposed `b3GetGraphColor(world, index)` conflicts with the existing public `b3GetGraphColor(int)` in `include/box3d/types.h` and `src/constraint_graph.c`. C cannot provide both signatures. | Give the new record accessor a distinct name, such as `b3Graph_GetColorRecord`. |
| 2 | L | §2; §4 H3c | The `b3GetWorldByteCount` reader inventory omits its direct reads of graph bitsets and arrays in `src/physics_world.c`. `b3World_GetCounters` also reads graph colours directly. H3c hides that storage, but names no graph byte-count route. | Add these readers to §2 and specify their H3c route through const graph access or a graph byte-count function. |

### 3. Checked, no change

- The ring’s interval, budget, window, and rewind sequences agree with the ring doc.
- The awake-tree image, proxy journal, moved lists, and derived-tree restore pass agree with the trees doc.
- The stated shape-filter behavior, serializer omissions, test-runner filtering, and benchmark command behavior match the live source.
- The named H5 and H6 cost checks distinguish the earlier image paths from the intended later paths.

### 4. Proposed edits

Rename the H3c graph-record accessor and document how both graph readers in `src/physics_world.c` cross the new boundary.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Both are against H3c's graph
boundary, written in revision 1 and unchanged since; neither is a consequence of this block's
fixes.

1. **Adopted (L).** `b3HexColor b3GetGraphColor( int index )` is public (`include/box3d/types.h`,
   defined in `src/constraint_graph.c`), the debug-draw colour of a graph colour index; the H3c
   bullet named a `const b3GraphColor* b3GetGraphColor( world, index )`, a second signature C
   cannot give the same name. Fix: the record accessor is `b3GetGraphColorRecord( world, index )`,
   the form of `b3GetBodyRecord`, `b3GetContactRecord`, `b3GetJointRecord` and
   `b3GetIslandRecord`; the bullet names the public function as untouched.
2. **Adopted (L).** `b3GetWorldByteCount` (`src/physics_world.c`) reads `b3GetBitSetBytes` of
   each colour's `bodySet` and `b3Array_ByteCount` of each colour's three arrays through
   `world->constraintGraph.colors + i`, and `b3World_GetCounters` reads the three arrays' counts
   into `colorCounts` and `awakeContactCount` the same way; §2's inventory listed neither and H3c,
   which makes `constraintGraph` a pointer to an incomplete type, named no route for them.
   `b3GetBitSetBytes( b3BitSet* )` (`src/bitset.h`) takes a non-const pointer, so a const colour
   alone would not compile it. Fix: §2 lists both readers; H3c routes both through the record
   accessor, with `capacity` and `count` readable on `b3ConstArray` and `b3GetBitSetBytes`
   taking `const b3BitSet*`; `src/bitset.h` joins H3c's file list. The log's three graph lines
   keep their values.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 62: 2 findings (H0-M0-L2), 2 applied, 0 declined. Series: 191 findings, 190 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> pending
- Tripwire: no mechanism added or removed; fix 1 renames an accessor, fix 2 is two inventory
  sentences, a route clause and a file-list entry. Clustering: none with round 61 (H1's
  registry bullet, four rounds running through 61, drew nothing this round; §2's cold list
  nothing); both findings sit in H3c's graph bullet, last touched in revision 1, and neither
  follows from a previous fix. Lowest-severity round of the series: the first with no H and no
  M. Reviewer read of round records or changelog: none in the raw log. Doc length: 1935 ->
  1942 lines. Above the split-check length (U-T6): no new decision, no split.
