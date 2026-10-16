---
title: Review round 28 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round27.md
mode: full pass
session: 01a1428d-809a-77a0-8544-b1e6d1a178b6 (resumed to finalize)
---

# Round 28

Target at revision 28 (1655 lines).

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 70 source reads and no final
message), 2026-10-16T12:32:11+10:00 to 12:41:11+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a1428d-809a-77a0-8544-b1e6d1a178b6`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 12:41:37 to 12:41:54,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three binding designs, and the ring transitions and restore order checked against the source are consistent. Three corrections are needed before handoff: a missed SIMD write route, an overbroad source claim, and measurement instructions that do not preserve the data needed for their stop rule.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2, §4 H3b | The contact writer inventory attributes impulse writes to `src/contact_solver.c`, but `src/contact_solver_wide.inl`, compiled through `src/wide4.c` and `src/wide8.c`, also writes manifold fields and takes writable manifold pointers. Making `manifolds` pointer-to-const will require migrating those routes for H3b to build. | Name the SIMD route in §2 and H3b; direct its writes through the hot manifold accessor. |
| 2 | L | §2 | `b3Body_SetTransform` is listed as waking any non-static body. For a disabled body, `b3WakeBody` does nothing; the call changes its stored sim without waking it or migrating a proxy (`src/body.c`). | Qualify the claim to enabled sleeping bodies and identify the disabled-body route as cold. |
| 3 | L | §7 | The stated regression stop rule depends on the spread of four benchmark runs, but each scene’s CSV contains only the minimum run time per thread count (`benchmark/main.c`). The prescribed CSV comparison cannot establish that spread. | Require saving the per-run console output, or run four separate single-repeat measurements, and define the comparison from those values. |

### 3. Checked, no change

- The budget widening, minimum window, eviction, and tick sequences in `HistoryRingTest` agree with the ring doc §§5–7.
- The phase-0 snapshot omissions and the prompt’s queue and event handling agree with `src/world_snapshot.c` and the design §§9, 13.
- The static, sleeping, and kinematic tree treatment across H4–H6 agrees with the trees doc §§5–9.
- The named compiler-boundary expressions match the current declarations; the H2 qualifier warning was verified with Clang.

### 4. Proposed edits

Apply the three table actions to §§2, 4, and 7. No design change is proposed.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `src/contact_solver_wide.inl` reads `contact->manifolds` into a `b3Manifold*`
   in `b3PrepareConstraintW` (writes `points[i].normalVelocity` under hit events) and
   `b3StoreImpulsesW` (writes `normalImpulse` and `totalNormalImpulse`); `src/CMakeLists.txt`
   compiles it through `src/wide4.c` and `src/wide8.c`, and `src/contact_solver.c`'s
   `b3StoreImpulses_Convex` and `b3StoreImpulses_MeshWide` dispatch to the W4/W8 instances. The
   scalar mesh path in `src/contact_solver.c` writes `normalVelocity` in `b3PrepareContacts_Mesh`
   as well as the impulses in `b3StoreImpulses_Mesh`, so the "store impulses" wording was narrow
   for that file too. Fix: §2 names the four writers and their files; H3b's file list adds the
   `.inl`; the context list and H3b's companion sentence say prepare and store-impulses passes.
2. **Adopted (L).** `b3Body_SetTransform` calls `b3WakeBodyWithLock` for every non-static body,
   and `b3WakeBody` returns false below `b3_firstSleepingSet`, so a disabled body (set 1) is not
   woken; the call then writes its sim in the disabled set, `shape->aabb` and the `fatAABBs`
   entry, and skips the proxy move on `proxyKey == B3_NULL_INDEX` (`src/body.c`). Those are the
   non-awake record and set writes the design §5.2 already covers. Fix: the wake-inventory entry
   says a sleeping body and names the disabled route.
3. **Adopted (L).** `benchmark/main.c` prints `run %d : %g (ms)` per repeat and writes
   `threads,ms` rows holding `minTime[threadIndex - 1]` only; `-r=` defaults to 4. Fix: §2's
   benchmark line states what goes where; §7's run line tees the console to a log and the
   no-regression measurement reads the per-run times from it and the minimum from the csv, with
   the stop rule stated on the minimum and the spread.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 28: 3 findings (H0-M1-L2), 3 applied, 0 declined. Series: 113 findings, 113 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> pending
- Tripwire: fixes add or remove a named mechanism: none; finding 1 extends the H3b companion's
  caller set to a file the inventory had missed, finding 3 adds a `tee` to a run line. Clustering:
  §4 H3b (rounds 2, 8, 17, 27, 28; finding 1 is an inventory gap present since revision 1, not a
  consequence of any fix), §2 wake inventory (rounds 10, 20, 24, 28; finding 2 qualifies a
  claim present since revision 1), §7 (rounds 21, 24, 25, 28; finding 3 is an inventory fact
  about the benchmark present since revision 1). No finding this round was a consequence of
  round 27's fixes. Each fix is a clause or a short paragraph. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 1655 -> 1667 lines (finding 1 +9, finding 2 +2,
  finding 3 +1). Above the split-check length (U-T6): no new decision, no split.
- First round with no High finding since round 20; first round whose findings were all
  revision-1 inventory facts rather than consequences of the series' own edits.
