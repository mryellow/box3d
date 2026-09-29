---

title: Codex review — bit-exact history ring, round 15
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 15

## Review report (Codex final message)

## Summary

The whole-world rewind approach is plausible, and the source supports several of its key premises. The design needs clearer acceptance gates and a correction to its capture description before it converges. This was a read-only source review; I did not run the proposed implementation or verify the performance figures.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§2, 8 | “Bounded memory” conflicts with the stated policy of admitting a slot, or minimum window, even when it exceeds `maxBytes`. Structural churn can therefore exceed the configured budget without a defined upper bound. | State the guarantee as a **target budget**, or specify a hard-limit policy and its effect on restorable ticks. |
| 2 | Medium | §§7.4, 13 | Phase 1 images the entire kinematic and dynamic trees each captured tick. The design acknowledges that this violates requirement 2 when many dynamic bodies sleep, but does not make phase 2 a gate for satisfying that requirement. | Mark phase 1 as an intermediate implementation; require phase 2 and a many-sleeping-dynamic-proxy cost test before claiming the O(awake) goal. |
| 3 | Medium | §§5.1, 6 | Capture calls sensor overlaps a “flat copy” and says those sources are contiguous. In the source, each sensor owns separate `overlaps1` and `overlaps2` arrays; `overlaps2` must be counted and gathered per sensor (`sensor.c`, `world_snapshot.c`). | Describe the per-sensor gather, destination layout, and count/offset pass; retain the stated O(sensor count + overlaps) cost. |
| 4 | Low | §§8, 14 | History initialization is unspecified. The API defines `historyTick` as the last completed tick and captures at step end, but does not say whether enabling history creates an image of the current world. This leaves `Rewind(0)` and correction before the first captured step ambiguous. | Define the initial tick and whether `EnableHistory` captures it; add an initial-state API example or test. |

## Checked, no change

- The recording scrub test does compare hashes after backward seeks, and the existing hash covers body transforms and velocities rather than the full proposed state (`test_recording.c`, `recording.c`).
- Each dynamic tree has its own proxy free list, separate from the world ID pools (`dynamic_tree.c`, `id_pool.c`).
- CCD currently uses a running fraction and caps sensor hits at eight; explosion impulses are currently applied during tree traversal. Those support the proposed order changes (`solver.c`, `physics_world.c`).
- The serializer clears body, shape, and joint `userData`, as phase 0 says (`world_snapshot.c`).
- The source files reviewed have no changes between the document’s cited commit `5643cd8` and current HEAD.

## Proposed edits

Resolve findings 1–4 in the requirements, capture procedure, phase acceptance criteria, and API initialization contract. In §6, replace the claim that all listed flat-copy sources are contiguous with the actual per-sensor layout.

## Unresolved / disagreements

The cited performance review is under `docs/designs/reviews/`, which was excluded by the review instructions. Its measured timings, byte counts, and capture targets remain unverified here. The design’s larger claim of full bit-exact restoration also remains a proposed property until its full-state hash and churn/replay tests exist.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:54:35+10:00, ended 12:59:59+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 13 and 14, no declined-findings list (round 14's
two declines were answered in the doc). Before the round the round-14 edits were checked intact and
against AC-1 through AC-4 with no failure. The final message above has its source links reduced to
plain file names, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Declined; doc sharpened.** Requirement 5 already says the byte budget is a target under normal
   churn and not a hard ceiling, and §8 already says an oversized slot or minimum window is admitted
   and that `bytesUsed` reports the overrun; Codex's proposed action is the policy the doc states. The
   requirement's label ("Bounded memory") was the one thing reading as a cap, so it is now "Budgeted
   memory" and says the overrun is limited to the retained minimum window's own slots. This is a
   repeat of round 13's finding 6, which was answered in the API comment; the requirement text was
   the remaining place.
2. **Applied.** §7.4 already says the raw kinematic and dynamic tree images fail requirement 2 for
   `large_world`-shaped worlds, and §13 called phase 1's images a temporary fallback, but nothing
   said when requirement 2 starts to hold. §13's phase 2 entry now says phase 1 does not meet it
   for many sleeping kinematic or dynamic proxies, that it is met from phase 2, and that §12 test
   5's `large_world` cost target gates it.
3. **Applied.** `b3Sensor` (`sensor.h`) holds `overlaps1` and `overlaps2` as separate `b3Array`
   fields, each its own heap allocation, so "sensor overlaps ... are contiguous today" in §6 step 2
   was wrong; §5.2 already calls them heap-owned pointees. §6 step 2 no longer lists them, and step
   3 has a per-sensor gather counted in the sizing pass and copied to its own offset.
4. **Applied.** §8 defined `historyTick` as the last completed tick and capture as writing after a
   step, but nothing captured the state before the first step. `b3World_EnableHistory` now sets
   `historyTick` to 0 and images the world as it stands as tick 0's slot with an empty journal
   segment, so `Rewind(0)` and a correction before the first captured step are defined.

3 applied, 1 declined.

## Status

Finding count fell to 4 (0H-3M-1L), the first round of the series with no High, and level with
the lowest count so far. The findings are
lower-severity: a wording repeat (1), a phase-gating statement (2), one wrong "contiguous" claim
(3, the same kind of source mismatch as rounds 12 to 14's ownership sweep, in the capture
procedure this time), and an unstated initial condition (4). Finding 1 reopened ground the doc
already covered without engaging the stated rationale. None was a consequence of round 14's fixes.
The trend is improving but the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 15: 4 findings (0H-3M-1L), 3 applied, 1 declined`

`Series total: 92 findings (39H-48M-5L) across 15 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L)`
