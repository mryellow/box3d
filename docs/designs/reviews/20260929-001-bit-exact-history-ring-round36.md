---

title: Codex review — bit-exact history ring, round 36
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 36

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible, but the document leaves several parts of the exactness contract underspecified. The most consequential gaps are the status of cross-step world fields, the canonical proxy-pool hash, and tests for ownership transitions. This review used the design and current source only; I did not read review files, consult git history, or modify files.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§4–5, 9, 12 | The claim that every cross-step world byte is classified omits `stepIndex` and `maxCapacity`. The existing serializer preserves both in [world_snapshot.c](/home/user/box3d/src/world_snapshot.c), while the proposed scalar image and hash do not name them. Source use suggests `stepIndex` is not read back into physics, but the design should establish that distinction explicitly. | Classify both fields, state whether rewind restores them, and align the hash and API-visible behavior with that choice. |
| 2 | Medium | §§7.4, 12 | The proposed proxy-pool hash normalization is not precise enough to implement or test. [dynamic_tree.c](/home/user/box3d/src/dynamic_tree.c) grows capacity by linking a new ascending free-id run; after rewind, that run can remain from a discarded timeline. “Drop a trailing ascending run” does not specify how to encode the next allocation consistently across the two capacities. | Define a canonical allocation-sequence algorithm with examples for a full pool, existing free ids, and growth followed by rewind; test equal hashes and subsequent proxy ids. |
| 3 | Medium | §§7.1, 12 | The verification plan covers destroyed blocks but does not explicitly exercise *replacement while the owner remains alive*: a shape changing hull or borrowed geometry, material writes across a rewind, and contact manifold/cache resizing across sleep and wake. These use different ownership paths in [shape.c](/home/user/box3d/src/shape.c), [contact.c](/home/user/box3d/src/contact.c), and [solver_set.c](/home/user/box3d/src/solver_set.c). Random churn alone is a weak gate for each path. | Add targeted backward, forward, and replay cases for each live-owner replacement and resize, with allocator counts and restored contents checked. |
| 4 | Low | §§8, 14 | `maxBytes` is described both as capping the arena and as permitting the arena to grow past it for an admitted slot or minimum window. The intended target-budget behavior is explained, but “caps” and “ring arena target” give conflicting API expectations. | Use “target for retained bytes” consistently, and state separately that arena capacity and staging capacity can exceed it. |
| 5 | Low | §§1, 3 | The `ScrubBackward` evidence supports replay of that recorded scene under the existing, limited body-state hash. Calling bit-exact whole-world rewind a demonstrated engine property overstates what [test_recording.c](/home/user/box3d/test/test_recording.c) and [recording.c](/home/user/box3d/src/recording.c) verify. | Narrow the claim to transforms and velocities in the tested recording path; retain the proposed full-state and event tests as the gate for the broader claim. |

## Checked, no change

- The source uses six world id pools with LIFO free lists; the broad-phase trees have separate proxy free lists ([id_pool.c](/home/user/box3d/src/id_pool.c), [dynamic_tree.c](/home/user/box3d/src/dynamic_tree.c)).
- Contact candidates are sorted by shape-pair key before creation, while CCD sensor collection is traversal-order-sensitive and capped at eight ([broad_phase.c](/home/user/box3d/src/broad_phase.c), [solver.c](/home/user/box3d/src/solver.c)).
- Sensor overlap arrays are swapped and rebuilt for every sensor each step, including sensors whose owners are not awake ([sensor.c](/home/user/box3d/src/sensor.c)).
- The current explosion callback wakes bodies and accumulates impulses during tree traversal, as the proposed order change assumes ([physics_world.c](/home/user/box3d/src/physics_world.c)).
- The serializer clears body, shape, and joint `userData` on restore, supporting the phase-0 limitation ([world_snapshot.c](/home/user/box3d/src/world_snapshot.c)).

## Proposed edits

Clarify the two omitted world fields and proxy-pool hash algorithm, add the targeted ownership tests, and tighten the budget and existing-evidence wording.

## Unresolved / disagreements

The document explicitly leaves callback ordering with order-dependent side effects outside its bit-exact contract. That exception is coherent, but it means the headline claim should continue to be qualified wherever presented to callers.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, stdin from
`/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout, raw output to the session
scratchpad. Started 2026-09-29T16:44:07+10:00, ended 16:48:26+10:00, exit code 0; no resume. Codex ran
read-only; `git status --short` (excluding review files) before and after was identical and `src/` and
`include/` have no diff, so the working tree was untouched by Codex. Fresh full-scope prompt (target doc,
source tree, no read of `docs/designs/reviews/` or git history, file-only citations by design), no
declined-findings list: every earlier decline was one where the doc was right and now says why. The final
message above is the run's, one copy kept.

Before the round, Claude read the whole uncommitted diff of the target doc (rounds 26 to 35's edits since the
last commit; not mangled), re-read the mechanism sections (§5.2, §7.1, §9) and checked each rule against its
sibling cases (step 2), fixing these gaps directly:

(a) §5.2 said a field assignment through anything but the write accessor or the hot accessor is a compile
error, but the journal walk's undo and redo and the image scatter are neither: the hot accessor asserts the
world is inside a step, and the write accessor journals. It also routed the world scalar struct's only write
through `b3World_WriteScalars`, which truncates the ring after a rewind, so restore's own copy of the imaged
struct would truncate the slots a forward scrub returns to. §5.2 now names restore's writer as a third route
(records, containers and the scalar struct), which neither journals nor truncates. (b) §5.2 said the write
accessor writes an awake owner's record in place without an entry, and §7.1 says structural writes journal
unconditionally, awake owner or not (a create must save the free slot's old bytes, generation included, for
undo); nothing said how the accessor tells them apart. The accessor now has a structural entry point that
always journals and a setter entry point that journals unless the owner is awake. (c) §5.2 said the hot
accessor never returns a container's count or storage, yet an awake set's arrays and the graph colours' arrays
grow and shrink in the step; the containers paragraph now says they are the same container type and make no
entry, since they are imaged.

**Findings verified against the doc and source.**

1. **Applied.** `world->stepIndex` (`physics_world.h`) is set to 0 at creation and incremented once per step in
   `solver.c`; no other source file reads it, and the serializer alone writes and reads it (`world_snapshot.c`).
   `world->maxCapacity` is raised at the start of each step from tree and pool counts (`physics_world.c`) and is
   returned by a public getter; nothing in the step reads it. Both are counters, §5.3's class, and were left out
   of round 35's scalar row for that reason, but the doc did not say so, so a fresh reviewer re-found the
   omission. §5.3 now names both, says neither is restored nor hashed, and gives the reason (what reads them).
   No change to the image, the hash or the design.
2. **Applied in part.** The algorithm in §12 is right. `b3AllocateProxy` (`dynamic_tree.c`) grows only when the
   free list is empty and links the new ids `oldCapacity` to the new capacity minus one in ascending order with
   the head at `oldCapacity`, so a tree left with extra capacity by a discarded timeline holds a free list whose
   tail is a run of consecutive ids ending at its last capacity slot, and that run's head is the id the
   original timeline's growth would have returned. Dropping that run and hashing the id at its head (or the
   capacity, if there is none) gives the same value in both trees; the worked case with a retained head id
   (`3` then `8` to `11`, against a never-grown tree with free list `3`) confirms it. What was wrong is one
   word: "ascending run" read as merely ascending would drop `3, 8, 9, 10, 11` whole and hash the never-grown
   tree's `3` differently. §12 now says a trailing run of consecutive ascending ids. The requested examples and
   extra tests are not added: §12 test 2 already has a step that grows a tree's proxy capacity followed by a
   rewind across it, and the hash is compared there.
3. **Applied.** §7.1 has entries for a material block and for a manifold block and mesh cache written while the
   owner stays alive (`b3Shape_SetSurfaceMaterial` and `b3Shape_SetMeshMaterial` in `shape.c`, the wake and sleep
   transitions' pre-wake and final bytes), and §12 tests geometry replacement (`b3Shape_SetHull`,
   `b3Shape_SetMesh`) but its churn list named neither material writes nor a manifold count changing across
   sleep and wake. The churn now includes both. Allocator-count checks already sit in test 6.
4. **Applied.** §8's first bullet said `maxBytes` caps the arena, against §2 requirement 5, §8's own following
   text and §14's `b3HistoryDef` comment, which say it is a target for retained bytes that an admitted slot can
   exceed. §8 now says it is the target for retained bytes, and §14's field comment says so and that arena and
   staging capacity are reported separately and can exceed it.
5. **Declined.** §1 already limits the claim: "Bit-exact rewind of a whole world along the dimensions that hash
   covers — body transforms and velocities" and "The hash does not yet cover everything requirement 1 claims",
   and names `test_recording.c`'s `ScrubBackward` as a keyframe restore plus re-step whose hashes equal the
   forward pass's. `b3HashWorldState` (`recording.c`) hashes body transforms and velocities, which is the
   dimension the doc claims, and the full-state hash and replay tests in §12 are already the gate for the wider
   claim. The finding's own proposed action restates what §1 says, so it did not engage the doc's text.

4 applied, 1 declined. The status line's finding sequence and round count in the target doc were updated
to include this round. AC-1 to AC-4 were applied to the doc before the round: no shadow structure over a
population it doesn't own (the ring's image and journal are the design's own; the restore-time shape list is
rebuilt per restore, not kept), no completeness that rests on a caller list (§5.2's caller table is a migration
worklist and the compiler is the choke point, which restore's writer now joins), no reconciliation between
two independent structures, and no check that changes control flow (the assertions are invariants).

## Status

Finding count is 5 (0H-3M-2L), up from 3, still with no High (rounds 35 and 36). None of the five touches
§5.2's boundary text, which was again the subject of the pre-round check and the source of three gaps found
before Codex ran; that means the module-boundary rules are still the place gaps survive, and Codex did not find
these three, so a clean pass over §5.2 is not evidence the shape is right (AC-5). Finding 1 reopened ground
round 35 had settled and declined to change: the reasoning was in the round file, not the doc, and now is in
§5.3. Finding 2 is a wording fault in older §12 text, not a consequence of round 35's fix; findings 3 and 4 are
older test-plan and API-wording text. No finding contradicted stated
rationale, and finding 5 re-raised the alternative without engaging §1's qualification. The counts since round
17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5) are flat, not falling; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 36: 5 findings (0H-3M-2L), 4 applied, 1 declined`

`Series total: 166 findings (51H-91M-24L) across 36 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L)`
