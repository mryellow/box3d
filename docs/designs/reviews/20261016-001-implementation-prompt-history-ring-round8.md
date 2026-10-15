---
title: Review round 8 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round7.md
mode: full pass
---

# Round 8

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:26:56+10:00 to 07:33:57+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14176-0692-7b43-9fae-71ed3231adcc`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is close, but two H4 instructions would make the implementation diverge from a binding design or fail during undo. I also found false source and recording-compatibility claims, plus one duplicated measurement scene.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4 | The prompt makes `pairSet` a `const b3HashSet*`. The design §5.2 requires a distinct, incomplete `b3JournaledPairSet` type so generic hash-set mutators cannot accept the world-owned container. | Specify an incomplete `b3JournaledPairSet*` and read-only accessors for its existing readers. |
| 2 | H | §§2, 4 H4 | The proposed assertion that every static or kinematic leaf write equals the *current* `fatAABBs` entry fails during reverse undo. For example, `b3Shape_SetHull` updates the fat AABB before recreating a static proxy; undo reinserts the old leaf before undo restores the old fat AABB (`src/shape.c`). | Apply that assertion to forward writes. Check an undo insertion against its recorded leaf box, then validate equality after the restore pass. |
| 3 | M | §2 | The wake inventory says non-static bodies move proxies into the awake tree and that filter-driven recreation does likewise. Kinematic proxies remain in the kinematic tree regardless of wake state; static proxies remain static, and a disabled shape has no proxy to recreate (`src/broad_phase.c`, `src/shape.c`). | Qualify the inventory by body type and enabled state; make the filter test fixture’s proxy type explicit. |
| 4 | M | §§2, 4 H1 | The prompt says a recording written before this branch still replays with the same hashes, but H1 raises `B3_SNAP_VERSION` from 11 to 12 without migration. The reader rejects a snapshot whose version differs (`src/world_snapshot.c`). | Remove the compatibility claim and state that the unchanged hash applies to recordings produced with the new snapshot version. |
| 5 | L | §7 | The cost scene list says “each scene of the benchmark table plus `large_world`,” although `large_world` is already in that table (`benchmark/main.c`). | Remove the duplicate name. |

### 3. Checked, no change

- The capture point follows the final migration drain, as required by the design §6 and shown in `src/physics_world.c`.
- The expected interval widening and window ticks in `HistoryRingTest` cases 1–5 match the ring doc §§5–7.
- The H2–H4 compile-boundary examples match the current writable types; the H5 and H6 cost fixtures distinguish the stated earlier image paths.
- The Debug benchmark fixtures are reduced by `BENCHMARK_DEBUG` (`shared/benchmarks.c`), so the phase-0 suite does not retain 61 full million-body images.

### 4. Proposed edits

Correct the H4 pair-set type and qualify the leaf-box assertion during undo. Then revise the §2 proxy and recording claims, and remove the duplicate `large_world` entry in §7.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** The design §5.2 names `b3JournaledPairSet` as a distinct type from the generic
   `b3HashSet`, incomplete outside its file, so a generic mutator cannot take it; revision 5's
   pointer-to-const `b3HashSet` kept the generic type. The design wins (§1). Fix: `pairSet` is a
   `b3JournaledPairSet*` defined in `src/broad_phase.c`, with contains, count and bytes readers for
   `src/physics_world.c`'s two uses; the compile-boundary statement's diagnostic is restated.
2. **Adopted (H).** `b3Shape_SetHull` (`src/shape.c`) writes the fat AABB, destroys the proxy and
   creates it again; the journal holds those three entries in that order and the undo runs them in
   reverse, so the undo's insertion of the old box happens while `fatAABBs` still holds the new
   one. Fix: the assertion applies to forward writes and never in an undo; the undo asserts the
   recorded id only; H5's validation checks every static and kinematic leaf against its entry
   after the restore pass. The sentence's citation into the trees doc's changelog is removed with
   it (U-P5), and the two references to "the gate entries of the three changelogs" in §6 and §8
   with it: the checklist is one line per structure the designs add.
3. **Adopted (M).** `b3ResetProxy` recreates the leaf in the shape's own tree (`proxyType` from the
   key, `src/shape.c`): the awake tree only for a sleeping dynamic body's shape; a kinematic body's
   proxies stay in the kinematic tree, a static body never wakes, a disabled body has no proxy.
   Fix: §2's wake bullet is qualified by body type; `HistoryRewindTest`'s filter churn names the
   four shape kinds it runs on.
4. **Adopted (M).** The reader rejects a snapshot whose version differs (`src/world_snapshot.c`),
   so a version-11 recording does not replay on this branch at all; the unchanged hash holds for
   recordings written on it. Fix: the sentence says that.
5. **Adopted (L).** `large_world` is a row of the benchmark table. Fix: named as among them.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 8: 5 findings (H2-M2-L1), 5 applied, 0 declined. Series: 40 findings, 40 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: the pair set's type changes from a
  pointer-to-const generic set to the design's own journaled type, with three readers; nothing
  else. Clustering: §4 H4 produced findings again (rounds 2, 4, 5, 6, 8), finding 1 being a
  consequence of round 4's fix 3 (which chose the wrong type for the pair set) and finding 2 of
  round 1's leaf-assertion wording; §2's inventory produced two (a qualification and a false
  compatibility claim). Each fix is a clause or a sentence; no paragraph was added for a minor
  finding. The H4 recurrence is the design's container rule being applied container by container;
  with the pair set now on the design's type every world-owned container is covered (slot pools,
  arrays, bitsets, pair set, trees), so there is no next container for a round to find. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 1370 -> 1380 lines; the
  fixes add one line (1), five (2), four (3), one (4) and none (5), one line removed by the
  changelog-citation cut. Above the split-check length (U-T6): one decision, no split.
