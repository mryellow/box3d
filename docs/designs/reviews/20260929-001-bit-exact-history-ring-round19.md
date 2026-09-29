---

title: Codex review — bit-exact history ring, round 19
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 19

## Review report (Codex final message)

## Summary

The whole-world rewind approach is plausible, and the document identifies several real engine dependencies. I found one gap that can break its branching semantics, plus gaps in the mutation inventory and cost claims. This was a read-only source review; I did not run the proposed implementation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.2, 8 | The first-write rule truncates future history only when a write goes through an accessor or journaled container. Between-step APIs such as `b3Body_ApplyForce`, `ApplyTorque`, and `ApplyLinearImpulse` currently change awake sim or state data directly (body.c). The migration list does not include them. After a rewind, such a call could change the world while forward scrub remains available on the old branch. | Put **every** mutating API through a branch-truncation gate, including force, impulse, wind, and explosion paths. Test a rewind, one such call, then an attempted forward scrub before stepping. |
| 2 | Medium | §5.2 | The table labeled “Current direct-write callers, to migrate” omits body APIs outside the `Set*` family. `b3Body_EnableSleep`, `Disable`, `Enable`, and `ApplyMassFromShapes` can change records, sims, or set membership (body.c). The proposed compile-time boundary could catch these, but the stated migration inventory is incomplete. | Expand the inventory to all mutating API families and require the compile-time boundary before declaring phase 1 complete. |
| 3 | Medium | §§4, 9, 14 | Restore cost is stated as `O(awake + journal)`, while the image and restore also process every sensor’s `overlaps2`, including sensors on static or sleeping bodies (sensor.c). A sensor-heavy world can therefore have restore cost unrelated to awake count. | Include sensor count and overlap bytes in the restore complexity and performance acceptance criteria, as §2 already does for capture. |
| 4 | Low | §3 | “No RNG, no timers in the simulation path” is too absolute. Step and CCD code call timing functions for profiling and stall diagnostics (physics_world.c, solver.c). These do not appear to feed physics results. | Say that timer readings do not affect simulation results. |

## Checked, no change

- The recording hash covers body transforms and velocities, and `ScrubBackward` compares replayed hashes; the document correctly limits what that demonstrates (recording.c, test_recording.c).
- The tree has its own proxy free list, separate from world id pools; proxy identity needs treatment beyond rebuilding layout (dynamic_tree.c).
- CCD’s running fraction, capped sensor hits, and explosion’s tree-order impulse application support the proposed order changes (solver.c, physics_world.c).
- Sensor overlap arrays are swapped each step, and current overlaps are sorted before event comparison (sensor.c).

## Proposed edits

Add an explicit branch-truncation hook for all mutating public APIs; broaden §5.2’s caller inventory; update restore cost formulas and tests for sensors; narrow the timer claim.

## Unresolved / disagreements

The document’s serializer performance baseline cites a file outside the permitted review scope, so I could not independently verify those figures. The proposed ring’s cost targets remain measurements to establish during implementation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:27:23+10:00, ended 13:32:57+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` was empty before and after, so the working tree was untouched by
Codex. Same fresh full-scope prompt as round 18, no declined-findings list (round 18's one decline
was answered in the doc). The final message above has its source links reduced to plain file names,
per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Applied.** `b3Body_ApplyForce` and `b3Body_ApplyForceToCenter` (`body.c`) write an awake body's
   `bodySim->force` and `torque` directly. §5.2's rule already routes every awake-owner API write
   through the structure's write accessor, and the first accessor write after a rewind truncates the
   ring, so the mechanism covers them; the caller table named only the `b3Body_Set*` family, so a
   migration read from it would have missed the `Apply*` family. The table row now names every mutating
   `b3Body_*` API, and §12 test 6 covers a rewind followed by only a force, torque or impulse call and
   an attempted forward scrub.
2. **Applied.** `b3Body_EnableSleep`, `b3Body_Disable`, `b3Body_Enable` and
   `b3Body_ApplyMassFromShapes` exist in `body.c` and were not in the row's `Set*` family; the same
   row edit covers them. The phase 1 completion gate (build fails on a remaining direct write) was
   added in round 17 and already requires the boundary.
3. **Applied.** Capture is stated as O(sensor count + overlaps) in §2 requirement 2, and the image
   carries every sensor's `overlaps2` (§6 gathers), so restore copies them too. §4's restore cost, §12
   test 5's acceptance and the API comment in §14 said only awake plus journal; each now includes
   sensors.
4. **Applied.** `b3GetTicks` and `b3GetMilliseconds` are called throughout `physics_world.c` and
   `solver.c`, all writing `world->profile`. §3 now says timer readings feed only the profile.

4 applied, 0 declined.

## Status

Finding count 4 (1H-2M-1L). The High is an inventory omission in a table whose mechanism already
covered the case, and its siblings (2 and 3) are the same shape as earlier rounds: a rule stated for
capture and not for restore, a caller list narrower than the boundary it feeds. None was a
consequence of round 18's fixes and none reopened stated rationale. The series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 19: 4 findings (1H-2M-1L), 4 applied, 0 declined`

`Series total: 109 findings (41H-59M-9L) across 19 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L)`
