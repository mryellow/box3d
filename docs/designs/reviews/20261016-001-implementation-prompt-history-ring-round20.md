---
title: Review round 20 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round19.md
mode: full pass
---

# Round 20

Target at revision 20 (1574 lines). The last round of the block the owner authorised after round
15.

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T11:07:57+10:00 to 11:14:55+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14240-6155-7ac3-921b-5c3cb181181a`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely matches the three binding designs and the cited source. Two implementation mechanics need to be specified before handoff; a separate sensor scheduling ambiguity comes from the binding design.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H4 | Proxy create, destroy and move must journal through `world->history`, and the proposed leaf-box assertions need `fatAABBs`. The current choke points take only `b3BroadPhase*`; `test/test_awake_tree.c` also calls them on a standalone broad phase with no world. The prompt gives no route that satisfies both requirements. | Specify world-aware signatures and migrate the standalone fixture, or specify another explicit route for world-backed and standalone calls. |
| 2 | M | §4 H5 | The sizing pass calls `b3DynamicTree_GatherMovedProxies` into a scratch array, but `src/dynamic_tree.c` writes to that array without a capacity argument or count-only mode. Reserving `proxyCount` slots for one moved static proxy makes capture scratch memory proportional to the static population, contrary to the design §2 cost bound. | Specify an O(moved-subtree) count pass before reserving scratch space, then gather into the sized array. |

### 3. Checked, no change

- Verified §2’s serializer, recording, replay, scalar-writer and `b3Shape_SetFilter` claims against the cited source.
- Checked the ring’s widening and eviction sequences in §4 against the ring doc §§5–7.
- Checked the H2–H4 compile-boundary examples against the current types and call sites.
- Checked the benchmark scene count, command options, test runner filtering and event fields used by the proposed digest.

### 4. Proposed edits

Resolve findings 1 and 2 in H4 and H5 respectively, including any changed test fixture and file list.

### 5. Unresolved

The design §6 describes sensor-overlap gathers within a parallel pass over the awake population, while requiring overlaps to be captured even with zero awake bodies. The prompt repeats that scheduling wording in H5. Because it originates in a binding design, the sensor work schedule needs design-level clarification.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `b3BroadPhase_CreateProxy`, `DestroyProxy` and `MoveProxy` take
   `b3BroadPhase*` (`src/broad_phase.h`); `fatAABBs` is a world array (`src/physics_world.h`);
   `test/test_awake_tree.c` runs one case on a `b3CreateBroadPhase` with no world. The entry and
   the leaf assertion need the world. A back pointer on the broad phase would be a field the
   designs do not name (§3). Fix: the three take `b3World*` and the standalone case moves onto a
   world's broad phase.
2. **Adopted (M).** `b3DynamicTree_GatherMovedProxies( tree, int* )` takes no capacity and has
   no count-only mode (`src/dynamic_tree.h`); sizing its scratch by the proxy count is O(static
   population) per capture. Fix: a count function that descends moved nodes only, in the tree
   files, sizes the array; the files join H5's list.

**Unresolved item (reviewer's).** The design §6 words the gathers as "a parallel-for over the
awake population" and lists the sensors' `overlaps2` among them; with no awake body a literal
reading gathers no sensor, while §5.1 images every sensor's overlaps. A design question for the
owner. The prompt now states its reading (the loop's items are the awake kinds and every sensor)
as a scheduling mechanic, marked as recorded for the owner, who may move it into the design or
overrule it.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 20: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 89 findings, 89 applied, 0
  declined. Two Unresolved items recorded for the owner over the series (rounds 15 and 20), both
  design questions.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: `b3DynamicTree_CountMovedProxies`, a read-only
  count the design §6's own enumeration implies; the choke points' signatures. No new structure
  (AC-1). Clustering: §4 H4's trees bullet (rounds 5, 7, 9, 20), §4 H5's sizing pass (rounds 9,
  20). Both findings of revision 1. Each fix is a clause or a short paragraph. Reviewer read of
  round records or changelog: none in the raw log. Doc length: 1574 -> 1588 lines. Above the
  split-check length (U-T6): no new decision, no split.
- End of the owner's second block (rounds 16 to 20): 20 findings in five rounds, none High in the
  last three, eight of the twenty consequences of the series' own earlier fixes. The owner decides
  whether to continue, hand off, or reconsider Part B (U-S8; B8).
