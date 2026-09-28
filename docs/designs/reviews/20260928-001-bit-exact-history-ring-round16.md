---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 16), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 16

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the source, but three points need correction: the memory budget exception is stated inconsistently, the proposed hash conflicts with restore’s treatment of a transient body field, and the phase 0 serializer does not preserve user-data pointers that callbacks may use.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §2, §8 | §2 permits exceeding `maxBytes` when a *tick’s own* storage exceeds it. §8 also exceeds the budget when the required two-tick window exceeds it, even if each tick fits individually. | State the minimum-window exception in §2 and §14. |
| 2 | Medium | §9, §12 | §9 resets restored bodies’ `bodyMoveIndex`, while §12 proposes hashing body records for restore equality. Finalize sets that field on moving awake bodies at the end of a step, so a literal body-record hash can differ immediately after rewind. `solver.c`, `solver_set.c` | Explicitly exclude this transient field from the state hash, or define a canonical value for it at both capture and restore. |
| 3 | Low | §10, §13 | Phase 0 uses `b3SerializeWorld`/`b3DeserializeIntoShell`, which clear body, shape, and joint `userData`. That prototype therefore cannot meet §10’s general claim that setters’ T-time values return automatically; callbacks that consult those pointers also need host restoration. `world_snapshot.c` | Document the phase 0 limitation and require the caller to restore those pointers before replay, or add a separate host-data preservation mechanism. |

## Checked, no change

- The existing recording hash covers transforms and velocities, rather than the full state proposed in §12. `recording.c`
- Sensor overlap changes compare both shape IDs and generations, supporting §5.2’s use of the sensor change signal. `sensor.c`
- Proxy category bits are stored separately from shape filters, supporting §5.2’s separate journal entry. `dynamic_tree.c`, `shape.c`
- Broad-phase candidate pairs are sorted before contact creation, as §7.4 states. `broad_phase.c`
- Sleeping-set wake and merge operations preserve ordered dense arrays, supporting the design’s decision to journal their structural changes. `solver_set.c`

## Proposed edits

1. In §2, say that the **required minimum window**, including its journals and one image, may exceed `maxBytes`; align the API comment in §14.
2. In §12, define the body hash field by field and omit `bodyMoveIndex`, consistent with §9’s event reset.
3. In §13, state that phase 0 clears body, shape, and joint `userData`, and add the required host restoration step to its replay contract.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T21:31:08+10:00, end 21:34:54+10:00, `EXIT_CODE 0` — a single, clean session this time
(no stale-filename collision; a fresh, timestamped raw-log path avoided round 14/15's issue).
Codex ran read-only; working tree was untouched by it going in (round 15's edits were already
applied). The curated declined-findings list from rounds 1–7 (nine items, plus the round-14 pool-
`nextIndex` item) was included in the prompt; Codex did not re-raise or dispute any of them.

All three findings verified directly against source:

- **#1 (requirement 5's exception wording ["a tick whose own required storage exceeds the
  budget"] is narrower than §8's actual mechanism [the *minimum window's total* required storage
  exceeding the budget, even with no single tick over]), CONFIRMED, applied.** Re-read requirement
  5 against §8's exception text (added by round 2, refined by round 3's requirement-5 sync fix,
  refined again by round 14): §8 explicitly sums "every tick's journal segment plus that one
  required image" across the whole minimum window, not any single tick in isolation — a case
  requirement 5's narrower wording doesn't cover (e.g. two ticks each individually well under
  budget whose journals plus the window's one mandatory image together exceed it). This is the
  same class of drift round 3 already fixed once for this exact pair of sections, recurring after
  §8 was edited again (round 14) without a matching pass over requirement 5. Fixed by widening
  requirement 5's wording to match §8's actual, whole-window scope.

- **#2 (`bodyMoveIndex` is listed as part of the imaged `bodies[id]` hot state in §5.1, but §9
  step 5 unconditionally resets every restored body's `bodyMoveIndex` to none immediately after
  the image copy already restored it — so a literal hash of body records taken right after
  `Rewind(T)` cannot equal the hash recorded at the original end of step T for any body that moved
  during step T), CONFIRMED, applied.** Confirmed `bodyMoveIndex` is a `b3Body` field (`body.h`)
  set to a real event-array index during finalize for every moving awake body (`solver.c`) and
  read back, then cleared, when that event is consumed (`solver_set.c`) — genuinely part of the
  imaged record, not a separate structure. Confirmed §9 step 5's reset applies to "every restored
  body," unconditionally, after step 3's image copy already wrote back T's real captured value —
  the reset silently overwrites what the image just correctly restored. Since move events
  regenerate deterministically on replay (§10), and the field's only purpose is indexing into a
  per-step, already-cleared event array, its post-restore value is legitimately scratch, not
  restore-stable simulation state, and the hash oracle needs to say so explicitly rather than
  leave it implied by "bodies" being hashed wholesale. Fixed §12 point 1 to name the exclusion.

- **#3 (Phase 0's borrowed serializer clears body/shape/joint `userData` on restore, which
  §10/§13 don't mention, and which can propagate into caller-visible regenerated events, not just
  direct getter calls), CONFIRMED, applied.** Confirmed `b3SerializeWorld`/
  `b3DeserializeIntoShell` (`world_snapshot.c`) explicitly zero body, shape, and joint `userData`
  on deserialize (commented "host-owned; do not free" / "is host wiring, zero it on the copy") —
  a deliberate, pre-existing property of that serializer, not a design defect in it. Checked
  whether this is merely a same-class non-issue as the already-declined world-level `userData`
  (round 3's decline, corrected for its "no setter" wording in round 14): traced further and found
  `solver.c` copies `body->userData` into `b3BodyMoveEvent.userData` and `joint->userData` into
  the analogous joint-event field during finalize — meaning, unlike the world-level pointer, these
  per-object `userData` values flow into engine-*generated*, caller-visible event payloads that
  §10 already promises are "regenerated during replay exactly as originally." If Phase 0 clears
  the source field before a subsequent step's finalize runs, that promise silently breaks for
  event `userData` specifically, for Phase 0 only (Phase 1's own per-body/shape/joint image and
  journal entries carry the whole record, `userData` included, so this doesn't recur there). Fixed
  §13's Phase 0 bullet to state the limitation and the caller's required workaround.

## Status

Round 16 of an ongoing series (rounds 1–15 committed). All three findings genuine and applied, none
declined. Finding #1 is the second time the requirement-5/§8 pair has drifted out of sync after one
side was edited without a pass over the other (round 3 fixed the same pair once already, for a
different mismatch) — this specific pair of sections appears prone to drifting whenever §8 alone
gets touched by a later round's fix, worth a standing note to double check requirement 5 any time
§8 changes again. Finding #2 is a new category for this series: not a missing journal/image write
(the recurring defect shape rounds 5, 6, 8, 9, 10, 11, 13, 14 have found), but the hash oracle
itself disagreeing with the restore algorithm's own, correct behavior on a field that restore
deliberately treats as scratch — a reminder that §12's hash needs to be audited against §9's
restore steps specifically, not only against §5.2's journal inventory (which round 14's cold-hash-
guard fix already tied together). Finding #3 continues this round's theme of catching gaps in
peripheral/prototype text (§13's Phase 0 description) that a reviewer focused on the core
mechanism might skip. Finding rate (7, 4, 3 applied across rounds 14–16) is trending down but each
round has still found genuine, source-confirmed, previously-unnoticed issues with zero stale/
duplicate noise this round — not yet a clean `CONVERGED` round. Another full-scope round is
warranted.
