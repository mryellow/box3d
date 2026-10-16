---
title: Review round 39 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round38.md
mode: full pass
---

# Round 39

Target at revision 38 (1730 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T14:19:57+10:00 to 14:27:00+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142f0-28c6-7f53-bc53-a5cddcf75b20`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt gives the implementer a detailed path through the three binding designs, but four points need correction. The most consequential are an H4 test reader that will use the changed pair-set type incorrectly and an H0 hash rule that includes inactive cache values.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2; §4 H4 | `test/test_broad_phase.c` reads `&bp->pairSet` with `b3ContainsKey`. H4 changes `pairSet` from a `b3HashSet` value to a pointer to an incomplete type, but omits this known reader from its file list. The mismatched argument can remain a warning rather than the compile error on which §3 relies. | List the test file in H4 and route its check through `b3PairSet_Contains`. |
| 2 | M | §4 H0; §5 | The hash mixes all four `simplexCache.indexA` and `indexB` entries. `b3SimplexCache.count` identifies how many entries are stored (`include/box3d/types.h`); inactive entries are not semantic cache values. Hashing them conflicts with the design §12 field-wise semantic oracle and can reject equivalent state or a correct change. | Mix `count`, then only indices below `count`; make the sensitivity probe change an active index. |
| 3 | M | §4 H3c | The instruction names existing “`b3CreateJointInternal` set branches,” but the source has `b3CreateJoint`. It appends a sim and returns a writable pointer that the joint constructors fill afterward (`src/joint.c`). H3c simultaneously requires completed-value, single pushes and hides writable array access, without stating how those constructors complete the sim before the push. | Name the actual function and specify the constructor handoff that supplies a completed type-specific sim before appending it. |
| 4 | L | §2 | The traversal inventory says seven public query and cast functions. `src/physics_world.c` opens traversal in eight: two overlaps, `CollideMover`, two ray casts, `CastShape`, `CastMover`, and one further cast path. | Change the count to eight and name the functions in the inventory. |

### 3. Checked, no change

- The H1 ring test’s stated interval and window sequences agree with the ring doc §§6–7.
- The H1 capture point follows the step’s final migration drain in `src/physics_world.c`, as the design §6 requires.
- The H4 proxy journal and H5/H6 image versus derived-tree split follow the trees doc §§5–9.
- The serializer, recording, replay, and allocator mechanisms cited for H0/H1 exist in the named source files.

### 4. Proposed edits

Apply the four table actions, then rerun H4’s full test target and the H0 hash probes with the corrected cache walk.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `test/test_broad_phase.c` calls `b3ContainsKey( &bp->pairSet, ... )`; H3a's
   file list names the file but H4's, the commit that turns `pairSet` into a pointer to an
   incomplete type, did not. Fix: H4 lists it and routes the read through `b3PairSet_Contains`.
2. **Adopted (M).** `b3SimplexCache.count` is the number of stored simplex points
   (`include/box3d/types.h`); entries at or above it are not state the design §12's field-wise
   semantic hash should see. Fix: the rule mixes `count` and the first `count` entries; the two
   simplex probes write `indexA[0]` with `count` `ENSURE`d at least 1 first.
3. **Adopted (M).** The function is the static `b3CreateJoint` of `src/joint.c`, returning a
   `b3JointPair` whose sim the typed constructors (`b3CreateRevoluteJoint` and the others) fill
   afterwards; round 33's text named a function that does not exist and left the type-specific
   fill unplaced. Fix: `b3CreateJoint` returns the sim by value with the common fields, the
   constructor fills its union member, and a second function of the file runs today's branches
   (push, the sleeping branch's merge, the link) on the completed value.
4. **Declined (L).** `b3BroadPhase_BeginTraversal` (`src/broad_phase.c`) has ten callers in
   `src/physics_world.c`, grepped uncapped (U-E4): `b3World_Step`, `b3World_Draw`,
   `b3World_Explode`, `b3World_OverlapAABB`, `b3World_OverlapShape`, `b3World_CollideMover`,
   `b3World_CastRay`, `b3World_CastRayClosest`, `b3World_CastShape` and `b3World_CastMover`. The
   seven public queries and casts are seven; the proposed eighth cast path does not exist. The
   seven are now named in §2 so the claim is checkable, and the reasoning sits in §10 (U-V10,
   U-C4). First declined finding of the series.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8; a round with a decline and adopted findings
  is not clean).
- Round 39: 4 findings (H0-M3-L1), 3 applied, 1 declined. Series: 141 findings, 140 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  pending
- Tripwire: fixes add or remove a named mechanism: the joint constructors' second function in
  `src/joint.c` (the push site H3c already required, now placed). Clustering: §4 H3c's
  constructor sentence (rounds 33, 39; finding 3 is a consequence of round 33's fix), §4 H0's
  cache rule and probe (rounds 32, 33, 36, 38, 39; finding 2 is a consequence of round 32's
  field list), §4 H4's file list (rounds 7, 17, 21, 27, 30, 39; finding 1 present since revision
  1), §2's traversal inventory (round 39, declined). Two of three adopted findings are
  consequences of the series' own fixes; the cache rule has now produced a finding in five of the
  last eight rounds (U-S4: each fix was a clause; the rule and probe are now stated to the field,
  and a recurrence is reported). Each fix is a clause or a short sentence; the Provenance entry
  is three lines. Reviewer read of round records or changelog: none in the raw log. Doc length:
  1730 -> 1738 lines. Above the split-check length (U-T6): no new decision, no split.
- First round without a High since the clean round 35; first declined finding of the series.
