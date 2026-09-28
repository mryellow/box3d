---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 34), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 34

## Review report (Codex final message)

## Summary

The hot-image and cold-journal design is broadly consistent with the source. I found one gap that can restore the wrong broad-phase proxy category and change later query or collision results. This was a source-only review; I did not run tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | 5.2, 7.4 | The proxy-category journal covers `b3ResetProxy` through `b3Shape_SetFilter(..., true)`, but geometry setters also call `b3ResetProxy` with `destroyProxy=true`. A prior `b3Shape_SetFilter(..., false)` can leave the live proxy's category different from `shape->filter.categoryBits`; a later geometry setter then recreates the proxy using the filter value. Without a category entry, rewinding across that setter restores the wrong proxy category. See `shape.c` and `dynamic_tree.c`. | Journal proxy-category changes at **every** proxy creation or recreation, including geometry setters, and test this sequence. |

## Checked, no change

- The existing backward-scrub test compares a hash of transforms and velocities; section 12 correctly calls for broader coverage (`test/test_recording.c`, `recording.c`).
- A zero-time-step call still updates pairs and sensors while skipping the solver, supporting section 8's separate `historyTick` (`physics_world.c`).
- Sensor overlap changes are signaled through `eventBits`, as section 5.2 describes (`sensor.c`).
- Broad-phase pair keys are sorted before contact creation, supporting the tree reconstruction rationale in section 7.4 (`broad_phase.c`).
- The existing serializer clears body, shape, and joint `userData`, matching the Phase 0 limitation in section 13 (`world_snapshot.c`).

## Proposed edits

1. In section 5.2, replace the restricted `b3ResetProxy` category rule with a rule covering every path that recreates a proxy. In section 7.4, specify how that category entry supplies the value during restore. Add a section 12 test: set a filter with `invokeContacts=false`, capture, change geometry, capture, then rewind and verify the proxy category and state hash at both ticks.

## Unresolved / disagreements

None beyond the design decisions already listed in section 16.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T08:43:06+10:00, end
2026-09-29T08:46:52+10:00, `EXIT_CODE 0`, returned inline in about 3 minutes 46 seconds. The raw log
had the final `## Review report` block duplicated once (a known `codex exec` streaming artifact,
identical content both times); one copy is kept above. Codex ran read-only; the raw log has no
write/patch attempt of any kind, and the only working-tree changes present are this session's own
round 33 edits plus this round's own post-review edits accounted for below — confirming Codex made
no file changes itself.

**Declined-findings list used for the prompt.** Unchanged from round 33 (round 33 found 0 new
declines — its one finding was applied): §11.1's citation into `docs/designs/reviews/`, §7.4's CCD
"Change:" paragraph's two equivalent phrasings, and §12's unqualified "pool state" wording already
covering `nextIndex`. Codex did not re-raise or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Checked whether §5.2's
`shapes[id].aabb`/`fatAABBs` row's cited functions (`b3ResetProxy`, `b3CreateShapeProxy`,
`b3Body_Enable`, `b3Body_SetType`, `b3WakeSolverSet`, `b3TrySleepIsland`) exist under those exact
names in source, given round 32's finding that a different row (non-awake `bodySims`) had named a
nonexistent function (`SetFixedRotation`) in an earlier round's citation. Grepped each name against
`src/*.c`/`src/*.h`: all six exist with the cited signatures. No gap found from this check; it did
not anticipate this round's actual finding (a missing function in a *different*, sibling row's list
— the categoryBits row, not the aabb/fatAABBs row checked here).

The finding verified directly against source:

- **(§5.2's tree proxy `categoryBits` row claims to cover "every site that creates a fresh proxy,"
  and lists `b3CreateShapeProxy`'s three call sites plus `b3ResetProxy` "when called with
  `invokeContacts=true`," but the geometry setters `b3Shape_SetSphere`/`SetCapsule`/`SetHull`/
  `SetMesh` also call `b3ResetProxy` with `destroyProxy=true` — unconditionally, with no
  `invokeContacts` parameter at all — creating a fresh proxy from `shape->filter.categoryBits` at
  that moment, a site the row's stated function list excludes even though its own "every site"
  claim requires it), CONFIRMED, applied.** Read `b3ResetProxy` (`shape.c`) in full: when
  `destroyProxy` is true it unconditionally destroys the live proxy and creates a new one via
  `b3BroadPhase_CreateProxy( ..., shape->filter.categoryBits, ... )` — the same fresh-category
  assignment the categoryBits row already attributes to every other fresh-proxy site. Traced all
  five `b3ResetProxy` call sites in `shape.c`: `b3Shape_SetFilter` (conditional `destroyProxy`,
  already named) and `b3Shape_SetSphere`/`SetCapsule`/`SetHull`/`SetMesh` (each hardcodes
  `destroyProxy = true`, none named). Cross-checked against §5.2's separate "tree proxy reset"
  row, which already lists these same four geometry setters under "other shape setters that
  recreate the proxy" — so the reset-entry journal (§5.2's other row) already fires correctly for
  this sequence, but the categoryBits-entry journal (this row) does not, and §7.4's restore logic
  for "a shape with a journaled proxy-reset entry" explicitly reads "T's journaled category bits"
  to recreate the proxy — a value this gap leaves unjournaled (or stale from an earlier event) for
  exactly the sequence Codex describes: an `invokeContacts=false` `SetFilter` diverges the live
  proxy's category from `shape->filter.categoryBits`, then a geometry setter's `b3ResetProxy`
  resets the proxy fresh from the (now different) filter value with no categoryBits entry recording
  it, so a rewind into that window has no journaled value to restore the reset's true post-event
  category from. Fixed by adding the four geometry setters to the categoryBits row's function list,
  matching the same wording the "tree proxy reset" row already uses for them, so both rows'
  coverage is aligned. Declined Codex's second proposed action (a named §12 test case): as with
  round 32's declined test-list addition, the actual defect was the missing inventory entry, and
  §7.4's restore logic already states it reads "T's journaled category bits" for a reset entry — an
  implementer or the cold-hash guard following the corrected §5.2 row now has the write site to
  cover, without needing a specific scripted repro named in the requirements doc itself.

## Status

Round 34 of an ongoing series (rounds 1–33 committed). One Medium finding, genuine, applied. Same
category as rounds 32–33 (a missing function in a §5.2 inventory row, not a structural or
bit-exactness defect in the design's own logic) but this is the first round to find the gap by
comparing *two sibling rows describing the same underlying event* (categoryBits vs. proxy reset)
rather than comparing a row against source directly — the "tree proxy reset" row already had the
correct, broader function list, and the categoryBits row's own narrower list was the sole defect;
the pre-prompt pre-check this round (verifying function names in a different row) did not happen to
target this specific cross-row comparison, so the gap was Codex's own find, not preempted by the
pre-check. Not a consequence of round 33's own fix (round 33 touched §12/§13's Phase 0 userData
caveat, disjoint from §5.2/§7.4) and does not reopen ground the doc's own rationale already covered.

Per the user's instruction, this batch is capped at 2 rounds (34–35) or sooner on `CONVERGED`. This
round's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, so round 35 follows next, at the same full
scope, per WORKFLOW.md step 8.

Round 34: 1 finding (0H-1M-0L), 1 applied, 0 declined
Series total: 115 findings (42H-56M-17L) across 34 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L)
