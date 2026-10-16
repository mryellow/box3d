---
title: Review round 16 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round15.md
mode: full pass
---

# Round 16

Target at revision 16 (1521 lines). The owner chose to continue the series after round 15's
report, to round 20 at most unless it converges first.

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T10:37:07+10:00 to 10:42:38+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14224-35b9-7fb0-8675-4b50e2a475e7`. The stream emitted the final message twice; the last copy
follows (the reviewer numbered its sections as bold list items this round; reproduced as sent).

## Review report (Codex final message)

1. **Summary**

The implementation prompt has several correct boundaries, but it is not ready to hand to an implementing agent. Two tree tests do not establish the layout condition they claim, the large-island timing fixture lacks a way to create that island, and the hash instructions conflict over a mesh contact’s array header.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H0 | “Every field except `manifolds`” includes a mesh contact’s `triangleCache` pointer and capacity, while the same instructions prohibit hashing pointers and prescribe hashing cache elements. Hashing allocation details would reject equal engine states. | Explicitly exclude the cache header’s pointer and capacity; hash its count and active elements, and only the active union member. |
| 2 | L | §2 | The wake inventory says there are seven force, torque and impulse calls. `src/body.c` and `include/box3d/box3d.h` expose six with a `wake` argument. | Say six and name them. |
| 3 | M | §4 H0, H3c | H0 says a graph contact spec’s `manifoldCount` is written at graph add. `src/physics_world.c` also updates it during collision processing. H3c does not identify that element write when it makes graph storage read-only outside `src/constraint_graph.c`. | Add the collision-pass writer and specify its hot element write route in H3c. |
| 4 | M | §4 H5; §6 | `HistoryTreeTest` compares visit order after rewind with order after an additional deliberate rebuild. That proves those two layouts differ, but does not prove the restored layout differs from the original run’s layout as the trees doc §10 test 1 requires. | Record the original order at T and compare it with the order immediately after `Rewind(T)`, before perturbing. |
| 5 | M | §4 H6 | The kinematic witness says the restore pass rebuilds the tree in full. The specified pass moves listed proxies; the trees doc §6 places any resulting rebuild in the first replayed step and makes it conditional. | Build the witness around the specified proxy moves, and verify a rebuild separately if its cost is being claimed. |
| 6 | M | §7 | The 4,096-box “one sleeping island” timing fixture is underspecified. `src/body.c` gives each body created asleep its own set, and `src/broad_phase.c` runs no sleeping-tree self-pair pass; touching asleep-created boxes alone do not become one island. Waking one would not exercise a 4,096-proxy undo. | Specify how the slab is connected before sleeping, then assert its set size and that the timed undo fires the intended sleeping-tree rebuild. |

3. **Checked, no change**

- The ring’s stated window, interval-widening sequence, and tick-0 retention examples agree with the ring doc §§5–8.
- The serializer version, user-data scrubbing, tree fields, and missing migration queue agree with `src/world_snapshot.c`; H1 provides routes for the needed additions.
- The phase-0, H5, and H6 cost bounds distinguish the serialized image, sleeping dynamic population, and kinematic tree as intended.
- The stated compile-boundary checks align with the current writable getters and world fields.

4. **Proposed edits**

Apply the six actions in the findings table. In particular, make the tree-order comparison observe the original and restored runs directly, and make the large-island fixture verify the event it is intended to time.

5. **Unresolved**

None.

6. **Verdict:** CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** H0's contact row said "every field except `manifolds`", which takes in the
   union and so a mesh contact's `b3Array( b3TriangleCache )` header, whose pointer and
   `capacity` are allocation state; the same walk forbids pointers. Fix: the union is skipped and
   the active member hashed, the cache by count and elements.
2. **Adopted (L).** Six functions in `src/body.c` take `wake`; §2 said seven. Fix: six, named.
3. **Adopted (M).** `src/physics_world.c`'s collide pass writes a touching mesh contact's spec
   `manifoldCount` in its colour; H0 named graph add as the only writer and H3c gave the write no
   route once colours are read-only outside `src/constraint_graph.c`. Fix: H0 names the second
   writer; H3c routes it through a colour hot element accessor under the step mark, the colour
   arrays' instance of the awake-array hot accessor the section already has.
4. **Adopted (M).** The witness compared the order after the rewind with the order after the
   perturbation, which shows the perturbation took and not what the trees doc §10 test 1 states,
   that the replay runs on a layout the original run did not have. Fix: the order is recorded at
   tick 10 in the original run and compared with the order after the rewind and the perturbation.
5. **Adopted (M).** The trees doc §6 has the restore pass move listed proxies and leaves any
   kinematic rebuild to the first replayed step, conditional on the tree being left unordered or
   marked; round 15's witness said the pass rebuilds in full. Fix: the kinematic case uses the
   same witness as the others, the perturbation's full rebuild making the difference.
6. **Adopted (M).** A body created asleep is a set of its own (`src/body.c`) and the broad phase
   creates no pairs between sleeping proxies, so 4,096 asleep-created touching boxes are 4,096
   islands and waking one re-inserts one proxy. Fix: the slab is created awake and stepped to
   sleep, so touching contacts link it into one island that sleeps as one set; the harness asserts
   the shared `islandId` through the internal headers and confirms the rebuild ran.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 16: 6 findings (H0-M5-L1), 6 applied, 0 declined. Series: 75 findings, 75 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> pending
- Tripwire: fixes add or remove a named mechanism: `b3Graph_HotContactSpec`, an instance of the
  hot element accessor H3c already defines for the awake arrays, for a writer the inventory had
  missed; nothing else. Clustering: §4 H0 (rounds 2, 5, 13, 16), the tree test (rounds 11, 13,
  15, 16: each round's witness was corrected by the next; the condition now quotes the trees doc),
  §7's million-body fixture (rounds 9, 12, 15, 16). Findings 4, 5 and 6 are consequences of
  rounds 11, 15 and 15 respectively; 1, 2 and 3 of revision 1. Each fix is a clause or a short
  paragraph. Reviewer read of round records or changelog: none in the raw log. Doc length: 1521
  -> 1534 lines. Above the split-check length (U-T6): no new decision, no split.
- No High finding this round, the first such round since round 3. Three of six were consequences
  of the series' own recent fixes.
