---
title: Codex review — rewind-only history ring, round 11
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Attempt 1 killed by the shell timeout, resumed once (see Post-review verification).
session: 01a0eeb7-7488-7742-9b02-dfa88caec953
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 11

## Review report (Codex final message)

## Summary

The rewind-only ring and reverse-order ownership model are coherent, but the document has gaps in its mutation inventory and in two contracts meant to make journaling complete. I found no file changes to make; this was a read-only review.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2 | The “current direct-write callers” table is incomplete. Island linking and splitting write contact and joint records, and graph insertion writes their indices. These paths are absent from the corresponding rows. | Add these callers and specify their structural write-accessor migration. |
| 2 | Medium | §§5.2, 7.1 | `world->sensors` is said to permit only push/removeswap, but the named `b3JournaledArray` type also offers clear/set. Those operations have no ownership rule for a sensor’s three heap arrays. | Specify a sensor-specific restricted type, or define ownership-safe clear/set entries. |
| 3 | Medium | §§5.2, 7.1 | The inline material route is underspecified. Single-material shapes store material in the record, and setters write through the same accessor used for heap materials. The design says inline writes use a record entry but does not say how `b3Shape_WriteMaterial` makes that entry for a cold shape. | State the accessor’s inline branch explicitly and test rewind after a static or sleeping shape’s material setter. |
| 4 | Medium | §10 | The borrowed-geometry lifetime rule mentions retained ticks but omits *currently live* shapes. With capture interval greater than one, a newly created live mesh can be absent from every retained image and still require its geometry. | Require geometry to remain valid while referenced by any live shape **or** restorable tick. |
| 5 | Low | §1 | ScrubBackward tests one small scene and a hash limited to body transforms and velocities. Calling whole-world rewind along those dimensions a demonstrated general engine property overstates that evidence. | Describe it as evidence for the tested scene; leave the general claim to §12’s expanded tests. |

## Checked, no change

- The source supports the proposed hot-state inventory’s key distinctions: awake solver arrays, id-addressed contact manifolds, and sensor overlaps require different capture paths.
- Reverse journal order and the stated transfer-or-evict rule are consistent for destroyed sets, islands, contacts, shapes, sensors, and hull references.
- The sleep/wake argument accounts for state written while awake and later absent from an end-of-tick image.
- Proxy IDs have a separate tree free list; journaling their allocation is necessary even when tree layout is rebuilt.
- The design consistently treats step-T events as unavailable immediately after rewind and requires replay to regenerate later events.
- Phase 1 explicitly retains an O(proxies) dynamic/kinematic tree image and defers the full awake-proportional cost target to phase 2.

## Proposed edits

Update the §5.2 caller table and define the sensor and inline-material mutator interfaces so the claimed compile-time boundary is precise. Correct §10’s geometry lifetime rule and narrow §1’s statement of existing proof. Add focused verification cases for the two mutator interfaces.

## Unresolved / disagreements

The performance figures attributed to review documents were not independently checked; those documents were outside the permitted reading scope. The proposed measurements in §12 remain the cost acceptance gate.

## Verdict

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 10 with no curated decline list. Start 2026-09-30T05:49:54+10:00, end 05:58:54, exit code 124. Attempt 2, resume of session 01a0eeb7-7488-7742-9b02-dfa88caec953 with `-m gpt-6-sol -c model_reasoning_effort="high"` before `resume` and a finalize-only prompt, start 05:59:01, end 05:59:33, exit code 0. `git status` after the run shows no change Codex could have made. The target doc's diff (rounds 7 to 10's fixes, uncommitted) was skimmed for round 10's edits before the run and was not mangled. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** `island.c` writes `islandId` on contacts and joints in its link, unlink, merge and split paths, and `constraint_graph.c` writes `colorIndex` and `localIndex` on both; none was in the table's contact and joint rows. Those rows now list them, and the paragraph above the table says the table is the migration's starting worklist and the compiler says when the migration is complete, since a completeness claim resting on a list is what §5.2 rules out.
- **#2 (Medium), applied.** §5.2 said the sensors array's type offers only push and removeswap, and two paragraphs later said `b3JournaledArray`, which holds `world->sensors`, offers clear and set. The container paragraph now says the instance holding `world->sensors` omits clear and set.
- **#3 (Medium), applied.** `b3Shape` keeps a single material inline (`shape.h`, `b3GetShapeMaterials` presents it as a one-element array), and every material setter writes through that accessor. §5.2 now says `b3Shape_WriteMaterial` writes an inline material as a whole-record write through `b3Shape_Write`'s setter entry and makes a block entry only for a shape with a block; §7.1's material row says the same; §12 test 2's material writes now cover single-material and multi-material shapes on awake, sleeping and static owners.
- **#4 (Medium), applied.** §10's rule named retained ticks only. A live shape created after the newest image is in no retained image and still needs its geometry. The rule now covers any live shape or any shape a retained tick or journal segment can restore, and the caller frees geometry only when neither holds.
- **#5 (Low), applied.** `ScrubBackward` (`test_recording.c`) builds a ground plane and four hull boxes over 80 frames, and its hash is `b3HashWorldState` (transforms and velocities). §1 now says the property is shown on that four-box scene and that §12 tests it across the benchmark scenes.

## Status

Five findings, four Medium and one Low, from a count of 6 the round before. Categories: an incomplete caller table (#1, the same shape as round 10's #1 but a different structure), an internal contradiction in the sensor container's interface (#2), an unspecified inline-material path (#3), a lifetime rule missing live shapes (#4) and an overstated claim about existing evidence (#5). None was a consequence of round 10's fixes and none reopened stated rationale. Two rounds in a row have found interface-precision gaps in §5.2 and §7.1, which is the part of the doc with the most rules; a clean round has not occurred. Next: another fresh full-scope round 12, same brief.

`Round 11: 5 findings (0H-4M-1L), 5 applied, 0 declined`

`Series total: 35 findings (7H-20M-8L) across 11 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L)`
