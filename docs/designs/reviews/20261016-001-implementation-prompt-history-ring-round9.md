---
title: Review round 9 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round8.md
mode: full pass
---

# Round 9

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:36:06+10:00 to 07:41:28+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1417e-6b45-7f81-878d-7c75f009b9c9`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

1. **Summary**

The prompt has two restore-path defects that could break rewinds, plus an incomplete source inventory for a world scalar. The ring arithmetic and the principal fail-before checks I examined agree with the binding designs.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3a | With `releaseUserShapes` off, H3a detaches only handles whose slot and generation still match. Its new `b3Shape_Restore` releases a live handle for a removed or replaced shape, while the serializer’s saved-handle sweep in `src/world_snapshot.c` releases that same handle again. | Detach **every** saved live handle before restore; reattach matching handles and release each remaining handle once. |
| 2 | H | §4 H4 | H4 journals all three hull-database functions whenever history is enabled, but its phase-0 rewind still calls `b3DeserializeIntoShell`. That reader removes live hull references and adds restored ones through those functions (`src/world_snapshot.c`, `src/physics_world.c`), creating new pending journal entries *during* rewind. §4’s rewind procedure never removes those entries. | Specify a restore-only, non-journaling route for hull refcount changes during deserialization, and check that repeated rewinds leave `pendingBytes` at zero. |
| 3 | M | §2; §4 H2 | The scalar-writer inventory attributes `compoundShapeCount` changes to shape creation and destruction, but `b3DestroyBody` also decrements it directly in `src/body.c`. H2’s “every writer” instructions omit that site. | Name `b3DestroyBody` explicitly in §2 and H2’s scalar migration. |

3. **Checked, no change**

- The ring’s stated window sizes and interval-widening examples match the ring doc §§5–7.
- The described filter behavior, proxy routes, serializer omissions, and test-runner filtering match the cited source.
- The H0/H1/H4 build-failure boundaries and H5/H6 cost-bound failures have a plausible pre-commit mechanism as written.
- The H2–H4 compile-boundary examples target access that their increments remove.

4. **Proposed edits**

Apply the three actions in the findings table. Add a serializer restore case with a removed or replaced debug shape handle, and an H4 repeated-rewind check on a world containing a hull.

5. **Unresolved**

None.

6. **Verdict: CHANGES_PROPOSED**

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `b3DesShapes` (`src/world_snapshot.c`) saves every live slot's handle and
   releases, in its sweep, each one it did not carry over. Revision 3's wording detached only the
   handles that would be carried over, so a live slot whose shape is replaced or removed kept its
   handle attached through the restore write, which releases it, and the sweep released it again.
   Fix: the reader detaches every saved handle before any restore write, puts the matching ones
   back and releases each remaining one once.
2. **Adopted (H).** `b3DeserializeIntoShell` calls `b3AddHullToDatabase` for each restored hull
   shape and `b3FreeLiveSimElements` calls `b3RemoveHullFromDatabase` (`src/world_snapshot.c`); from
   H4 both journal, so a phase-0 rewind would leave entries in the staging buffer that nothing
   undoes. The design §5.2 says the reader writes through the restore writers and the containers'
   file-local loads; the pools, arrays and bitsets the reader rebuilds are journaled containers
   too (U-G6). Fix: H4 states that every journaled container and the hull database offer a load
   route that makes no entry, which the reader uses from H4 and the scatter from H5;
   `HistoryRingTest` checks `pendingBytes` is 0 after every successful rewind, two in a row
   included, on its scene with a hull.
3. **Adopted (M).** `b3DestroyBody` (`src/body.c`) decrements `compoundShapeCount` directly. Fix: §2
   names the three sites; H2 refers to them.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 9: 3 findings (H2-M1-L0), 3 applied, 0 declined. Series: 43 findings, 43 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: the restore routes are named as a class (one
  load per journaled container, which the design already requires) with the hull database's two
  added to it; no mechanism beyond the design. Clustering: §4 H4 again (rounds 2, 4, 5, 6, 8, 9)
  and §4 H3a (rounds 2, 3, 9); finding 1 is a consequence of round 2's fix 2 (its detach rule was
  too narrow); finding 2 is a missing statement of a design rule in the increment that first needs
  it, and finding 3 a missed writer in §2. Each fix is a sentence. Nine rounds is one short of the
  criteria review threshold (10 rounds without converging); the finding count per round has been
  3 to 6 with no round at zero, every finding has been adopted, and none has reopened a Provenance
  decision, so the series is converging on completeness of the inventory and the increments'
  mechanics, which is what an Implementation Prompt's review is for. Reviewer read of round records
  or changelog: none in the raw log. Doc length: 1380 -> 1389 lines; the fixes add one line (1),
  eight (2) and one (3), one line removed by rewording. Above the split-check length (U-T6): one
  decision, no split.
