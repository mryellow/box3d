---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 25), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 25

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the current source tree. I found one verification gap: the proposed full-state hash excludes user data pointers that callbacks can read, so equality of that hash alone cannot establish the stated replay guarantee.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12, §13 | The "full state hash" excludes body, shape, joint, and world `userData`, while §12 calls it the oracle for restore and replay exactness. Those values are observable through APIs such as `b3Body_GetUserData` and `b3Shape_GetUserData` and can affect a pure callback's result. The pointers are captured by the proposed image or journal, but a missed restoration would pass the hash tests. Phase 0's documented clearing of these pointers demonstrates the practical distinction (`body.c`, `shape.c`, `physics_world.c`, `world_snapshot.c`). | Keep the portable hash pointer-free, but add same-process pointer equality checks for every retained `userData` field to restore and replay tests. Describe the hash as an oracle for the state it covers, rather than for every simulation-affecting value. |

## Checked, no change

- The current serializer and recording tests support the document's narrower claim about replaying the state covered by the existing hash (`world_snapshot.c`, `recording_replay.c`, `test/test_recording.c`).
- Sensor overlap comparison uses the sensor task's own change bit, as §5.2 describes (`sensor.c`).
- The tree proxy reset and category-bit cases in §5.2 and §7.4 correspond to separate live proxy behavior (`shape.c`, `broad_phase.c`, `dynamic_tree.c`).
- The CCD candidate fraction, sensor-hit cap, and closest-ray tie behavior identified in §7.4 are present in the current paths (`solver.c`, `physics_world.c`).

## Proposed edits

- **§12:** Add a same-process test assertion comparing restored and replayed `userData` pointers for all live and retained slots, including world `userData`. State explicitly that the portable hash excludes these values and cannot alone certify callback-relevant state.
- **§13:** Make the Phase 0 test plan require the documented caller restoration of user data before comparing replay behavior.

## Unresolved / disagreements

None beyond the verification gap above.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-29T06:30:31+10:00, end 06:33:01+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 3 minutes. Raw log had the final message duplicated (a
`codex exec` streaming artifact); one copy is kept above. Codex ran read-only; the working tree was
untouched by it going in (round 24's edits were already applied to the working file, confirmed via
`git status` before the call). The same eight-item curated declined-findings list used for round 24
was included in the prompt; Codex did not re-raise or dispute any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, round 24's own three
edits (the §10 closest-ray caller-contract addition, the §9 geometry-setter debug-handle
invalidation, and the §12 body/shape/joint `userData` hash-exclusion clause) were re-read for any
propagation gap of the same shape round 24 itself was built from. Found exactly the gap this
round's own finding independently confirms: round 24's new §12 exclusion clause states *what* is
excluded from the hash but says nothing about *how* — or whether — the excluded fields are still
verified, leaving requirement 7's "verified by test, not by audit" promise unfulfilled for exactly
the fields the previous round just carved a named exception for. This should have been caught in
round 24's own self-check rather than left for a fresh round to independently rediscover; noting
this here rather than silently treating it as a new, unrelated finding.

The finding verified directly against source:

- **(the full-state hash's `userData` exclusion, correct on its own portability/opacity grounds,
  leaves `userData` restoration unverified by any test, even though it is observable through public
  getters and can be read inside a caller's own supposedly-pure callback), CONFIRMED, applied.**
  Confirmed all four getters exist and return the stored pointer directly: `b3Body_GetUserData`
  (`body.c`), `b3Shape_GetUserData` (`shape.c`), `b3Joint_GetUserData` (`joint.c`),
  `b3World_GetUserData` (`physics_world.c`) — a caller's `preSolve`/custom-filter/friction/
  restitution callback, or ordinary game code driven by a cast/overlap/mover result, can call any of
  these on a body/shape/joint id it already has and branch on the result, meaning a `userData`
  restoration bug (e.g. a future refactor of the generic record-copy path that drops the field)
  would silently violate §10's callback-purity contract without any of §12's stated tests — hash
  comparisons in tests 2 and 3 — ever failing, since round 24 explicitly removed `userData` from
  what those hashes cover. Confirmed the design does still commit to restoring these values (the
  generic whole-record image/journal mechanism for bodies/shapes/joints carries `userData` along
  as an ordinary field, and round 21 specifically added world `userData` to the imaged scalars for
  the same reason) — so this is a real verification-completeness gap for requirement 7, not a
  question of whether the value should be restored at all (settled: yes, it should, and mostly
  already is by construction). Fixed by adding same-process `userData` pointer-equality checks to
  both test 2 (restore exactness) and test 3 (replay exactness), stated as supplementary to the
  hash comparison rather than folded into it — preserving the hash's own portability property (round
  21's and round 24's reason for excluding `userData` from it) while still giving requirement 7's
  "a missed write site fails a test rather than a review" promise something to catch a `userData`
  regression with. Declined the narrower half of Codex's own proposed action — reframing §12's
  opening sentence to describe the hash as covering only "the state it covers" rather than "every
  simulation-affecting value" — as unnecessary: `userData` was never simulation-affecting by this
  document's own requirement-1 definition (established across rounds 3, 14, and 21), so the hash's
  existing framing was already accurate; the actual gap was the missing supplementary test, not an
  overclaim in the hash's own description.

## Status

Round 25 of an ongoing series (rounds 1–24 committed, most recently round 24 pending commit
alongside this round). One finding, genuine, applied — and, per the pre-prompt self-check above, a
direct, source-confirmed consequence of round 24's own `userData`-exclusion fix: the second
consecutive round to find a gap in the *immediately preceding* round's own edit (round 24 itself
found gaps in rounds 21–23's territory, though not in the *immediately* preceding round specifically
— this is a tighter one-round lag). This is a smaller, narrower kind of consequence than the
mechanism-level bugs rounds 21–23 kept finding in §7.4 (this one is "a correct fix left its own
new promise unverified," not "a correct-looking fix was itself wrong") — the actual `userData`
exclusion decision, re-checked here, holds up. This is the second round of the currently-requested
three-round batch (rounds 24–26); one more full-scope round follows before stopping per that
instruction, unless it lands on `CONVERGED` first.
