---
title: Review round 47 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round46.md
mode: full pass
---

# Round 47

Target at revision 45 (1772 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T15:38:00+10:00 to 15:45:15+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14337-9e00-7220-b7b1-2a09e4662842`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt has one blocking increment defect: H4’s serializer restore can invalidate pointers still held by retained journal entries, making a later rewind unsafe. I also found a required H3a reader migration missing from its file list and a phase-0 measurement that cannot use the stated access path.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H4 | H4 undoes later segments, then calls `b3DeserializeIntoShell` while keeping older segments. The reader frees live hulls and reloads clones (`src/world_snapshot.c`), but an older hull-refcount entry retains the original pointer. For example, create a hull in tick 1, reach tick 2, call `Rewind(2)`, then `Rewind(0)`: the second walk uses a pointer freed by the first restore (`src/physics_world.c`). This contradicts H4’s claim that the retained ring remains usable after the serializer overwrite. | Put pointer-stable, in-place restore in H4 together with the journal, or revise the increment boundary and tests so no retained journal entry survives a serializer replacement of its pointee. |
| 2 | L | §2; §4 H3a | H3a makes `world->bodies` an incomplete pointer, yet `b3HashWorldState` in `src/recording.c` and `b3RecSeedBodyIds` in `src/replay.c` read its array directly. Both files therefore require H3a edits and are absent from its file list; the replay reader is also absent from §2’s relevant reader inventory. §3’s compiler exception permits the edits but does not make the stated list accurate. | Name both readers in §2 and both files in H3a’s file list. |
| 3 | L | §4 H1; §7 | H1 says `b3WorldHistory` is incomplete outside its implementation and that the harness reads only the exposed slot functions. §7 nevertheless directs the phase-0 payload measurement to read the recording and reader inside `world->history` “through the internal headers.” That access is unavailable as specified. | Specify temporary instrumentation inside `src/world_history.c` for the scratch build, or expose a read-only payload-size accessor for that measurement. |

### 3. Checked, no change

- The ring test’s nine retained images after 20 steps with `tickCount` 8, and its interval-doubling sequence, agree with the ring doc §6–§7.
- The stated step-end capture point follows traversal drain and proxy validation in `src/physics_world.c`.
- The filter’s always-recreate observation, sleeping proxy staging, and the static versus kinematic tree treatment agree with `src/shape.c`, `src/broad_phase.c`, and the trees doc §5–§9.
- The serializer’s omitted migration queue and existing user-data and renderer-handle behavior agree with `src/world_snapshot.c`. The H1 options address those differences.
- The H5 and H6 cost fixtures distinguish the serialized world, the phase-1 kinematic tree image, and the phase-2 derived tree.

### 4. Proposed edits

Revise H4’s restore mechanics and its two-rewind evidence first; the current nine-commit boundary is unsafe. Add the two H3a readers to the inventory and file list, then give §7 an access path that exists for the payload measurement.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `b3FreeLiveSimElements` (`src/world_snapshot.c`) releases every live shape's
   hull, which `b3RemoveHullFromDatabase` destroys at count zero, and `b3DesShapes` re-adds a
   clone from the slot bytes; a hull-refcount entry or a record's bytes in a retained slot at or
   below T keep the old pointer, and the next rewind's walk hands it to the database, whose lookup
   compares content through it. Reviewer's scenario reproduced on paper: hull at tick 1,
   `Rewind( 2 )`, `Rewind( 0 )`. The design §5.2 names the hull as the one shared pointee; every
   other entry holds bytes, ids or detached handles the reader does not touch. Of the two
   remedies offered, moving H5's in-place restore into H4 would merge two increments; the
   adopted one keeps the boundary: the reader adds the image's hulls first (content
   deduplication returns the live pointer) and releases the old references after, so a hull live
   at T keeps its pointer, while a hull live only after T is freed by the walk before the reader
   runs. `HistoryRingTest`'s H4 extension gains the scenario.
2. **Adopted (L).** `b3HashWorldState` (`src/recording.c`) and `b3RecSeedBodyIds`
   (`src/replay.c`) index `world->bodies.data` directly. Fix: §2 names both; H3a lists both
   files.
3. **Adopted (L).** H1 keeps `b3WorldHistory` incomplete outside `src/world_history.c`, so "the
   internal headers" reach nothing inside it. Fix: a scratch-build-only function of that file
   walks the registry's entries and the reader's slots.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 47: 3 findings (H1-M0-L2), 3 applied, 0 declined. Series: 159 findings, 158 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> pending
- Tripwire: fixes add or remove a named mechanism: the phase-0 reader's hull order (add, then
  release), a reordering of two existing calls that H5 removes with the rest of the serializer
  restore. Clustering: §4 H4's restore paragraph (rounds 19, 23, 27, 36, 47; finding 1 is a gap
  present since revision 1 in the interaction of the H4 walk with the phase-0 reader, which no
  earlier round modelled across two rewinds), §4 H3a's file list (rounds 12, 34, 47; finding 2
  present since revision 1), §7's phase-0 figures (rounds 23, 44, 46, 47; finding 3 is a
  consequence of round 44's access path). One of three findings a consequence of an earlier fix.
  Finding 1's fix is one sentence of six lines plus a test clause; the others are clauses.
  Reviewer read of round records or changelog: none in the raw log. Doc length: 1772 -> 1783
  lines. Above the split-check length (U-T6): no new decision, no split.
- First High since round 38 and the first of the series against an increment boundary's safety
  rather than a fixture or inventory; the boundary stands with a two-call reorder in the reader.
