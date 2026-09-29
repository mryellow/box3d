---

title: Codex review — bit-exact history ring, round 31
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 31

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the source tree, but three boundary cases need clearer contracts before implementation. I read the design in full and made no file changes.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§5.3, 8–10 | Enabling history after an API call that queues an end event can make replay differ. [Sensor destruction](src/sensor.c:413) queues events between steps, but tick 0 captures no event state and `Rewind(0)` clears both end-event buffers. The next step can therefore lose an event even when its inputs are unchanged. | Require `EnableHistory` at a clean step boundary, or capture and restore events already queued when history is enabled. Test this case. |
| 2 | Medium | §§2.7, 5.2, 13 | The promised compile-time write barrier needs a concrete storage boundary. The current [`b3World` definition](src/physics_world.h:110) exposes writable arrays across translation units. Migrating callers to accessors alone would not make a missed direct write fail to compile, which is central to requirement 7. | Specify how world-owned storage becomes inaccessible for writes outside its accessor or hot-path module, and make that boundary a phase-1 completion gate. |
| 3 | Low | §§8, 10, 14 | “The first mutating call” truncates forward history in the API description, while callback registration and world `userData` are explicitly live host configuration. Their setters do not pass through the proposed scalar accessor, so they would not truncate. | Define truncation in terms of tracked simulation-state writes, and state explicitly how host-configuration setters behave during forward scrub. |

## Checked, no change

- [`ScrubBackward`](test/test_recording.c:251) supports the document’s narrow claim: the existing hash checks body transforms and velocities, not full world state.
- The source supports the separate treatment of awake records, sleeping sets, sensor overlaps, hull references, and proxy identity.
- The CCD sensor cap and running-fraction gate, and explosion’s traversal-order wake and impulse writes, support the proposed tree-order changes.
- The FAQ and simulation documentation support the stated determinism baseline. I treated §11.1’s supplied performance figures as context.

## Proposed edits

Add the tick-0 queued-event rule and test; specify the storage boundary that enforces journal completeness; narrow the API’s truncation wording to tracked simulation-state writes.

## Unresolved / disagreements

The callback-order exception is explicit, but it limits the otherwise broad “bit-exact” claim for callers with order-dependent callback side effects. I have no further disagreement with the whole-world replay recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:57:17+10:00, ended 15:00:59+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 30, no declined-findings list. The final
message above is unchanged apart from absolute path prefixes on its links.

**Findings verified against the doc and source.**

1. **Applied.** §9 step 5 drops queued end events on restore, and §8's tick 0 slot images no event
   state, so an end event queued by an API call (a sensor destruction) before `EnableHistory` would
   be lost on `Rewind(0)` although no segment holds the call for the caller to replay. §8 now
   requires `EnableHistory` at a step boundary with no such queued events, asserted. No test is
   added: the case is an assertion, not a behaviour.
2. **Applied as rationale.** §5.2 already required const-only element access outside the defining
   translation unit and a phase 1 completion gate, but did not say how the world struct's fields
   become unassignable. It now states that each journaled container and record array type is an
   opaque struct whose storage fields sit in a header private to its defining translation unit, so
   assignment elsewhere does not compile. `b3World` in `physics_world.h` exposes the raw arrays
   today, which is the migration the gate covers.
3. **Applied.** §8 truncates on a write through a write accessor or journaled container; callback
   registration and `world->userData` go through neither, and §9 does not restore them. §10 now says
   they neither journal nor truncate, and §14's comment says "writes simulation state".

3 applied, 0 declined.

## Status

Finding count is 3 (0H-2M-1L) for the third round running, no High. The findings are an
`EnableHistory` boundary case, a mechanism the doc stated by effect but not by construction, and a
truncation rule that did not cover host configuration; all in text that predates round 30, none a
consequence of round 30's fixes, none reopening ground the doc's rationale covered. The counts since
round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3) remain flat at 2 to 4, so the series is not
converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 31: 3 findings (0H-2M-1L), 3 applied, 0 declined`

`Series total: 144 findings (46H-81M-17L) across 31 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L)`
