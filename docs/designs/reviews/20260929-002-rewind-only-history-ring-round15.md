---
title: Codex review — rewind-only history ring, round 15
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high; resumed once after a shell timeout.
session: 01a0eeec-c6ec-7dc0-b438-59c5901de421
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 15

## Review report (Codex final message)

## Summary

The rewind-only design is broadly coherent, but its cost comparison overstates the scoped alternative, and one proposed journal optimization is unsafe. This was a read-only source review; no tests were run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§1, 11, 15 | The claim that scoped resimulation is proportional to the predicted subset omits its full dynamic-tree scan when pairs update and its scan of every sensor. The cited scoped design acknowledges both costs. These stages also limit how much sleep reduces whole-world replay cost in this proposal. | Qualify both cost comparisons and report broad-phase and sensor time separately in replay measurements. |
| 2 | Medium | §16, question 6 | “Only the first recorded old value of a record” is insufficient when an id is freed and reused within one tick. The LIFO id pool permits that sequence; undoing its structural operations can require the intervening record value. | Remove the suggestion or restrict deduplication to one uninterrupted record lifetime, with a reverse-undo proof and reuse test. |
| 3 | Low | §5.3 | “`world->names` is not read by the step” is literally false: CCD diagnostics read it. The reads do not appear to affect physics. | Say that name-cache lookups do not affect simulation results, while diagnostic output may use retained names. |
| 4 | Low | §§8, 14 | Both configuration fields require only values of at least 1, so `captureInterval` can initially exceed `tickCount`; §8 then says the effective interval never exceeds `tickCount`. | Validate `captureInterval <= tickCount`, or specify how larger initial intervals behave. |

## Checked, no change

- The existing recording test checks backward seek and replay on a four-box scene; the design correctly limits what its transform and velocity hash proves.
- The source supports the inventory’s distinction between awake state and sleeping solver sets, including the wake and sleep transitions.
- Sensor processing visits sensors regardless of owner wake state, supporting the stated sensor-count exception.
- Proxy allocation uses a separate tree free list, so proxy identity needs treatment apart from the six world id pools.
- The proposed CCD and explosion order changes address order dependence present in the CCD traversal and explosion callback.

## Proposed edits

Correct the scoped-cost comparison, constrain or remove the record-write deduplication suggestion, and clarify the name-cache and interval statements as described above.

## Unresolved / disagreements

The numerical performance baseline was not verified because its cited review file was excluded by the requested reading boundary. The capture targets and the effects of the three solver order changes remain implementation and measurement questions.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: fresh full-scope prompt, no curated decline list, Codex told not to read `docs/designs/reviews/`. Attempt 1: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms; start 2026-09-30T06:48:08+10:00, end 06:57:08, exit code 124 (shell timeout). Attempt 2: same model and effort, `resume 01a0eeec-c6ec-7dc0-b438-59c5901de421` with a finalize prompt, start 06:57:17, end 06:57:48, exit code 0. `git status` shows no change Codex could have made. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** `broad_phase.c`'s moved-sibling gather walks every sibling pair of the whole dynamic tree, and `sensor.c` runs a parallel-for over every sensor; the scoped design's own §9.3 states both. The doc's "proportional to the predicted subset" (§1) and "O(predicted) resim" (§15) overstated it. Both now say collide and solve are proportional to the subset while pair discovery and the sensor pass stay O(world). The measurement suggestion is not applied: §12 test 5 already reports the first replayed step and replay against N × step.
- **#2 (Medium), applied.** Ids are LIFO (`id_pool.c`, §3), so a record's id can be freed and reallocated inside one tick, and the intervening pool entries read the record's live bytes. §16 question 6 now scopes "first old value" to one id lifetime.
- **#3 (Low), applied.** `solver.c` reads `world->names` in the CCD stall diagnostic (log text only). §5.3 now says the step reads it only for log text and results do not depend on it.
- **#4 (Low), applied as an assertion.** §14's `captureInterval` had no upper bound, while §8's doubling stops at `tickCount`. The field is now asserted at most `tickCount`.

## Status

Four findings, two Medium and two Low, from five the round before. Categories: an overstated cost comparison against the scoped design (#1), an open-question suggestion that was unsound across id reuse (#2), a literally false statement about the name cache (#3), an unvalidated config range (#4). None was a consequence of round 14's fixes. None reopened rationale the doc already stated. A clean round has not occurred, and the series has not converged. AC-1 to AC-4 were re-applied before the round and found no shadow structure, caller list, second structure or gate-shaped check. Next: another fresh full-scope round 16, same brief.

Round 15: 4 findings (0H-2M-2L), 4 applied, 0 declined
Series total: 52 findings (7H-31M-14L) across 15 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L)
