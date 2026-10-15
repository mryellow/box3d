---
title: Codex review - history ring budget, window and eviction, round 3
target: docs/designs/20261015-003-history-ring-budget-and-eviction.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-003-history-ring-budget-and-eviction-round2.md
mode: full pass, same prompt and scope as round 1, on revision 3
---

# Round 3

## Exact request sent to Codex

Identical to round 2 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T18:08:18+10:00 to 2026-10-15T18:12:16+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13e9a-dcfc-7170-beff-67fe854562af`.

## Review report (Codex final message)

## 1. Summary

The ring’s image anchor, rewind discard, slot ownership, and allocator rules are consistent with the cited parent sections and live source. Two budget statements need correction: the algorithm does not handle the API’s zero value for an unbounded budget, and the memory formula omits charges from non-imaged ticks.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §6 | The stated comparison treats every positive charge as over budget when `maxBytes == 0`, although the parent’s §14 defines zero as unbounded. At tick 1, this would widen the default interval and skip an image without memory pressure. | Explicitly bypass budget comparison, widening, and budget eviction when `maxBytes == 0`. |
| 2 | L | §2 | “Per-tick slot multiplied by the imaged ticks” undercounts retained memory when the interval exceeds 1. Every tick still has a journal, header, and directory entry, as §§5–6 specify. | State the total as image charges for imaged ticks plus journal, header, and directory charges for every retained tick, plus entry-owned blocks. Keep the 60-tick interval-1 estimate as its special case. |

## 3. Checked, no change

- The oldest retained image remains an anchor for later segments; the interval cap and window rule do not require evicting that sole anchor.
- Rewind’s pending-entry undo, later-slot removal, and block ownership match the parent’s §§6, 7.1, and 9.
- The history info and restorable-tick query otherwise match the parent’s §14, including queries later than the current tick.
- `src/replay.c` supports the claim that the recording player doubles keyframe spacing. `src/core.c` supports the stated allocator size types; `src/physics_world.c` supports freeing history-owned blocks before manifold allocator and hull database teardown.
- The cited 5–51 MB image range and 0.3–3 GB interval-1 estimate are consistent.

## 4. Proposed edits

Add an explicit zero-budget branch to §6’s decision rule, and replace §2’s general memory formula with one that charges every retained tick’s non-image bytes.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the doc and the parent's §14.

- **#1 (M) `maxBytes` = 0: confirmed, applied.** The parent's §14 defines 0 as unbounded; §6's
  comparison had no branch for it and would have widened at tick 1. §6 now states that 0 disables
  the budget bullet and leaves only the window bound.
- **#2 (L) the §2 memory formula: confirmed, applied.** Every retained tick carries a journal
  segment, header and directory entry whatever the interval; §2 now sums image charges over imaged
  ticks and the other charges over every retained tick, with the interval-1 figure as the special
  case.
- **Cut (U-S4).** §6 has produced findings in rounds 1, 2 and 3, the second and third on wording the
  previous fix wrote. The budget bullet is rewritten as a four-step procedure holding the same
  facts in order (window bound, minimum window, size and compare, widen and write), with the
  reasons reduced to one clause each; no rule changed beyond the two findings above.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one Medium and one Low, both on §6 again. Three rounds running on §6 (rounds 1, 2,
3): the owner's stop condition and U-S4. Cut applied (above); the series is paused for the owner
to decide whether to run round 4 on the cut text. Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings clustered in §6 three rounds running;
#1 is a gap the round-1 and round-2 wording shared, #2 is in §2 (U-T2); the reviewer read no round
record. Doc length 188 -> 182 lines; the cut replaced a 24-line bullet with a 22-line one (U-T3).
Under the split check (U-T6).

Round 3: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 9 findings (2H-5M-2L) across 3 rounds
Findings: 5 (1H-3M-1L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L)
