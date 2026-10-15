---
title: Review round 13 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round12.md
mode: full pass
---

# Round 13

Target at revision 13 (1460 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T08:12:08+10:00 to 08:17:53+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1419f-6846-7950-9902-2b15852ed8a0`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The implementation prompt is largely consistent with the three binding designs, but four defects could misdirect implementation or verification. The most consequential is an omitted solver-read field in the proposed state hash; a tree fixture and a budgeted measurement also claim evidence they cannot reliably provide.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H0; §6 | The hash mixes each `b3ContactSpec`’s contact id but omits `manifoldCount`. The design §5.1 identifies only `manifoldStart` as scratch; `src/solver.c` reads `manifoldCount` to arrange solver work. A wrong restored value could therefore pass the hash checks. | Mix `manifoldCount` with the contact id, excluding only `manifoldStart`. |
| 2 | M | §4 H5; §6 | `HistoryTreeTest` assigns an explosion involving a static body’s shapes to the static tree, but `b3World_Explode` queries only the awake and sleeping dynamic trees (`src/physics_world.c`). Also, placing a sensor on a static body does not establish that its visitors come from the static tree: the sensor pass queries all four trees (`src/sensor.c`). These fixtures do not show the named tree coverage. | Use dynamic shapes for the explosion; give the static-tree sensor case explicit static visitor shapes and verify they are the queried candidates. |
| 3 | M | §7 | The quarter-budget restore procedure says `GetRestorableTick(currentTick − 9)` prevents timing an unavailable rewind. It need not: with interval and window both 60, at tick 61 the window can evict image 0, leaving image 60; the request for tick 52 returns `UINT64_MAX` under the ring doc §§5–7. | Check the returned tick before timing. Choose a retained target when the nine-tick target is unavailable, and report its actual distance. |
| 4 | M | §2 | The inventory presented as the API wake paths omits conditional wakes in `b3Body_SetLinearVelocity`, `b3Body_SetAngularVelocity`, `b3Body_SetTargetTransform`, and force, torque and impulse calls with `wake` set (`src/body.c`). H1 explicitly churns a sleeping body’s linear velocity, so this omission can misclassify an operation used by the tests as cold-only. | Add the conditional wake paths and state the input conditions under which each moves a dynamic body’s proxies. |

### 3. Checked, no change

- H1’s stated interval-doubling sequence and 32-tick image spacing agree with the ring doc’s budget and window rules.
- The phase-0 serializer options, migration-queue copy and borrowed-geometry repointing address the source behavior described in §2.
- The H2–H4 compile-boundary checks use writable access that the proposed const or opaque boundaries would reject.
- H5 imaging the awake tree and H6 deriving the kinematic tree match the trees doc’s phase split.

### 4. Proposed edits

Apply the four actions in the findings table, then update §6’s evidence descriptions for the revised hash and tree fixtures.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `b3ContactSpec` carries `contactId`, `manifoldStart` and `manifoldCount`
   (`src/contact.h`); the count is written at graph add (`src/constraint_graph.c`), read by the
   solver's bucketing and SIMD grouping (`src/solver.c`) and imaged with the colour array; the design
   §5.1 calls only `manifoldStart` and the wide-constraint pointers scratch. H0 hashed the id alone,
   so a restore that scattered the wrong count would pass. Fix: the hash mixes the count.
2. **Adopted (M).** `b3World_Explode` queries the awake and sleeping trees only
   (`src/physics_world.c`), so a static body's shapes are never its candidates; the sensor pass
   queries all four trees and filters by convexity, the sensor-events flag, body and filter, with
   no body-type test (`src/sensor.c`), so a static sensor's visitors sit wherever their bodies are.
   The continuous stage queries all four trees (`src/solver.c`). Fix: the bullet sweep carries
   static and sleeping candidates and witnesses both trees, the sensor sweep's visitors are static
   convex shapes with the flag, and both explosions hit sleeping bodies.
3. **Adopted (M).** Under the quarter budget the interval reaches the window (60); at tick 61 the
   bound evicts image 0 (`HistoryRingTest` case 2's pattern, the ring doc §7) and no image lies at
   or below tick 52, so the resolved target is `UINT64_MAX`. Fix: fall back to the newest
   restorable tick, which exists while history is enabled (the ring doc's sole-image rule, case 5),
   and report the distance.
4. **Adopted (M).** `b3Body_SetLinearVelocity` wakes when the velocity is nonzero,
   `b3Body_SetAngularVelocity` when a component the locks leave is nonzero,
   `b3Body_SetTargetTransform` when the implied velocity reaches `sleepThreshold`, and the seven
   force, torque and impulse calls when `wake` is true and the body sleeps (`src/body.c`). §2's
   inventory had none of them and the rewind churn named `SetLinearVelocity` on sleeping bodies
   without saying which value wakes. Fix: both say so; the journal tests still draw from the cold
   list alone, which never used these calls.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 13: 4 findings (H1-M3-L0), 4 applied, 0 declined. Series: 61 findings, 61 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none; one hash field, fixture placement, a
  fallback in a measurement, inventory. Clustering: §4 H0 (rounds 2, 5, 13), §4 H5's tree test
  (rounds 11, 13), §7's quarter-budget line (rounds 11, 12, 13: each round's fix left a gap the
  next found; the line is now explicit about the fallback), §2's wake bullet (rounds 4, 7, 9, 13).
  Finding 2 is a consequence of round 11's fix 5 (naming trees); finding 3 of round 11's fix 6;
  the others of revision 1. Each fix is a clause or a short paragraph. Reviewer read of round
  records or changelog: none (the changelog path appears in an `rg --files` listing only). Doc
  length: 1460 -> 1476 lines. Above the split-check length (U-T6): no new decision, no split.
