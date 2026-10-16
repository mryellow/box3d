---
title: Review round 60 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round59.md
mode: full pass
session: 01a143d2-566b-7521-af51-714f7731c858
---

# Round 60

Target at revision 57 (1907 lines).

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
Run: foreground, `timeout 540`, exit 0 after 68 source reads, 2026-10-16T18:27:00+10:00 to
18:33:31+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a143d2-566b-7521-af51-714f7731c858`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt is largely aligned with the three binding designs, but H1's geometry registry has two lifecycle defects that can break its memory bound or a later restore. The event digest also names a field that does not exist in the public event struct.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1; §10 | The count-only `b3SerializeWorld` sizing pass interns geometry through `src/world_snapshot.c` and `src/recording.c`. If the budget then widens the interval and leaves that tick unimaged, the new entries have no owning slot and remain allocated. This defeats §10's retained-image bound. | Make sizing avoid interning; intern only after the decision to write an image. Add a budget-widening geometry-churn case. |
| 2 | H | §4 H1 | `seenTick` is compared with `captureTick`, which the prompt sets to the history tick. Rewinding and capturing that tick again can reuse the same value, omit a live geometry entry from the new slot's list, and free its reader data when an older slot is evicted. | Use a capture identifier that is unique across rewinds, with a defined inactive value and initialized `seenTick`. Test recapture of the same retained geometry followed by eviction and restore. |
| 3 | L | §4, event digest | The named hit-event field is `speed`; the public struct calls it `approachSpeed` in `include/box3d/types.h`. Following the field list literally cannot compile. | Name `approachSpeed`. |

### 3. Checked, no change

- The stated step capture point and broad-phase traversal callers match `src/physics_world.c`.
- The serializer omits the migration queue and event buffers as §2 states; the existing test runner supports exact entry-name filtering.
- The ring's budget and window examples, the H5/H6 cost boundaries, and the listed tree write routes are consistent with the binding designs and cited source.

### 4. Proposed edits

Specify side-effect-free phase-0 sizing and a rewind-safe capture identifier in H1, extend `HistoryRingTest` to cover both registry cases, and correct the event field name.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Findings 1 and 2 are against
round 59's registry mechanics; finding 3 against the digest paragraph as drafted in revision 1.

1. **Adopted (H), by the release route rather than the sizing change.** The serializer's shape
   writer interns each live shape's geometry with no `countOnly` branch (`src/world_snapshot.c`,
   the geometry write from line 588; `b3InternGeometry` at `src/recording.c:802`), and the ring doc
   §6 step 4 sizes the slot "as a journal segment alone when the new interval says no image is
   due", so a widened tick leaves the sizing run's new entries unowned. Teaching the serializer
   not to intern under `countOnly` would change its id assignment between the two runs; instead
   the sizing run's list is kept, and when the tick ends unimaged every listed entry at
   `refCount` zero is released at once; the reader's slots are appended at the write, empty for a
   released entry, so ids stay indices. Case 11's third world runs the churn under case 3's
   half-slot budget and bounds the figure by the retained imaged slot count.
2. **Adopted (H).** The tick is reused after a rewind, so a `seenTick` equal to the new capture's
   tick suppressed the entry from the new slot's list. Fix: a per-run serial from a counter that
   is never reused, zero inactive, `seenSerial` zero at creation, a fresh serial for the sizing
   run and the write run alike; case 11's second world keeps a persistent mesh through the twenty
   branches, twelve steps past tick 0's eviction and a rewind that restores through it.
3. **Adopted (L).** `b3ContactHitEvent.approachSpeed` (`include/box3d/types.h:1167`); the field
   list names it.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 60: 3 findings (H2-M0-L1), 3 applied, 0 declined. Series: 184 findings, 183 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> pending
- Tripwire: the fixes change round 59's mechanism (a serial for a tick stamp) and add a release
  point and two test parts. Clustering: H1's registry bullet (rounds 58, 59, 60), three rounds
  running, reported under U-S4: round 58 opened a new phase-0 mechanism and the two rounds since
  have been its correctness; both Highs are against the previous round's fix, the fifth such
  round since 50. Each fix is a paragraph or a clause. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 1907 -> 1925 lines. Above the split-check length
  (U-T6): no new decision, no split.
- Finished inside the timeout at the first attempt.
