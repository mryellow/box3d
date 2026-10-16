---
title: Review round 65 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round64.md
mode: full pass
session: 01a14479-fcf3-7271-9cd3-8809ea459229
---

# Round 65

Target at revision 61 (1951 lines). Confirmation round after round 64 (U-P9).

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
Run: foreground, `timeout 540`, exit 0 after 54 source reads, 2026-10-16T21:30:07+10:00 to
21:36:12+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14479-fcf3-7271-9cd3-8809ea459229`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The source inventory and the main phase boundaries largely match the live code and the three binding designs. I found one gap in the ring’s write mechanics and one measurement plan that cannot establish its stated cost claim.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1 | Slots are variable length, but the second serializer pass writes into one contiguous arena region. The specified growth rule covers slots that do not fit by total size; it gives no procedure when enough bytes remain in aggregate but neither side of the circular arena has a contiguous span for the new image. | Specify whether that case wraps via a temporary buffer, compacts retained slots, or grows the arena, and how directory offsets are updated. |
| 2 | M | §§6–7 | The cost measurement claims to show restore work proportional to awake bodies, sensors and journal bytes. Its controlled 64-versus-256-sensor runs vary only sensors; benchmark scenes vary several factors together, so they cannot isolate awake-body scaling. | Add runs that vary awake-body count while holding the sleeping population, sensors and scripted journal work fixed. |

### 3. Checked, no change

- Verified the §2 record, container, tree-write, serializer, recording, replay and test-runner claims against the cited live source.
- Checked H2–H4 compile-boundary examples against the current types and write routes.
- Checked the ring’s interval and window examples against ring doc §§5–7.
- Checked the H5 and H6 tree-image split against trees doc §§5–9 and the cited dynamic-tree operations.

### 4. Proposed edits

Add the contiguous-slot reservation rule to H1. Add a controlled awake-population comparison to §7 and narrow the corresponding §6 cost claim until that comparison exists.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Both are against revision-1
text; neither is a consequence of a previous fix.

1. **Adopted (M).** The ring doc §3 makes the arena circular with variable-length slots in tick
   order and a directory of offsets, grows it "when the slots those rules retain and the slot
   being written do not fit", and §6 copies retained slots "in tick order from offset 0" on
   growth; H1's capture bullet repeated the growth rule and said nothing about where a slot is
   placed when the free bytes are split between the tail and the front. Fix: the placement is
   stated: a slot is one contiguous region that never straddles the end; at the write position
   when the span to the end (or, when the write position is below the oldest slot, to that
   slot's offset) holds it, else at offset 0 when the span to the oldest slot's offset holds it,
   else growth; the fit test the ring doc names is this contiguous one.
2. **Adopted (M).** The design §12 test 5 accepts "restore proportional to awake plus sensors
   and journal". §7 controls the sensor term (the million-body scene twice, 64 and 256 sensors,
   the awake population and churn the same) and reads the awake term across the benchmark table
   scenes, which differ in awake count, contacts, joints and churn at once. Fix: a third run of
   the million-body scene with 64 sensors and 256 awake boxes in place of sixteen, the added
   boxes falling where no sensor overlaps them, so those two rows differ in the awake columns
   alone. The §6 row for the cost tests points at §7 and needs no change.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation of round 64's clean round failed (U-P9); not
  converged (U-S8). The series' four clean rounds (35, 41, 48, 64) have each been followed by a
  confirmation with findings (four, three, three, two).
- Round 65: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 195 findings, 194 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> 0/0/0 -> 0/2/0 -> pending
- Tripwire: no mechanism added or removed; fix 1 states a placement rule for an existing
  structure (the arena the ring doc decides), fix 2 adds one harness run. Clustering: none with
  round 64 (clean) or 63; H1's capture bullet last drew a finding in round 63 (the size guard)
  and 58, so H1's phase-0 text has now drawn findings in rounds 58 to 63 and 65. Neither
  finding follows from a previous fix. Fix 1 is a sentence of nine clauses, the largest this
  block; it states one rule. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 1951 -> 1958 lines. Above the split-check length (U-T6): no new decision, no
  split.
