---
title: Review round 38 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round37.md
mode: full pass
---

# Round 38

Target at revision 37 (1725 lines).

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

Run: foreground, `timeout 540`, exit 0, 2026-10-16T14:12:47+10:00 to 14:18:11+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a142e9-9763-7913-b9ab-52fa6c5c2448`. The stream emitted the final message twice; the last copy
follows.

## Review report (Codex final message)

### 1. Summary

The ring budget rules and the stated capture point agree with the binding designs and live source. Three implementation instructions need changes: one describes a contact the engine cannot create, one leaves geometry ownership unsafe, and one makes a replay assertion depend on an unspecified setter.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H0, `HashSensitiveCase` | The fixture places a compound hull child against a static box, then expects a touching compound contact whose `satCache` can be changed. Compound shapes are static-only, and `b3CreateContact` rejects static–static contacts (`src/shape.c`, `src/contact.c`). The stated fourth contact cannot exist. | Place a dynamic hull against the compound child; identify that contact and assert it exists before changing its cache. |
| 2 | M | §4 H1, registry and reader | The instruction says to append registry entries to reader slots “as `b3RecSeedKeyframeRegistry` does,” but that function seeds a registry **from** reader slots (`src/replay.c`). Reader slots and registry entries each own and free their `bytes` (`src/replay.c`, `src/recording.c`). Aliasing those bytes would double-free them at disable; the prompt does not specify the required ownership route. | Specify separate byte copies for reader slots, or specify a non-owning reader-slot route and its teardown. |
| 3 | M | §4 H1, `HistoryReplayTest` | The corrected replay drops only “one setter op” and requires a differing hash or event. The op log includes `b3World_SetUserData`; the hash excludes world `userData`, and the stated callbacks do not read it. Dropping that setter can leave the required witness unchanged on a correct implementation. | Name a physics-affecting setter and value, and state the first expected differing tick or event. |

### 3. Checked, no change

- §4 H1’s exact interval-widening and eviction sequences match the ring doc §6–§7.
- The step capture location and the static, sleeping, awake, and kinematic tree roles match the live broad-phase and solver paths.
- The H2–H4 compile-boundary examples match the described type changes.
- The test runner supports the named-entry and `condition false:` evidence procedure.

### 4. Proposed edits

Apply the three changes in the findings table before handing the prompt to an implementing agent.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2).

1. **Adopted (H).** `b3CreateShapeInternal` refuses a compound or height shape on a non-static
   body (`src/shape.c`) and `b3CreateContact` asserts the two bodies are not both static
   (`src/contact.c`); round 36's fixture rested a compound's child on the static box. Fix: a
   static compound with a second dynamic hull resting on its hull child, every probed contact
   `ENSURE`d touching first.
2. **Adopted (M).** `b3RecSeedKeyframeRegistry` appends the reader's slots into the player's
   registry, the reverse of what H1 cited it for, and a `b3RegistrySlot`'s bytes are "always
   freed at teardown" (`src/replay.h`, `src/replay.c`) while the registry frees its entries'
   bytes (`src/recording.c`). Fix: each appended slot holds its own copy of the entry's bytes,
   both teardowns named, the player's direction stated as the mirror.
3. **Adopted (M).** The op log carries `b3World_SetUserData`, which the hash excludes, so "one
   setter op dropped" could leave the corrected replay equal to the record. Fix: the dropped op
   is the churn's tick-70 `b3Body_SetLinearVelocity` on an awake body and the witness is the
   tick-70 hash.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 38: 3 findings (H1-M2-L0), 3 applied, 0 declined. Series: 137 findings, 137 applied, 0
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: none. Clustering: §4 H0's
  `HashSensitiveCase` (rounds 22, 32, 33, 36, 38; finding 1 is a consequence of round 36's
  compound fixture), §4 H1's registry bullet (rounds 6, 23, 38; finding 2 present since revision
  1), §4 H1's `HistoryReplayTest` corrected replay (rounds 29, 38; finding 3 is a consequence of
  round 29's "setter op" wording). Two of three findings are consequences of the series' own
  fixes; `HashSensitiveCase` has produced a finding in three of the last seven rounds, each on
  the fixture added the round before (U-S4: the fixture is now fully specified, each contact
  `ENSURE`d; if it recurs, pause and report). Each fix is a clause or a short sentence. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 1725 -> 1730 lines.
  Above the split-check length (U-T6): no new decision, no split.
- Third consecutive round with a High since the clean round 35 (36, 37, 38): U-S1 names three
  consecutive rounds each finding a substantial structural problem; these three Highs are a
  scatter list, a memory budget and an impossible fixture, none structural, so the series
  continues and the owner is told at the next report.
