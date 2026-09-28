---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 13), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 13

## Review report (Codex final message)

## Summary

The whole-world history approach is sound, but the draft has two restore gaps and one wake-path hazard that need changes before its bit-exact claim can be tested reliably.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1, §9 | Shape bounds are imaged while a body is awake, but become cold when it sleeps. The awake hot path updates `shape->aabb` and `fatAABBs`; `b3TrySleepIsland` does not write either field. Redo from an earlier image to a later tick where the body is asleep therefore has no entry carrying its final bounds. The wake checkpoint only supplies the pre-wake value for undo. (`src/solver.c`, `src/solver_set.c`) | Journal each shape's current bounds when its body enters a sleeping set. Test forward scrub across awake → asleep and asleep → awake → asleep sequences. |
| 2 | Medium | §12 | The proposed "full state hash" lists joint sims but omits joint records. `b3Joint.collideConnected` affects body-pair filtering; changing it when the bodies have no contact can leave the listed hash inputs unchanged while changing future simulation. (`src/joint.h`, `src/joint.c`, `src/body.c`, `src/broad_phase.c`) | Hash simulation-affecting `b3Joint` record fields, including `collideConnected`, for every live joint. |
| 3 | Medium | §5.2, §7.1, §10 | `b3Shape_ApplyWind` gets a pointer to a sleeping body sim, wakes the body, then writes force and torque through the old pointer. Wake currently destroys that allocation; under the proposed ownership transfer, the pointer would instead refer to journal-held pre-wake data. The draft's classification of this as a non-awake sim write does not make the wake path safe. (`src/shape.c`, `src/body.c`, `src/solver_set.c`) | Reacquire the awake sim after waking, before using or writing it. Correct the inventory and add a replay test for wind that wakes a sleeping body. |

## Checked, no change

- Broad-phase candidate pairs are sorted by shape-pair key before contact creation, supporting §7.4's tree-order claim. (`src/broad_phase.c`)
- Sensor overlap results are sorted and deduplicated before event comparison, as §3 states. (`src/sensor.c`)
- The existing recording hash covers transforms and velocities at a much narrower granularity than §12 proposes. (`src/recording.c`, `test/test_recording.c`)
- A wake copies sleeping body sims into the awake set and destroys the sleeping set, consistent with §7.1's need to retain those arrays for undo. (`src/solver_set.c`)

## Proposed edits

1. Add an awake-to-sleep bounds checkpoint to §5.2 and §7.1, and explain in §9 how redo restores it when the target body is asleep.
2. Expand §12's hash inventory to include live joint records.
3. Add the `b3Shape_ApplyWind` pointer repair to the required engine changes and its wake case to the replay tests.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:49:55+10:00, end 20:54:33+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 5 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–7 (nine items) was included in the prompt; Codex did not
re-raise or dispute any of them.

All three findings verified directly against source:

- **#1 (the round-6 wake checkpoint protects undo but nothing protects the symmetric redo case —
  a forward walk to a later asleep tick has no entry recovering the correct post-awake bounds),
  CONFIRMED, applied.** Confirmed `b3TrySleepIsland` (`solver_set.c`) contains no reference to
  `aabb`, `fatAABB`, or `shape` at all — it writes nothing to shape bounds at the sleep
  transition, exactly mirroring round 6's finding that `b3WakeSolverSet` writes nothing at the
  wake transition. Traced the failure precisely: for a forward walk (§9 step 2, redo) from
  position P to a target tick T where the body is asleep, having passed through an awake stretch
  in between, the hot-path aabb writes during that awake stretch are — by design — never
  journaled (only that stretch's own per-tick images cover them, and §9 only ever applies the
  *target* tick's image, never an intermediate one). The round-6 wake checkpoint only records the
  *pre-wake* (frozen-while-asleep) value, for undo. Nothing records the *post-awake* value at the
  moment the body re-enters sleep, so a redo walk reaching T has no source for the correct final
  bounds and leaves them at whatever pre-existing value the walk inherited from before the awake
  stretch — stale. This is the mirror image of round 6's bug (undo vs. redo, wake vs. sleep), not
  the same bug re-found. Fixed by adding a symmetric checkpoint at the sleep transition
  (`b3TrySleepIsland`), so both boundaries — wake for undo, sleep for redo — are covered.

- **#2 (`b3Joint.collideConnected`, which gates contact-pair filtering, lives outside
  `b3JointSim` and is missing from §12's hash, which only names "joint sims"), CONFIRMED,
  applied.** Confirmed `collideConnected` is a `b3Joint` field (`joint.h`), set at creation and by
  its own setter (`joint.c`), and is read directly to filter whether two jointed bodies' shapes
  may generate a contact at all (`body.c`: `if ( joint->collideConnected == false && ... )`) — a
  binary, genuinely simulation-affecting decision, not merely a value baked into `b3JointSim`'s
  already-hashed solve state. `b3Joint` is already correctly journaled (confirmed by an earlier
  round via the `joints[id]` row), so this was a hash-coverage gap only, the same class as round
  9's proxy `categoryBits` and round 11's `shape->aabb` — the restore mechanism was already
  correct, the verification oracle just didn't name the field. Fixed by naming joint records
  (including `collideConnected`) alongside joint sims in §12's hash list.

- **#3 (`b3Shape_ApplyWind` writes force/torque through a body-sim pointer captured *before* a
  wake it itself triggers), CONFIRMED, applied.** Read `b3Shape_ApplyWind` (`shape.c`) in full:
  `b3BodySim* sim = b3GetBodySim( world, body )` is fetched, then, if the body isn't already
  awake, `b3WakeBodyWithLock` is called — which moves the body's `bodySim` into the awake set's
  array, a different (and differently-owned, post this design's own ownership-transfer fix)
  allocation — and the function proceeds to use `sim->transform`, `sim->localCenter`, and finally
  `sim->force = b3Add( sim->force, force )` / `sim->torque = ...`, all through the *original,
  pre-wake* pointer, never re-fetched. Tracing every path that reaches this final write confirms
  the body is *always* awake by the time of the write (asleep-and-`wake==false` returns early
  before ever writing) — so this function's write was never actually meant to be a "non-awake sim
  write" at all, contrary to its listing in §5.2's non-awake `bodySims`/`bodyStates` row; every
  reachable write is meant to land in the *awake* array. Under this design's own ownership-transfer
  mechanism specifically, the consequence is worse than a plain use-after-free: the pre-wake
  sleeping-set array is kept alive (owned by the journal entry, for undo) rather than freed, so
  the stale write would silently corrupt journal-held historical data instead of crashing —
  exactly the kind of bypassed-journal corruption requirement 7's cold-hash guard exists to catch,
  but only once, well after the fact. Fixed by removing `b3Shape_ApplyWind` from §5.2's non-awake
  row (its write, once correct, is a hot/awake write like any other, covered by the image, not a
  cold journaled one) and adding the required source fix (re-fetch `sim` after waking, before any
  read or write through it) to §11.3's enumerated engine changes.

## Status

Round 13 of an ongoing series (rounds 1–12 committed). All three findings genuine and applied.
Finding #1 is the fourth round (after 5, 8, 10) to find a live-state-corrupting bug in the
restore/journal mechanism, and specifically confirms round 10's own status note's suspicion that
one-sided undo/redo protection was a recurring defect shape — this time for a *different*
mechanism (the round-6 wake checkpoint) than round 10's hull fix, showing the earlier self-check
(which cleared the ownership-transfer arrays of this concern) correctly scoped itself to that one
mechanism rather than the class of bug in general. Finding #3 is notable for being a genuine,
independently-confirmable bug in the *current* engine source, whose consequences specifically
change character (crash today → silent historical corruption under this design) once the ring's
ownership-transfer mechanism ships — exactly the kind of interaction between new and existing code
this review series exists to catch before the design is built. Finding rate (8, 8, 8, 7, 8, 3, 1,
2, 1, 1, 2, 2, 3 applied) has now found genuine defects in every one of the last five rounds.
Another full-scope round is warranted, with attention to whether any other caller-visible function
follows the same "fetch a pointer, conditionally wake, use the stale pointer" pattern as
`b3Shape_ApplyWind`.
