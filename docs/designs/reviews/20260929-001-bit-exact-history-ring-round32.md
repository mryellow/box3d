---

title: Codex review — bit-exact history ring, round 32
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 32

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible, and several key source claims check out. The design is not ready to converge: its central claim that the compiler will enforce complete journaling does not hold as specified.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.3 | The design stores opaque journaled types **by value** in `b3World` while declaring their storage only in a private header. C requires a complete type for a by-value field. The proposed module boundary cannot compile as written. See [physics_world.h](src/physics_world.h) and [container.h](src/container.h). | Specify a compilable representation, such as a public wrapper containing an opaque implementation pointer, and account for its allocation and access costs. |
| 2 | High | §2 requirement 7, §5.2, §7.1 | The separate hot accessor exposes writable storage whenever the world is inside a step, but structural writes also happen during a step. A step assertion therefore cannot enforce the stated rule that structural writes always journal. [b3CreateContact](src/contact.c) is one relevant structural path. The claimed “verified by construction” guarantee remains unproved. | Define narrow hot write operations that cannot reach cold fields, and make structural mutations use distinct journaled operations even during a step. State the compile-time boundary precisely. |
| 3 | Medium | §2 requirement 5, §8, §14 | `maxBytes` and `bytesUsed` exclude the reusable staging buffer and unused arena capacity. A large churn burst can leave substantial resident memory after its slots are evicted. Thus the stated limit on overruns describes retained slot charges, not the history system’s memory footprint. | State that distinction in the requirement and API, or add resident-memory accounting and a policy to shrink excess capacity. |
| 4 | Low | §14 | The API sketch does not define behavior for `tickCount <= 0` or `captureInterval <= 0`, although interval arithmetic and the minimum-window policy depend on positive values. | Specify validation, defaults, and the result of invalid definitions. |

## Checked, no change

- The last commit touching `src/` or `include/` is `5643cd8`, matching the document’s source baseline.
- [ScrubBackward](test/test_recording.c) does check backward seeks against the recording hash; [that hash](src/recording.c) covers transforms and velocities, so the document correctly limits what this demonstrates.
- The source supports the stated sensor overlap swap, CCD candidate cap and running fraction, explosion traversal order, proxy free lists, and dynamic/kinematic tree rebuild paths.
- The phase-1 exception for raw dynamic and kinematic tree images is stated clearly; the document does not claim phase 1 meets the sleeping-proxy cost requirement.

## Proposed edits

Revise §5.2 and §7.3 around a compilable storage boundary, then show how each hot accessor prevents unjournaled structural writes. Clarify whether the memory budget covers retained bytes or resident allocation, and add validation rules to the API sketch.

## Unresolved / disagreements

The performance baseline cites a file under `docs/designs/reviews/`, which the review instructions prohibited reading. I could not independently verify those measurements. The proposed capture costs also remain estimates until §12’s benchmarks run.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T15:40:48+10:00, ended 15:47:45+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` for the target doc was unchanged by the run, so the working tree was
untouched by Codex. Same fresh full-scope prompt as rounds 18 to 31, no declined-findings list. The
final message above is unchanged apart from absolute path prefixes and line-number suffixes on its links.

**Findings verified against the doc and source.**

1. **Applied.** `b3World` in `physics_world.h` holds `bodies`, `contacts`, `shapes` and the id pools
   by value, and C needs the complete type for a by-value field, so §5.2's "opaque struct declared in
   a private header, held by the world by that type" could not compile: including the definition in
   the world's header exposes the fields everywhere. §5.2 now says the world holds each journaled
   container and record array by pointer to an incomplete type, allocated at world creation, and that
   a stage takes the base pointer once per loop.
2. **Declined as a defect; §5.2 sharpened.** The hot accessor was already limited to an awake set's
   elements and the sensor pass, and the structural writes the finding names in `b3CreateContact`
   (the contact record, both body records, `pairSet`, a non-awake set's `contactIndices`) go through
   `b3Contact_Write`, `b3Body_Write`, `b3JournaledPairSet` and `b3JournaledArray`, none through the hot
   accessor; a write to an awake owner is unjournaled by design (§7.1). The doc did not say what the
   accessor returns, which let the finding read it as general write access, so §5.2 now says it
   returns element pointers into the awake set's arrays and the sensor elements only, never a count, a
   sparse record array or a non-awake set.
3. **Declined as a defect; requirement 5 sharpened.** §8 already excludes arena and staging capacity
   from `maxBytes` and `bytesUsed`, and §14's `b3HistoryInfo` reports both capacities separately. The
   finding is right that requirement 5 alone reads as a whole-footprint budget, so it now says
   `maxBytes` bounds retained history and points at §8 and §14. No resident-memory accounting or
   shrink policy is added: the arena and staging capacities are warm-up sizes reused tick to tick
   (§8), and shrinking them would reintroduce the allocation §8 avoids.
4. **Applied.** `b3HistoryDef`'s `tickCount` and `captureInterval` had no stated valid range, and §6
   and §8 divide and compare by them. §14 now says each is at least 1, asserted, the engine's usual
   handling of an invalid definition.

2 applied, 2 declined. No other file was touched; the front matter's `status:` finding sequence and
round count in the target doc were updated to include this round.

## Status

Finding count is 4 (2H-1M-1L), up from 3 and with the first Highs since round 27. Both Highs
concern §5.2's compile-time boundary: #1 is a real defect in text that round 31's fix (its
finding 2) introduced, since that fix stated the opaque-struct mechanism without checking C's
complete-type rule, so it is a consequence of the immediately preceding round's own fix; #2 is a
misreading the doc's wording allowed. #3 and #4 are edge cases. None reopened ground the doc's
rationale covered in a way that engaged it: #3 re-raised §8's accounting from the requirement's
side without engaging §8. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4) are
still 2 to 4, so the series is not converged.

AC check before this round and again on the result: the record-array and container boundary is a
choke point by type (AC-2 satisfied), no shadow structure is introduced (AC-1, AC-3), and the
`EnableHistory` assertion added in round 31 and the new `tickCount` assertion document invariants and
are not control flow (AC-4).

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 32: 4 findings (2H-1M-1L), 2 applied, 2 declined`

`Series total: 148 findings (48H-82M-18L) across 32 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L)`
