---
title: Review round 14 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round13.md
mode: full pass
---

# Round 14

Target at revision 14 (1476 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T08:20:48+10:00 to 08:27:28+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a141a7-5a4a-7ce3-abdc-6ac94669b63e`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The increments largely follow the three binding designs, and the source claims I checked support their mechanics. One boundary instruction is internally inconsistent and needs resolution before implementation; the verification mapping also understates the assertions it requires.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §3; §4 H3a | H3a requires restore writers to assert that the world is inside rewind or deserialization, as the design §5.2 requires. The stated writer signatures have no context argument, the live source has no restore mark, and §3 permits only two new assertion flags: player world and inside step. The required assertion cannot distinguish a restore from an ordinary between-step call under those rules. | Specify a restore scope set by both `b3World_Rewind` and `b3DeserializeIntoShell`, and explicitly permit its assertion state in §3. |
| 2 | L | §6 | The design §10 mapping calls for inspection of “the three assertions,” while H1 requires checks in `EnableHistory`, `StartRecording`, `SeekFrame` and `Restart`; `EnableHistory` checks both recording and player status. The count could leave a required site unchecked. | Name each assertion site and predicate in the mapping instead of giving a count. |

### 3. Checked, no change

- The section 2 descriptions of the record layouts, id pools, serializer omissions, filter behavior, step capture point and tree write sites match the cited source.
- The H1 budget and window examples follow the ring doc’s widening and eviction rules.
- The H4 proxy entries and H5/H6 tree image choices follow the trees doc; the stated compile boundaries use members and qualifiers that exist in the preceding source layout.
- The test runner, event fields, benchmark table and public sensor overlap queries support the proposed test and measurement mechanisms.

### 4. Proposed edits

Resolve the restore assertion conflict in §3 and H3a, then replace §6’s assertion count with the explicit H1 check sites.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** The design §5.2 gives the restore writer one assertion, "inside
   `b3World_Rewind` or inside `b3DeserializeIntoShell`"; H3a wrote "the rewind or deserialize
   mark" and §3 allowed two marks (player, step) only, so the writer could not be built as both
   sections stand. The source has no field that distinguishes a restore. Fix: the restore mark,
   `world->inRestore`, set by `b3DeserializeIntoShell` around its writes and by `b3World_Rewind`
   around steps 2 and 3 from H4, read by the restore writers' assertions only; §3 counts three
   exceptions and §10's entry is "The three marks" with the design clauses that need each. It is
   the same kind as the two the gate admitted: assertion-only, never control flow (AC-4).
2. **Adopted (L).** H1 names four sites with five predicates (`b3World_EnableHistory` on the
   recording and the player mark, `b3World_StartRecording`, `b3RecPlayer_SeekFrame`,
   `b3RecPlayer_Restart`); §6 said "the three assertions". Fix: the row names the sites and
   predicates.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 14: 2 findings (H0-M1-L1), 2 applied, 0 declined. Series: 63 findings, 63 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: one assertion-only `bool` (the restore mark),
  the third of its kind, needed by an assertion the design §5.2 names; read by assertions only
  (AC-4), no shadow structure (AC-1). Clustering: §3's exception rule (rounds 1, 14), §4 H3a
  (rounds 3, 5, 8, 14), §6 (rounds 2, 4, 14). Finding 1 is a consequence of revision 1 (H3a
  named a mark §3 did not admit); finding 2 of revision 1. Each fix is a clause or a short
  paragraph. Reviewer read of round records or changelog: none (the changelog paths appear in a
  `wc -l` listing only). Doc length: 1476 -> 1484 lines. Above the split-check length (U-T6): no
  new decision, no split.
