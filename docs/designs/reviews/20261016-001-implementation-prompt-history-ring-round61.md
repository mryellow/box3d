---
title: Review round 61 - implementation prompt, rewind-only world history
target: 20261016-001-implementation-prompt-history-ring.md
date: 2026-10-16
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261016-001-implementation-prompt-history-ring-round60.md
mode: full pass
session: 01a143da-9da1-7f20-825f-9368316cb377
---

# Round 61

Target at revision 58 (1925 lines).

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
Run: foreground, `timeout 540`, exit 0 after 56 source reads, 2026-10-16T18:36:02+10:00 to
18:42:19+10:00. Header confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a143da-9da1-7f20-825f-9368316cb377`. The stream emitted the final message twice; the last
copy follows.

## Review report (Codex final message)

### 1. Summary

The implementation prompt is largely aligned with the three binding designs, but it has file-scope and test-fixture defects that could stop an implementer or weaken the claimed evidence. The H1 file list needs correction before the nine-commit procedure is executable as written.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 H1; §3 | H1 adds fields to `b3GeometryEntry` and `b3GeometryRegistry`, which are defined in `src/recording.h`, but omits that file from its list. §3 permits an unlisted compiler-driven file only for boundary commits and H4, so H1 cannot follow its own file rule. | Add `src/recording.h` to H1's file list. |
| 2 | M | §4 H1 | `b3ReleaseGeometryEntry(reg, id)` is said to free both the registry entry and its reader-slot copy. The registry in `src/recording.h` has no access to the reader slots in `src/replay.h`; the stated signature cannot perform both frees without an unstated ownership link. | Specify a reader argument or assign reader-slot release explicitly to the ring, including eviction, rewind drop, and sizing-only release. |
| 3 | M | §2; §4 H4 | §2 classifies static-shape destruction as cold. In `src/shape.c`, destruction passes `wakeBodies=true` to contact destruction; `src/contact.c` wakes both bodies when a destroyed contact is touching. H4's direct cold-only journal test therefore needs a contact-free target, which its fixture does not require. | Correct the inventory and require the destroyed static shape to have no touching contact; assert that precondition in the test. |
| 4 | M | §4 H1 | Geometry-churn case 11 calls its tick-1 payload `g` "one entry and its slot," yet its static ground box is a hull, whose geometry the serializer also interns (`src/shape.c`, `src/world_snapshot.c`). Including that persistent payload in `g` loosens the multiples-of-`g` bounds and can hide retained mesh payload. | Remove the ground box, which the churn does not need, or measure and subtract its persistent payload before setting the bounds. |
| 5 | L | §2 | The six-pool bump-allocation inventory omits the joint-record push in `src/joint.c` and the three fixed solver-set pushes in `src/physics_world.c`. Those are creation routes H3c and H4 must account for. | Add those sites to the inventory, distinguishing fixed-set initialization from allocations that can occur while history is enabled. |

### 3. Checked, no change

- The ring's interval-widening and window examples agree with the ring doc §§5–7, including retention of two images exactly `tickCount` ticks apart.
- The capture point and event-buffer flip match `src/physics_world.c`; the seven public queries and casts do open broad-phase traversals.
- The tree split and proxy-entry plan match the trees doc §§5–9 and the cited tree write routes.
- The proposed compile-boundary expressions use fields and getters present in the current source. The test runner filters by exact entry name, and the benchmark table contains 17 scenes.
- The H5 and H6 cost fixtures target the intended phase distinction: sleeping dynamic proxies leave the awake-tree image small, while sleeping kinematic proxies remain in the raw kinematic-tree image until H6.

### 4. Proposed edits

Apply findings 1–5 to §2 and H1/H4 in §4, then update the affected geometry-churn and cold-undo evidence descriptions in §6.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the designs (U-V2). Findings 1, 2 and 4 are
against rounds 58 to 60's registry mechanics; 3 and 5 are §2 inventory facts from revision 1.

1. **Adopted (M).** `b3GeometryEntry` and `b3GeometryRegistry` are defined in `src/recording.h`
   (lines 100 and 116); H1's list had `src/recording.c` alone. Fix: the header is listed.
2. **Adopted (M).** The registry (`src/recording.h`) has no pointer to the reader
   (`src/replay.h`); the ring owns both. Fix: the registry function frees the entry's bytes; the
   ring frees the reader slot's copy at every release point through a new `b3RecReleaseSlot` of
   `src/replay.c`.
3. **Adopted (M).** `b3DestroyShape` sets `wakeBodies` true (`src/shape.c:599`) and
   `b3DestroyContact` wakes both bodies when the contact is touching (`src/contact.c`, the
   `wakeBodies && touching` branch). Fix: the cold list's destroy is of a static shape with no
   touching contact, with the route named, and the H4 fixture `ENSURE`s the body's contact count
   is 0 before the call.
4. **Adopted (M).** A box shape is a hull in this engine (the scene bullet itself says "hull
   boxes") and the serializer interns hulls, so `g` included the ground's entry. Fix: case 11's
   first world has no shape but the per-tick mesh.
5. **Adopted (L).** `b3CreateJoint` pushes a zeroed `b3Joint` on a bump (`src/joint.c:224`) and
   `b3CreateWorld` pushes the three fixed sets (`src/physics_world.c:278` to `289`). Fix: the
   inventory names both, the fixed sets as a pre-enable push H3c routes and H4 does not journal.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 61: 5 findings (H0-M4-L1), 5 applied, 0 declined. Series: 189 findings, 188 applied, 1
  declined.
- Findings: 3/5/1 -> 2/3/0 -> 0/4/0 -> 3/3/0 -> 2/1/1 -> 2/2/0 -> 1/1/1 -> 2/2/1 -> 2/1/0 ->
  1/1/1 -> 2/4/1 -> 1/3/0 -> 1/3/0 -> 0/1/1 -> 2/4/0 -> 0/5/1 -> 2/2/0 -> 0/2/1 -> 1/4/0 ->
  0/2/0 -> 1/1/1 -> 1/3/1 -> 1/1/0 -> 1/3/0 -> 1/2/0 -> 0/1/0 -> 2/1/0 -> 0/1/2 -> 1/2/0 ->
  0/3/0 -> 0/1/0 -> 0/1/0 -> 1/2/1 -> 0/3/0 -> 0/0/0 -> 1/3/0 -> 1/1/0 -> 1/2/0 -> 0/3/1 ->
  0/1/1 -> 0/0/0 -> 0/2/1 -> 0/3/0 -> 0/2/0 -> 0/2/0 -> 0/2/1 -> 1/0/2 -> 0/0/0 -> 2/1/0 ->
  0/2/0 -> 1/1/0 -> 0/2/0 -> 0/1/0 -> 0/1/1 -> 1/1/0 -> 0/2/1 -> 0/1/0 -> 1/1/0 -> 1/1/0 ->
  2/0/1 -> 0/4/1 -> pending
- Tripwire: fix 2 adds a reader function; the rest are a file, two qualifications, a fixture
  change and two inventory sites. Clustering: H1's registry bullet (rounds 58, 59, 60, 61), four
  rounds running, reported under U-S4: the mechanism round 58 opened has drawn eight findings
  in four rounds, the most of any passage in the series; §2's cold list (rounds 54, 61). Three
  of five findings against this block's fixes. Each fix is a clause or a sentence. Reviewer read
  of round records or changelog: none in the raw log. Doc length: 1925 -> 1935 lines. Above
  the split-check length (U-T6): no new decision, no split.
- The owner asked for a pause after this round's fold-in; no round 62 is sent.
