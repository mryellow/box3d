---
title: Review round 56 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round55.md
mode: full pass
session: 01a14398-b9f4-7e70-b320-9e3ef96046d8 (resumed to finalize)
---

# Round 56

Target at revision 53 (1847 lines).

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
Run: foreground, `timeout 540`, exit 124 (work timeout after 62 source reads and no final
message), 2026-10-16T17:24:04+10:00 to 17:33:04+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a14398-b9f4-7e70-b320-9e3ef96046d8`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 17:33:14 to 17:33:39,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The increment sequence largely follows the three binding designs, but the replay evidence has two gaps: it substitutes an event hash for the required event comparison, and it does not check events on the corrected timeline. I also found one incorrect source-reader claim.

### 2. Findings

| # | Severity | Location in target | Issue and source evidence | Concrete fix |
|---|---|---|---|---|
| 1 | Medium | §4, event-digest paragraph, lines 474–484; §6, replay-exactness row | The prompt calls a 64-bit digest a comparison of event type, contents, and order. The design §12 test 3 requires those fields to compare equal; the public event structs in `include/box3d/types.h` contain more than 64 bits, so different streams can share a digest. | Save the event fields and compare each event in order. Keep the digest for reporting if useful. |
| 2 | Medium | §4, H1 `HistoryReplayTest`, corrected-replay paragraph, approximately lines 840–854 | The corrected run asserts only that the state hash differs at tick 70. It never checks the regenerated events, so stale events from the original timeline could pass. The design §10 requires events from the corrected timeline. | Make the correction change a known event, then compare the corrected event stream with an independent run using the corrected inputs. |
| 3 | Low | §4, H3a record-array reader list, approximately lines 945–955 | The prompt lists `b3World_GetBounds` as a reader of `fatAABBs`. In `src/physics_world.c`, that function reads the four trees' root bounds and does not read `world->fatAABBs`. | Remove `b3World_GetBounds` from that reader list. |

### 3. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). None is a consequence of a
previous round's fix; the first two are against H1's test mechanism as drafted in revision 1.

1. **Adopted (M).** The design §12 test 3 (line 963) says each replayed step's events "(types,
   contents and order) equal those the original run produced"; a 64-bit digest over fields that
   exceed 64 bits per event is a probabilistic proxy, and the in-process tests hold both sides in
   one process, so the copies cost nothing but memory for ninety ticks. Fix: the event record
   (each tick's arrays copied as public structs in reported order) is compared count by count and
   field by field in `HistoryReplayTest`, `HistoryCrossWorkerTest` and `HistoryTreeTest`, the
   digest printed on a failure; the goldens (§5) and the §7 harness, which compare across
   processes and platforms, keep the digest.
2. **Adopted (M).** The corrected replay `ENSURE`d a differing hash at tick 70 and asserted the
   events' provenance in a parenthesis; the design §10 (line 792) says a corrected replay's
   events are its own. Fix: the test first runs a fresh world on the corrected inputs (the op log
   with the tick-70 call dropped and the tick-50 host change as in the original) and keeps its
   hash and event record; the rewound world equals that reference from tick 61 to 90, and the
   body's tick-70 move event differs from the original's record in its transform.
3. **Adopted (L).** `b3World_GetBounds` (`src/physics_world.c`) unions
   `b3DynamicTree_GetRootBounds` over the four trees and reads no entry. Fix: removed from the
   list of `b3GetShapeFatAABB`'s readers.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 56: 3 findings (H0-M2-L1), 3 applied, 0 declined. Series: 176 findings, 175 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> pending
- Tripwire: fix 1 replaces the digest with the event record as the comparator in the in-process
  tests (a mechanism changed, the digest kept where it is needed); fix 2 adds a reference run to
  one case. Clustering: H1's `HistoryReplayTest` and the test mechanism paragraphs, drafted in
  revision 1 and edited in several rounds; neither finding against the previous round's fixes.
  Each fix is a sentence or a paragraph. Reviewer read of round records or changelog: none in
  the raw log. Doc length: 1847 -> 1857 lines. Above the split-check length (U-T6): no new
  decision, no split.
- Twelfth timed-out round (28, 34, 43, 44, 45, 46, 49, 51, 52, 53, 54, 56), each finalized on
  resume.
