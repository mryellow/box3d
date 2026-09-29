---
title: Codex review — rewind-only history ring, round 17
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium; resumed once after a shell timeout.
session: 01a0ef07-c1fd-7a40-b5e3-aefcf687abf2
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 17

## Review report (Codex final message)

## Summary

The proposed undo journal and hot image approach is broadly consistent with the source paths checked. I found one performance acceptance gap and two specification inconsistencies. This was a read-only review; no files were changed.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §7.4, §12, §13 | Phase 2 can force a full dynamic or kinematic tree rebuild on the first replayed step, including in a world with many sleeping proxies. The document acknowledges this cost, but its acceptance criteria only require reporting it separately. A correction could therefore pass the stated gates while missing the intended frame budget. | Set an acceptance bound for first-step replay latency on the million-sleeper scene, and measure it as part of correction latency. |
| 2 | Low | §7.1 | The “leaving the awake set” proof says that when an owner has stayed asleep since T, “nothing wrote” its bytes. Setters can write sleeping records; the design relies on their journal entries to undo those writes. The conclusion can hold, but this sibling case is stated incorrectly. | Amend both T ≥ t cases to account explicitly for journaled writes made while asleep. |
| 3 | Low | §14 | `GetRestorableTick` maps a request later than `currentTick` to the newest image, while `Rewind` rejects a later tick. The sample correction flow can silently turn a future server tick into a successful rewind to an earlier tick. | Return `UINT64_MAX` for future requests, or require an explicit `serverTick <= currentTick` check in the sample and API guidance. |

## Checked, no change

- The recording test checks backward seek and replay on its four-box scene; its current hash covers body transforms and velocities, as the design says.
- The six id pools use a bump index and LIFO free array.
- Wake, sleep, transfer, contact, shape, sensor, and hull paths support the proposed hot/cold inventory and identify the structural writes the design plans to journal.
- CCD sensor collection is currently capped and traversal dependent; explosion impulses currently accumulate in tree traversal order. The proposed order changes address those identified paths.
- The source and user documentation support the stated worker-count and cross-platform determinism claims, subject to the document’s build and caller contract.

## Proposed edits

Address findings 1–3 in the cited sections. No repository edits were made.

## Unresolved / disagreements

The cited performance review is under `docs/designs/reviews/`, which the review instructions prohibited reading. Its benchmark numbers were therefore not independently verified here.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: fresh full-scope prompt as in round 16, no curated decline list, first round at reasoning effort medium. Attempt 1: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms; start 2026-09-30T07:17:37+10:00, end 07:26:37, killed by the shell timeout (the exit code the command printed was read after a command substitution and showed 0; the raw log ends mid-research with no final message, so it was a timeout). Attempt 2: same model and effort, `resume 01a0ef07-c1fd-7a40-b5e3-aefcf687abf2` with a finalize prompt, start 07:26:48, end 07:27:09, exit code 0. `git status` shows no change Codex could have made. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), declined, rationale added to the doc.** §7.4 and §12 test 5 already say the first replayed step pays the tree rebuild and that it is reported separately. A numeric bound cannot be set before phase 2 is measured, and the cost is the engine's own O(nodes) rebuild for a tree whose proxies move. §12 test 5 now says why it has no fixed bound and that the measured figure is for the caller to weigh.
- **#2 (Low), applied.** Setters on sleeping records are journaled writes (§7.1's record-write rule), so "nothing wrote them" was wrong for T ≥ t with no wake. The bullet now says every write since T was a journaled non-awake write that the walk undoes. The "woken since" bullet is unaffected: the wake's pre-wake entries record the bytes the sleep left, and those bytes already include any journaled writes made asleep.
- **#3 (Low), declined.** A repeat of round 16's #3 without engaging the rule added to §14 that `b3World_GetRestorableTick` applies the same rule to every tick, and the sample already states the `serverTick <= currentTick` precondition in its comment. `Rewind` on a rewound-to-earlier tick is not silent: the caller gets the tick back from the helper and the sample compares it with `serverTick`.

## Status

Three findings, one Medium and two Low, from three the round before. Categories: an acceptance-criterion request for a number the design cannot yet know (#1), an inaccurate sibling-case statement in the sleep proof (#2), a repeat of a declined API convention (#3). None was a consequence of round 16's fixes. #3 reopened ground the doc's rationale covered and did not engage it; #1 asked for rationale the doc lacked, now added. A clean round has not occurred, and the series has not converged. Next: another fresh full-scope round 18 at Sol/Medium, same brief. Nothing committed.

Round 17: 3 findings (0H-1M-2L), 1 applied, 2 declined
Series total: 58 findings (8H-32M-18L) across 17 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L)
