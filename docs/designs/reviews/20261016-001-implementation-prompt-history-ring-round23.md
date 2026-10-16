---
title: Review round 23 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round22.md
mode: full pass
---

# Round 23

Target at revision 23 (1611 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T11:40:37+10:00 to 11:48:33+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1425e-4791-7303-8bc6-d8a5bbb3ebe0`. The stream emitted the final message twice; the last copy
follows (sections numbered as bold list items, as sent).

## Review report (Codex final message)

1. **Summary**

The prompt tracks the binding designs and most source write routes closely. Two test and measurement instructions need changes: callback fixtures can pass without exercising the callbacks, and the million-body cost fixture includes a lasting awake-tree cost that can distort its acceptance result.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1, H5; §6 | `HistoryReplayTest` registers custom-filter and `preSolve` callbacks but does not require shapes to enable them. Both flags default off; `src/broad_phase.c`, `src/contact.c` and `src/solver.c` gate callback calls on those flags. The H5 bullet case likewise lacks an explicit `preSolve` flag and rejection witness. Hash and event equality could pass without testing the callback-dependent clauses assigned in §6. | Set `enableCustomFiltering` when creating a relevant shape, enable `preSolve` on a colliding dynamic shape, and assert callback calls and the intended rejection in each fixture. |
| 2 | M | §7 | The million-body cost scene creates a 4,096-body slab awake before letting it sleep, then applies the **<1 µs per awake body** target with sixteen awake bodies. The awake tree’s `proxyCapacity` never shrinks and its full proxy array is imaged (trees doc §5; `src/dynamic_tree.c`), so that slab leaves thousands of proxy slots in every image. A correct implementation can miss the stated ratio because of this permitted high-water cost. | Measure the million-body target with its awake-tree high-water mark kept near sixteen; time the 4,096-proxy undone wake in a separate scene and report its rebuild cost separately. |

3. **Checked, no change**

- The nine-commit count, phase boundaries and API names match the three designs.
- The ring’s stated widening, minimum-window and rewind cases match the ring doc §§3–9.
- The serializer omissions and the described proxy write routes match the live source.
- The H2–H4 compile boundaries have corresponding source accesses; §3’s compiler-identified-file rule covers additional affected files.

4. **Proposed edits**

- Make callback enablement and observed callback outcomes explicit in the H1 and H5 test recipes.
- Separate the sleeping-tree rebuild fixture from the million-body capture acceptance fixture.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `enablePreSolveEvents` and `enableCustomFiltering` default off in
   `b3ShapeDef` (`include/box3d/types.h`) and gate the calls (`src/contact.c`, `src/solver.c`,
   `src/broad_phase.c`); the fixtures registered the callbacks without the flags, so the clauses
   §6 assigns to them could pass unexercised. Fix: the flags are set on the shapes involved and the
   callbacks count calls and rejections, asserted above zero.
2. **Adopted (M).** Round 15's slab is created awake, lifting the awake tree's `proxyCapacity`
   past 4,096 for the scene's life; the capacity never shrinks and the proxy array is imaged whole
   (the trees doc §5), so the sixteen-awake capture target could miss for a reason the design
   permits. Fix: the slab moves to a scene of its own, reported but not held to the ratio; the
   kinematic move stays in the capture-target scene.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 23: 2 findings (H1-M1-L0), 2 applied, 0 declined. Series: 99 findings, 99 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: the callback fixtures
  (round 23, first time), §7's million-body fixture (rounds 9, 12, 15, 16, 17, 19, 23: finding 2
  is a consequence of round 15's slab). Finding 1 of revision 1. Each fix is a clause or a short
  paragraph. Reviewer read of round records or changelog: none in the raw log. Doc length: 1611
  -> 1623 lines. Above the split-check length (U-T6): no new decision, no split.
