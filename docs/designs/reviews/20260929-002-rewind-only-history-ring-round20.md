---
title: Codex review — rewind-only history ring, round 20
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 20

## Review report (Codex final message)

## Summary

The rewind-only design is broadly consistent with the inspected engine paths. The hot image, undo journal, and derived-tree split is a sound direction. I found gaps in the verification contract and API example, plus one implementation detail that cannot work as written in C.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12 | The full-state hash deliberately excludes body, shape, and joint `userData`, although §10 requires those values to be restored and callbacks may read them. Test 2 compares them separately, but the hash-based replay and cross-platform tests do not independently verify them. | Compare stable test-assigned `userData` values at every restored and replayed tick, alongside the hash. |
| 2 | Medium | §12 | The replay test says the world `userData` and a callback context change during the interval, but does not say to reset host state to its value at T before replay. §10 makes that reset the caller’s responsibility. | Specify the reset and timed replay of host-state changes in test 3; verify callback outputs as well as engine hashes. |
| 3 | Low | §14 | `GetRestorableTick` returns the newest past image for a future tick. The correction example’s comment says a future server tick should wait, but the example has no guard and would rewind and replay without applying that correction. | Guard `serverTick > currentTick` before calling the helper, or make the helper return `UINT64_MAX` for future ticks. |
| 4 | Low | §5.2 | The proposed `const b3WorldScalars* const` member cannot be assigned by the world creation function after a `b3World` object has been created. The current world is a reused global array in `src/physics_world.c`. | Keep the member pointer assignable within the world module while hiding its writable route from other modules. |

## Checked, no change

- The existing recorder serializes world images; `ScrubBackward` checks replayed body-state hashes on its four-box scene. The document correctly limits what that proves (`src/world_snapshot.c`, `src/recording.c`, `test/test_recording.c`).
- The six world id pools, proxy free lists, awake solver arrays, contact manifold storage, sensor overlap swap, and sleeping-set transfers match the inventory’s main distinctions (`src/physics_world.h`, `src/id_pool.c`, `src/dynamic_tree.c`, `src/contact.h`, `src/sensor.c`, `src/solver_set.c`).
- Contact creation and destruction rewrite neighbouring contact edges and body list heads, including records outside the awake set (`src/contact.c`).
- A shape filter change with `invokeContacts` false leaves its proxy in place; the invoked path resets it (`src/shape.c`).
- The proposed CCD sensor cap and explosion ordering changes address traversal-order dependencies present in `src/solver.c` and `src/physics_world.c`.
- The pending-segment rule covers API mutations made without an intervening step. The restore rule also accounts for end events being cleared and regenerated on replay.
- Phase 1 explicitly misses the sleeping dynamic and kinematic proxy cost target; phase 2 addresses it. The verification plan includes that distinction.

## Proposed edits

Incorporate findings 1–4 into §§5.2, 12, and 14. In §12, make the host-state checks independent of the engine hash so a successful hash comparison cannot mask a failed `userData` restoration.

## Unresolved / disagreements

The measured baseline and percentage targets cite a review file that was outside the permitted reading scope, so I did not independently verify those numbers. The solver owner’s acceptance of the three ordering changes remains an explicit design decision.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, a fresh full-scope prompt (target doc and source only, reviews directory excluded, no curated decline list). Session 01a0ef3b-bd48-7303-ae78-f3be6c07809c. Start 2026-09-30T08:14:23+10:00, end 08:19:52, exit code 0, no resume needed. `git status` shows no change Codex could have made.

- **#1 (Medium), applied to §12 test 3.** The hash excludes every `userData` word (a host value), and test 2 is the only test that compares them, so a replay that left a wrong `userData` on a body, shape or joint would be caught only indirectly, through callbacks that happen to read it. Test 3 now compares the distinct words of test 2 at every tick from T+1 to P against those recorded at that tick.
- **#2 (Medium), applied to §12 test 3.** §10 makes the caller set world `userData` and each callback context back to its value at T before the first replayed step and replay its changes per tick; test 3 said only that they change partway through the interval. It now says the test does both.
- **#3 (Low), sample guarded, API unchanged.** The fourth fresh reviewer (rounds 16, 17, 19, 20) to land on `GetRestorableTick` with a future tick. This round's argument is specific to the sample: round 19's comment said a later server tick is applied when that tick is stepped, but the code beneath it had no guard, so with `serverTick > now` it rewound to the newest image, replayed to `now` and never reached `t == serverTick`. That is a real inconsistency between the comment and the code. The sample now wraps the flow in `serverTick <= now`. `GetRestorableTick` keeps one rule for every argument (the newest imaged tick at or before it); making it return `UINT64_MAX` for a future tick would give it a second rule for a case that is the caller's precondition, and the round did not engage that rationale.
- **#4 (Low), applied to §5.2.** Confirmed in `physics_world.c`: `b3World` is an element of the global `b3_worlds` array, and the create path zeroes it with `memset` and then assigns its fields. A member declared `T* const` cannot be assigned there in C, so the design's "const-qualified pointers set only by the creation function" could not be built. The fields are now plain pointers to the incomplete container types (`const b3WorldScalars*` for the scalars), which is what keeps every other file from reaching the storage; the doc says why the pointer itself is not const. Reassigning the pointer is no longer claimed to be a compile error.
- **Unresolved item, no change.** Codex could not verify §11.1's measured numbers because the prompt excluded the perf review it cites; the table is labelled as the serializer baseline, and its cost claims are gated by §12 test 5.

## Status

Four findings, two Medium and two Low, from two the round before; the count rose but none is High. Categories: two gaps in the verification plan's host-state coverage (#1, #2), a repeat sample finding that exposed a real comment-versus-code mismatch left by round 19's fix (#3), and a C-language buildability defect in the const-pointer rule (#4). #3 was a consequence of the immediately preceding round's own fix. #3 also reopened ground the doc's rationale covered, and this round's argument engaged the sample but not the stated rule. #1 and #2 are new test-plan categories; #4 is a new category (language-level buildability of the module-boundary types). A clean round has not occurred, and the series has not converged. Next: another fresh full-scope round 21 at Sol/Medium, same brief. Nothing committed.

Round 20: 4 findings (0H-2M-2L), 4 applied, 0 declined
Series total: 67 findings (8H-37M-22L) across 20 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L)
