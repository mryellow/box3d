---
title: Review round 22 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round21.md
mode: full pass
---

# Round 22

Target at revision 22 (1596 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T11:30:42+10:00 to 11:38:44+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14255-32c5-72a1-8559-487b727ba6cd`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three binding designs, but several instructions would give the implementing agent incorrect or incomplete verification evidence. The most consequential defect is the event digest: as specified, it can miss a change from a contact-begin event to a contact-end event.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1; §§5–6 | The event digest mixes each event’s fields but does not specify an event-type tag or a count for each event array. Contact-begin and contact-end events have the same fields in `include/box3d/types.h`; exchanging one for the other can leave the digest unchanged, despite the claimed type comparison. | Mix each array’s type and count before its field-wise events. |
| 2 | M | §4 H5–H6; §7 | The sensor sweep fixtures enable sensor events on visitors but do not require enabling them on the sensor shape. `src/sensor.c` rejects overlaps unless **both** shapes have that flag; it is false by default. The §7 fixture likewise cannot establish its claimed nonzero sensor term as specified. | Set the flag on sensors and visitors, and assert the expected overlap count. |
| 3 | M | §4 H1 | `HistoryRingTest` cases 2–5 use tick 0’s charge `s` and require every later image to have the same charge. The stated scene includes bodies created asleep: `src/shape.c` parks their marked proxies and queues the bodies, and the first step drains that queue. Tick 0 therefore has queue bytes that later images lack, so the equality check can fail after H1. | Drain migrations before enabling history, then probe `s` on the settled scene. |
| 4 | M | §2 | The list of public routes that wake a sleeping body omits `b3Body_EnableSleep(false)`, `b3World_EnableSleeping(false)`, `b3Joint_WakeBodies`, and conditional `b3Shape_ApplyWind(..., wake=true)` calls. `src/body.c`, `src/physics_world.c`, `src/joint.c`, and `src/shape.c` contain those routes. | Complete the inventory and distinguish actual wakes from shape changes that park a proxy while its body remains asleep. |
| 5 | L | §4 H2, H3c | H2 says H4 replaces the step stages’ direct array access, while H3c requires those stages to use hot element accessors when it hides the arrays. Following the H2 instruction would leave H3c unable to compile. | Change the H2 reference from H4 to H3c. |

### 3. Checked, no change

- The ring’s interval widening, minimum window, eviction, and rewind-discard instructions agree with the ring doc.
- H4’s reverse journal walk and ownership transfers agree with the design; H5 and H6 assign the trees to the specified image and restore paths.
- The stated test-runner filtering, benchmark `-t=8` behavior, validation configuration, and compiler-warning flags match the live build files.
- The cited serializer omissions, proxy creation paths, and material and manifold ownership routes match the live source, apart from the findings above.

### 4. Proposed edits

Apply the five actions in the findings table. For the sensor fixtures, specify both shape flags and an overlap assertion so an empty sweep cannot pass as tree-order evidence.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `b3ContactBeginTouchEvent` and `b3ContactEndTouchEvent` carry the same
   fields (`include/box3d/types.h`); the digest mixed fields in reported order with no kind tag or
   count, so a begin exchanged for an end at the same position left it unchanged while the design
   §12 test 3 compares by type. Fix: each array is tagged with its kind and count first.
2. **Adopted (M).** The sensor pass skips a sensor shape without `b3_enableSensorEvents` and a
   visitor without it (`src/sensor.c`), and the flag is off by default; the fixtures set it on
   visitors only, so the sweeps could be empty and the §7 sensor column zero. Fix: both shapes carry
   the flag and the overlap count is asserted.
3. **Adopted (M).** The ring test's scene creates eight boxes asleep, which park marked proxies and
   queue the bodies until the first step drains them (`src/shape.c`); the budget variant inherited
   them, so tick 0's image carried queue bytes the later images lack and the equal-charge assertion
   would fail. Fix: the variant creates every body awake and steps once before the probe.
4. **Adopted (M).** `b3Body_EnableSleep( false )` wakes (`src/body.c`), `b3World_EnableSleeping(
   false )` wakes every sleeping set (`src/physics_world.c`), `b3Joint_WakeBodies` wakes
   (`src/joint.c`), `b3Shape_ApplyWind` wakes when `wake` is true (`src/shape.c`); none was in
   §2's list. Fix: added. The journal tests draw from the cold list, which never used them.
5. **Adopted (L).** H2 said H4 replaces the stages' base-pointer indexing; H3c does, when it hides
   the arrays. Fix: H3c.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 22: 5 findings (H1-M3-L1), 5 applied, 0 declined. Series: 97 findings, 97 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: the event digest (rounds 8,
  22), the sensor fixtures (rounds 13, 15, 22), `HistoryRingTest`'s budget cases (rounds 4, 6, 18,
  19, 21, 22: finding 3 is a consequence of round 19's variant), §2's wake bullet (rounds 4, 7, 9,
  13, 16, 18, 22), §4 H2 (rounds 2, 15, 18, 22). Findings 1, 2, 4 and 5 of revision 1. Each fix is
  a clause or a short paragraph. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 1596 -> 1611 lines. Above the split-check length (U-T6): no new decision, no split.
