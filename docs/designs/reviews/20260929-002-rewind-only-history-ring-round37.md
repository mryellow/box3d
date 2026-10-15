---
title: Codex review - rewind-only history ring, round 37
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round36.md
mode: full pass at the split scope (U-T6): the round 22 prompt with section 8's rule removed and the ring doc added to the cited documents, on revision 37
session: 01a13e75-ba51-7662-b41a-c87a07ca15c8, resumed once to finalize
---

# Round 37

## Exact request sent to Codex

```
Review the design document docs/designs/20260929-002-rewind-only-history-ring.md as an adversarial design reviewer for the Box3D physics engine. It is the design for a rewind-only, bit-exact world history: a hot image of the awake state captured per tick into a ring, an undo journal of every write to cold state, and a public Rewind API that restores the whole world to a retained tick and discards everything after it.

Scope: the document's own content and the source it cites (src/solver.c, src/broad_phase.c, src/broad_phase.h, src/physics_world.c, src/shape.c, src/sensor.c, src/body.c, src/world_snapshot.c, src/dynamic_tree.c, src/dynamic_tree.h, src/constraint_graph.c, src/solver_set.c, src/island.c, src/contact_solver.c, src/contact.c, src/joint.c, src/id_pool.c, src/core.c, src/compound.c, src/name_cache.c, src/recording.c, src/replay.c, src/recording_replay.c if it exists, include/box3d/, test/test_determinism.c, test/test_recording.c, shared/benchmarks.c), and the design documents it cites only where it cites them (docs/designs/20261012-001-preflight-sorted-candidates-tree-order-independent-solver.md, docs/designs/20261012-005-preflight-sweep-order-continuous-candidates.md, docs/designs/20261013-001-preflight-awake-tree-split.md, docs/designs/20261015-003-history-ring-budget-and-eviction.md (the ring, which section 8 cites; its own rules are reviewed in its own series, so a finding about them goes under Unresolved), docs/designs/20260927-002-bit-exact-history-ring.md, docs/designs/20260927-001-client-prediction-rollback.md, docs/faq.md, docs/simulation.md). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it. The source has changed since the document was first written: the dynamic-body broad-phase tree is now split into an awake tree and a sleeping tree with a migration queue, and the continuous and explosion candidates are sorted; check the document against the code as it is now.

Pressure-test the design. Find real defects: claims about the code that are false; a simulation-affecting byte of state that the state inventory in section 5 neither images nor journals; a write to cold state that reaches a field without passing through the accessor or journaled container type the design names; a journal entry whose undo does not restore what its mutation changed, or whose ownership rule leaks or double-frees a block; a restore order in section 9 that reads something an earlier step already overwrote; a tree, proxy id, moved bit or migration-queue case the trees section misses under the awake and sleeping trees; a wrong or inconsistent number; contradictions between sections, including between the API sketch in section 14 and the caller contract in section 10. Treat existing primitives (b3DynamicTree, the solver sets, the constraint graph, the id pools, the pair set, the recording serializer) as settled tools; judge what this design adds and the question each primitive is asked. Decisions recorded in section 17 are settled unless you have fresh source evidence that their stated reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, the open question in section 16, and test mechanism, fixtures and test-to-requirement mapping, which belong to a later Implementation Prompt. Do not review or follow any other design document except where this one cites it. Do not read docs/designs/reviews/ or any changelog.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(11,040 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T17:27:43+10:00
to 17:36:43. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 17:37:03 to 17:37:55. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13e75-ba51-7662-b41a-c87a07ca15c8`.

## Review report (Codex final message)

1. **Summary**

The design’s hot image, cold journal, and rewind sequence are consistent with the inspected source. I found no substantiated defect within the requested scope.

2. **Findings**

None.

3. **Checked, no change**

- The source has separate awake and sleeping dynamic trees, with marked sleeping leaves parked in the awake tree and a migration queue drained after pair discovery.
- Pair creation, continuous candidates, and explosion candidates use the ordering described in §3 and §7.4.
- The inspected contact, sensor, shape, solver set, island, pool, and proxy lifetimes fit the stated image and undo ownership rules.
- The API sketch’s correction sequence agrees with §10’s convention that calls between steps are inputs to the next tick.

4. **Proposed edits**

None.

5. **Unresolved**

None within this review. The ring rules delegated by §8 belong to the cited ring design’s review.

6. **Verdict**

CONVERGED

## Post-review verification (Claude)

No findings to verify; local consistency only, not architectural endorsement (U-G4). The reviewer
placed the ring's rules under Unresolved as belonging to the ring doc's own series, which is the
scope the prompt set.

Raw log: the reviewer opened no round record or changelog; five changelog filenames appear in one
directory listing it took of `docs/designs/`, none of them read.

## Status

Zero findings; verdict CONVERGED on revision 37, the first round after the split. Not yet settled:
the confirmation round (U-P9) is next, round 38, same prompt, same scope. The status word stays
Draft (U-I5).

Tripwire (U-T): no fixes, no mechanism added or removed (U-T1); no findings to cluster (U-T2);
doc length 1437 -> 1338 by the split, nothing added this round (U-T3). Split check (U-T6): the
ring is out; what remains (state classification, write boundary, journal, trees, restore, hash,
API) is one decision on one mechanism.

Round 37: 0 findings (0H-0M-0L), 0 applied, 0 declined
Series total: 98 findings (18H-50M-30L) across 37 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L)
