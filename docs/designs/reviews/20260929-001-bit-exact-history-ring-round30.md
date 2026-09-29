---

title: Codex review — bit-exact history ring, round 30
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 30

## Review report (Codex final message)

## Summary

The recommendation is sound in broad outline. The existing recording path supports the proposed starting point, and the document correctly treats its current transform and velocity hash as narrower than the bit-exact claim. I found three specification changes to make before convergence. This was a read-only review; I did not remeasure the supplied §11.1 figures.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §7.4, §12.7 | The proposed CCD sensor cap selects the first eight hits “by shape id,” but [the CCD context](src/solver.c:313) collects `(sensorId, visitorId)` hits across all shapes on a fast body. Two hits can share a sensor id, leaving the selection rule incomplete. | Define a total ordering over hit pairs, specify how fraction ranks against ids, and test a multishape fast body with tied sensor ids and more than eight candidates. |
| 2 | Medium | §5.2, §6, §7.3 | The compile-enforced write boundary does not specify how the step mutates live sensor elements. [The sensor pass](src/sensor.c:185) swaps and writes their inner arrays every tick, including for sensors on sleeping or static bodies; the described hot accessor covers awake-set elements. | Define a sensor hot write path for the step, with its access and journaling rules, so the const-qualified container boundary remains enforceable. |
| 3 | Low | §14 | The API comment says the first **journaled** call after rewind discards future history. [§8](docs/designs/20260927-002-bit-exact-history-ring.md:559) and §12.6 say an awake-body write discards it even when that write makes no journal entry. | Change the API comment to “the first mutating call or step.” |

## Checked, no change

- [Recording serialization and backward seek](src/world_snapshot.c:1016) provide the claimed phase-0 basis; [ScrubBackward](test/test_recording.c:251) checks replayed hashes. The existing hash covers transforms and velocities, so the proposed broader hash is necessary.
- [The FAQ](docs/faq.md:136), [simulation guide](docs/simulation.md:1996), determinism tests, and compiler flags support the document’s qualified determinism discussion.
- The source supports the separate CCD solid-hit, CCD sensor-hit, and explosion traversal-order concerns.
- The document accounts for non-awake record writes, heap-owned contact and material data, hull ownership, proxy identity, moved bits, pending journal entries, and events dropped on restore.
- The phase-1 dynamic and kinematic tree cost exception is stated explicitly and gated on phase 2.

## Proposed edits

Specify the CCD hit key and cap policy, add the sensor write accessor to the state inventory and phase-1 migration, and correct the API truncation comment.

## Unresolved / disagreements

The document already identifies owner decisions on the simulation-order changes and public hash API. I found no additional disagreement with the whole-world approach.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:49:38+10:00, ended 14:55:54+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 29, no declined-findings list. The final
message above is unchanged apart from absolute path prefixes on its links.

**Findings verified against the doc and source.**

1. **Applied.** `b3ContinuousContext` (`solver.c`) holds one `sensorHits` array of
   `(sensorId, visitorId)` pairs, capped at eight, filled across every shape of the fast body, so two
   hits can share a sensor id and "by shape id" was not a total order. §7.4 now orders by the
   (sensor shape id, visitor shape id) pair, and §12 test 7 adds a multishape fast body whose shapes
   hit the same sensor.
2. **Applied.** The sensor pass (`sensor.c`) writes `world->sensors` elements every step, including
   sensors on sleeping and static bodies, and §5.2's hot accessor was defined for awake-set elements
   only, so those writes had no legal path under the const-qualified container boundary. The
   accessor now covers them, and the doc states why they are unjournaled: `overlaps2` is imaged and
   `hits` and `overlaps1` are scratch.
3. **Applied.** §8 truncates on the first write through any write accessor or journaled container,
   journaled or not; §14's comment said "journaled call". It now says "mutating call".

3 applied, 0 declined.

## Status

Finding count is 3 (0H-2M-1L) for the second round running, with no High. The findings are an
under-specified ordering key, a missing access path for one hot writer, and a comment out of step
with §8; all in text that predates round 29, none a consequence of round 29's fixes, and none
reopened ground the doc's rationale covered. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3,
4, 3, 3, 3) remain flat at 2 to 4, so the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 30: 3 findings (0H-2M-1L), 3 applied, 0 declined`

`Series total: 141 findings (46H-79M-16L) across 30 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L)`
