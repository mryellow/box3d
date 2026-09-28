---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 33), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 33

## Review report (Codex final message)

## Summary

The hot-image and cold-journal design is broadly consistent with the source and the cited test behavior. I found one Phase 0 mismatch: its proposed restore tests require a world user-data value that the existing serializer does not restore.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | 12, 13 | Section 12 compares `world->userData` with its value at tick T, and section 13 says Phase 0 can run those tests. `b3SerializeWorld` does not serialize that field, so `b3DeserializeIntoShell` leaves the shell's current value in place. Section 13's pointer-restoration caveat names only body, shape, and joint user data. (`world_snapshot.c`, `physics_world.c`) | Add world user data to the Phase 0 caller-restoration contract, or make the prototype snapshot and restore it. State how the Phase 0 tests supply its T-time value. |

## Checked, no change

- The existing state hash covers live body transforms and velocities, matching the document's limited claim for the recording test; it is not the proposed full-state oracle. (`recording.c`, `test_recording.c`)
- A zero-time-step call still runs pair and sensor updates while skipping the solver, supporting the separate `historyTick` rule in section 8. (`physics_world.c`)
- The sensor task detects changes to its own overlap set, supporting the section 5.2 journal trigger. (`sensor.c`)
- Pair keys are sorted before contacts are created, supporting the tree-layout claim for pair discovery. (`broad_phase.c`)
- The documented closest-ray tie gap for mesh and height-field shapes is acknowledged in sections 7.4 and 10.

## Proposed edits

1. In section 13, extend the Phase 0 caveat to `world->userData`. Specify either that the caller saves and restores its tick-T value before the section 12 pointer checks, or that the Phase 0 wrapper stores it alongside each serialized image.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T08:36:36+10:00, end
2026-09-29T08:40:21+10:00, `EXIT_CODE 0`, returned inline in about 3 minutes 45 seconds. The raw log
had the final `## Review report` block duplicated once (a known `codex exec` streaming artifact,
identical content both times); one copy is kept above. Codex ran read-only; the raw log has no
write/patch attempt of any kind, and the only working-tree changes present are this session's own
round 32 edits plus this round's own post-review edits accounted for below — confirming Codex made
no file changes itself.

**Declined-findings list used for the prompt.** Unchanged from round 32 (round 32 found 0 new
declines — both its findings were applied): §11.1's citation into `docs/designs/reviews/`, §7.4's
CCD "Change:" paragraph's two equivalent phrasings, and §12's unqualified "pool state" wording
already covering `nextIndex`. Codex did not re-raise or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Round 32 found that §5.2's
non-awake `bodySims`/`bodyStates` row omitted the mass-recomputation write path
(`b3UpdateBodyMassData`). Checked whether the same class of omission — an internal helper that
writes a cold/non-awake field from multiple call sites the row doesn't enumerate — exists for any
other §5.2 row's helper functions. `b3SyncBodyFlags` (called from most of §5.2's listed setters) only
touches `body->flags`, already covered generically by every row that names its caller; `b3RefreshBodyContactIndices`
(already named explicitly in the contacts row) and `b3LinkJoint`/`b3UnlinkJoint`-style island helpers
are already named at their own call sites in the islands and contacts rows. No second instance of
the round-32 pattern found; no further doc change needed from this check.

The single finding verified directly against source:

- **(§12 test 2 requires `b3World_GetUserData` to match its value recorded at T after a same-process
  restore, and §13 says Phase 0 — built on the existing `b3SerializeWorld`/`b3DeserializeIntoShell`
  — can run tests 2, 3 and 5, but §13's own Phase 0 caveat named only body/shape/joint `userData` as
  needing caller-side restoration, not the world's own field), CONFIRMED, applied.** Read
  `b3SerWorldConfig`/`b3DesWorldConfig` (`world_snapshot.c`) in full: neither reads nor writes
  `world->userData` anywhere — unlike the body/shape/joint sparse-array loops a few lines below in
  `b3SerializeWorld`, which explicitly zero `elem.userData` in the serialized copy and (on the
  deserialize side, `b3DeserializeIntoShell`) explicitly null the live record's field, the world
  scalar path never touches `world->userData` at all, in either direction. So a Phase 0 restore
  doesn't clear the world's `userData` the way it clears body/shape/joint `userData` — it simply
  leaves the live shell's current value untouched, which is a different failure shape than the
  documented one (silently stale rather than reliably `NULL`), and either way isn't restored to T's
  value, so a caller running §12 test 2's world-`userData` comparison through a Phase 0 restore needs
  to save and reapply it itself, exactly like the three fields the caveat already named. Fixed by
  adding the world's own `userData` to §13's Phase 0 caveat, describing its distinct behavior
  (untouched, not nulled) accurately rather than folding it into the "clears" wording that only
  correctly describes the other three, and naming §12 test 2 as the caller obligation this creates —
  since Phase 1's own world-scalar image (§5.1, which already lists `userData` as one of the imaged
  scalar fields) carries it naturally, matching the existing "Phase 0-only" framing for the other
  three fields.

## Status

Round 33 of an ongoing series (rounds 1–32 committed or pending commit). One Medium finding, genuine,
applied. This continues the pattern of rounds 24–32: a missing-caveat or missing-inventory-entry
class of gap rather than a structural or bit-exactness defect, and it is the same *category* of gap
round 32 found (an internal helper/path not restored by the Phase 0 prototype, left off a caveat
list) but in a different field (`world->userData` vs. body-owned `bodySim`) and a different section
pairing (§12/§13 vs. §5.2/§7.3/§12) — not a consequence of round 32's own fix, since round 32 touched
§5.2's non-awake sim row and §8/§10's budget wording, disjoint from §12/§13. It does not reopen ground
the doc's own rationale already covered; this is the first round to examine the Phase 0 caveat's
completeness against §12's own test list specifically. The pre-prompt pre-check (checking for a
second instance of round 32's own omission pattern elsewhere in §5.2) found none, suggesting that
particular gap-shape was localized to the one row round 32 already fixed, not systemic across the
inventory.

Per the user's instruction, this batch was capped at 2 rounds (32–33) or sooner on `CONVERGED`.
Round 33's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, and the batch limit is now reached, so
this session stops here per that instruction; the series remains open, and whenever review next
resumes it should continue at the same full scope as every round so far, not a narrower one.

Round 33: 1 finding (0H-1M-0L), 1 applied, 0 declined
Series total: 114 findings (42H-55M-17L) across 33 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L)
