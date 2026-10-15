---
title: Review round 2 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round1.md
mode: full pass
---

# Round 2

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T06:44:00+10:00 to 06:48:44+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1414e-ba1b-74e3-8137-249241957832`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt has several defects that would misdirect the implementation or make required evidence fail. The most consequential are the shape restore policy’s conflict with recording playback and an undo-only test that cannot restore the state it asserts.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2; §4 H1 | The claim that both `invokeContacts` values recreate a proxy is false. In `src/shape.c`, `b3Shape_SetFilter` calls `b3ResetProxy` only when `invokeContacts` is true. The H1 churn description therefore misses the binding design §12 test 2 case where a filter change leaves the proxy untouched. | Correct §2 and keep the two cases distinct in `HistoryRewindTest`: false leaves the proxy in place; true recreates it. |
| 2 | H | §4 H3a | `b3Shape_Restore` is specified to release and clear every live `userShape`, yet H3a also routes `src/world_snapshot.c` through it and promises no behavior change. Recording playback currently carries a handle across a restore when the live slot and generation match; the design §13 keeps that behavior when the release option is off. | Specify how the serializer’s release option reaches the restore writer, preserving recording carry-over while releasing handles for history rewind. |
| 3 | M | §4 H3c | The required compile-boundary statement uses `bodySims.data[0].mass`. `b3BodySim` has `invMass`, not `mass`, in `src/body.h`. The statement fails to compile before H3c, so it cannot show the claimed boundary. | Use an existing writable field, such as `invMass`, in the scratch translation unit. |
| 4 | H | §4 H4, `SegmentWalkCase` | The test’s “no hot write” premise is false: `b3World_Step` writes `inv_h` and `inv_dt` even without awake bodies. An undo-only walk from tick 12 cannot match tick 0’s hash, taken before those writes. The second `UndoSegments` call also starts after an earlier walk to tick 6 without resetting the walk’s starting point. | Test cold journal state independently of imaged scalars, and run each target from a fresh state or define and update an explicit walk cursor. |
| 5 | M | §4 H4, `OwnershipCase` | `b3GetByteCount() == b0` immediately after `UndoPending` assumes capacity returns to its pre-churn value. The reused, growable staging buffer and container arrays may retain capacity after a correct undo (`src/container.h`). | Assert restored ownership and state there; keep the exact byte-count baseline for after disable and world destruction. |

### 3. Checked, no change

- The ring test’s `tickCount + 1` slot expectation and its interval-doubling sequence match the ring doc §§6–7.
- The H1 capture point follows the final migration drain in `src/physics_world.c`, as required by the design §6.
- The stated proxy create, destroy, and bulk-placement routes match `src/broad_phase.c`; the H4 tree boundary targets those routes.
- The listed benchmark scene count and test runner’s named-entry mechanism match `benchmark/main.c` and `test/main.c`.

### 4. Proposed edits

Apply the five corrections above, then rerun the H3c compile-before check and the H4 journal tests. Keep the corrected filter cases explicit so the no-proxy path remains visible in the test-to-requirement evidence.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED
tokens used
138,809
### 1. Summary

The prompt has several defects that would misdirect the implementation or make required evidence fail. The most consequential are the shape restore policy’s conflict with recording playback and an undo-only test that cannot restore the state it asserts.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2; §4 H1 | The claim that both `invokeContacts` values recreate a proxy is false. In `src/shape.c`, `b3Shape_SetFilter` calls `b3ResetProxy` only when `invokeContacts` is true. The H1 churn description therefore misses the binding design §12 test 2 case where a filter change leaves the proxy untouched. | Correct §2 and keep the two cases distinct in `HistoryRewindTest`: false leaves the proxy in place; true recreates it. |
| 2 | H | §4 H3a | `b3Shape_Restore` is specified to release and clear every live `userShape`, yet H3a also routes `src/world_snapshot.c` through it and promises no behavior change. Recording playback currently carries a handle across a restore when the live slot and generation match; the design §13 keeps that behavior when the release option is off. | Specify how the serializer’s release option reaches the restore writer, preserving recording carry-over while releasing handles for history rewind. |
| 3 | M | §4 H3c | The required compile-boundary statement uses `bodySims.data[0].mass`. `b3BodySim` has `invMass`, not `mass`, in `src/body.h`. The statement fails to compile before H3c, so it cannot show the claimed boundary. | Use an existing writable field, such as `invMass`, in the scratch translation unit. |
| 4 | H | §4 H4, `SegmentWalkCase` | The test’s “no hot write” premise is false: `b3World_Step` writes `inv_h` and `inv_dt` even without awake bodies. An undo-only walk from tick 12 cannot match tick 0’s hash, taken before those writes. The second `UndoSegments` call also starts after an earlier walk to tick 6 without resetting the walk’s starting point. | Test cold journal state independently of imaged scalars, and run each target from a fresh state or define and update an explicit walk cursor. |
| 5 | M | §4 H4, `OwnershipCase` | `b3GetByteCount() == b0` immediately after `UndoPending` assumes capacity returns to its pre-churn value. The reused, growable staging buffer and container arrays may retain capacity after a correct undo (`src/container.h`). | Assert restored ownership and state there; keep the exact byte-count baseline for after disable and world destruction. |

### 3. Checked, no change

- The ring test’s `tickCount + 1` slot expectation and its interval-doubling sequence match the ring doc §§6–7.
- The H1 capture point follows the final migration drain in `src/physics_world.c`, as required by the design §6.
- The stated proxy create, destroy, and bulk-placement routes match `src/broad_phase.c`; the H4 tree boundary targets those routes.
- The listed benchmark scene count and test runner’s named-entry mechanism match `benchmark/main.c` and `test/main.c`.

### 4. Proposed edits

Apply the five corrections above, then rerun the H3c compile-before check and the H4 journal tests. Keep the corrected filter cases explicit so the no-proxy path remains visible in the test-to-requirement evidence.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (M).** `b3Shape_SetFilter` (`src/shape.c`) returns on an unchanged filter, assigns
   it, and calls `b3ResetProxy` only under `invokeContacts`; the always-true `destroyProxy` is
   inside that branch. So `invokeContacts` false writes the record and leaves the proxy, which is
   the design §12 test 2's untouched-proxy case; the revision 1 claim that both values recreate
   it was wrong. Fix: §2 states the two paths; `HistoryRewindTest`'s churn names them as the two
   cases; §8's observation is the always-recreate under `invokeContacts` true.
2. **Adopted (H).** `b3DesShapes` (`src/world_snapshot.c`) saves a live same-generation slot's
   handle, writes the record with `userShape` NULL and puts the handle back, releasing the rest.
   A restore writer that releases unconditionally would destroy the handle the recording's reader
   carries over, and H3a promises no behaviour change. Fix: the reader with the option off saves
   the handle and detaches it through `b3Shape_SetUserShape( ..., NULL )` before the restore write,
   so the writer finds nothing to release, and puts it back through the same writer after; with the
   option on it saves nothing. `b3Shape_Restore` stays unconditional, as the design §10 states.
3. **Adopted (M).** `b3BodySim` (`src/body.h`) has `invMass`, no `mass`. Fix: `invMass`.
4. **Adopted (H).** `b3World_Step` writes `world->inv_h` and `world->inv_dt` before `b3Solve`'s
   early return and flips `endEventArrayIndex` at its tail (`src/physics_world.c`), all world
   scalars in the hash; the journal never carries them (the design §5.1 images them). And the walk
   starts at `historyTick`, which only `b3World_Rewind` moves, so a second walk after the first
   would re-undo the same segments. Fix: the scene is stepped once before the enable; the test
   records the scalars struct per tick and copies tick T's back through `b3World_WriteScalars`
   before comparing; the tick-0 target runs on a fresh run of the scene.
5. **Adopted (M).** The staging buffer and the world's arrays keep capacity across a correct undo,
   so `b3GetByteCount() == b0` after `b3History_UndoPending` can fail on a correct journal; the
   same holds for the "fell by at least the freed blocks' bytes" clause after the eviction, where
   the arena may also have grown (U-G6). Fix: after the undo the test asserts the hash and the
   manifold allocators' count; after the eviction the count alone; the exact byte-count baseline
   is checked after the destroy, which is where an evicted block not freed would show.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 2: 5 findings (H2-M3-L0), 5 applied, 0 declined. Series: 14 findings, 14 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no; the serializer's carry-over is restated as
  a detach and put-back through the one writer that H3a already named. Clustering: three of five
  findings in §4 H4's journal tests and H3a/H3c, which round 1 also touched (the compile-boundary
  statements); finding 3 is a consequence of round 1's fix 1 (a wrong field name in a rewritten
  statement); finding 5 is a sibling of round 1's finding 2. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 1284 -> 1294 lines; the fixes add one line (1), six
  (2), none (3), three (4) and none (5), one line removed by rewording. Above the split-check
  length (U-T6): one decision, no split.
