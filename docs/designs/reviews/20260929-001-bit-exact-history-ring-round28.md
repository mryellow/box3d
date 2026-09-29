---

title: Codex review — bit-exact history ring, round 28
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 28

## Review report (Codex final message)

## Summary

The source supports the design’s overall direction: the recording system demonstrates rewind and replay for the state its current hash covers, while an awake image and structural journal could avoid whole-world capture cost. Three rules need clarification before the bit-exact and verification claims are internally consistent.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12.2, §8 | The restore test calls `SetAwake(false)` immediately after each rewind while also testing forward scrub. That call changes simulation state and, by §8’s rule, discards future slots. Both cases cannot run on the same branch. | Separate an untouched backward/forward scrub test from a rewind–mutate–replay test. |
| 2 | Medium | §7.4 | The proposed CCD sensor cutoff does not specify whether a sensor hit exactly at the final solid fraction is retained. [solver.c](/home/user/box3d/src/solver.c) currently collects with `<=` but publishes hits only with `<`. A two-pass implementation using `<=` at publication would change events. | State the final comparison explicitly, then test equal-fraction sensor and solid hits. |
| 3 | Medium | §5.1, §9–10 | “World scalars” does not define a precise restore boundary. The world also holds callback contexts, worker configuration, task pointers, and host data; §10 keeps host configuration live and permits a different worker count. Copying an insufficiently separated scalar struct could violate that contract. | Enumerate the imaged scalar fields and explicitly exclude host and worker infrastructure fields and callback contexts. |

## Checked, no change

- [test_recording.c](/home/user/box3d/test/test_recording.c) checks backward seek against forward-pass hashes; [recording.c](/home/user/box3d/src/recording.c) confirms that hash covers transforms and velocities, not the proposed full state.
- [id_pool.c](/home/user/box3d/src/id_pool.c) uses a LIFO free array and bump index as described.
- [solver.c](/home/user/box3d/src/solver.c) confirms the running CCD fraction and eight-hit sensor cap; [physics_world.c](/home/user/box3d/src/physics_world.c) confirms explosion impulses currently follow tree query order.
- [sensor.c](/home/user/box3d/src/sensor.c) processes sensors each step and sorts overlap results. [simulation.md](/home/user/box3d/docs/simulation.md) also documents unspecified query callback order.
- [world_snapshot.c](/home/user/box3d/src/world_snapshot.c) confirms that phase 0 clears object `userData` on restore, matching the stated limitation.
- The determinism description agrees with [faq.md](/home/user/box3d/docs/faq.md), [simulation.md](/home/user/box3d/docs/simulation.md), and the floating-point build setting in [CMakeLists.txt](/home/user/box3d/CMakeLists.txt).

## Proposed edits

Apply the three table actions to the [design document](/home/user/box3d/docs/designs/20260927-002-bit-exact-history-ring.md). Keep the separate §12.6 test that deliberately checks truncation after a post-rewind force, torque, or impulse.

## Unresolved / disagreements

The accessor and container boundary is central to the “verified by construction” claim. The document appropriately makes completion of that source migration a phase gate; the claim cannot be confirmed from the current direct-write source alone. The phase 1 and phase 2 cost targets likewise remain proposed acceptance criteria, not measured results.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:37:19+10:00, ended 14:41:34+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 27, no declined-findings list. The final
message above is unchanged.

**Findings verified against source.**

1. **Applied.** §12 test 2 runs both directions and also calls `SetAwake(false)` after each rewind;
   §8 truncates the ring after T on that first write, so a forward scrub on the same branch cannot
   follow it. The call is now stated as part of the backward pass only, the forward pass omitting it.
2. **Applied.** `b3ContinuousQueryCallback` (`solver.c`) collects a sensor hit with
   `output.fraction <= continuousContext->fraction`, and the publishing loop keeps it only when
   `sensorFractions[i] < context.fraction`. §7.4's two-pass description did not state the final
   comparison, so an equal-fraction hit was ambiguous. It now says strictly less than the final solid
   fraction, and §12 test 7 adds a sensor and a solid hit at exactly equal fractions.
3. **Applied.** `physics_world.h` keeps `preSolveContext`, `customFilterContext`, `workerCount`,
   `enqueueTaskFcn`, `finishTaskFcn` and `userTaskContext` on the world beside the simulation
   scalars; §9 step 3 excluded only callback pointers and `userData`, while §10 lets the worker count
   differ between capture and replay. Step 3 now also excludes the callback contexts, `workerCount`
   and the task-system callbacks and context.

3 applied, 0 declined.

## Status

Finding count fell to 3 (0H-3M-0L) from 4, with no High. All three are specification gaps in text
that predates round 27 (a test's internal contradiction, an unstated comparison, an exclusion list);
none is a consequence of round 27's fixes and none reopened ground the doc's rationale covered. The
counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3) remain flat at 2 to 4, so the series is not
converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 28: 3 findings (0H-3M-0L), 3 applied, 0 declined`

`Series total: 135 findings (46H-75M-14L) across 28 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L)`
