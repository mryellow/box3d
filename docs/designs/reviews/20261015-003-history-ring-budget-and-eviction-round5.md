---
title: Codex review - history ring budget, window and eviction, round 5
target: docs/designs/20261015-003-history-ring-budget-and-eviction.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-003-history-ring-budget-and-eviction-round4.md
mode: full pass, same prompt and scope as round 1, on revision 5
---

# Round 5

## Exact request sent to Codex

Identical to round 4 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T20:46:26+10:00 to 2026-10-15T20:49:37+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f2b-a289-76c2-92ba-b178a4c450dd`.

## Review report (Codex final message)

1. **Summary**

The ring’s eviction and rewind rules preserve an image for retained segments and handle journal-owned blocks consistently with the cited parent sections. Two changes are needed: the document overstates what the configured capture interval guarantees after budget widening, and its large-arena allocator claim contradicts the live source.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §§2, 5–6 | §5 says `captureInterval = K` images every K-th tick, and §2 says every tick is restorable when that interval is 1. §6 can widen the *effective* interval while the configured `captureInterval` remains 1. Those statements then promise restorable ticks that have no image. | State that the configured interval is the initial spacing; after widening, restorable ticks are the retained imaged ticks determined by `effectiveInterval`. |
| 2 | M | §6 | The claim that allocator accounting takes the full `size_t` before an arena exceeds `INT_MAX` is false. `src/core.c` adds `(int)size` to its byte count, while `b3Free` subtracts the full size. An arena above `INT_MAX`, which this design permits, corrupts that count. | Correct the claim and require allocator accounting to be fixed before supporting arenas above `INT_MAX`. |

3. **Checked, no change**

- Window and budget eviction retain an oldest image and the segments needed to reach later retained images.
- Rewind removal and eviction free or transfer journal-owned blocks in the manner specified by the parent’s §§7.1 and 9.
- The slot and pending-byte definitions agree with the parent’s §14 fields.
- The recording player does double its keyframe interval under budget pressure (`src/replay.c`); the step-end capture boundary agrees with `src/physics_world.c`.
- The 61-slot, approximately 0.3–3 GB example is consistent with the parent’s §5.1 estimates.

4. **Proposed edits**

Apply the two wording and allocator changes in the findings table. The allocator change also needs a corresponding `src/core.c` fix if arenas above `INT_MAX` are supported.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against the doc and the live source.

- **#1 (M) the configured interval promised restorable ticks the effective interval does not image:
  confirmed, applied.** §2 said every tick is restorable when the capture interval is 1 and §5 said
  `captureInterval = K` images every K-th tick, while §6 widens the effective interval and never
  narrows it. §2 now says the effective interval, and §5 says `captureInterval` is the effective
  interval at enable, imaging every K-th tick until §6 widens it.
- **#2 (M) the allocator byte-count sentence read as a claim about today's code: confirmed as
  wording, applied.** `core.c` still adds `(int)size`; the sentence meant the change the design
  requires, as it does for the parent's const-returning getters, and now says so: the add becomes
  a full `size_t` add, a `core.c` change required before an arena can exceed `INT_MAX` bytes.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, both Medium, both wording; #2 is on the sentence round 4's fix wrote. §6 has
produced findings in rounds 1 to 5, the second round after its U-S4 cut; the series is stopped
for the owner again. Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). §6 five rounds running; #2 is a consequence
of round 4's fix #3, #1 is on §2 and §5 text no earlier fix wrote (U-T2); the reviewer read no
round record. Doc length 188 -> 190 lines; added lines per fix: #1 one, #2 one (U-T3). Under the
split check (U-T6).

Round 5: 2 findings (0H-2M-0L), 2 applied, 0 declined
Series total: 15 findings (4H-8M-3L) across 5 rounds
Findings: 5 (1H-3M-1L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 4 (2H-1M-1L) -> 2 (0H-2M-0L)
