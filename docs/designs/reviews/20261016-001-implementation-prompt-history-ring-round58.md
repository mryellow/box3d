---
title: Review round 58 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round57.md
mode: full pass
session: 01a143b5-46c0-7e90-af10-f6cc7772d573 (third attempt, resumed to finalize)
---

# Round 58

Target at revision 55 (1861 lines).

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
Run: three attempts, same model and effort (U-P4). Attempt 1, 2026-10-16T17:47:02+10:00 to
17:47:21, exit 1 with no reads: `503 Service Unavailable` and "Selected model is at capacity"
(session `01a143ad-c0a7-7571-9b56-94a61a5f57bc`; log kept as `round58.attempt1.raw.log`).
Attempt 2, 17:47:40 to 17:51:01, exit 1 after 27 source reads on the same capacity error
(session `01a143ae-55c7-71c3-952f-872d17c0fbcc`); a resume to continue at 17:51:2x failed at once
with the same error (log `round58.attempt2.raw.log`). After a three-minute pause, attempt 3,
foreground, `timeout 540`, exit 124 (work timeout after 34 source reads and no final message),
17:55:15 to 18:04:15; header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a143b5-46c0-7e90-af10-f6cc7772d573`. Resumed per the workflow's Running-a-round row with the
instruction "Finalize the review now from the research already done, with no further tool
calls", 18:04:27 to 18:05:25, exit 0 (U-R8: provenance recorded). The resumed stream emitted the
final message twice; the last copy follows. The failed attempts produced no result and no
finding.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the three binding designs and the live code's write paths. Two H1 mechanics need correction: its snapshot size counter can overflow for a valid large world, and its geometry registry can grow without regard to the stated history budget.

### 2. Findings

| # | Severity | Location in target | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §4 H1, count-only sizing paragraph (around line 650) | H1 uses `b3RecBuffer` to size and write each image. Its `size` and `capacity` are `int`, and `b3RecBufAppend` adds into `size` as an `int` (`src/recording.h`, `src/recording.c`). A valid world near the shape limit can produce an image over `INT_MAX`; the counter overflows before the ring's `size_t` arena growth can help. The ring doc §6 explicitly requires capacity growth beyond `INT_MAX`. | Specify a size-safe phase-0 snapshot sizing and write path, including checked conversions at serializer calls, or state and justify a phase-0 size limit consistent with the binding design. |
| 2 | Medium | §4 H1, registry/reader paragraph (around line 670) | H1 retains every interned geometry entry until history is disabled and excludes those copies from `bytesUsed` and `maxBytes`. Repeated geometry churn can therefore grow history-owned memory indefinitely after the images that referenced it have been evicted. This defeats the bounded-retention purpose of the ring doc §§2 and 6; the registry exists to serve retained images under the design §13. | Define how entries cease owning payload bytes once no retained image references them, and account for any retained registry payload in the budget and measurements. |

### 3. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Neither is a consequence of a
previous round's fix; both are against H1's phase-0 mechanics as drafted in revision 1.

1. **Adopted (H), by the finding's second option.** `b3RecBuffer.size` and `.capacity` are `int`
   and `b3RecBufAppend`'s `countOnly` branch adds into `size` unchecked (`src/recording.h:80`,
   `src/recording.c:23`); `b3SerializeWorld` returns `int`. The ring doc §6's `INT_MAX` sentence is
   about the arena and the `core.c` byte-count add, which the prompt's §2 already assigns to H1;
   the image itself is the existing serializer's format (the design §13, "with existing code"), and
   its images are 5 to 51 MB on the design §5.1's scenes. Fix: a stated phase-0 limit of `INT_MAX`
   bytes per image, enforced by an assertion in the count-only branch so an oversized world fails
   at capture rather than writing a truncated slot, with the arena's `size_t` path and §7's
   per-scene phase-0 figures named.
2. **Adopted (M) for release and measurement; the budget charge declined, reasoning in §10
   (U-V10).** The design §13 keeps the registry alive "for the retained window", not for the
   session; the prompt kept every entry until disable. `b3GeometryEntry` has id-as-index with a
   dedup map and hash chains (`src/recording.h:97`), and `b3RegistrySlot` holds a copy per entry
   (`src/replay.h:67`), so an entry can be tombstoned in place. Fix: the capture stamps every entry
   the image interns with the tick; eviction releases entries stamped below the oldest retained
   imaged tick through a new function of `src/recording.c` that frees both copies, keeps the index
   and leaves the dedup map; a rewind leaves stamps alone (bounded over-retention, stated);
   `b3History_GetGeometryBytes` is declared for the tests and §7 (replacing the scratch-build
   reader), and `HistoryRingTest` case 11 churns one mesh per tick and bounds the figure at nine
   entries before and after a rewind. Charging the figure against `maxBytes` would add a budget
   term for an artefact H5 removes and shift the budget cases' arithmetic; the §10 item says so.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 58: 2 findings (H1-M1-L0), 2 applied, 0 declined. Series: 179 findings, 178 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> pending
- Tripwire: fix 2 adds mechanisms to phase 0 (a stamp, a release function, a read-only figure, a
  test case), all removed with the registry at H5; fix 1 adds an assertion. Clustering: H1's
  phase-0 slot and registry bullets (round 47's hull-pointer fix was the last against them).
  Neither finding against the previous round's fixes. Fix 2 is a paragraph and a test case, the
  largest single fix since round 49. Reviewer read of round records or changelog: none in the
  raw log. Doc length: 1861 -> 1893 lines. Above the split-check length (U-T6): no new
  decision, no split.
- Fourteenth timed-out round (28, 34, 43, 44, 45, 46, 49, 51, 52, 53, 54, 56, 57, 58), each
  finalized on resume; the first round with failed attempts (two, model at capacity).
