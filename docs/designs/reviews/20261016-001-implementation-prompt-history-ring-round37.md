---
title: Review round 37 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round36.md
mode: full pass
---

# Round 37

Target at revision 36 (1720 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T14:02:51+10:00 to 14:10:16+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142e0-7f19-7b23-a011-73e740e146a5`. The stream emitted the final message twice; the last copy
follows (bold list numbering in place of headings this round).

## Review report (Codex final message)

1. **Summary**

The prompt largely matches the three binding designs and the live source. Two test procedures need revision: the phase-0 large-world run can exceed the available memory, and the hash layout case can pass without changing tree layout.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H1, §3 | `HistoryRewindTest` uses an unbounded 60-tick phase-0 ring for every benchmark scene. `large_world` creates one million static bodies and shapes (`shared/benchmarks.c`); `src/world_snapshot.c` serializes their records and sims into every image. Those fields alone exceed 560 MB per image, so 61 retained images exceed 34 GB before tree data. The prescribed suite cannot reliably run in the current 7.7 GiB RAM, 16 GiB swap workspace. | Give the phase-0 `large_world` fixture a smaller retained window or a bounded budget while keeping its random restore and churn checks. Run the full 60-tick window after H5, when images are partial. |
| 2 | M | §4 H0, §6 | `HashLayoutCase` compares hashes before and after full tree rebuilds but never checks that any layout changed. The prompt itself notes in H5 that a full rebuild can reproduce a layout. This case can therefore pass without showing that the hash excludes layout. | Require a changed layout or visit-order witness for at least one rebuilt tree before accepting the equal-hash check. |

3. **Checked, no change**

- The live `src/` and `include/` files match the cited 55a40d1 baseline.
- The ring’s interval, minimum-window, budget-overrun, and rewind rules in §§4–6 match the ring doc.
- The H4–H6 split between imaged and derived trees, proxy identity, and the restore pass matches the trees doc.
- The cited serializer omissions, filter behavior, event fields, and benchmark scene definitions match the source.

4. **Proposed edits**

- Bound phase-0 memory for `large_world` in `HistoryRewindTest`, and state the chosen window or budget and expected restorable-tick selection.
- Add an explicit layout-change witness to `HashLayoutCase` and its §6 evidence description.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `shared/benchmarks.c` builds `large_world`'s floor as a `STATIC_FLOOR_GRID`
   of 1000 by 1000 static bodies, one shape each; the phase-0 image is the whole-world
   serialization (§2), so each image carries about a million `b3Body`, `b3BodySim` and `b3Shape`
   records (the design §5.1's sizes, 136, 216 and 208 bytes) before tree data, and the test's
   `tickCount` 60 at interval 1 retains 61 of them. Fix: `large_world` runs at `tickCount` 2
   until H5 with the pair bounds expressed in `tickCount`, its sub-scene counts required from H5
   when the partial image makes the 60-tick window affordable, H5's test list says the scene
   moves to 60 there, and §7's phase-0 harness runs at `tickCount` 1. The sixteen other scenes
   are unchanged.
2. **Adopted (M).** H5's `HistoryTreeTest` already states that a full rebuild can reproduce a
   layout and rebuilds with more candidates until the visit order differs; `HashLayoutCase` had
   no such witness, so it could pass on an unchanged layout. Fix: the case records the visit
   order per tree before and after and `ENSURE`s it differs for at least one tree, with the same
   re-rebuild rule.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 37: 2 findings (H1-M1-L0), 2 applied, 0 declined. Series: 134 findings, 134 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §4 H1's
  `HistoryRewindTest` (rounds 29, 36, 37; finding 1 present since revision 1, a resource fact no
  earlier round computed), §4 H0's `HashLayoutCase` (rounds 13, 37; finding 2 present since the
  case was written). Neither is a consequence of the previous round's fix. Finding 1's fix is one
  parenthesis plus two clauses elsewhere; finding 2's is two clauses. Reviewer read of round
  records or changelog: none in the raw log. Doc length: 1720 -> 1725 lines. Above the
  split-check length (U-T6): no new decision, no split.
- Two consecutive rounds with a High after the clean round 35 (U-S1 counts three): both Highs
  are fixture facts (a scatter list, a memory budget), not structure.
