---
title: Review round 19 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round18.md
mode: full pass
---

# Round 19

Target at revision 19 (1562 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T11:00:53+10:00 to 11:05:48+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14239-ea02-7ce0-83e1-38c666bdf779`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is not ready for implementation. H4 lacks a required source edit and read route, while several prescribed tests can reject a correct ring or cannot be run as described.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4 | H4 hides `pairSet` and the id pools, but omits `src/world_hash.c`, whose H0 hash must read their membership and allocation order. Its proposed pair-set readers also omit the capacity used by `src/physics_world.c`. Under §3’s file rule, the implementer has no stated route to keep the hash and memory diagnostic working. | Add `src/world_hash.c` to H4 and specify read-only routes for the hash inputs and pair-set capacity. |
| 2 | M | §4 H1, `HistoryRingTest` case 2 | With `tickCount` 32, tick 0 is evicted at step 33 once tick 32 is imaged. The case instead expects tick 0 through step 64, contrary to the ring doc §7. | Correct the retained-image and `oldestImageTick` expectations at each window boundary. |
| 3 | M | §4 H1, `HistoryRingTest` cases 2–4 | The budget is derived from tick 0’s charge `s`, then exact widening and over-budget results are asserted for later ticks. Those assertions require later images to stay near `s`; the prompt does not ensure that, and H5’s awake-only image can shrink as the pile sleeps. A correct implementation could fail tests that H5 says remain unchanged. | Use a fixture with controlled, stable charges, or calculate expected transitions from the slots’ measured charges. |
| 4 | M | §4 H1, `HistoryRewindTest` | Sampling T from `[P−60, P]` does not ensure T is retained after earlier rewinds and short re-steps. For example, a ring whose oldest image is 40 can rewind to 90, step to 95, then sample T=35 and wrongly expect success. | Bound T by `b3World_GetHistoryInfo().oldestImageTick` as well as `P−60`, and check it is imaged. |
| 5 | M | §7 | The fallback sleeping-tree rebuild timing is assigned to `HistoryRewindTest` “under the same scratch build,” but §7 configures that Release build with `BOX3D_UNIT_TESTS=OFF`. `CMakeLists.txt` therefore does not build that test there. | Enable the test target in the measurement build or put the timed rebuild case in the scratch harness. |

### 3. Checked, no change

- The H1 capture point matches the tail of `b3World_Step` in `src/physics_world.c`.
- The described `b3Shape_SetFilter` always-recreate path matches `src/shape.c`.
- The phase-0 serializer omissions and proposed user-word, renderer-handle and migration-queue handling match `src/world_snapshot.c` and the three designs.
- The H5 and H6 cost cases target the serializer image and raw kinematic-tree image respectively, as the phase plan requires.
- The benchmark’s `-t=8` loop and CSV output match the cited measurement command.

### 4. Proposed edits

Resolve findings 1–5 in §4 and §7, then update the affected §6 evidence entries where their test expectations change.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** H0's walk reads the six id pools' `freeArray` and `nextIndex` and the pair
   set's keys; H4 hides both behind incomplete types and did not list `src/world_hash.c`;
   `src/physics_world.c` logs the pair set's bytes, count and capacity and asserts its count.
   Fix: the file is listed, the pair set gains capacity and a key walk, the slot pool a const
   id-pool getter.
2. **Adopted (M).** Round 18's rewrite of case 2 kept two images "through step 64"; with the
   window at 32 ticks, tick 0 is 33 behind at step 33 and leaves. Fix: two images only at the
   step that writes one, with the spans and `oldestImageTick` restated (which is what the case
   said before round 18, now with the per-step detail).
3. **Adopted (M).** The exact widening and over-budget sequences assume every image's charge is
   near tick 0's; the churn scene's images drift and H5's awake-only image shrinks as the pile
   sleeps. Fix: cases 2 to 5 run on a variant with bodies apart, gravity zero and sleep off, so
   every image has the same charge, asserted per slot.
4. **Adopted (M).** After a rewind and a short re-step the oldest retained image can be newer than
   `P − 60` (the reviewer's example: oldest 40, rewind to 90, step to 95, T = 35 drawn). Fix: T is
   drawn from `[oldestImageTick, P]` read at P.
5. **Adopted (M).** §7's Release measurement build sets `BOX3D_UNIT_TESTS=OFF`, so round 17's
   fallback to the rewind test's case could not run there. Fix: the harness carries a scene that
   reproduces the height-trigger case.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 19: 5 findings (H1-M4-L0), 5 applied, 0 declined. Series: 87 findings, 87 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 -> pending
- Tripwire: fixes add or remove a named mechanism: read-only routes for state the hash and the
  memory log already read (AC-1: no new structure; AC-4: readers). Clustering: §4 H4's pair set
  (rounds 8, 10, 19), `HistoryRingTest` case 2 (rounds 4, 6, 18, 19: round 18's fix introduced
  finding 2), `HistoryRewindTest`'s sampling (rounds 2, 19), §7's slab (rounds 15, 16, 17, 19:
  round 17's fallback introduced finding 5). Findings 2 and 5 are consequences of the series'
  own fixes; 1, 3 and 4 of revision 1. Each fix is a clause or a short paragraph. Reviewer read
  of round records or changelog: none in the raw log. Doc length: 1562 -> 1574 lines. Above the
  split-check length (U-T6): no new decision, no split.
- Round 20 is the owner's stated stopping point for this block if the series has not converged.
