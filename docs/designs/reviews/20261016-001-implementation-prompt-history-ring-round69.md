---
title: Review round 69 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round68.md
mode: full pass
session: 01a144a4-eae8-7d72-b624-e87848e2f596
---

# Round 69

Target at revision 65 (2002 lines).

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
Run: foreground, `timeout 540`, exit 0 after 64 source reads, 2026-10-16T22:17:00+10:00 to
22:24:34+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a144a4-eae8-7d72-b624-e87848e2f596`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt is broadly aligned with the three binding designs and the live source, but two procedures need clarification before an implementing agent can follow them as written. One concerns phase-0 geometry release under budget pressure; the other prevents the specified H3b prior-commit test run from building.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1 | Reader slots are appended only when an image is written, but a sizing run that becomes unimaged releases new geometry through `b3RecReleaseSlot`. That geometry ID has no reader slot yet (`src/replay.h`). The required behavior for that call and the later slot gap is unstated. | Specify that releasing an ID beyond the reader’s current slot count is a no-op, and that the next write appends empty slots for all released IDs before appending live entries. |
| 2 | M | §3; §4 H3b | The prescribed prior-commit procedure copies all of H3b’s `test/test_history.c` into H3a to run new case 12. H3b also changes `HistoryHashTest` to call `b3Contact_Write` and `b3Contact_SetBlocks`, which do not exist in H3a. The scratch build therefore fails before case 12 can run, despite its stated pass-before role. | Give case 12 a separate prior-commit fixture, or specify a way to copy only that case while retaining H3a’s version of the other tests. |

### 3. Checked, no change

- The cited `src/` and `include/` baseline is unchanged in the live tree.
- The ring’s window, widening sequence, and minimum-window overrun cases in §4 agree with the ring doc §§5–8.
- The H4–H6 tree routes and restore pass agree with the trees doc §§5–9; the cited broad-phase and solver call sites exist.
- The CMake test and benchmark targets, test-name filtering, and public sensor overlap count API support the stated checks.

### 4. Proposed edits

Resolve the missing reader-slot behavior in H1 and replace H3b’s scratch-worktree instructions for case 12 with a buildable prior-commit procedure.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Finding 1 is against rounds
60 and 61's registry text; finding 2 against round 66's case 12.

1. **Adopted (M).** The reader's `slots` (`src/replay.h`) are appended at a write, one per
   entry added since the last write; an entry released after an unimaged sizing run is new in
   that run and has no slot, and round 61's text sent `b3RecReleaseSlot` to it anyway. An entry
   with a slot has been counted by a write, so a zero-count entry still in the map never has one.
   Fix: the sizing-only release frees the entry alone; the slot release stays for eviction and a
   rewind's drop; the next write appends the empty slot, as the text already said for released
   entries. No bounds branch is added.
2. **Adopted (M).** §3's before-run procedure copies the working tree's `test/test_history.c`
   into a scratch worktree at the previous commit for every commit that adds a case; H3b's file
   calls `b3Contact_Write` and `b3Contact_SetBlocks` from `HistoryHashTest`, new in H3b, so it
   does not build at H3a, and round 66's "passes at every commit" could not be carried out. Fix:
   case 12 is marked as having no before run with the reason, and §3 states the rule for such a
   case (run in the working tree alone, the closure doc says so).

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 69: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 205 findings, 204 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> 0/0/0 -> 0/2/0 -> 1/1/1 -> 0/2/0 -> 1/2/0 -> 0/2/0 -> pending
- Tripwire: no mechanism added or removed; fix 1 removes a call from one release path, fix 2 is
  a marking and a §3 clause. Clustering: both findings are consequences of this series' own
  fixes (rounds 60 and 61; round 66). H1's registry bullet has now drawn ten findings in rounds
  58 to 69 (58, 59, 60, 61, 69), the most of any passage, all phase-0 plumbing H5 removes;
  reported again under U-S4. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 2002 -> 2006 lines. Above the split-check length (U-T6): no new decision, no
  split.
