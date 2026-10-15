---
title: Codex review - rewind-only history ring, round 50
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round49.md
mode: full pass, same prompt and scope as round 40, on revision 49
---

# Round 50

## Exact request sent to Codex

Identical to round 49 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T21:20:46+10:00 to 2026-10-15T21:26:38+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f4b-10b9-7772-9c5b-0d259737f356`.

## Review report (Codex final message)

## 1. Summary

The design accounts for the current awake and sleeping trees, migration queue, and sorted collision candidates. I found one correctness gap in the proposed compile-enforced journal boundary for sleeping joint sims.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §5.2, §7.3 | The design makes `b3GetJointSim` const-returning but omits `b3GetJointSimCheckType`. That helper exposes a writable sim from a sleeping set (`src/joint.h`, `src/joint.c`), leaving joint setters a path around the journaled array `set` operation. Changing only the underlying getter produces a discarded-const diagnostic; the default build does not treat warnings as errors (`CMakeLists.txt`). The claimed compile-enforced boundary therefore does not hold. | Make every joint-sim lookup, including `b3GetJointSimCheckType`, read-only to callers. Route setter writes through the journaled array accessor and make qualifier violations a build failure. |

## 3. Checked, no change

- Pair creation, continuous candidates, and explosion hits use the ordering described in §3 (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The live broad phase has separate awake and sleeping dynamic trees and a migration queue, as §5.1 describes (`src/broad_phase.h`, `src/broad_phase.c`).
- Sensor overlaps are processed every step; `overlaps2` is the cross-step overlap state identified in §5.1 (`src/sensor.c`).
- The recording hash covers transforms and velocities, consistent with the limited evidence claimed in §1 (`src/recording.c`, `test/test_recording.c`).

## 4. Proposed edits

Revise §5.2 and §7.3 to name `b3GetJointSimCheckType` explicitly, specify the read-only and write-accessor split for joint setters, and require a build gate that rejects discarded qualifiers.

## 5. Unresolved

None.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (H) `b3GetJointSimCheckType` left writable, and a discarded qualifier only a warning:
  confirmed, applied.** `joint.h` declares `b3GetJointSimCheckType` returning a writable
  `b3JointSim*` and every joint file's setters call it, on a sleeping joint included; the
  getter sentence named only `b3GetBodySim`, `b3GetBodyState` and `b3GetJointSim`. Assigning a
  const-returning getter's result to a non-const pointer is a constraint violation C compilers
  report as a warning, and `src/CMakeLists.txt` makes warnings errors only under the opt-in
  `BOX3D_COMPILE_WARNING_AS_ERROR`. §5.2 now names the fourth getter and states that the box3d
  target compiles a discarded qualifier as an error; §7.3 counts that flag in the one-time work.
  The reviewer's "route setter writes through the journaled array accessor" is what §5.2 already
  says for the sim-writing setters.

Raw log: the reviewer read no round record or changelog.

## Status

One High finding, in §5.2, a consequence of round 44's fix, which named three of the four
writable sim getters. §5.2 last produced a finding in round 45, four rounds ago, so this is not a
recurrence under U-S4. Fix folded; a fresh clean round and its confirmation are needed (U-S8).
Not converged.

Tripwire (U-T): no mechanism added or removed; a fourth getter made const-returning and a
compiler flag named (U-T1). #1 is a consequence of round 44's fix #1, which the owner's stop rule
and the §5.2 cut did not cover (U-T2); the reviewer read no round record. Doc length 1170 ->
1172 lines; #1 added 2 (U-T3). Over the split check (U-T6): unchanged, one decision.

Round 50: 1 findings (1H-0M-0L), 1 applied, 0 declined
Series total: 111 findings (21H-56M-34L) across 50 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L)
