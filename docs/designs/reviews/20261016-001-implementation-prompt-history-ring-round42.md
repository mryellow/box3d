---
title: Review round 42 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round41.md
mode: full pass (confirmation round after round 41, U-P9)
---

# Round 42

Target at revision 40 (1743 lines), unchanged since round 41.

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T14:44:05+10:00 to 14:51:46+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14306-406c-7901-a234-4827ce1ec65e`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely matches the three binding designs and the cited source. I found three mechanics that need tightening before an implementing agent can rely on the stated test and measurement outcomes.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H0, H5, H6 | The layout tests say to rebuild “with more candidates” if visit order does not change. A full rebuild cannot add candidates; adding shapes after the baseline hash or rewind target changes the state being compared. `test/test_tree_order.c` uses staged insertions to establish a differing-order witness. | Specify a fixed fixture that produces different visit orders, or restart the case with an expanded fixture and record a new baseline before perturbing it. |
| 2 | M | §7 | The quarter-of-unbounded-budget run is described as evicting at interval 1. That is not assured for a front-loaded scene: `CreateLargeWorld` marks its million static proxies on creation (`shared/benchmarks.c`), so an image taken before the first step can exceed that budget. The ring doc §6 then widens the interval permanently at the next capture. | State when history is enabled relative to warm-up, and choose an interval-1 budget using the largest measured minimum-window charge. Otherwise report the observed interval without claiming it remains 1. |
| 3 | L | §4 H1, `HistoryRingTest` case 7 | Skipping an unspecified logged operation does not guarantee that the original and re-stepped tick-13 hashes differ. The churn includes `userData` setters, which H0 explicitly excludes from the hash, so the named inequality could fail on a correct implementation. | Name a hash-visible operation whose effect persists through tick 13, and assert that the fixture produced that effect. |

### 3. Checked, no change

- §2’s record, container, tree-write, serializer, recording, replay, and benchmark inventories agree with the cited source on the routes material to these increments.
- The H1 ring test’s stated window and interval-widening sequences agree with the ring doc §§5–7.
- The H2–H4 compile-boundary examples use types or members that become inaccessible at the named commits.
- The H5 and H6 cost cases have credible fail-before checks: the serializer images the sleeping dynamic population before H5, and phase 1 images the kinematic tree before H6.

### 4. Proposed edits

Apply the actions in findings 1–3, then update the affected test instructions and §7 measurement wording consistently.

### 5. Unresolved

None beyond the choices identified in the proposed edits.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (M).** `PerturbTreeLayout` (`test/test_tree_order.c`, `test/test_awake_tree.c`)
   calls `b3DynamicTree_Rebuild( tree, true )` on every tree and takes nothing else, so "rebuilt
   with more candidates" named no mechanism. The tree-order tests get a differing witness from
   the fixture: the corridor plan creates its boxes in two parts around a step, so the
   incremental layout is not the one a full rebuild produces, and `WitnessDiffers` is `ENSURE`d.
   Fix: `HashLayoutCase` and `HistoryTreeTest` stage their shapes the same way and the retry
   clause is deleted (U-F4).
2. **Adopted (M).** Static proxies are marked moved at creation and the first step's pair
   update clears them; H5's image carries the static tree's moved list, non-empty only in tick
   0's image, so a harness that enables history before the first step on `large_world` images a
   million ids in its first slot and a quarter of the steady state can be below the minimum
   window, widening the interval at the next capture (the ring doc §6 step 4). Fix: the harness
   enables history after ten warm-up steps as `HistoryCostTest` does; the quarter budget is
   "expected" to evict at interval 1 and the observed interval is reported, as already stated.
3. **Adopted (L).** Case 7 skipped "one op" on the re-step; the churn includes `userData`
   setters the hash excludes. Fix: the skipped op is the churn's tick-11 body create, which the
   body array and id pool show at 13.

Checked, no change items agree with the source read for the revision. The reviewer's
Unresolved line names no further item.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round of round 41's clean result found three
  findings, so the series is not converged (U-P9, U-S8); a fresh clean round and its
  confirmation are required.
- Round 42: 3 findings (H0-M2-L1), 3 applied, 0 declined. Series: 146 findings, 145 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: the retry clause ("rebuilt with more
  candidates") is removed from two tests. Clustering: §4 H0's `HashLayoutCase` (rounds 13, 37,
  42; finding 1 is a consequence of round 37's witness fix, which copied H5's wording), §7's
  budget measurements (rounds 21, 24, 25, 28, 31, 40, 42; finding 2 is a consequence of round
  40's eviction claim), §4 H1's `HistoryRingTest` case 7 (rounds 4, 42; finding 3 present since
  revision 1). Two of three findings are consequences of the previous rounds' fixes; §7's
  measurement bullet has now produced a finding in seven rounds (U-S4: each fix was a clause;
  the bullet states its budgets, timing and expectations to the number, and a recurrence is
  reported to the owner). Each fix is a clause or two. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 1743 -> 1749 lines. Above the split-check length
  (U-T6): no new decision, no split.
- Second clean round followed by a non-clean confirmation (35/36, 41/42); both confirmations
  found only fixture facts, none against an increment's mechanics or a design clause.
