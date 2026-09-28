---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 32), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 32

## Review report (Codex final message)

## Summary

The design's main approach is consistent with the source, and the existing recording test supports its stated, narrower baseline claim. I found one omission in the enumerated cold write sites and one internal contradiction about the memory budget.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | 5.2, 7.3, 12 | The non-awake `bodySims` write-site row omits mass recomputation. `b3Shape_SetDensity(..., updateBodyMass=true)` and `b3Body_ApplyMassFromShapes` call `b3UpdateBodyMassData`, which writes the owning body's sim without waking it (`shape.c`, `body.c`). The general rule in section 7.1 appears broad enough to require journaling, but the document says section 5.2 enumerates every write site; an implementation following that inventory could miss these calls. | Name both entry points and require a non-awake sim journal hook in the common mass-update path. Test both calls on a sleeping body. |
| 2 | Low | 8, 10 | Section 8 counts the arena, journal-owned arrays, and hull data retained solely by journal references against `maxBytes`. Section 10 says `maxBytes` "caps only the ring arena." These budget definitions conflict. | Change section 10 to say the name cache is excluded from the section 8 budget, without describing that budget as arena-only. |

## Checked, no change

- `ScrubBackward` compares hashes after backward seeks, while `b3HashWorldState` has narrower coverage than the proposed full-state hash (`test/test_recording.c`, `recording.c`).
- The sensor task sets `eventBits` when its sorted overlap list changes, supporting the proposed overlap journal trigger (`sensor.c`).
- A live proxy's category bits can differ from its shape filter, so restoring proxy category bits separately is justified (`shape.c`, `dynamic_tree.c`).
- The id pool uses a free list and bump index; restoring pool state is necessary for repeatable allocation (`id_pool.c`).

## Proposed edits

1. Add `b3Shape_SetDensity` and `b3Body_ApplyMassFromShapes` to section 5.2's non-awake sim row, identify `b3UpdateBodyMassData` as the common hook point, and add sleeping-body cases to section 12.
2. Replace section 10's "caps only the ring arena" wording with the full section 8 budget definition.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T08:29:54+10:00, end
2026-09-29T08:34:10+10:00, `EXIT_CODE 0`, returned inline in about 4 minutes 16 seconds. The raw log
had the final `## Review report` block duplicated once (a known `codex exec` streaming artifact,
identical content both times); one copy is kept above. Codex ran read-only; the raw log has no
write/patch attempt of any kind, and the only working-tree changes present are this round's own
post-review edits accounted for below — confirming Codex made no file changes itself.

**Declined-findings list used for the prompt.** Unchanged from round 31 (round 31 found 0 new
declines — all three findings were applied): §11.1's citation into `docs/designs/reviews/`, §7.4's
CCD "Change:" paragraph's two equivalent phrasings, and §12's unqualified "pool state" wording
already covering `nextIndex`. Codex did not re-raise or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Checked whether §5.2's "id
pools ×6" line and §7.1's paired-sparse-array list (bodies, shapes and fatAABBs, contacts, joints,
islands, solverSets) match the actual id pools in source: grepped `physics_world.h` for `b3IdPool`
fields and found exactly six — `bodyIdPool`, `shapeIdPool`, `contactIdPool`, `jointIdPool`,
`islandIdPool`, `solverSetIdPool` — matching both the count and the paired-array list with no gap.

Both findings verified directly against source:

- **#1 (§5.2's non-awake `bodySims`/`bodyStates` row named only `b3Body_SetTransform` "and the other
  setters" plus the explosion callback, omitting the mass-recomputation path through
  `b3UpdateBodyMassData`, which writes the owning body's `bodySim` fields — `invMass`,
  `invInertiaLocal`, `invInertiaWorld`, `localCenter`, `minExtent`, `maxExtent`, `center`, `center0`
  — with no awake check anywhere in its body), CONFIRMED, applied.** Read `b3UpdateBodyMassData`
  (`body.c`) in full: it calls `b3GetBodySim` unconditionally and writes those fields regardless of
  whether the body is in the awake solver set. Traced every call site: the shape-creation family via
  `def->updateBodyMass` (`shape.c`, feeding `b3CreateShape`), `b3DestroyShape` when its
  `updateBodyMass` argument is set (`shape.c`), `b3Shape_SetDensity` when its `updateBodyMass`
  argument is set (`shape.c`), `b3Body_ApplyMassFromShapes` unconditionally (`body.c`),
  `b3Body_SetType` unconditionally (`body.c`, both the disabled-body early-return stage and the
  general path), and `b3Body_SetMotionLocks` when a fixed-rotation change is detected (`body.c` —
  Codex's finding text said "SetFixedRotation," but no such function exists; the actual call site is
  inside `b3Body_SetMotionLocks`'s `fixedRotation1 != fixedRotation2` branch, corrected here rather
  than transcribed as stated). None of these six call paths gates on the owning body's awake state,
  so each can write a sleeping body's `bodySim` through a route §5.2's row did not name — a real gap
  in the write-site inventory requirement 7 and §12 point 4's cold-hash guard depend on being
  exhaustive. Fixed by adding `b3UpdateBodyMassData` and its six call paths to §5.2's row, naming
  `b3Body_SetMotionLocks` rather than Codex's nonexistent `SetFixedRotation`. Declined Codex's second
  proposed action (add sleeping-body mass-update cases to §12 by name): §12's existing churn list
  ("creates, destroys, setters on sleeping bodies, forced sleep/wake toggles, explosions") is already
  generic enough to include a density setter or `ApplyMassFromShapes` call on a sleeping body without
  needing every individual API named — the actual defect was the missing inventory entry in §5.2, not
  a narrower test list, and §5.2's own fix now makes the write site something an implementer and the
  cold-hash guard both know to cover.
- **#2 (§10's name-cache paragraph says the cache's bytes are "not counted against `maxBytes` (§8),
  which caps only the ring arena," but §8 itself defines `maxBytes` as capping "the arena **and** the
  arrays ownership-transfer journal entries hold, plus any hull-database bytes kept alive only by a
  journal-held reference" — so §10's parenthetical mischaracterizes §8's own stated scope), CONFIRMED,
  applied.** Re-read §8's first bullet and §10's name-cache sentence side by side: §8 explicitly
  extends the budget beyond the arena's own bytes, so §10's "which caps only the ring arena" is simply
  wrong about what §8 says, independent of whether the name cache itself should be excluded (it
  should — the cache is a separate, unbounded, always-on structure per §10's own reasoning, not
  ring-owned state). Fixed by deleting the incorrect qualifying clause rather than restating §8's full
  definition a second time in §10 (the smallest edit that removes the contradiction, per this series'
  own fix-size rule — a reader who wants §8's scope reads §8).

## Status

Round 32 of an ongoing series (rounds 1–31 committed). One Medium and one Low finding, both genuine,
both applied. Both are in the same category as most recent rounds (24–31): a missing inventory
enumeration and a wording contradiction between two sections, not a structural or bit-exactness
defect — the finding rate and severity mix continues the downward trend from the series' early
rounds, though round 31 broke that trend briefly with a High-severity bit-exactness bug in the
enable-time boundary, showing the series can still surface real defects in previously-unscrutinized
corners even this late. Neither of this round's findings is a consequence of round 31's own fix
(round 31 touched §4/§8/§9's enable-time segment rule and §7.4/§12/requirement 7's proxy-reset
coverage; this round's findings are in §5.2's mass-update write sites and §8/§10's budget wording,
disjoint sections) and neither reopens ground the doc's own rationale already covered — both are new
territory. One correction of Codex's own finding text was needed (its proposed action named a
nonexistent `b3Body_SetFixedRotation`; the actual site is inside `b3Body_SetMotionLocks`), consistent
with this series' recurring need to re-verify Codex's citations against actual source symbol names
rather than transcribing them.

Per the user's instruction, this batch is capped at 2 rounds (32–33) or sooner on `CONVERGED`. This
round's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, so round 33 follows next, at the same full
scope, per WORKFLOW.md step 8.

Round 32: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 113 findings (42H-54M-17L) across 32 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L)
