---
title: Codex review - history ring budget, window and eviction, round 2
target: docs/designs/20261015-003-history-ring-budget-and-eviction.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-003-history-ring-budget-and-eviction-round1.md
mode: full pass, same prompt and scope as round 1, on revision 2
---

# Round 2

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T17:54:11+10:00 to 2026-10-15T17:57:19+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13e8d-efd0-7871-8112-c5c77169f61d`.

## Review report (Codex final message)

1. **Summary**

The design has two rule conflicts that can widen the capture interval unnecessarily. The two-image minimum also conflicts with the stated `tickCount` bound once the older image ages out of the window.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§3, 6, 7 | The minimum window protects the two newest images, but §7 must evict the older one when it falls outside `tickCount`. With `tickCount = captureInterval = 4`, images at ticks 0 and 4 make tick 0 part of the stated minimum at tick 5, while §7 and the parent’s §14 require it to be gone. | Make the window bound take precedence. Define the protected suffix after window expiry, and update the two-image minimum and overrun rules accordingly. |
| 2 | M | §6 | The budget comparison counts the candidate slot twice: the “minimum window … through this tick” already includes it, yet the rule compares that slot with `maxBytes` minus the minimum window’s bytes. For example, a 40-byte minimum including a 20-byte candidate fits a 50-byte budget, but `20 > 50 − 40` widens the interval and may skip its image. | Sum each prospective retained slot once. Apply window expiry before assessing budget pressure, then widen only if the applicable retained set still cannot fit. |

3. **Checked, no change**

- The image and segment placement, pending-entry undo, and discard after rewind agree with the parent’s §§6, 7.1 and 9.
- The restorable-tick query and reported capacity and pending-byte fields agree with the parent’s §14.
- `src/replay.c` does double keyframe spacing under pressure; `src/core.c` gives `b3Alloc` and `b3Free` `size_t` sizes while `b3GrowAlloc` uses `int`.
- `src/physics_world.c` flips the end-event buffers at step end and tears down the hull database and manifold allocators late enough for the proposed history cleanup order. The 5–51 MB times 60 estimate is correctly rounded.

4. **Proposed edits**

- Reconcile §§3, 6 and 7 around a protected suffix that always satisfies `tickCount`; state explicitly when an older image ceases to be protected.
- Replace §6’s slot-versus-remaining-budget comparison with a total-charge comparison that includes each slot once.

5. **Unresolved**

None.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against the doc and the parent's §14.

- **#1 (H) the two-image minimum against the window bound: confirmed, applied.** §7 evicts the
  older image once the ticks from it to `historyTick` exceed `tickCount` and a later image remains,
  so with `tickCount` = `captureInterval` = 4 the image at tick 0 leaves at tick 5 while §3 and §6
  called it protected. The minimum window is now the newest imaged tick and every slot after it
  through the current tick, plus the next older image and the slots between when the window bound
  retains it; the window bound is applied first. §3 cites §6 for the definition instead of
  restating it.
- **#2 (M) the candidate slot counted twice: confirmed, applied.** Revision 2's comparison put the
  slot on one side and a window that already held it on the other. The bytes the minimum window
  would retain, the slot counted once, are compared with `maxBytes`.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High and one Medium, both on §6's budget procedure, which round 1 also found.
#2 is a consequence of round 1's fix (the comparison it wrote). Not converged. Next: round 3,
same prompt, same scope; a third round on §6 would trip U-S4.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings clustered in §6 two rounds
running, one a consequence of the previous fix (U-T2); the reviewer read no round record. Doc
length 185 -> 182 lines; added lines per fix: #1 three (one removed from §3), #2 zero (U-T3).
Under the split check (U-T6).

Round 2: 2 findings (1H-1M-0L), 2 applied, 0 declined
Series total: 7 findings (2H-4M-1L) across 2 rounds
Findings: 5 (1H-3M-1L) -> 2 (1H-1M-0L)
