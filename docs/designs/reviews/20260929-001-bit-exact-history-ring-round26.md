---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 26), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 26

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the current source tree, and the document handles several important restore boundaries explicitly. Two details need specification before the API and journal can deliver the stated rewind and forward scrub behavior.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §6, §8, §14 | History has no defined initial image. Capture begins after a `b3World_Step`, so enabling history on an existing world leaves its enable-time state without an imaged tick. That prevents a correction back to that state, including tick zero when history is enabled before the first step. The world already has meaningful state before stepping (`physics_world.c`). | Capture an image when history is enabled, assign it a tick, and specify how calls before the first subsequent step enter the open journal segment. |
| 2 | Medium | §7.1, §7.3 | Entries require **old and new** bytes for redo, but the stated hook is a call *before* each write. That call alone cannot obtain the resulting bytes. Mutators can make several writes during one operation, as in `b3Shape_SetFilter` (`shape.c`), so the timing matters to forward application. | Specify paired before/after hooks, or a per-record coalescing protocol that captures final new bytes before closing the segment and preserves the required entry order. |

## Checked, no change

- Awake contact records and manifolds need gathering from id-addressed storage; they are not in a contiguous contact-sim array (`contact.h`, `constraint_graph.h`).
- Sensor overlap results are sorted and deduplicated, and the sensor task marks changed overlap lists independently of the owning body's awake state (`sensor.c`).
- Static body contact-list heads and neighboring contact edges can change during contact creation and destruction, as §5.2 records (`contact.c`).
- Tree moved flags are propagated to ancestors and can be cleared by traversing marked branches (`dynamic_tree.c`).
- The documented `b3Shape_SetFilter` path can recreate a proxy, and a filter change without contact invocation can leave the live proxy's category bits different from the shape filter (`shape.c`).

## Proposed edits

- **§6 and §14:** Define the enable-time image and its tick in the API contract.
- **§7.1 and §7.3:** State precisely when each redo payload is captured, including repeated writes to one record in a segment.
- **§13:** Clarify how Phase 0 tests perform the documented `userData` restoration before pointer-equality checks.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-29T06:34:57+10:00, end 06:38:29+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 4 minutes. Raw log had the final message duplicated (a
`codex exec` streaming artifact); one copy is kept above. Codex ran read-only; the working tree was
untouched by it going in (round 25's edits were already applied to the working file, confirmed via
`git status` before the call). The declined-findings list carried into this round (nine items: the
eight used in rounds 24–25, plus a ninth pre-emptive context item describing round 25's own
resolution — the hash's exclusion of `userData` is intentional and already compensated by a
separate same-process pointer check, not an oversight a fresh reviewer should re-flag) was included
in the prompt; Codex did not re-raise or dispute any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, round 25's own §12 edit
(the same-process `userData` pointer-equality checks added to tests 2 and 3) was re-read for
internal consistency against §13's Phase 0 description, since round 25's own Status section flagged
this as the most likely next place a propagation gap could hide. Traced §13's existing Phase 0
paragraph: it already states, in full, that a caller relying on `userData` inside a callback "must
restore them itself immediately after a Phase 0 restore" — meaning a test running tests 2/3 against
Phase 0 specifically needs the caller to have already done that restoration before the new
`userData` pointer-equality check would pass, which §13 already documents as Phase 0's known
limitation (distinct from Phase 1, whose per-record image/journal entries "carry these fields
naturally"). Confirmed this is already correctly stated, not a new gap; no edit made ahead of
Codex's own pass. (Codex's own "Proposed edits" list independently suggested the same §13
clarification as its third bullet — checked below alongside the two formal findings, since it
wasn't promoted to the Findings table but is the same territory this self-check had just covered.)

Both findings verified directly against source or by internal-consistency tracing (this design has
no implementation yet — Phase 1's journal/ring code doesn't exist in `src/` — so verification here
is against the document's own completeness and self-consistency, the same standard round 1 applied
to gaps in an unimplemented mechanism):

- **#1 (the document never specifies an image for the tick at which history is enabled, so a world
  with pre-existing state when `b3World_EnableHistory` is called — the ordinary case, since a game
  typically loads a level, creates bodies, then enables history — has no restorable image of that
  state until the first subsequent `b3World_Step` completes), CONFIRMED, applied.** Confirmed the
  document's only stated capture trigger is "at the end of every `b3World_Step`" (§6) and that
  `historyTick` (§8) is described only in terms of what *capture* (i.e., step-end) does to it, never
  what `EnableHistory` itself does. Confirmed the world already carries meaningful state before any
  step — bodies/shapes/joints can be created on a freshly-created world, and `stepIndex` (§5.1) is
  already part of "world scalars," implying a well-defined tick value (0) exists even pre-step.
  Traced the practical consequence: a game that creates its level, enables history, then steps —
  intending "rewind to right after load" as a safety net — would find that state unreachable,
  since the *only* image ever captured is on or after the first step, not at enable time itself.
  Fixed by adding one paragraph to §8 (immediately after `historyTick`'s existing definition, which
  it directly extends): `b3World_EnableHistory` captures an image at the world's current
  `stepIndex` and sets `historyTick` to it, using the same capture procedure §6 already defines for
  a step boundary, just run once at enable time instead of triggered by a step; API calls made
  before the first subsequent step enter that tick's still-open journal segment, reusing §9's
  already-existing open-segment mechanism (round 1's fix) rather than inventing a new one.

- **#2 (the journal's stated hook timing — "before the write" — only supports capturing a record's
  *old* bytes at that call site; nothing in §7.1 or §7.3 says how the *new* bytes a redo entry also
  needs ever get captured, and this matters more than a single-write case might suggest once a
  mutator makes more than one write to the same record), CONFIRMED, applied.** Read
  `b3Shape_SetFilter` (`shape.c`) in full as the cited example: it makes exactly one write to
  `shape->filter` (a whole-struct assignment), not several — so this specific function doesn't
  itself demonstrate a *multi-write* case, but it does cleanly demonstrate the *general* problem
  Codex's finding actually rests on: even for this single write, a hook literally called "before
  the write" has no way to know the write's outcome, since the new value only exists in memory
  *after* the assignment executes. Confirmed no existing section pairs the before-write hook with
  any after-write counterpart, and no section says redo bytes are captured by any other means. This
  is a genuine specification gap in the journal mechanism's own description, independent of whether
  any single site happens to write once or many times. Chose the coalescing option over Codex's
  paired-hook alternative: a paired before/after hook would double the instrumentation at every one
  of §7.3's "audit surface" sites, while this design's journal is already segment-scoped (one
  segment closes and is fully finalized at each step's end, per §6 step 4) — so old bytes can be
  captured once, on a record's first touch in the open segment (a natural place to also detect and
  skip a redundant second hook call for the same record in the same tick, which a multi-write
  mutator like a hypothetical future `SetFilter` variant could trigger), and new bytes captured
  once per touched record when the segment closes, by re-reading each one's then-current live
  value — correct regardless of how many intermediate writes happened, and adding no new hook call
  sites beyond the ones §7.3 already specifies. Fixed by adding this coalescing rule to §7.3,
  immediately after the existing hook-site description it directly qualifies.

- **(Codex's own third proposed edit, clarifying §13's Phase 0 userData restoration before the
  round 25 pointer-equality checks), checked, no change — already correctly stated.** As traced in
  the pre-prompt self-check above: §13's existing Phase 0 paragraph already states the caller must
  restore `userData` itself immediately after a Phase 0 restore, which is the exact precondition
  round 25's new same-process checks need to pass against Phase 0. Not applied; no doc change
  needed beyond what round 25 and this round's own findings already cover.

## Status

Round 26 of an ongoing series (rounds 1–25 committed, most recently rounds 24–25 pending commit
alongside this round). Two findings, both genuine and applied — neither in territory the
immediately preceding rounds (24, 25) had just edited, unlike the tight one-round lag rounds
21–23 and 25 each showed; these are both in mechanisms (initial capture at enable time, redo-byte
capture timing) that no round in this series had examined closely before, despite both being
present in the document since round 1 and touched only tangentially by later rounds' edits to
adjacent text (§8's `historyTick` paragraph, §7.3's hook-site paragraph) without anyone tracing
either one's own internal completeness. This is the third and final round of the
currently-requested three-round batch (rounds 24–26); per that instruction, stopping here. No
`CONVERGED` verdict — this round found two Medium-severity, source/self-consistency-confirmed gaps
in previously-unexamined mechanism, the same signal every prior round in this series has shown:
another full-scope round is warranted whenever review next resumes, not a narrower one.
