---
title: Codex review — rewind-only history ring, round 13
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 13

## Review report (Codex final message)

## Summary

The hot image and undo journal design is broadly consistent with the source paths reviewed, including sleep and wake transitions, sensor storage, proxy identity, and the recording hash’s limited coverage. I propose three changes to the contract and verification plan.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §2 requirement 1; §5.2; §10 | “Two named exceptions” is incomplete. `Rewind` deliberately leaves world callback registrations, callback contexts, and `world->userData` at their current values. Those values can affect replay through the callbacks (physics_world.c). §10 gives the caller a restoration duty, but requirement 1 says every simulation-affecting byte is restored and names only callback order and name collisions as exceptions. | Scope requirement 1 to engine state restored by the ring, and state the host-state restoration precondition there. Make the callback registration and context rule explicit alongside the two exceptions. |
| 2 | Medium | §7.4; §12 test 7; §16 | The proposed CCD sensor pass keeps hits with fractions **strictly less** than the final solid fraction and says that matches today’s engine. The current callback accepts `output.fraction <= continuousContext->fraction` (solver.c). Equality therefore changes reported hits. Test 7 compares replay with an original run under the changed code, so it would not establish whether the change was intended. | Decide and document the equality rule. Add a test that asserts the expected sensor event at a sensor and solid hit with equal fractions; include the behavior change in §16 if equality is to be excluded. |
| 3 | Low | §14 API sketch | `b3World_GetRestorableTick` maps a requested tick *later* than `currentTick` to the newest image, while `b3World_Rewind` rejects a future tick. In the sample correction flow, a future `serverTick` can consequently produce a successful rewind and no correction. | Return `UINT64_MAX` for future requests, or guard `serverTick > currentTick` in the sample and specify that the caller queues that correction. |

## Checked, no change

- The recording test checks backward seeks on a four-box scene, while `b3HashWorldState` hashes body poses and velocities only (test_recording.c, recording.c).
- Sleep and wake move bodies, contacts, joints, and islands between solver storage as the design describes (solver_set.c).
- Sensors swap overlap buffers and process sensors regardless of their owner’s awake state (sensor.c).
- Proxy ids come from a per-tree free list; pair creation sorts candidate keys before creating contacts (dynamic_tree.c, broad_phase.c).
- The current explosion callback wakes bodies and accumulates impulses during tree traversal, supporting the proposed ordering change (physics_world.c).

## Proposed edits

1. Align requirement 1 with §10’s caller-owned callback and host-state obligations.
2. Specify CCD sensor equality semantics and test the resulting event, not only replay equality.
3. Make future-tick handling safe in the helper contract and correction example.

## Unresolved / disagreements

The performance baseline is attributed to a design review file outside the permitted reading scope, so I did not independently verify those measurements. The document appropriately treats its new cost targets as requiring measurement.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 12 with no curated decline list. Session 01a0eed1-0582-7ed3-93a8-eb6ed79d54a4. Start 2026-09-30T06:17:50+10:00, end 06:24:15, exit code 0, no resume needed. The doc's round 12 edits were made in this session and not re-diffed. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** §2 requirement 1 said every simulation-affecting byte is restored while §10 and §5.2 say callback registrations, contexts and `world->userData` are left as the caller last set them. Requirement 1 now says the restored bytes are engine state and names host configuration as not engine state, with the caller setting back what its callbacks read.
- **#2 (Medium), declined, with the doc's wording sharpened.** The premise mixes two comparisons in `solver.c`. The `<=` Codex cites is the in-traversal gate inside the continuous callback, which the doc replaces by a two-pass scheme (§7.4 says it does so, "rather than gating and capping during traversal"). The doc's "strictly less than the final solid fraction, as the engine does today" describes the filter after the sweep (`context.sensorFractions[i] < context.fraction`, `solver.c`), which is strict, so an equal-fraction sensor hit is already dropped today and the rule is unchanged. §12 test 7 already includes a sensor and a solid hit at exactly equal fractions. §7.4 now says "the engine's filter at the end of the sweep" so the reference is unambiguous. A test asserting the unchanged behavior against the pre-change engine is a regression test for the implementation, not a design defect.
- **#3 (Low), applied.** `b3World_GetRestorableTick` returning the newest image for a future tick is deliberate and documented (§14), and the tick it returns is always valid for `b3World_Rewind`. The sample's `serverTick` is at or before the current tick in any client prediction flow, and the sample now states that as its precondition rather than changing the helper.

## Status

Three findings, two Medium and one Low, from a count of 5 the round before. Categories: a requirement statement inconsistent with §10 (#1), a misread of which comparison the doc refers to (#2, declined) and a sample-code precondition (#3). None was a consequence of round 12's fixes. #2 re-raised ground the doc had stated but ambiguously, and its argument did not engage the two-pass rationale; the reference was sharpened. Finding count is falling and the categories are now contract wording, not mechanism. Next: another fresh full-scope round 14, same brief.

`Round 13: 3 findings (0H-2M-1L), 2 applied, 1 declined`

`Series total: 43 findings (7H-26M-10L) across 13 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L)`
