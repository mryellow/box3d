---
title: Review round 10 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round9.md
mode: full pass
---

# Round 10

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:43:14+10:00 to 07:47:18+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14184-f31b-7e71-9ac0-a54ef21d6ce1`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is close to implementable, but H1 leaves live borrowed-geometry pointers dangling when history is disabled after a rewind. I also found a compiler-dependent H4 boundary check and one incorrect writer count.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1 | Phase-0 restore makes live mesh, height-field and compound shapes point into the ring reader’s geometry slots (`src/world_snapshot.c`). H1 then says `b3World_DisableHistory` frees those slots while the world remains live. A later step or query can read freed memory. The H1 tests disable history only immediately before destruction, so they miss this. | Specify how live shapes retain valid geometry after disable, and test rewind → disable → continued simulation with each borrowed geometry type. |
| 2 | M | §2, §6, §4 H4 | The H4 boundary check says `b3AddKey(&world->broadPhase.pairSet, 0)` must fail to compile after `pairSet` becomes an incomplete-type pointer. With the stated Clang flags, this is an `-Wincompatible-pointer-types` warning, not an error; the reproduced Clang 19 C17 compile exits successfully. Thus this check does not establish the claimed boundary on Clang. | Use a statement that is a hard error on every supported compiler, or promote the actual incompatible-pointer diagnostic to an error. |
| 3 | L | §4 H2 | H2 says `splitIslandId` has four write sites, but §2 itself lists five: two in `src/island.c`, two in `src/solver.c`, and one in `src/solver_set.c`. | Change “four” to “five” and retain all five in the migration checklist. |

### 3. Checked, no change

- The ring’s window and widening rules support H1’s stated retained-tick sequences.
- The H1 capture point matches the end of `b3World_Step` after the migration drain.
- The serializer’s omitted migration queue and event state match the phase-0 additions.
- The H2 and H3 compile checks align with the current getter and record access types.
- The H5 and H6 cost bounds distinguish serialized, raw kinematic-tree and derived-tree images as claimed.

### 4. Proposed edits

Resolve finding 1 in H1’s disable mechanics and add its lifetime test. Correct the H4 compiler gate and H2 writer count before handing the prompt to an implementer.

### 5. Unresolved

The binding designs do not choose how phase-0 geometry copies remain valid in a live world after `DisableHistory`. That ownership choice needs an explicit decision before H1 can be implemented.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** §2 itself said a restored mesh, height field or compound shape points at the
   reader's slot bytes, and H1 said disable frees the reader; a world that rewinds, disables and
   steps would read freed memory. The reviewer's Unresolved item says the designs do not choose the
   geometry's owner after disable. They do decide the premise: the design §10 makes the caller
   keep borrowed geometry alive while a retained tick can restore its owner, so the pointer a shape
   held at T is valid at a rewind to T. Fix: the phase-0 slot carries each live borrowed-geometry
   shape's pointer beside the image, and the rewind repoints each restored shape at the recorded
   pointer through its restore writer, so nothing live points into the registry and disable frees
   it; `HistoryReplayTest`'s third scene disables after a rewind and steps on with casts through
   the mesh and the compound. The decision and the two alternatives not taken (leave shapes in the
   registry; hand the registry's copies to the world at disable) are in §10. H5 removes the words
   with the registry.
2. **Adopted (M).** Clang in C17 treats an incompatible pointer type as a warning
   (`-Wincompatible-pointer-types`), and H2's flags promote only the discards-qualifiers form, so
   `b3AddKey( &world->broadPhase.pairSet, 0 )` would compile on Clang at H4. Fix: the statement is
   `world->broadPhase.pairSet.count = 0;`, a member of a pointer to an incomplete type, an error on
   every compiler; §2's compiler-flags paragraph says which diagnostic classes the boundary
   statements may rely on.
3. **Adopted (L).** §2 lists five `splitIslandId` writers (two in `src/island.c`, two in
   `src/solver.c`, one in `src/solver_set.c`); H2 said four. Fix: five.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 10: 3 findings (H1-M1-L1), 3 applied, 0 declined. Series: 46 findings, 46 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: the phase-0 slot gains the geometry pointer
  words beside the queue copy (scaffolding H5 removes, on the queue's existing pattern); nothing
  else. Clustering: §4 H1 (rounds 1, 2, 4, 6, 10) and §4 H4's compile row; finding 1 is a
  consequence of revision 1's phase-0 registry decision, not of a round's fix; finding 2 is a
  consequence of round 8's fix 1 (the restated diagnostic); finding 3 of round 7's fix 3. Each
  fix is a clause or a short paragraph; the Provenance entry is the one-paragraph reason the
  decision needed. Reviewer read of round records or changelog: none in the raw log. Doc length:
  1390 -> 1411 lines; the fixes add eighteen lines (1, six of them the Provenance entry), three
  (2) and none (3). Above the split-check length (U-T6): one decision, no split.
- Criteria review threshold: this is the tenth round without a clean round, which is the
  project's threshold (Parameters; B8). The series' 46 findings are all adopted, none reopened a
  Provenance decision, none named a shape AC-1 to AC-4 forbid, and none was declined; they are
  missed writer sites, fixtures that assumed cold what wakes, boundary statements with the wrong
  diagnostic, and increment mechanics the designs imply but the prompt had not spelled out. The
  owner decides whether Part B is reconsidered or the series continues at the same scope (U-S8).
