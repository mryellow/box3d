---
title: Review round 5 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round4.md
mode: full pass
---

# Round 5

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T07:05:48+10:00 to 07:08:48+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14162-b082-7f53-80b5-d0518f1c50ff`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt is close to executable, but H4 has a build blocker and two test defects. Its source list omits a required CMake change, and its journal-only tests assume tree state that the binding designs restore through an image.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4, §3 | H4 adds `src/journal.c` and `src/slot_pool.c` but omits `src/CMakeLists.txt` from its permitted file list. That CMake file explicitly enumerates sources, so H4 cannot link as written. | Add `src/CMakeLists.txt` to H4 and list both sources there. |
| 2 | H | §4 H4 | `ColdChurnUndoCase` requires `b3History_UndoPending` alone to restore the full state hash after waking sleeping bodies. Waking moves proxies into the awake tree, which H4 does not journal; the design restores that tree from an image in §9. The direct walk therefore cannot satisfy the named hash check. | Keep the direct-walk check to journal-owned state. Use `b3World_Rewind` for a full-hash check involving wake and tree changes. |
| 3 | M | §4 H4 | `ProxyIdentityCase` says changing a sleeping body’s shape filter destroys and recreates its proxy **in the sleeping tree**. With `invokeContacts` true, `b3ResetProxy` recreates the marked leaf in the awake tree and queues migration (`src/shape.c`). The fixture neither exercises the claimed sleeping-tree free-list operation nor supports its direct-walk hash check. | Use a sleep/wake migration to test sleeping-tree proxy ids, and check the complete restored state through rewind. |
| 4 | L | §2 | The source inventory says `shared/benchmarks.c` provides `Create*` functions for seventeen scenes. `shared/benchmarks.h` declares eighteen, including `CreateWidePyramid`; `benchmark/main.c` selects seventeen. | Say “eighteen `Create*` functions; seventeen scenes in the benchmark table.” |

### 3. Checked, no change

- The H1 capture point matches the step tail in `src/physics_world.c`.
- The H1 interval and window expectations in `HistoryRingTest` cases 1, 2 and 5 match the ring doc §§5–7.
- The serializer and filter behavior described in §2 matches `src/world_snapshot.c` and `src/shape.c`.
- `benchmark/main.c` uses `-t=8` to run worker counts one through eight, as §7 expects.

### 4. Proposed edits

Add the H4 CMake file to its file list. Rewrite the two H4 tree fixtures so their direct journal checks cover journal-owned state, and put full-state checks after `b3World_Rewind`. Correct the benchmark function count in §2.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** `src/CMakeLists.txt` enumerates every source and header by name; H4 adds two
   sources and two headers and did not list the file, and H3a, H3b and H3c add storage headers
   without listing it either (U-G6). Fix: all four file lists name it.
2. **Adopted (H).** A wake moves the set's proxies into the awake tree, which the journal does not
   cover (the trees doc §5 images it), and the sleep that follows destroys them from it, so the
   awake tree's proxy free list differs after the walk alone. The same holds for other operations
   the case used: `b3Body_SetTransform` wakes any non-static body (`src/body.c`), `b3Body_SetType`
   wakes, a shape created on a sleeping body is created in the awake tree and queued (`src/shape.c`),
   `b3DestroyBody` wakes attached bodies, and `b3DestroyJoint` wakes under `wakeAttached`. Fix: §2
   gains a bullet listing which API calls wake and which are cold, from the source; the three
   direct-walk cases use the cold list only (static shapes and sensors, filter without contacts,
   the material setters, a joint between two sleeping bodies and its destroy without wake, a lone
   sleeping body's destroy and disable, the mesh-contact body's destroy); the wake, sleep and bulk
   placement remain `HistoryRewindTest`'s and `HistoryUndoSufficiencyTest`'s, which rewind.
3. **Adopted (M).** `b3Shape_SetFilter` with `invokeContacts` true calls `b3ResetProxy`, which
   recreates the leaf in the awake tree and queues the body; the sleeping tree's free list is not
   touched. Fix: `ProxyIdentityCase` uses a lone sleeping body's destroy, whose sleeping-tree
   destroy entry the walk undoes with the recorded id; the id reuse across a step is a named case
   of `HistoryRewindTest`.
4. **Adopted (L).** `shared/benchmarks.h` declares eighteen `Create*` (`CreateWidePyramid` among
   them); `benchmark/main.c`'s table has seventeen rows. Fix: §2 says so; `HistoryRewindTest` runs
   the table's seventeen.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 5: 4 findings (H2-M1-L1), 4 applied, 0 declined. Series: 28 findings, 28 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no; three test fixtures change their
  operations and one inventory bullet is added. Clustering: §4 H4's journal tests produced
  findings in rounds 2, 4 and 5, and §2's inventory in every round (U-S4); findings 2 and 3 are
  consequences of revision 1's fixtures assuming operations were cold that wake, not of a previous
  round's fix; the fix is a corrected inventory and fixtures drawn from it, a clause each. The
  recurring §2 findings are missed sites, each answered by a fact, and the count per round is not
  rising. Reviewer read of round records or changelog: none in the raw log. Doc length: 1329 ->
  1351 lines; the fixes add none (1), eighteen (2, of which fourteen are the §2 bullet), four (3)
  and one (4), one line removed by rewording. Above the split-check length (U-T6): one decision,
  no split.
