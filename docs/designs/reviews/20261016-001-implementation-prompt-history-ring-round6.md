---
title: Review round 6 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round5.md
mode: full pass
---

# Round 6

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:13:33+10:00 to 07:18:18+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14169-c887-7cf3-a8a3-9172f90fd925`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt gives the implementer a detailed sequence, but H4 contains a test fixture that exercises an awake transition while claiming to test journal-only undo. Three implementation routes also need to be made explicit before the increments can be carried out as written.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4 | `ColdChurnUndoCase` and `OwnershipCase` destroy a sleeping body with a touching mesh contact, then expect `b3History_UndoPending` alone to restore the hash. `b3DestroyBody` destroys contacts with `wakeBodies = true`; `b3DestroyContact` wakes both bodies when the contact is touching (`src/body.c`, `src/contact.c`). That changes the awake set and tree, which H4’s journal-only walk does not restore. This also contradicts §2’s narrower cold list. | Replace that operation with a genuinely cold contact-removal fixture, or test it through full `Rewind` and stop claiming it proves walk-only restoration. |
| 2 | H | §4 H3a–H4 | H3 makes each record array’s storage header private to its record file. H4 then assigns `src/slot_pool.c` the bump operation that pushes into those arrays, including the paired shape and fat-AABB arrays. With those headers private, `slot_pool.c` has no stated route to perform the operation; exposing their storage there would break the specified boundary and the design §5.2 rule. | Specify owner-file operations that the slot pool calls, or revise the storage ownership across H3 and H4 while preserving the design’s single write boundary. |
| 3 | M | §4 H4 | Making `pairSet` a pointer to const storage removes `src/world_snapshot.c`’s existing deserialize route: `b3DesHashSet` writes through `&bp->pairSet`. H4 specifies a broad-phase load route for trees but none for the pair set, although recording and replay still use this reader (`src/world_snapshot.c`). | Add an explicit broad-phase pair-set load operation and route snapshot deserialization through it without journaling a restore. |
| 4 | M | §4 H1 | The phase-0 writer must decide whether an image fits the arena *before* serializing directly into it. The stated direct-write-or-scratch procedure gives no sizing pass. `b3RecBufAppend` grows an overflowing `b3RecBuffer` with `b3GrowAlloc`; using an arena subregion as that buffer would try to grow/free memory it does not own (`src/recording.c`). | Specify an exact count pass, using the existing `b3RecBuffer.countOnly` facility, before selecting a bounded arena view or an owned scratch buffer. |

### 3. Checked, no change

- The ring cases’ stated interval widening and window expectations agree with the ring doc §§5–7.
- The `b3Shape_SetFilter` observation in §2 matches `src/shape.c`: the category comparison occurs after assignment.
- The step capture point follows the final migration drain in `src/physics_world.c`, as required by the design §6.
- H5 and H6’s 256 KB cost cases target the respective phase-0 whole-world image and phase-1 kinematic-tree image.
- The H2 and H3 compile-boundary examples target accesses that exist before their stated commits.

### 4. Proposed edits

Correct the two H4 fixtures and specify the slot-pool, pair-set load, and phase-0 sizing routes in their increment mechanics. Update the affected test-to-requirement entries in §6 to match what the corrected fixtures demonstrate.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** `b3DestroyBody` destroys the body's contacts with `wakeBodies` true and
   `b3DestroyContact` (`src/contact.c`) wakes both bodies of a touching contact, the destroyed
   body's own set with them, so the mesh-contact body's destroy is not cold; revision 6's §2 list
   said a sleeping body with no contact, and the fixtures broke their own rule. Fix: §2 names the
   reason beside the rule; `ColdChurnUndoCase` drops the destroy; `OwnershipCase` is a cold part
   through the walk and a contact part through `b3World_Rewind`, each with its own checks.
2. **Adopted (H).** H3a put the bump push in the record file and H4 said the slot pool "folds it
   in", which would need the private storage headers in `src/slot_pool.c`. Fix: each record file
   exposes push-zeroed and truncate; the slot pool holds the `b3IdPool` and those two functions,
   installed at creation, and the headers stay where H3 put them.
3. **Adopted (M).** `b3DesHashSet( r, &bp->pairSet )` (`src/world_snapshot.c`) writes the pair set
   in place; with a pointer-to-const it needs a route like the trees' load. Fix: H4 names
   `b3BroadPhase_LoadPairSet`, a restore that journals nothing, beside `b3BroadPhase_LoadTree`.
4. **Adopted (M).** `b3RecBufAppend` grows an overflowing `b3RecBuffer` with `b3GrowAlloc`
   (`src/recording.c`), so a buffer over an arena region would try to free memory it does not own,
   and the ring doc §6 sizes the slot before the budget steps anyway. `b3RecBuffer.countOnly`
   tallies without writing (`src/recording.h`). Fix: a count-only serialization sizes the slot, the
   budget, window and arena growth use the count, and a second run writes into a buffer whose
   capacity is exactly that count.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 6: 4 findings (H2-M2-L0), 4 applied, 0 declined. Series: 32 findings, 32 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: one load function for the pair set on the
  trees' pattern; the slot pool's two installed functions replace an implied access to private
  storage, which is the design's boundary, not a new mechanism; the count-only pass uses an
  existing facility. Clustering: §4 H4 again (rounds 2, 4, 5, 6) and the journal tests' fixtures
  for the third round running (U-S4); finding 1 is a consequence of round 5's fix 2, which moved
  the fixtures onto a cold list that itself had one entry wrong. The fixes are clauses; the
  fixture is now drawn from a rule stated with its source reason, so a fourth recurrence would be
  the signal to pause (U-S4). Reviewer read of round records or changelog: none in the raw log.
  Doc length: 1351 -> 1361 lines; the fixes add two lines (1), two (2), two (3) and three (4),
  with one line added to §2's rule. Above the split-check length (U-T6): one decision, no split.
