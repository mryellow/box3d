---

title: Codex review — bit-exact history ring, round 34
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 34

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the source tree, and the document identifies real tree order and ownership problems. I found one construction gap that threatens the stated completeness guarantee, plus several contract and test inconsistencies. I read only the specified design file and permitted source and documentation paths; I did not read review files, modify files, or run code.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§2.7, 5.2, 7.3 | The claimed compile-time guarantee does not follow from the proposed accessors. §5.2 still lets a stage obtain a writable array base pointer once per loop. A caller can then write a cold element through that pointer without passing through its write accessor. The current world stores sparse records and solver arrays in shared structures ([physics_world.h](src/physics_world.h), [solver_set.h](src/solver_set.h)); hiding the container type alone does not constrain writes through a returned element pointer. | Specify an interface that exposes writable access only to individually verified hot elements, and keep cold storage unreachable through any writable base pointer. Make the compilation claim conditional on that boundary. |
| 2 | Medium | §§5.3, 10, 12.1 | The proposed hash excludes `nameId` as diagnostic state, yet public name getters exist and callbacks can use them when deciding a collision ([body.c](src/body.c), [shape.c](src/shape.c), [box3d.h](include/box3d/box3d.h)). A name change could therefore change replay behavior while leaving the advertised full state hash unchanged. | Hash restored name content or IDs, or explicitly exclude callback decisions based on names from the bit-exact contract and describe the hash’s narrower coverage. |
| 3 | Medium | §§2.4, 7.1, 16.4 | Forward scrub requires new values in journal entries throughout §§4, 7–9 and the API. Open question 16.4 proposes an old-values-only journal, which cannot meet requirement 4 or the forward-scrub tests. | Resolve this as a design choice: retain redo and remove the question, or define a separate rewind-only mode with revised requirements, API, and tests. |
| 4 | Medium | §§5.2, 10 | “Every world scalar setter” going through `b3World_WriteScalars`, which truncates after rewind, conflicts with §10’s rule that `b3World_SetUserData` and callback registration are host configuration that does not truncate. `b3World_SetUserData` writes a field of the current world struct ([physics_world.c](src/physics_world.c), [physics_world.h](src/physics_world.h)). | Define the scalar accessor’s exact field list and separate simulation setters from host configuration setters. |
| 5 | Medium | §§8, 14 | `tickCount` is described as allowing one retained image, while §8’s budget policy keeps a minimum window of two imaged ticks. Those rules cannot both hold for `tickCount = 1`; the API comment also says images are kept for at most `tickCount`. | Define whether one-image history is supported. If it is, give it a one-image budget rule; otherwise raise the minimum `tickCount` and correct the API comment. |
| 6 | Low | §§7.4, 12.7, 13 | Phase 1 images the dynamic and kinematic trees, so a restore cannot exercise layout independence for those trees. Phase 2 removes those images, but its delivery criteria do not explicitly repeat the tree-order replay test then. | Make test 7 a phase 2 acceptance gate with layout-changing dynamic and kinematic cases. |

## Checked, no change

- The recording player does restore serialized world state, and `ScrubBackward` compares replay hashes; the document correctly limits what that existing hash proves ([world_snapshot.c](src/world_snapshot.c), [recording_replay.c](src/recording_replay.c), [test_recording.c](test/test_recording.c), [recording.c](src/recording.c)).
- The FAQ says rollback determinism is unavailable through the current public mechanism. The simulation documentation describes worker and platform determinism, with the application caveat the design needs ([faq.md](docs/faq.md), [simulation.md](docs/simulation.md)).
- Sensor overlap processing runs for the sensor array and sorts and deduplicates results. Imaging `overlaps2` while treating the other overlap buffer as step scratch matches its swap pattern ([sensor.c](src/sensor.c)).
- The CCD sensor cap and running fraction, explosion callback mutation order, per-tree proxy free lists, and split island path support the document’s identified order and ownership concerns ([solver.c](src/solver.c), [physics_world.c](src/physics_world.c), [dynamic_tree.c](src/dynamic_tree.c), [island.c](src/island.c)).
- The design correctly treats shape material arrays, contact manifold blocks, sensor arrays, and shared hull data as distinct ownership cases rather than assuming a record copy captures their contents ([shape.h](src/shape.h), [contact.h](src/contact.h), [sensor.h](src/sensor.h), [physics_world.c](src/physics_world.c)).

## Proposed edits

Address finding 1 before treating journal completeness as proven. Then settle the forward-scrub and one-image contracts, list the exact scalar fields and setters, adjust hash coverage or the callback contract, and add the phase 2 tree test gate.

## Unresolved / disagreements

I could not verify the benchmark figures quoted from `docs/designs/reviews/`, because that path was excluded from this review. The proposed performance targets and cross-platform full-state hash also remain predictions until implementation and execution tests; the existing recording hash covers a narrower state.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T16:01:58+10:00, ended 16:08:01+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` (excluding review files) before and after was identical, so the
working tree was untouched by Codex. Same fresh full-scope prompt as rounds 18 to 33, no
declined-findings list. The final message above is unchanged apart from absolute path prefixes and
line-number suffixes on its links. Before the round, Claude re-read §5.2 against every record type
(step 2) and added the awake contact's manifold block and mesh triangle cache to the hot accessor's
returns, a sibling of round 33's finding 1 (the collide and impulse-store passes write them in place).

**Findings verified against the doc and source.**

1. **Applied.** §5.2 said a stage fetches an array's base pointer from "its accessor" without saying
   which, and a writable base pointer of a sparse record array (`contacts`, `bodies`) exposes cold
   entries. It now says a stage takes a const base pointer for reads, writes an element through the hot
   accessor by id or index, and no accessor returns a writable base of a sparse record array.
2. **Applied as rationale.** The public getters `b3Body_GetName` and `b3Shape_GetName` let a callback
   read a name. A name set after T is an API call `Rewind` undoes and the caller replays, and a
   `nameId` is restored with its record, so replay is not affected; the gap is hash coverage. §5.3 now
   says so, and §12 hashes each name's string content, not `nameId`, since ids follow first-seen order.
3. **Declined as a defect; §16 sharpened.** Question 16.4 is an explicit open product question
   ("does the game need forward scrub"), not a claim the doc relies on, and requirement 4 stands until
   it is answered. It now states what a "no" would drop (the forward direction of requirement 4, the
   forward-scrub tests, the redo half of §7.1).
4. **Applied.** `world->userData` is a field of the world struct while §5.2 said every world scalar
   setter goes through the truncating `b3World_WriteScalars`, against §10. §5.2 now limits that to
   setters of simulation scalars and names callback registration, `userData`, worker count and the
   task-system callbacks as live host configuration that bypasses it.
5. **Applied.** §8 evicts while the span from the oldest retained image to `historyTick` exceeds
   `tickCount` and a later image remains, so `tickCount` 1 retains two images, matching the two-image
   minimum window; there is no conflict in the rule. The API comment "images are kept for at most this
   many" was wrong, since `tickCount` bounds the span in ticks, and now says so.
6. **Applied.** Phase 1 images the dynamic and kinematic trees, so test 7's layout cases run against
   them only from phase 2. Phase 2's gate now names test 7 with layout-changing dynamic and kinematic
   cases beside test 5.

5 applied, 1 declined. The status line's finding sequence and round count in the target doc were
updated to include this round.

## Status

Finding count is 6 (1H-4M-1L), up from 4 and the highest since round 17, with one High for the third
round running. Finding 1 is the same §5.2 boundary paragraph as rounds 32 and 33 and follows from
round 33's own fix (its wording of the base-pointer cost sentence, from round 32's fix, left the
accessor unnamed); finding 4 follows from round 31's statement that host configuration does not
truncate, which conflicted with §5.2's older "every scalar setter". Findings 2, 3, 5 and 6 are older
text or open questions. Finding 3 re-raised an open question the doc labels as one, without engaging
it. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6) are not falling;
not converged, and §5.2's boundary paragraph remains the source of the Highs.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 34: 6 findings (1H-4M-1L), 5 applied, 1 declined`

`Series total: 158 findings (51H-87M-20L) across 34 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L)`
