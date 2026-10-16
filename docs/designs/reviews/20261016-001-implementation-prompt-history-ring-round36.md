---
title: Review round 36 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round35.md
mode: full pass (confirmation round after round 35, U-P9)
---

# Round 36

Target at revision 35 (1711 lines), unchanged since round 35.

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T13:54:16+10:00 to 14:00:44+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142d8-a3f9-75c0-b186-bb749c20e98d`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is close to the binding designs, but four defects could leave state unrestored or let required coverage pass without exercising the relevant rewind. The most serious is that H5 gathers awake contact records but omits them from its restore scatter.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H5 | The image gathers each awake contact record, but the restore mechanics scatter only body, shape, joint and island records. Resizing manifolds and caches does not restore contact flags, links, cached poses or the convex cache, as required by the design §9. | Explicitly scatter each imaged contact through `b3Contact_Restore`, then restore its separately owned blocks. |
| 2 | M | §2; §4 H3b | The manifold writer inventory omits `src/physics_world.c`: its collide pass writes `separation`, `normalVelocity`, `persisted` and `baseSeparation` through `contact->manifolds`. H3b makes that pointer const but describes only the collide pass’s flag writes there. | Add these writes to §2 and route them through H3b’s hot manifold accessor. |
| 3 | M | §4 H0 | The hash selects a contact cache union member from the two shape types, but `src/contact.c` handles compound contacts using the child selected by `childIndex`. The top level compound type does not identify whether its convex child uses the SAT cache, simplex cache or neither. | Specify selection from the effective child type and add a compound contact cache sensitivity case. |
| 4 | M | §4 H1; §6 | `HistoryRewindTest` counts each named structural sub-scene’s runs, but a randomly chosen T can fall after its mutation. The test can pass without rewinding across that case, despite §6 assigning it evidence for every churn case. | Bracket each named sub-scene with a retained T before it and P after it, and assert the rewind result; keep the random pairs as additional coverage. |

### 3. Checked, no change

- The ring test’s interval widening and two-image overrun sequence matches the ring doc §§6–7.
- The awake-tree image, derived-tree restore pass and proxy identity plan match the trees doc §§5–9.
- The H1 serializer additions address the live serializer’s omitted migration queue and scrubbed `userData` words.
- The named public event fields, sensor overlap query and benchmark command options match the live headers and source.

### 4. Proposed edits

Make the four changes in the findings table, including the corresponding §2 inventory and §6 evidence wording.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** H5's gather lists "per awake contact ... its record, its `manifoldCount`
   manifolds and its cache"; its restore sentence scattered body, shape, joint and island records
   and then resized each contact's blocks, so the contact record (flags, links, cached poses, the
   convex cache) was imaged and never written back. The design §9 step 3 restores every imaged
   record. Fix: the scatter names contacts, through H3b's `b3Contact_Restore`, before the block
   resize.
2. **Adopted (M).** `b3CollideTask` (`src/physics_world.c`) writes `separation`, `normalVelocity`
   and `persisted` on a persisted manifold's points and `baseSeparation` after the update, through
   `contact->manifolds` as a `b3Manifold*`; §2's writer list (rounds 28's addition) and H3b's
   companion sentence named the file for flag writes only. Fix: both name the manifold-point
   writes.
3. **Adopted (M).** `b3CreateContact` and `b3UpdateConvexContact` (`src/contact.c`) substitute a
   compound's `childIndex` child for shape A, so the collide function and the cache member follow
   the child's type. Fix: the rule says so; the fixture gains a compound with a hull child and a
   probe of its `satCache`.
4. **Adopted (M).** T was drawn in [`oldestImageTick`, P] for every pair, so a sub-scene applied
   at tick m could be counted with T ≥ m and never rewound across. Fix: in a pair that runs a
   sub-scene, T is drawn below m; the pair's start tick is retained at P (P − start ≤ 60 at
   `tickCount` 60) and m is after it, so the range is never empty.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round of round 35's clean result found four
  findings, so the series is not converged (U-P9, U-S8); a fresh clean round and its confirmation
  are required.
- Round 36: 4 findings (H1-M3-L0), 4 applied, 0 declined. Series: 132 findings, 132 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §4 H5's restore (rounds 8,
  19, 23, 36; finding 1 present since revision 1, a gap round 35's clean pass did not see), §2's
  manifold writers (rounds 28, 36; finding 2 a gap left by round 28's fix, which inventoried the
  solver files and not the collide task), §4 H0's cache rule (rounds 32, 33, 36; finding 3 a
  consequence of round 32's rule), §4 H1's `HistoryRewindTest` schedule (rounds 29, 36; finding 4
  a consequence of round 29's counting fix). Three of four findings are consequences of fixes
  made in rounds 28 to 32. Each fix is a clause or a short sentence. Reviewer read of round
  records or changelog: none in the raw log. Doc length: 1711 -> 1720 lines. Above the
  split-check length (U-T6): no new decision, no split.
- A clean round followed by a four-finding confirmation (one High on a revision-1 gap) is the
  case U-G4 and U-P9 exist for; nothing is marked Canonical.
