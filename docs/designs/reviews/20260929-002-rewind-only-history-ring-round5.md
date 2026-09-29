---
title: Codex review — rewind-only history ring, round 5
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 5

## Review report (Codex final message)

## Summary

I reviewed the design against `src/`, `include/`, and `test/`. The rewind-only approach is sound in broad outline, but two specified restore operations need correction, and the compile-time completeness claim is stronger than the proposed accessors can enforce. This was a read-only review; no files were changed.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §9 | The solver-set undo entries restore arrays but omit `setIndex`. Destruction clears that field, while the proposed set-destroy undo only reattaches arrays. Rewinding across a wake would leave a restored sleeping set marked unused. The set-create undo also needs to clear the field when undoing creation in a reused slot. | Specify restoration of the complete solver-set scalar state in both entries; include it in the state hash and a wake/rewind test. |
| 2 | Medium | §4, §9 | Restore clears task-context bitsets, contradicting its O(awake + journal) cost claim. Contact and joint bitsets are sized by world-wide ID capacity, and clearing writes every active block. | Leave these scratch bitsets alone during rewind; their owning step stages already clear them before use (contact, joint and hit). |
| 3 | Medium | §2.7, §5.2 | The design says an unjournaled cold write cannot compile, but the hot accessor returns a writable element pointer. Its awake and in-step assertions run when the pointer is obtained; they cannot prevent a caller from retaining it and writing after the owner leaves the awake set. | Constrain hot writes within a boundary that cannot return a persistent mutable pointer, or qualify the construction claim and specify checks for pointer lifetime and ownership transitions. |

## Checked, no change

- ScrubBackward replays from keyframes, and the existing state hash covers body pose and velocity. The design correctly limits what that test demonstrates.
- The source supports the proposed need to retain proxy identity, moved flags, sensor overlaps, and material and manifold pointees. The identified CCD and explosion traversal-order dependencies are real.
- The undo argument for bytes written while awake and later put to sleep is consistent with the wake and sleep paths reviewed.

## Proposed edits

Specify solver-set scalar undo, remove restore-time scratch-bitset clearing, and revise the compile-time completeness claim to match the access the implementation would expose.

## Unresolved / disagreements

The choices already listed in §16—particularly accepting the three order changes and whether callers need forward scrub—remain product decisions. I found no source-based disagreement with the rewind-only recommendation itself.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 4. Start 2026-09-29T20:47:03+10:00, end 20:52:54, exit code 0. Session 01a0ecc6-778e-7381-9c7f-41e8340302f0 (no resume). `git status --short` before and after is identical, so Codex touched nothing. Codex's markdown file links were reduced to bare file names when copied here.

- **#1 (High), applied.** `b3SolverSet` (`solver_set.h`) has a `setIndex` scalar beside its five arrays, and `b3DestroySolverSet` (`solver_set.c`) destroys the arrays, frees the id and leaves `setIndex` at `B3_NULL_INDEX`. The set destroy entry restored only the arrays, so a rewind across a wake left a sleeping set marked unused; the set create entry left a reused slot's `setIndex` set. The set create undo now leaves the free-set state (arrays empty, `setIndex` `B3_NULL_INDEX`) and the set destroy undo sets `setIndex` back to the set index. `b3ValidateSolverSets` (§9 step 6) and test 8's wake cases catch a regression; no new test clause needed.
- **#2 (Medium), applied.** §9 step 5 cleared the task-context bitsets. `contactStateBitSet` (`physics_world.c`), `jointStateBitSet`, `hitEventBitSet` and `awakeIslandBitSet` (`solver.c`) are each sized to world-wide id capacity and cleared with `b3SetBitCountAndClear` by the stage that uses them, every step, so the restore-time clear was redundant and made restore O(world), against §4's cost claim. The clear is removed and §9 step 5 says why the bitsets are left alone.
- **#3 (Medium), declined; rationale sharpened.** This is round 4's #1 ground again, from the pointer-lifetime side. A retained hot-accessor pointer written after its owner left the awake set is a stage that spans a transition; transitions out of the awake set (`b3TrySleepIsland` in `solver.c`, wakes in `island.c`, `body.c` and `physics_world.c`) run in serial passes or between steps, not inside the parallel stages that make hot writes, and the accessor's shape is one pointer per write. An assertion at each obtain is what AC-4 allows; a lifetime checker would be a gate on a case the structure does not produce. Codex did not engage the stated restriction (who owns the record, not which field). §5.2 now states that the pointer is used for one write and not kept, and where transitions sit relative to hot writes.
- **Unresolved item:** the perf review file was excluded from Codex's scope by the prompt, as in the earlier rounds.

## Status

Three findings, one High. #1 is a new category (solver-set scalar state, absent from the block-ownership audit because the audit was about heap blocks); #2 is a cost-claim inconsistency in §9; #3 reopened round 4's accessor ground without engaging the doc's rationale. None was a consequence of round 4's fix. The count rose from 1 to 3, so the falling trend is not yet established. Next: another fresh full-scope round 6, same brief. A sibling check for #1: every other scalar in a record the create/destroy entries handle, not only arrays, should be listed before round 6 (islands: `setIndex`, `localIndex` are record fields covered by record writes, not the entries).

`Round 5: 3 findings (1H-2M-0L), 2 applied, 1 declined`

`Series total: 17 findings (6H-9M-2L) across 5 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L)`
