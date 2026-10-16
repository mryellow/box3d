---
title: Review round 21 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round20.md
mode: full pass
---

# Round 21

Target at revision 21 (1588 lines). The owner chose to continue the series after round 20's
report.

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T11:22:23+10:00 to 11:28:53+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1424d-9aeb-7a43-906d-2d64e487500f`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is largely aligned with the three binding designs and the cited source, but three handoff defects need correction. The most serious can leak renderer handles during phase-0 rewind after the shape array shrinks.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3a | With `releaseUserShapes` on, the proposed reader saves no handles, truncates the shape array through `b3Shape_LoadCount`, and relies on per-slot restore writes to release handles. A live slot removed by truncation receives no restore write, so its `userShape` is lost without calling `destroyDebugShape`. The current reader saves handles before resizing and sweeps all of them afterward (`src/world_snapshot.c`); the design §10 requires release on restore. | Release handles in slots that will be truncated before changing the count, or retain the existing full-array save-and-release sweep for this reader option. |
| 2 | M | §4 H1, `HistoryRingTest` cases 2–5 | The fixture says to read tick 0’s charge `s` *after* enabling history, then set `maxBytes` from `s`. The definition is supplied at enable and has no setter (the design §14), so these cases cannot use that value for the same enable as written. | Specify an unbounded probe enable, read `s`, disable, then enable the unchanged world with the calculated budget; verify the second tick-0 charge. |
| 3 | L | §2 | The statement that `b3Shape_SetFilter(..., invokeContacts=true)` “always destroys and recreates the proxy” omits disabled shapes. `b3ResetProxy` performs those operations only when `proxyKey` is non-null; otherwise it updates AABBs without a proxy operation (`src/shape.c`). | Qualify the claim to shapes with a live proxy and state the disabled-shape path. |

### 3. Checked, no change

- The stated step-tail capture point matches `src/physics_world.c` and the design §6.
- The filter’s always-true `destroyProxy` comparison is present in `src/shape.c` for shapes with a live proxy.
- The serializer’s handling of `userData`, `userShape`, and the missing migration queue matches §2 (`src/world_snapshot.c`).
- The ring test’s stated interval-widening sequence is consistent with the ring doc §6.

### 4. Proposed edits

Apply the actions in findings 1–3 before handing the prompt to the implementing agent.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** Round 9's fix had the reader with `releaseUserShapes` on save nothing and
   rely on the restore writes to release; a live slot above a shorter image's length gets no
   restore write once `b3Shape_LoadCount` truncates, so its handle would leak, against the design
   §10. Today's reader saves every handle before it resizes (`src/world_snapshot.c`). Fix: both
   options save and detach before the length changes; the on option releases every saved handle.
2. **Adopted (M).** `b3HistoryDef` is passed to `b3World_EnableHistory` and the design §14 has
   no setter, so a budget computed from a charge read after the enable cannot apply to that
   enable. Fix: a probe enable with an unbounded budget, disable, then the real enable, with the
   tick 0 charge asserted equal.
3. **Adopted (L).** `b3ResetProxy` destroys and recreates only when `proxyKey` is not null
   (`src/shape.c`); a disabled body's shape has no proxy. Fix: the claim is qualified.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 21: 3 findings (H1-M1-L1), 3 applied, 0 declined. Series: 92 findings, 92 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §4 H3a's reader (rounds 9,
  15, 21: finding 1 is a consequence of round 9's fix), `HistoryRingTest`'s budget cases (rounds
  4, 6, 18, 19, 21), §2's filter paragraph (rounds 5, 7, 21). Finding 2 and 3 of revision 1. Each
  fix is a clause or a short paragraph. Reviewer read of round records or changelog: none (the
  changelog paths appear in a `wc -l` listing only). Doc length: 1588 -> 1596 lines. Above the
  split-check length (U-T6): no new decision, no split.
