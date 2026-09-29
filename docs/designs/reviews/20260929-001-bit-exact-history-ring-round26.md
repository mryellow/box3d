---

title: Codex review — bit-exact history ring, round 26
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 26

## Review report (Codex final message)

## Summary

The hot-image and cold-journal design is broadly consistent with the source tree. I found no demonstrated restore-state error in the proposed mechanism, but three gaps in how its exceptions and verification are specified. This was a read-only review; I did not execute the proposed implementation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12.7 | The explosion test covers several shapes on **one body**, which tests impulse accumulation but not the separately identified wake-order effect across sleeping sets. `b3World_Explode` currently wakes bodies during tree traversal (`src/physics_world.c`). | Add a differing-tree-layout replay test where one explosion wakes bodies in multiple sleeping sets, including connected constraints; compare state hashes and events. |
| 2 | Medium | §§6, 12.6 | Pending between-step journal entries are essential to repeated rewinds, but the listed ring tests only make a pending awake force, torque, or impulse call. They do not directly exercise undo and ownership transfer for a pending structural change. | Add immediate rewind and forward-scrub cases after a create/destroy, a sleeping-body setter, and a heap-owning change such as sensor destruction or hull release, with no intervening step. |
| 3 | Low | §§3, 10 | §10 names layout-dependent custom-filter order “within a CCD sweep,” while the filter also runs during tree queries in `src/sensor.c` and pair discovery in `src/broad_phase.c`. Sorting sensor hits and pair keys occurs after those calls. The general purity requirement covers this, but the specific explanation is incomplete. | Name all three call paths when explaining the callback-order exception. |

## Checked, no change

- `ScrubBackward` checks replayed hashes, and the current hash covers transforms and velocities rather than the proposed full state (`test/test_recording.c`, `src/recording.c`).
- The separate proxy-id free lists, moved bits, CCD candidate processing, sensor overlap pass, and explosion traversal support the distinctions the design makes (`src/dynamic_tree.c`, `src/solver.c`, `src/sensor.c`, `src/physics_world.c`).
- The document accounts for material blocks, contact manifolds and caches, shared hull lifetimes, and host-owned `userShape` handles. It also states that phase 1 falls short of the awake-proportional cost requirement until phase 2 removes raw kinematic and dynamic tree images.

## Proposed edits

Add the two targeted tests in §12 and expand §10’s callback-order description as specified above.

## Unresolved / disagreements

The owner decisions already listed in §16 remain open, particularly acceptance of the CCD, sensor, and explosion order changes. I found no source-based reason to reject the overall recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:12:45+10:00, ended 14:17:19+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 25, no declined-findings list. The final
message above is unchanged.

**Findings verified against source.**

1. **Applied.** §7.4 states the explode wake order as a separate dependence from impulse
   accumulation (`b3World_Explode`, `physics_world.c`), and §12 test 7 exercised only the second.
   The test now adds an explosion waking bodies in several sleeping sets, some connected by
   constraints.
2. **Applied.** §6 makes pending between-step entries the basis of repeated rewinds, and §12 test 6
   made only an awake force, torque or impulse call before its immediate rewind and forward scrub,
   which produces no journal entry. The test now adds the same sequence after a create or destroy,
   a sleeping-body setter, and a heap-owning change (sensor destruction, hull release).
3. **Applied.** The custom filter is also called during the sensor query (`sensor.c`) and pair
   discovery (`broad_phase.c`), both in tree traversal order before their results are sorted. §10's
   callback-order sentence now names all three paths.

3 applied, 0 declined.

## Status

Finding count rose to 3 (0H-2M-1L) from 2, still with no High. All three are verification or
wording gaps in text that predates round 25, none a consequence of round 25's fixes; no finding
reopened ground the doc's rationale covered. Codex reports no demonstrated restore-state error this
round, and the categories are narrowing to test coverage and wording. The counts since round 17 (3,
4, 2, 4, 3, 2, 3, 2, 3) remain flat at 2 to 4, so the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 26: 3 findings (0H-2M-1L), 3 applied, 0 declined`

`Series total: 128 findings (45H-70M-13L) across 26 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L)`
