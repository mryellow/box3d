---
title: Review round 17 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round16.md
mode: full pass
---

# Round 17

Target at revision 17 (1534 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T10:44:42+10:00 to 10:51:24+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1422b-18b8-7162-b116-f90261af615c`. The stream emitted the final message twice; the last copy
follows (sections numbered as bold list items, as sent).

## Review report (Codex final message)

1. **Summary**

The prompt follows the three designs on the ring’s main semantics, capture point, and tree phases. Four implementation details need correction before handoff: two leave hidden owned blocks without a stated write route, one test cannot exercise the proxy ID change it describes, and one measurement assumes a tree rebuild its fixture does not force.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3a–H3b | The serializer still loads and frees shape material blocks, contact manifolds, and mesh caches in `src/world_snapshot.c`. H3a and H3b hide their storage and make the record restore writers skip those fields, but specify only sparse-array count loaders. The required snapshot load and teardown routes are absent, so these boundary commits cannot preserve the stated green build and restore behavior as written. | Specify file-local owned-block load and teardown functions in `src/shape.c` and `src/contact.c`, and route the serializer through them. |
| 2 | H | §4 H4 | `src/slot_pool.c` is told to move and clear a freed shape’s materials and a contact’s blocks. The H3 accessors deliberately cannot write those fields, and the only record-file callbacks H4 names are push-zeroed and truncate. The pool free operation therefore has no stated route through the owning files. | Add explicit ownership-transfer callbacks to the slot-pool contract for shape and contact records, including undo reattachment. |
| 3 | M | §4 H6; §6 | The kinematic filter case says a destroy-and-recreate may give the proxy a different ID. In `src/shape.c`, that call destroys and immediately recreates it in the same tree; `src/dynamic_tree.c` puts the freed ID at the head of its LIFO list, so the recreate takes that same ID. As written, the case does not demonstrate restoration of a changed kinematic proxy ID. | Remove the ID claim from the filter case. Add separate kinematic proxy churn across T and P that reuses an ID, and assert the restored identity; use a category-bit change to check the filter’s proxy payload. |
| 4 | M | §7 | The million-body timing fixture assumes undoing a wake of 4,096 proxies fires the sleeping tree’s full rebuild. `src/broad_phase.c` fires it only when height exceeds 64; the stated island size and insertion path do not establish that height. The harness may be unable to produce the required separate timing. | Construct and verify a deterministic threshold-crossing state before timing, using the rebuild count as the witness, or make the timing conditional and report when no rebuild occurred. |

3. **Checked, no change**

- The ring test’s interval-widening and `tickCount` window expectations agree with the ring doc §§5–7.
- The proposed capture point follows `b3BroadPhase_EndTraversal` and proxy-tree validation in `src/physics_world.c`.
- The source supports the prompt’s filter observation: `invokeContacts` true currently recreates the proxy.
- The serializer’s count-only buffer supports the stated two-pass sizing mechanism.
- The benchmark table has 17 scenes, and the named test runner accepts exact entry names.

4. **Proposed edits**

Resolve findings 1 and 2 by naming the owning-file functions and their call order in H3a, H3b, and H4. Revise the H6 fixture and §6 evidence claim to test actual ID reuse. Make §7’s rebuild timing depend on an observed threshold crossing.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `src/world_snapshot.c` allocates and fills a shape's material block and a
   contact's manifold block and cache in its reader and frees them in `b3FreeLiveSimElements`;
   H3a's restore writer skips `materials` and `materialCount`, H3b's skips the blocks, and
   neither commit named the route the reader and the teardown take once the fields are const.
   Fix: `b3Shape_SetMaterialBlock` is the material block's one writer (creation, the reader, the
   pool's callbacks), `b3Shape_FreeBlocks` the teardown; `b3Contact_LoadBlocks` and
   `b3Contact_FreeBlocks` likewise; all under the restore mark, no entry.
2. **Adopted (H).** H4's pool free "moves the record's owned blocks" and clears its pointers, which
   no function outside the owning file can do after H3; the pool held push-zeroed and truncate
   only. Fix: the record files install a detach and an attach beside them; free calls detach and
   the undo of a free calls attach.
3. **Adopted (M).** The tree's proxy allocation takes the head of its free list and a free pushes
   to the head (`src/dynamic_tree.c`), so the destroy-then-create of `b3ResetProxy` returns the
   same id; the filter case could not show a changed id. Fix: the filter case checks the leaf's
   payload; a new case destroys a kinematic shape and creates another in the next step, whose
   proxy takes the freed id, and checks the restored `proxyKey`.
4. **Adopted (M).** The full rebuild fires when the height exceeds 64 after the insertions
   (`src/broad_phase.c`), which 4,096 one-by-one inserts into a large grid need not reach. Fix:
   the harness reads the height and whether the rebuild ran; the timing is reported from this
   scene only when it did, otherwise from the rewind test's height-trigger case under the same
   scratch build.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 17: 4 findings (H2-M2-L0), 4 applied, 0 declined. Series: 79 findings, 79 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: the block writers' restore forms and teardowns
  (the design §5.2's file-local loads for the owned blocks, which the prompt had named for the
  containers but not the blocks) and the slot pool's detach and attach callbacks (the design's
  pool free entry needs the owning file to hand the blocks over; no file else can). Both are the
  design's shapes, no shadow structure (AC-1). Clustering: §4 H3a (rounds 3, 5, 8, 14, 15, 17),
  §4 H3b (rounds 3, 11, 17), §4 H4's slot pool (rounds 7, 17), §4 H6's kinematic cases (rounds 15,
  16, 17), §7's slab (rounds 15, 16, 17). Findings 3 and 4 are consequences of rounds 6/15 and
  15/16 respectively; findings 1 and 2 of revision 1, which named the restore writers' exclusions
  without the blocks' own routes. Each fix is a clause or a short paragraph. Reviewer read of
  round records or changelog: none (the changelog paths appear in a `wc -l` listing only). Doc
  length: 1534 -> 1555 lines. Above the split-check length (U-T6): no new decision, no split.
