---

title: Codex review — bit-exact history ring, round 17
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 17

## Review report (Codex final message)

## Summary

The hot image and cold journal recommendation is broadly consistent with the source. I found no demonstrated rollback counterexample, but the document leaves its central compile-time journal guarantee underspecified and does not explicitly test the tree-order changes on which derived trees depend.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §2.7, §5.2, §7.3 | “Verified by construction” needs a concrete module boundary. World record arrays are exposed through `physics_world.h`, and writes span files such as `solver_set.c` and the joint implementations. Accessors alone do not make a missed direct write a compile error. The document acknowledges this work but does not specify how storage becomes inaccessible to those callers. | Define the opaque storage or type boundary, including how hot solver access works, and make its completion a phase-1 gate. |
| 2 | Medium | §7.4, §12 | The tests do not explicitly exercise the order dependencies that justify deriving tree layout: competing CCD solid hits, more than eight CCD sensor hits, and an explosion hitting several shapes on one body. Those paths are present in `solver.c` and `physics_world.c`. General scene replay tests could miss them. | Add targeted restore-and-replay cases that alter tree layout and compare the full state hash and relevant events. |
| 3 | Low | §7.4 | The statement that the broad-phase wrappers are the only callers of tree proxy creation is too broad: `compound.c` also creates proxies for a separate compound tree. This does not appear to require world-history journaling. | Say explicitly that the wrappers are the creation path for the *three world broad-phase trees*. |
| 4 | Low | §8, §14 | The API comment describes `tickCount` as a maximum retained window, while §8 allows retention beyond it until the next image when the capture interval is wider. | State the possible interval-sized overrun in the API comment. |

## Checked, no change

- `test_recording.c` does verify backward seeks against recorded state hashes; `recording.c` hashes only body transforms and velocities, as the document says.
- The sensor pass swaps overlap buffers and clears hits before the step ends in `sensor.c`, supporting the proposed capture of `overlaps2`.
- The current CCD running-fraction and eight-hit behavior, explosion traversal-order effects, and tree proxy free list match the source descriptions.
- A filter update can leave proxy category bits different from the shape filter. Preserving actual proxy metadata through the proposed proxy journal is therefore the right treatment; this is **not** a finding.

## Proposed edits

Specify the storage boundary for journaled records, add the three targeted tree-order tests, and tighten the two descriptions identified above.

## Unresolved / disagreements

I did not verify §11’s benchmark measurements or its cited performance review: the requested reading boundary excludes that review. The performance acceptance targets still need execution-based validation. The design’s §16 solver and public-hash decisions remain open.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:09:21+10:00, ended 13:15:01+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 13 to 16, no declined-findings list (round 16's one
decline was answered in the doc). Before the round the round-16 edits were checked intact and
against AC-1 through AC-4 with no failure. The final message above has its source links reduced to
plain file names, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Declined; doc sharpened.** This reopens round 13's finding 2 without engaging the doc's stated
   position. §5.2 already says each journaled container and record array exposes const-qualified
   element access outside its defining translation unit, that the writable pointer exists only in
   the write accessor's file and the hot accessor, and that the one-time migration removes direct-write
   access from every other translation unit; §5.2's accessor paragraph (added after round 13) says
   accessors take completed values and never return record pointers. What the finding asks beyond
   that is a phase gate, which §13 lacked; phase 1 now says it is not complete until the direct-write
   access is removed from every other translation unit and the build fails on a remaining direct
   write.
2. **Applied.** §12 tests 2 and 3 use general benchmark scenes, and the three order changes §7.4
   depends on (`b3ContinuousQueryCallback`'s running fraction and eight-hit cap in `solver.c`, the
   explosion callback in `physics_world.c`) are reached only by specific geometry. §12 now has
   test 7: restore-and-replay cases built for competing CCD solid hits, more than eight CCD sensor
   candidates, and an explosion over several shapes of one body, comparing the full hash and each
   step's events.
3. **Applied.** `compound.c` calls `b3DynamicTree_CreateProxy` on its own local tree for compound
   child lookup, so "the only callers" in §7.4 was too broad. It now says the wrappers are the
   only callers for the world's three broad-phase trees, and that a compound's child tree is
   borrowed geometry (§10), not world state.
4. **Applied.** §8 now retains up to one interval beyond `tickCount` before the next image, and the
   API comment for `tickCount` said images are kept for at most that many ticks. The comment now
   states the interval-sized overrun.

3 applied, 1 declined.

## Status

Finding count fell to 4 (0H-2M-2L), the second round with no High. The findings are a repeat
(1, reopening round 13's finding 2 without engaging the stated boundary rationale), a missing test
class (2), an over-broad source claim (3, like round 15's "contiguous" claim) and a comment that
disagreed with §8 (4, adjacent to round 16's eviction finding). None was a consequence of round 16's
fixes. The last three rounds average 4.7 findings with low severity, but findings still come from
source mismatches and unstated rules, not only wording; the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 17: 4 findings (0H-2M-2L), 3 applied, 1 declined`

`Series total: 102 findings (40H-54M-8L) across 17 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L)`
