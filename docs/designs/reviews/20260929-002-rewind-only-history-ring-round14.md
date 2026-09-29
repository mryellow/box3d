---
title: Codex review — rewind-only history ring, round 14
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 14

## Review report (Codex final message)

## Summary

The rewind-only approach is broadly consistent with the allowed source. I found no source-level contradiction that rules it out, but the document needs changes to its compile-time guarantee, memory accounting, and journal cost description.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §2 requirement 7; §5.2 | The proposed opaque container pointers and pointer-to-const scalars do not make every bypass a compile error: code with access to the world struct can still reassign those fields. The claimed “verified by construction” boundary is stronger than the specified types enforce. | Hide the fields behind an opaque world boundary, or narrow the guarantee and specify how remaining writes are checked. |
| 2 | Medium | §8 | The oldest retained image’s journal segment is explicitly never read, yet its entries and owned heap blocks remain charged until the slot is evicted. A large destroy at that tick can consume the budget and widen the capture interval without improving rewindability. | Release that segment and its owned blocks when its image becomes the oldest restorable image. |
| 3 | Medium | §8; §14 | Blocks owned by the unclosed staging segment are absent from `bytesUsed`. Arbitrarily many between-step destroys can therefore hold substantial history memory while the reported usage stays unchanged; “bounded by one segment” does not bound its bytes. | Report pending entry bytes and owned-block bytes, and state the budget behavior before the next capture. |
| 4 | Low | §7.2 | The contact begin/end size example understates journal work. Linking a contact can merge islands, and island.c moves the smaller island’s bodies, contacts, and joints. The stated under-1-MB churn estimate is therefore not supported by an event count alone. | Account for the size of affected islands and treat the estimate as a measurement target. |
| 5 | Low | §9; §10 | Rewind releases `userShape` through a host callback while restoring records, but does not say whether it locks the world during that transaction. Existing shape destruction locks the world before calling the same release path (shape.c, physics_world.c). | Specify the rewind lock and callback reentry contract. |

## Checked, no change

- recording.c hashes body transforms and velocities; the backward-seek test in test_recording.c supports the document’s limited existing-evidence claim.
- sensor.c processes every sensor each step and sorts and deduplicates overlaps.
- broad_phase.c sorts discovered pair keys before contact creation. dynamic_tree.c has a separate proxy free list, as the design states.
- The filter-change cases in §12 match shape.c: `invokeContacts=false` leaves the proxy untouched; `true` reaches recreation.
- The documented phase-0 exceptions for per-object `userData` and renderer handles match world_snapshot.c. The sleep, wake, and cleared `bodyMoveIndex` cases also align with the inspected paths.

## Proposed edits

Revise the §5.2 boundary claim; release the unreachable oldest journal segment; expose pending memory in `b3HistoryInfo`; qualify §7.2’s cost estimate; and define rewind locking around debug-handle release.

## Unresolved / disagreements

The benchmark figures cited from other design-review files were not verified because those files were outside the permitted reading scope. The proposed CCD, sensor, and explosion order changes still need the solver-owner decision identified in §16. I have no disagreement with choosing one-way rewind for the stated correction flow.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as rounds 12 and 13 with no curated decline list. Session 01a0eed7-a891-7d02-b32c-94db5757ea5a. Start 2026-09-30T06:25:05+10:00, end 06:32:58, exit code 0, no resume needed. `git status` shows no change Codex could have made. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** The element, count and pointee writes were already unreachable, but the world's own pointer fields (`world->pairSet = other`) could still be reassigned by any file that sees the world struct. §5.2 now declares those fields const-qualified pointers, set only by the world's creation function, so the guarantee covers reassigning the container itself.
- **#2 (Medium), declined, rationale sharpened.** §8 already said the oldest slot's own segment stays charged until eviction so that a charge never changes while its slot is retained; the finding restates the alternative without engaging that reason. Releasing the segment when its image becomes the oldest would move the charge mid-life and walk the segment's entries at the moment the slot before it is evicted, both of which §11.3 lists as things this design avoids ("no charge that moves between a slot and the world"). §8 now names both costs in parentheses.
- **#3 (Medium), applied, this time as a field.** Round 12's #3 was answered with rationale only. This round's argument engages it: one segment is bounded in count of ticks but not in bytes, since a caller can make any number of between-step destroys. §8 now says staged owned blocks are charged when the segment closes and are reported until then by a new `b3HistoryInfo::pendingBytes` (staged journal bytes plus owned block bytes, outside `bytesUsed` and the budget), replacing the "bounded by one segment" claim.
- **#4 (Low), applied.** `island.c` merges and splits write the island membership of every moved body, contact and joint (§5.2's table lists them), so an event count alone understates a step's journal. §7.2 now says a merge or split writes every moved record and that §12 test 5 measures the estimate.
- **#5 (Low), applied.** Shape destruction and the other host-callback paths lock the world around the `destroyDebugShape` call (`shape.c`), so a callback cannot re-enter. §9 now states that `b3World_Rewind` locks the world for the call.

## Status

Five findings, three Medium and two Low, from a count of 3 the round before. Categories: a compile-time guarantee narrower than stated (#1), a declined restatement of the charge-fixed rule (#2), a repeat of round 12's #3 that this time engaged the doc's rationale and was applied as a field (#3), a cost estimate that omitted island merge work (#4) and an unstated lock (#5). None was a consequence of round 13's fixes. #2 and #3 reopened ground the doc's rationale covered; #2 did not engage it and #3 did. A clean round has not occurred, and the series has not converged. Next: another fresh full-scope round 15, same brief.

`Round 14: 5 findings (0H-3M-2L), 4 applied, 1 declined`

`Series total: 48 findings (7H-29M-12L) across 14 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L)`
