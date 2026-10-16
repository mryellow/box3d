---
title: Review round 66 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round65.md
mode: full pass
session: 01a14481-ae7f-7e31-8746-9c908c71e3f1
---

# Round 66

Target at revision 62 (1958 lines).

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
Run: foreground, `timeout 540`, exit 0 after 95 source reads, 2026-10-16T21:38:31+10:00 to
21:46:47+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a14481-ae7f-7e31-8746-9c908c71e3f1`. The stream emitted the final message twice; the last
copy follows (the reviewer used bold numbered headings this round; reproduced as given).

## Review report (Codex final message)

1. **Summary**

The prompt is close, but it leaves a contact-slot initialization step unstated that is needed for H3b to remain bisectable. Its history-disabled regression gate also misses changes to event `userData`.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 H3b | `b3CreateContact` currently zeroes the entire reused record (`src/contact.c`). H3b replaces that write with an accessor that preserves the triangle-cache header. When a convex contact’s slot is reused for a mesh contact, the header can retain convex-cache bytes; the first mesh-cache read or resize can then use an invalid count or pointer. H4’s creation entry does not make H3b safe as a standalone commit. | Specify an explicit, contact-owned cache-header reset on creation in H3b, including convex-to-mesh slot reuse, and retain the required H4 journal entry. |
| 2 | M | §5 | The proposed benchmark hash excludes host `userData`, and its event digest also excludes event `userData`. The benchmark scenes assign no `userData` (`shared/benchmarks.c`), although body-move and joint events expose it (`include/box3d/types.h`). A change to either event field could pass the stated history-disabled hash and digest gate. | Add a fixed fixture with distinct non-null user-data tokens and compare those event fields across base and branch, using stable tokens for cross-process output. |
| 3 | L | §2 | The “API calls that wake a body” inventory includes `b3Shape_SetFilter(..., true)` on a sleeping dynamic body. With no touching contact, `b3ResetProxy` parks its marked leaf and queues migration but leaves the body asleep (`src/shape.c`, `src/broad_phase.c`). | Distinguish body wakes from operations that change the awake tree while the body remains asleep. |

3. **Checked, no change**

- The nine-commit order matches the designs’ phase 0–2 sequence.
- The ring test’s stated interval-widening and window sequences agree with the ring doc’s rules.
- The tree write routes, phase-1 kinematic image, phase-2 derived tree, and capture point agree with the trees doc and the live step path.
- The named fail-before build boundaries and H5/H6 size-bound failures have plausible before/after mechanisms.

4. **Proposed edits**

Add the H3b cache reset to the increment’s code steps and reused-slot test case. Extend §5’s regression comparison to cover event user-data fields, and correct §2’s wake inventory.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). All three are against
revision-1 text (H3b's mechanics, §5's gate, §2's wake inventory); none follows from a
previous fix.

1. **Adopted (H).** `b3CreateContact` zeroes the whole reused record (`*contact = (b3Contact){
   0 }`, `src/contact.c`); H3b's `b3Contact_Write` leaves `manifolds`, `manifoldCount` and the
   triangle cache header alone, and the convex cache and the mesh cache header share union
   storage (`src/contact.h`), so after H3b a reused slot carried the previous contact's pointer,
   count and header bytes into the new record, a freed manifold pointer or a garbage cache
   header among them. Fix: the pool's free function clears those fields whatever the kind, as
   H4's detach does; `HistoryRingTest` case 12 (history never enabled, added in H3b) reuses a
   convex contact's slot for a mesh contact, the id pool's LIFO free list making the reuse
   certain; the §6 row names it as a regression guard.
2. **Adopted (M).** `b3BodyMoveEvent` and `b3JointEvent` carry `userData`
   (`include/box3d/types.h`); `shared/benchmarks.c` sets none, and the digest leaves the word out
   (cross-process stability), so the base-against-branch comparison could not see a change to
   either field. Fix: the comparison adds a harness scene with distinct integer tokens as
   `userData` on every body and joint, `forceThreshold` 0 on the chain so joint events fire,
   and the digest includes the two events' words as integers for that scene.
3. **Adopted (L).** `b3Shape_SetFilter( ..., true )` calls `b3ResetProxy( world, shape, true,
   destroyProxy )` (`src/shape.c`), which wakes only through `b3DestroyContact` on the shape's
   touching contacts; a contactless sleeping body keeps sleeping with its leaf recreated in the
   awake tree and the body queued, the behaviour §2 already stated for the four geometry
   setters, which make the same call. Fix: the two entries are merged into one with that
   statement.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 66: 3 findings (H1-M1-L1), 3 applied, 0 declined. Series: 198 findings, 197 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> 0/0/2 -> 0/2/0 -> 0/0/0 -> 0/2/0 -> 1/1/1 -> pending
- Tripwire: no mechanism added or removed; fix 1 adds a clear to an existing function and one
  test case, fix 2 one harness scene, fix 3 merges two inventory entries. Clustering: none with
  round 65 (H1's capture bullet and §7's cost runs drew nothing); the three sections (H3b, §5,
  §2) last drew findings in rounds 61 and 62 (§2) and earlier. No finding follows from a
  previous fix. The H is the first since round 60 and the first against H3b since round 23. Fix
  1's case is the largest edit of the block (ten lines). Reviewer read of round records or
  changelog: none in the raw log. Doc length: 1958 -> 1977 lines. Above the split-check length
  (U-T6): no new decision, no split.
