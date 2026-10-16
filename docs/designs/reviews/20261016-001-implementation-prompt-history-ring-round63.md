---
title: Review round 63 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round62.md
mode: full pass
session: 01a14466-ec72-7b62-a479-617483c8174a
---

# Round 63

Target at revision 60 (1942 lines).

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
Run: foreground, `timeout 540`, exit 0 after 52 source reads, 2026-10-16T21:09:18+10:00 to
21:17:03+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14466-ec72-7b62-a479-617483c8174a`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely follows the three Canonical designs and the current source. Two defects need edits before handoff: H1’s image-size check can miss an overflow, and H4’s proxy-move test can pass without exercising the journal entry it claims to verify.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1 | The promised `INT_MAX` capture guard is insufficient. `src/world_snapshot.c` multiplies array counts by element sizes before calling `b3RecBufAppend`; an overflowing `int` can become nonpositive, causing `b3RecBufAppend` to skip the bytes before its proposed `countOnly` assertion runs. | Require checked `size_t` byte calculations at every serializer array write, with an assertion before conversion to `int`. |
| 2 | M | §4 H4; §6 | `HistoryJournalTest`’s in-traversal proxy-move case uses `b3World_Rewind`, which still deserializes a whole-world image in H4. That image restores the sleeping tree even if the proxy-move journal entry is missing, so the named leaf-box and hash checks do not demonstrate that entry’s undo. | Add a direct journal-walk check of the moved sleeping leaf before snapshot restoration, or assign the proxy-move verification to a test under H5’s partial-image path. |

### 3. Checked, no change

- The stated ring interval, window, budget and rewind sequences agree with the ring doc §§3–8.
- The awake, sleeping, static and kinematic tree treatment agrees with the trees doc §§5–9.
- The principal source inventory matches the current record, container, serializer, recording, replay, broad-phase and test-runner code.
- H2–H4’s stated compile boundaries use types and diagnostics consistent with the planned changes.

### 4. Proposed edits

Update H1’s sizing procedure to check byte counts before serializer arithmetic narrows them to `int`. Revise the H4 proxy-move test and its §6 mapping so the named check depends on undoing that entry.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Finding 1 is against round
58's size guard; finding 2 against case 4's third part, written with H4 in revision 1.

1. **Adopted (M).** `b3RecBufAppend` (`src/recording.c`) returns at `size <= 0` before its
   `countOnly` branch, and every array write in `src/world_snapshot.c` reaches it through
   `b3SnapW_Bytes( buf, src, int n )` with the product already narrowed: `( arr ).count * (int)sizeof`
   in `b3SerPodArray`, the same form at the tree, material, manifold and triangle-cache writes,
   and `(int)( count * sizeof )` at the bitset and hash-set writes. A product past `INT_MAX`
   wraps before the assertion sees it, so the round-58 guard caught a total that crosses the
   limit by accumulation and not one array that crosses it alone. Fix: `b3SnapW_Bytes` takes
   `size_t`, its callers pass the product in `size_t`, and it asserts the bound before narrowing;
   the `countOnly` assertion stays for the accumulated total. H1 lists `src/world_snapshot.c`
   already.
2. **Adopted (M).** The phase-0 image serializes all four trees (`b3SerTree` over `bp->trees[t]`,
   `src/world_snapshot.c`), and H4's own text says the serializer image "overwrites what the
   walk restored". Case 4's third part checked the leaf's box after `b3World_Rewind`, which at H4
   loads that image, so the check held with the move entry's undo missing. Fix: the case calls
   `b3History_UndoPending` and checks the sleeping tree's leaf by the recorded proxy id against
   the pre-move box before any image loads (the migration's destroy-entry undo re-inserts the
   leaf at the moved box; only the move entry's undo returns it), then `b3World_Rewind(
   historyTick )` for the hash. Mutation check (U-F6): without the move entry's undo the box
   check fails; without the destroy entry's undo the proxy id is free. The §6 row for the trees
   doc §7 keeps "case 4's in-traversal move for the proxy move entry".

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 63: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 193 findings, 192 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> pending
- Tripwire: no mechanism added or removed; fix 1 widens one helper's parameter and moves an
  assertion to it, fix 2 reorders a test case's calls. Clustering: none with round 62 (H3c's
  graph bullet drew nothing). Finding 1 is a consequence of round 58's fix (H1's size guard,
  the fifth finding against H1's phase-0 plumbing since round 58, nine in total for that
  commit's registry and sizing text); finding 2 is against revision-1 text. Each fix is a
  sentence or two. Reviewer read of round records or changelog: none in the raw log. Doc
  length: 1942 -> 1951 lines. Above the split-check length (U-T6): no new decision, no split.
