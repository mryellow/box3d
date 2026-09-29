---
title: Codex review — rewind-only history ring, round 6
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 6

## Review report (Codex final message)

## Summary

The source supports the design’s main state inventory and undo approach. I found two specification inconsistencies that should be resolved before implementation. This was a read-only review; I did not run tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §6 | The numbered capture procedure runs after **every** `b3World_Step` and unconditionally counts, reserves, and copies an image. The later `captureInterval = K` rule says only every Kth tick is imaged. Implementing the numbered procedure literally with K=4 would retain four images instead of one, defeating the stated memory and capture-cost reduction. | Make image sizing and steps 2–3 conditional on the tick being selected for imaging; specify a journal-only slot for other ticks. |
| 2 | Low | §9 | The unqualified statement that `Rewind(P)` undoes pending API calls conflicts with step 1’s requirement that P be imaged. With interval 4, after step 5 a caller cannot use `Rewind(5)` to undo a setter made since that step: it returns `b3_historyTickUnavailable`. | Qualify the statement as applying when P is imaged, and state that a non-imaged P requires rewinding to an earlier restorable tick and re-stepping. |

## Checked, no change

- The existing serializer, backward-seek test, and limited scope of `b3HashWorldState` support the document’s qualified baseline claim (`world_snapshot.c`, `recording_replay.c`, `test_recording.c`, `recording.c`).
- The awake, sleeping, disabled, and static solver-set inventory matches the structures and transition paths in `solver_set.c`, `contact.c`, and `island.c`.
- Sensor overlap swapping, destruction of its three arrays, and moved-sensor index repair match `sensor.c` and `shape.c`.
- Shape materials, contact manifolds and mesh caches, island arrays, and shared hull references have the ownership paths the design identifies (`shape.c`, `contact.c`, `island.c`, `physics_world.c`).
- The proposed tree-order changes address order dependence visible in CCD and explosion queries; proxy ids use a separate free list (`solver.c`, `physics_world.c`, `dynamic_tree.c`).

## Proposed edits

Clarify §6’s journal-only capture path for non-imaged ticks. Qualify §9’s `Rewind(P)` paragraph with the imaged-tick precondition.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, a fresh full-scope prompt (target doc and source only, reviews directory, WORKFLOW.md and other design docs excluded, file-only citations stated, no curated decline list because every earlier decline is now answered by the doc's own rationale). Start 2026-09-29T20:57:21+10:00, end 21:05:49, exit code 0. Session 01a0eccf-e529-7161-a6c1-9a02df0e03ad (no resume needed). `git status --short` before and after is identical, so Codex touched nothing. Before the round: the target doc's uncommitted diff was read in full and was not mangled; AC-1 to AC-4 were re-applied to the doc (no shadow structure, enumerated-caller rule, second structure to reconcile or control-flow check found); the scalar-field check on the create/destroy entries had been done with no gaps. Codex's markdown file links were reduced to bare file names when copied here.

- **#1 (Medium), applied.** §6's numbered procedure runs at the end of every step and sized the slot from "the image's exact byte count", with steps 2 and 3 copying unconditionally, while the capture-interval paragraph and §8 (slots with no image, evicted after the imaged slot before them) say only every K-th tick is imaged. Read literally with K=4 the procedure images every tick. Step 1 now gives an image size of zero on a tick that is not imaged, and steps 2 and 3 say they run on an imaged tick; a slot for a tick that is not imaged is header plus journal segment.
- **#2 (Low), applied.** §9 step 1 requires T imaged, so with interval 4 `Rewind(P)` at a non-imaged P returns `b3_historyTickUnavailable`, while the paragraph after step 6 stated `Rewind(P)` undoes pending API calls unconditionally. It now says that holds when P is imaged and otherwise the calls are undone by rewinding to the newest imaged tick before P. Sibling statement with the same flaw, §14's closing note that `Rewind(t)` undoes a call made after step t, is qualified the same way. §6's "Rewind in the same window" paragraph is unaffected: after a first rewind `historyTick` is an imaged tick.

## Status

Two findings, none High, down from three. Both are new categories relative to earlier rounds (interval-K consistency between §6, §9 and §14; every earlier round's findings were about journal entries, ownership, restore steps and accessors). Neither was a consequence of round 5's fix, and neither reopened ground the doc's rationale covered. The count is 1 -> 3 -> 2 after round 4, so a falling trend is still not established, and the findings are edge and consistency issues, not structural. Next: another fresh full-scope round 7, same brief.

`Round 6: 2 findings (0H-1M-1L), 2 applied, 0 declined`

`Series total: 19 findings (6H-10M-3L) across 6 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L)`
