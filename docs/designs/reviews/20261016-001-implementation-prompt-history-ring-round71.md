---
title: Review round 71 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round70.md
mode: full pass
session: 01a144b3-f311-7162-a96d-9b2b80f2d9bb (resumed to finalize)
---

# Round 71

Target at revision 66 (2006 lines). Confirmation round after round 70 (U-P9).

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
Run: foreground, `timeout 540`, exit 124 (work timeout) after 71 source reads,
2026-10-16T22:33:26+10:00 to 22:42:26+10:00; resumed with `resume <session-id>` and the
finalize instruction (no further tool calls), exit 0, 22:42:26 to 22:42:58 (U-R8 provenance).
Header confirmed `model: gpt-6-sol`, `reasoning effort: high` on both runs, session
`01a144b3-f311-7162-a96d-9b2b80f2d9bb`. The report below is the resumed run's final message.

## Review report (Codex final message)

### 1. Summary

The implementation sequence is largely consistent with the three binding designs and the live source. Two H4–H5 instructions need correction before handoff: a journal-only undo is described as leaving a usable world, and a geometry-registry test is required to run unchanged after the registry is removed.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H4, `HistoryJournalTest` | The prompt says both direct undo helpers leave the ring consistent with the world and that a test may call either and then disable history. Its `ProxyIdentityCase` acknowledges that `UndoPending` leaves the awake tree at the later state until the image loads. The design §9 likewise requires an image copy after the journal walk for awake state. Disabling at that point can leave a world whose records, solver sets and trees disagree. | State that direct journal undo is a test-only intermediate state. Require the image restore before disabling or otherwise using the world; keep journal-only tests confined to genuinely cold changes. |
| 2 | M | §4 H1 and H5, `HistoryRingTest` case 11 | Case 11 depends on `b3History_GetGeometryBytes` measuring the phase-0 registry. H5 removes that registry yet requires `HistoryRingTest` to run unchanged. Removing the getter breaks the build; retaining it with a zero result makes case 11’s bounds relative to `g = 0` vacuous. | Limit the registry-lifetime case and getter to H1–H4. At H5, remove or explicitly replace that case while continuing to run the remaining ring tests. |

### 3. Checked, no change

- The ring test’s stated window and interval sequences match the ring doc §§5–7.
- The source supports the stated filter-recreation behavior, contact block ownership, traversal count and step-tail capture point (`src/shape.c`, `src/contact.c`, `src/physics_world.c`).
- The H2–H4 compile-boundary examples target fields or qualifiers that the increments actually hide.
- The awake, static, sleeping and kinematic tree treatment in H4–H6 follows the trees doc §§5–9.

### 4. Proposed edits

- Correct H4’s direct-undo helper contract and its permitted test cleanup sequence.
- Make H5’s removal of the phase-0 geometry getter and test case explicit.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Finding 1 is against the
test contract revision 1 wrote as round 63's case 4 change left it; finding 2 against round
58's case 11 beside revision 1's H5 sentence.

1. **Adopted (M).** The `HistoryJournalTest` preamble said a test may call either direct undo
   helper and then disable, while case 3's contact part and case 4's third part (round 63) call
   a helper after a hot write and the design §9 step 3's image copy is what brings the awake
   structures back. Fix: the preamble distinguishes the two: after cold writes alone the world
   is at the target; after a hot write it is an intermediate state and the case calls
   `b3World_Rewind` before any other use or the disable, which both cases already do.
2. **Adopted (M).** H5 required `HistoryRingTest` to run unchanged while removing the registry
   that case 11 measures through `b3History_GetGeometryBytes`. Fix: H5's sentence names cases 1
   to 10 and 12 and deletes case 11 and the getter in the same commit as the registry (U-F5).
   §7's phase-0 figures use the getter at H1 only and stand.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation of round 70's clean round failed (U-P9); not
  converged (U-S8). Five clean rounds (35, 41, 48, 64, 70), each followed by a confirmation
  with findings (four, three, three, two, two).
- Round 71: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 207 findings, 206 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> 0/0/0 -> 0/2/0 -> 1/1/1 -> 0/2/0 -> 1/2/0 -> 0/2/0 ->
  0/0/0 -> 0/2/0 -> pending
- Tripwire: no mechanism added or removed; fix 1 restates a test contract, fix 2 deletes a case
  and a getter at H5. Both findings follow from this series' own fixes (rounds 63 and 58).
  H1's registry text has now drawn eleven findings (58, 59, 60, 61, 69, 71), all phase-0
  plumbing H5 removes; reported under U-S4. Reviewer read of round records or changelog: none
  in the raw log. Doc length: 2006 -> 2011 lines. Above the split-check length (U-T6): no new
  decision, no split.
