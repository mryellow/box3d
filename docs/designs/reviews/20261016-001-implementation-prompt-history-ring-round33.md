---
title: Review round 33 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round32.md
mode: full pass
---

# Round 33

Target at revision 33 (1691 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T13:25:23+10:00 to 13:29:36+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142be-348a-7081-83a4-b93067ef5dc6`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the three binding designs, but several instructions need tightening before an implementing agent can follow them without making choices the prompt should settle. The most consequential gap is at the H3c compile boundary; two test descriptions also need correction.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3c–H4 | H3c makes solver-set and graph arrays read-only outside their owning files, but `src/body.c` and `src/joint.c` currently append a sim and then fill it through a writable pointer. H3c does not specify how those constructors produce completed values or perform awake-array pushes; H4 names the exclusive push functions only in the following increment. As written, the H3c compile boundary requires an unstated refactor. | Specify the H3c creation and push routes, including joint construction across set merges and graph insertion. H4 can add journaling to those routes. |
| 2 | M | §4 H0, §6 | After H3b, `HashSensitiveCase` puts the capsule’s body to sleep before changing its mesh triangle cache, then compares against the original scene hash. Sleep already changes hashed state, so the check passes even if the hash omits that cache. It does not show the cache sensitivity assigned to it in §6. | Compare hashes immediately before and after the cache-only edit, with the body in the same state for both. |
| 3 | M | §4 H4 | `OwnershipCase` calls for eviction “on a `tickCount` 4 ring,” but its enable configuration is not specified. `tickCount` cannot be changed after enable, so the stated allocation-count result is not guaranteed by the procedure. | Set `tickCount = 4` before enabling this case and state which slot owns the destroyed contact’s block. |
| 4 | L | §4 H6 | `SleepingKinematicCostCase` says bodies created asleep must sleep “on [their] timer.” `src/body.c` places an enabled body with `isAwake = false` directly in a sleeping set. The timer is not what makes this fixture sleeping. | Describe the direct sleeping creation and retain the awake-count check for the sixteen awake boxes. |

### 3. Checked, no change

- The ring’s interval, minimum-window, over-budget and rewind rules in §4 match the ring doc §§3–8.
- The H5/H6 split between imaged and derived trees, and the restore pass, match the trees doc §§5–9.
- The serializer omissions, recording hash scope, proxy write routes, filter behavior, event fields and benchmark commands cited in §2 match the live source.
- The H2, H3a, H3b and H4 compile-boundary examples reference members and types present at their stated pre-boundaries.

### 4. Proposed edits

Resolve finding 1 before implementation. Make the cache probe isolate one edit, fix the H4 ring definition, and correct the H6 fixture explanation.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `b3CreateBody` (`src/body.c`), `b3CreateJointInternal`'s set branches
   (`src/joint.c`), `b3CreateIsland` (`src/island.c`) and `b3CreateJointInGraph`
   (`src/constraint_graph.c`) each `b3Array_Emplace` a zeroed element and fill it through the
   returned pointer; H3c made the set and colour arrays `b3ConstArray` outside their files and
   named "push" functions without saying they take values, and H4 named the awake pushes as new.
   Fix: H3c's pushes take completed values and return the local index, the graph's two choose the
   colour as today, each constructor builds a local and pushes once, the sleeping joint branch
   keeps today's push-then-merge order (array order is a solver input, §5), and H4's journaling
   pushes are those functions' awake forms.
2. **Adopted (M).** Round 32's fixture compared the mesh-cache probe against the scene's hash
   after sleeping the capsule's body, and the sleep alone changes the hash. Fix: that copy's
   reference hash is taken after the sleep.
3. **Adopted (M).** Case 1 enables history without naming `tickCount`, and case 3 ran "on case
   1's scene" then spoke of "a `tickCount` 4 ring"; `b3World_EnableHistory` fixes `tickCount`
   and asserts `history == NULL`. The ring doc §7 keeps the slots from `historyTick` −
   `tickCount`, so a slot is evicted five steps after it is written at `tickCount` 4. Fix: case 3
   enables at `tickCount` 4, the step after the destroy writes the owning slot, five more steps
   evict it.
4. **Adopted (L).** `b3CreateBody` computes `isAwake` from the def and puts an enabled body with
   it false into a fresh sleeping set (`src/body.c`); no timer runs. Fix: the parenthesis says so
   and keeps the awake-count `ENSURE`.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 33: 4 findings (H1-M2-L1), 4 applied, 0 declined. Series: 125 findings, 125 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> pending
- Tripwire: fixes add or remove a named mechanism: H3c names the value-taking push functions
  (the design §5.2's container rule applied to the constructors; H4's journaling pushes were
  already named, so no new structure). Clustering: §4 H3c's solver-set bullet (rounds 3, 16, 25,
  27, 33; finding 1 is a gap present since the H3c/H4 split in revision 1), §4 H0's
  `HashSensitiveCase` (rounds 22, 32, 33; finding 2 is a consequence of round 32's fix), §4 H4's
  `OwnershipCase` (rounds 17, 27, 33; finding 3 present since revision 1), §4 H6's cost case
  (rounds 12, 33; finding 4 present since revision 1). One of four findings a consequence of the
  previous round's fix. Finding 1's fix is one sentence of ten lines; the rest are clauses.
  Reviewer read of round records or changelog: none in the raw log. Doc length: 1691 -> 1703
  lines. Above the split-check length (U-T6): no new decision, no split.
- Finding 1 is the first High since round 29 and the first against a boundary commit's own
  mechanics since round 27; it would have left H3c's implementer to invent the push interface.
