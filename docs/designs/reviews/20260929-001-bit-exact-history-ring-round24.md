---

title: Codex review — bit-exact history ring, round 24
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 24

## Review report (Codex final message)

## Summary

The hot-image and cold-journal approach is broadly consistent with the source tree, but the design needs changes before it supports its correctness and cost claims. This was a read-only review; I did not execute tests or modify files.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§10, 13–14 | The design does not define how history interacts with an active recording. Recording logs API calls and steps, but its operation list has no rewind. Rewinding and then continuing to record would leave playback following the old timeline (`src/recording_ops.inl`, `src/physics_world.c`). | Specify and enforce an API contract: either reject rewind while recording, or add a recording operation that represents rewind and branch truncation. Test the chosen behavior. |
| 2 | Medium | §§2.7, 5.2, 7.1 | The “verified by construction” rule lacks a concrete boundary for shared hull database mutations. `b3AddHullToDatabase`, `b3AddOwnedHullToDatabase`, and `b3RemoveHullFromDatabase` change membership and refcounts through the map in `src/physics_world.c`; §5.2 specifies journaled types for other containers but not this one. | Make those three functions the stated, exclusive journaled mutation boundary, including zero-ref ownership transfer. Add create, replace, deduplicate, and undo/redo cases. |
| 3 | Low | §§5.1, 11.1 | The estimated `b3Shape` image cost of ~140 B is too low for the listed whole-record copy. Its 40 B inline material, filter, pointers, and geometry union alone put the single-precision 64-bit layout near 200 B (`src/shape.h`, `include/box3d/types.h`). | Measure `sizeof(b3Shape)` in the intended builds and update per-shape and scene cost estimates. |

## Checked, no change

- `ScrubBackward` compares replayed state hashes, and the current hash covers body transforms and velocities; the design correctly limits what that test demonstrates (`test/test_recording.c`, `src/recording.c`).
- The serializer restores into an existing world and omits host `userData` values, as described for phase 0 (`src/world_snapshot.c`, `src/recording_replay.c`).
- Sensor overlap processing runs across the sensor array and sorts and deduplicates overlap results (`src/sensor.c`).
- Proxy IDs have a separate tree free list; CCD sensor collection and explosion impulses currently depend on tree traversal order (`src/dynamic_tree.c`, `src/solver.c`, `src/physics_world.c`).
- The phase-1 raw-tree image limitation is explicitly disclosed, rather than claimed to meet the final awake-proportional cost requirement.

## Proposed edits

Define recording coexistence in the caller contract and API, name the hull database’s journal boundary in §5.2, and replace the shape-size estimate with measured values.

## Unresolved / disagreements

The three solver order changes remain design decisions for the solver owner, as §16 states. The proposed full-state hash and performance targets still require execution to validate.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:59:59+10:00, ended 14:04:47+10:00, exit code 0 — no resume needed. Codex ran
read-only; the `git status --short` file list before and after was identical, so the working tree
was untouched by Codex. Same fresh full-scope prompt as rounds 18 to 23, no declined-findings list.
The final message above is unchanged.

**Findings verified against source.**

1. **Applied.** `recording_ops.inl` has a `Step` operation and the world mutators but no rewind
   operation, so a recording that continued after `Rewind` would replay the discarded timeline.
   §10 now makes recording and history mutually exclusive, each asserting the other is inactive.
   This is a contract choice, not a source fact; the alternative Codex offered (a rewind operation
   in the recording format) is a larger change and is not needed for v1.
2. **Applied.** The hull database is mutated only by `b3AddHullToDatabase`,
   `b3AddOwnedHullToDatabase` and `b3RemoveHullFromDatabase` (`physics_world.c`), but §5.2 did not
   name them as the boundary the way it names the journaled containers. It now does, and §12 test
   2's churn adds hull create, destroy and deduplicated create.
3. **Applied.** A probe compiled against `src/shape.h` gives `sizeof(b3Shape)` = 208 B, not the
   ~140 B estimate. §5.1 now states 208 B in both places.

3 applied, 0 declined.

## Status

Finding count rose to 3 (1H-1M-1L) from 2. The High is a new category (interaction with the
recording system), not a consequence of round 23's fixes; the Medium sits in text that predates
round 22; the Low was already flagged in the doc as an estimate needing a probe. No finding
reopened ground the doc's rationale covered. The last seven counts (3, 4, 2, 4, 3, 2, 3) hover at 2 to
4, so the trend is flat and the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 24: 3 findings (1H-1M-1L), 3 applied, 0 declined`

`Series total: 123 findings (45H-66M-12L) across 24 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L)`
