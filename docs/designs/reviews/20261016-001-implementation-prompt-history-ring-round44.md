---
title: Review round 44 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round43.md
mode: full pass
session: 01a14319-2d4b-7250-878b-ad041138ac26 (resumed to finalize)
---

# Round 44

Target at revision 42 (1755 lines).

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

Run: foreground, `timeout 540`, exit 124 (work timeout after 112 source reads and no final
message), 2026-10-16T15:04:45+10:00 to 15:13:45+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a14319-2d4b-7250-878b-ad041138ac26`. Resumed per the
workflow's Running-a-round row with the same model and effort and the instruction "Finalize the
review now from the research already done, with no further tool calls", 15:13:56 to 15:14:36,
exit 0 (U-R8: provenance recorded). The resumed stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the three binding designs and the live source, including the serializer, tree write routes, and ring transitions. Two instructions need clarification before an implementing agent can follow the stated scope and measurement procedure without making its own decision.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §3, §4 H4, §6 | §3 permits compiler-named files outside an increment’s file list only for H2 and H3a–H3c. H4’s file list also permits them, and §6 requires an H4 compile-boundary check. If H4’s type changes name an unlisted file, the instructions both permit the fix and require a stop. | Include H4 explicitly in §3’s compiler-named-file exception. |
| 2 | M | §4 H1, §7, §8 | §7 requires phase-0 registry bytes per scene, but H1 exposes only slot charges; it deliberately excludes the registry and reader allocations from those charges and from `b3HistoryInfo`. The specified harness has no way to obtain the requested figure or a definition of which of those allocations it includes. | Define the byte total and specify scratch instrumentation or a read-only measurement route. |

### 3. Checked, no change

- Verified §2’s principal source claims about snapshot fields, geometry interning, filter proxy recreation, tree writes, step capture point, test runner, and benchmark options.
- Checked H1’s interval-widening and window examples against the ring doc; their stated tick sequences are consistent.
- Checked H5–H6’s imaged and derived tree split against the trees doc.
- Checked the named compile-boundary examples against the current headers and source.

### 4. Proposed edits

Make the §3 exception include H4. In §7, state exactly how phase-0 registry and reader bytes are counted and obtained by the scratch harness; carry that definition into §8’s closure requirement.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the document and the source (U-V2).

1. **Adopted (M).** §3's exception named "the boundary commits", defined in the bullet above it
   as H2, H3a, H3b and H3c; H4's file list ends "and any file the compiler names (§3)" and §8
   lists H4 among the compile-boundary diagnostics, so an unlisted file H4's type changes named
   was both permitted and a stop. Fix: §3 names H4, for the containers it hides.
2. **Adopted (M).** H1 keeps the registry's and reader's bytes out of every `b3HistoryInfo`
   field and said only that the closure doc reports them; nothing defined the figure or how a
   harness obtains it. `b3GeometryEntry` (`src/recording.h`) and `b3RegistrySlot`
   (`src/replay.h`) each carry a `byteCount`. Fix: §7 defines the figure as the two sums and reads
   it in a scratch build through the internal headers; H1 points at §7.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 44: 2 findings (H0-M2-L0), 2 applied, 0 declined. Series: 151 findings, 150 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §3's file rule (rounds 10,
  44; finding 1 is a consequence of round 17's addition of the compiler clause to H4's list
  without the matching §3 change), §7's phase-0 figures (rounds 23, 44; finding 2 is a
  consequence of round 23's decision to keep the registry bytes out of the info struct without
  defining the figure). Both findings are consequences of earlier fixes. Each fix is a clause or
  two. Reviewer read of round records or changelog: none in the raw log. Doc length: 1755 ->
  1759 lines. Above the split-check length (U-T6): no new decision, no split.
- Fourth timed-out round (28, 34, 43, 44), each finalized on resume; the reviewer read 112
  files this round, the most of the series.
