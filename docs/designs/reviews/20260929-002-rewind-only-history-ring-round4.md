---
title: Codex review — rewind-only history ring, round 4
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 4

## Review report (Codex final message)

## Summary

The rewind design is broadly consistent with the source paths checked. One claimed correctness guarantee needs revision.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §2.7, §5.2, §7.3 | The doc says journal coverage is enforced by types, but its hot accessor returns a writable whole record. Structural code running inside a step could use it to change fields without a journal entry, and that would compile. Contact creation and sleep transitions both make structural writes during a step (contact.c, solver_set.c). | Restrict the hot accessor so structural fields cannot be written through it, or describe and require an explicit audit for those writes. Adjust the “verified by construction” claim accordingly. |

## Checked, no change

- The recording rewind test compares hashes after backward seeks; the existing hash covers body transforms and velocities, as the doc states (test_recording.c, recording.c).
- The sensor pass processes all sensors and sorts overlaps (sensor.c).
- Tree proxy IDs use a free list, and the broad phase wraps world proxy creation and destruction (dynamic_tree.c, broad_phase.c).
- The cited wake, sleep, CCD sensor-hit, and explosion order dependencies are present in the source (solver_set.c, solver.c, physics_world.c).

## Proposed edits

Clarify the access boundary in §5.2 and revise requirement 7 and §7.3 so the stated guarantee matches what the proposed API can enforce.

## Unresolved / disagreements

The doc’s performance targets and proposed order changes remain untested design gates. This was a read-only source review; no execution was performed.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, a fresh full-scope prompt (target doc and source only, reviews directory excluded, file-only citations stated, no curated decline list because no earlier decline was a reviewer error about source). Start 2026-09-29T20:32:00+10:00, end 20:40:19, exit code 0. Session 01a0ecb8-b3a6-7f90-beaf-7c836b8fe031 (no resume needed). `git status --short` before and after is identical, so Codex touched nothing. Codex's markdown file links were reduced to bare file names when copied here.

**Pre-round re-read of the heap-block ownership rules (§5.2 owned blocks, §7.1 entries, §9 step 3), before the prompt was sent.** Rounds 1-3's edits were checked as a whole against `contact.c`, `island.c`, `solver_set.c`, `shape.c`, `container.h` and `physics_world.c`. One gap found and fixed, plus two precision fixes:

- **Create-undo left dangling pointers in free slots.** `b3DestroyWorld` runs `b3Array_Destroy` over every island slot, free ones included, while shapes and contacts are guarded by their id. The island create entry said only "free the arrays" and the pool alloc undo freed a shape's `materials` or a contact's manifold block without clearing the pointer, so a slot returned to the free state by a rewind held freed pointers (a double free at `b3DestroyWorld` for an island). The island create and set create entries now destroy the arrays and leave each empty (`b3Array_Destroy` zeroes `data`, `count` and `capacity`), and the pool alloc undo clears each freed pointer and its count.
- **Set entries named one handle.** A solver set owns five arrays (`bodySims`, `bodyStates`, `jointSims`, `contactIndices`, `islandSims`, `solver_set.h`); the set create and destroy entries now list all five.
- Checked and sound: shape geometry setters (`b3Shape_SetSphere`, `SetCapsule`, `SetHull`, `SetMesh`) touch only the hull database, never `materials` or `materialCount`, so no material-count transition exists outside create and destroy; `b3DestroyContact` leaves `manifolds` NULL and `manifoldCount` 0, `b3DestroyShapeAllocations` clears them only when a block existed, which the pool free entry's recorded `materialCount` already covers.

Findings:

- **#1 (Medium), applied in a reframed form.** Codex's premise holds: the hot accessor returns a writable whole record of an awake owner, and nothing at compile time stops a structural function from using it. The doc's stated rationale (the accessor restricts who owns the record, not which field; an awake owner's record is covered by image T or by the transition's pre-wake entries) already makes that sound for a record that was awake at T. Codex did not engage it. The real hole is narrower and is a dependency on write order: `b3WakeSolverSet` writes `body->setIndex = b3_awakeSet` first, so from that point the hot accessor's owner-awake assertion passes for a record whose pre-wake bytes nothing has journaled if the migrated code picks the hot accessor for it. Codex's proposed fix (restrict the accessor so structural fields are unwritable) would fragment the accessor by field. The smaller fix keeps the design's split: the one function per record kind that moves a record into the awake set (`solver_set.c` wake, merge and transfer paths; joint and island included, not only body and contact) journals the record's pre-wake bytes itself before writing any field of it, so which accessor a later write uses no longer matters. §5.2's sentence "Structural writes reach a record through its write accessor, even inside a step" is now stated with that coverage, and §7.1's entering-awake paragraph carries the rule. Requirement 7 is unchanged: the hot accessor reaches only awake owners, which image T or the transition entry covers, so completeness is still a property of the module boundary.
- **Unresolved item:** the perf review file was excluded from Codex's scope by the prompt, as in the earlier rounds.

## Status

Finding count fell to one, a Medium, and it is a new category (accessor choice versus transition order) rather than another undo-rule edge case in heap-owning records. It was not a consequence of round 3's fix; the gap it points at predates the series and was surfaced by a fresh reviewer once the block-ownership gaps were closed. It did not reopen stated rationale in a way that reversed anything: it re-raised ground §5.2's rationale covers for records awake at T, without engaging that rationale, and the doc now states the one dependency the rationale left implicit. The pre-round re-read found and fixed a block-ownership gap of its own (dangling pointers after a create-undo), so the re-read paid for itself. The verdict is `CHANGES_PROPOSED`, so this is not convergence and the acceptance-criteria recheck for a `CONVERGED` close has not been reached. Next: another fresh full-scope round 5, same brief, no narrowing.

`Round 4: 1 findings (0H-1M-0L), 1 applied, 0 declined`

`Series total: 14 findings (5H-7M-2L) across 4 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L)`
