---
title: Review round 24 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round23.md
mode: full pass
---

# Round 24

Target at revision 24 (1623 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T11:49:43+10:00 to 11:58:42+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14266-9ede-7f43-8e3b-50ac9882230d`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three binding designs and the current source. I found four issues that could leave an implementing agent with an incorrect rewind path or evidence that does not establish the stated requirement.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1 | The ring stores the tick of its last image and uses it to schedule capture, but the rewind procedure truncates slots and resets `historyTick` without resetting that stored tick. After rewinding, later steps could remain non-imaged until the discarded timeline’s last image tick. | Set the stored last-image tick to T when rewinding to imaged tick T; check imaging on the first re-stepped ticks. |
| 2 | M | §4 H1 | The disabled-history path returns before the stated traversal assertion. The design §§9 and 14 require `Rewind` to assert that no query or cast traversal is open. | Look up the world and assert `traversalDepth == 0` before the disabled-history return. |
| 3 | M | §4 H1 | `HistoryRingTest` cases 6–10 lack explicit ring definitions and starting timelines. Case 7 needs tick 10 retained when it rewinds, whereas the specified 20-step, eight-tick fixture in case 1 has already evicted it. The implementer must choose an unstated fixture for the named check. | Specify `tickCount`, interval, and initial step count for cases 6–10; make case 7’s tick 10 demonstrably retained. |
| 4 | M | §7 | The restore-cost measurement claims to show scaling with sensors, but records only overlap count. The benchmark scenes have no sensors (`shared/benchmarks.c`), and the additional scene fixes the count at 64, so the results cannot distinguish work per sensor from work per overlap. | Record sensor count and overlap count separately, and vary sensor count at fixed awake population and journal size. |

### 3. Checked, no change

- The section 2 descriptions of the serializer, recording and replay paths, proxy writes, solver sets, and validation build match the inspected source.
- The ring budget and window examples in §4 follow the ring doc’s widening and eviction rules.
- The H2–H4 compile-boundary approach and the H5/H6 cost-bound fail-before checks are consistent with the stated increment order.
- The golden policy preserves the existing determinism values and checks event digests alongside state hashes.

### 4. Proposed edits

Make the four changes in the findings table, then align §6’s evidence descriptions with the revised fixtures and sensor measurement.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** H1's ring holds the tick of the last imaged slot and schedules images from it;
   the rewind's step 5 reset `historyTick` and truncated the directory but left that tick at the
   discarded timeline's value, so after a rewind from 60 to 20 with interval 4 the next image would
   wait for tick 64. Fix: the rewind sets it to T, the walk's discard keeps it at the newest
   retained image.
2. **Adopted (M).** The design §14 has `b3World_Rewind` assert no query or cast callback is
   running and return unavailable when disabled; H1 returned before the lookup, so a disabled
   world's call from inside a callback went unasserted. Fix: lookup and assertion first.
3. **Adopted (M).** Cases 6 to 10 named no ring; case 7 rewinds to tick 10, which case 1's
   eight-tick ring has evicted after 20 steps. Fix: a 60-tick unbounded ring stepped 20 ticks for
   the five cases.
4. **Adopted (M).** The restore figure recorded overlaps but not sensors, and one scene at a fixed
   64 sensors cannot separate the per-sensor from the per-overlap term. Fix: both counts recorded;
   the scene runs at 64 and 256 sensors with everything else the same.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 24: 4 findings (H1-M3-L0), 4 applied, 0 declined. Series: 103 findings, 103 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none (the last-imaged tick already existed; the
  rewind now resets it). Clustering: §4 H1's rewind bullet (rounds 1, 2, 4, 6, 10, 11, 24),
  `HistoryRingTest` (rounds 4, 6, 18, 19, 21, 22, 24), §7's sensor measurement (rounds 12, 22,
  24). All four of revision 1. Each fix is a clause or a short paragraph. Reviewer read of round
  records or changelog: none (a `git show --name-only` of a trees-doc commit listed its
  changelog's path; no content read). Doc length: 1623 -> 1631 lines. Above the split-check length
  (U-T6): no new decision, no split.
- Finding 1 is the first rewind-path defect since round 11 and would have produced a wrong ring
  after every rewind; revision-1 gaps of that weight are still surfacing at round 24.
