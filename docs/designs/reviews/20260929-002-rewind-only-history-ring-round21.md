---
title: Codex review — rewind-only history ring, round 21
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 21

## Review report (Codex final message)

## Summary

I found no source-backed correction to the design. This was a read-only review; I did not run tests or modify files.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|

## Checked, no change

- §5–9: The hot image, cold journal, sensor buffers, proxy identity, moved flags, ring eviction, and restore order are consistent with the relevant paths in `src/`.
- §10 and §14: The caller contract and API sketch agree on pending calls, one-way rewind, unavailable ticks, and replay.
- §12–13: The verification plan covers the main sleep, ownership, tree-order, and ring transitions. The phased limits are stated.
- The existing recording hash covers transforms and velocities, as the design says; `ScrubBackward` provides the narrower evidence claimed.

## Proposed edits

None.

## Unresolved / disagreements

The design’s §16 decisions, particularly acceptance of the three order changes, remain for the implementers. I found no additional disagreement.

## Verdict

CONVERGED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 20 (target doc and source only, reviews directory excluded, no curated decline list). Session 01a0ef47-4a91-7173-a502-7acafcbaadbb. Start 2026-09-30T08:27:00+10:00, end 08:32:20, exit code 0, no resume needed. `git status` shows no change Codex could have made beyond the uncommitted doc edits already present.

No findings to verify. Before recording `CONVERGED`, Claude rechecked the doc against AC-1 through AC-4 (WORKFLOW.md), since a clean round says nothing about them (AC-5):

- **AC-1.** The hot image is a point-in-time copy the ring owns and discards on rewind; nothing keeps it consistent with the live awake set as that set changes. The restore-time shape list exists only for the duration of one restore. No structure is held alongside a live population.
- **AC-2.** Completeness rests on types (incomplete container types, const-qualified records, one write accessor per structure, one function per structural create and destroy and per move into the awake set), not on a caller list. §5.2's table is stated as a migration worklist the compiler supersedes. Not violated.
- **AC-3.** No second structure is reconciled with a first; trees are derived from fat AABBs at restore rather than mirrored.
- **AC-4.** The setter entry's "journal unless the owner is awake" is the design's stated rule for what the image covers, not a guard against a case the structure permits; the hot accessor's owner-awake and in-step checks are assertions. Restore's tick check is API argument validation. No violation named.

## Status

First clean round after twenty rounds with findings. Round 20's four findings (two test-plan gaps, one comment/code mismatch in the API sample from round 19's fix, one C buildability defect) were all applied, and this round, fresh and full-scope, found nothing. The series converged at round 21: this round found nothing wrong with what the doc currently claims, which is not architectural endorsement, and any later revision reopens the doc at the same full scope. The outcome is recorded in the target doc's `status:` front matter. Nothing committed.

Round 21: 0 findings (0H-0M-0L), 0 applied, 0 declined
Series total: 67 findings (8H-37M-22L) across 21 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L)
