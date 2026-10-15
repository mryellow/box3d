---
title: Codex review - history ring budget, window and eviction, round 4
target: docs/designs/20261015-003-history-ring-budget-and-eviction.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-003-history-ring-budget-and-eviction-round3.md
mode: full pass, same prompt and scope as round 1, on revision 4 (the U-S4 cut of §6's budget bullet)
---

# Round 4

## Exact request sent to Codex

Identical to round 3 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T20:22:12+10:00 to 2026-10-15T20:26:20+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f15-7240-7fb1-86f0-2e9571626f92`.

## Review report (Codex final message)

## 1. Summary

The design has two ring placement rules that can discard or overwrite history the window still requires. Its multi-gigabyte arena assumption also conflicts with allocator accounting in the live source, and its interval-1 slot count is off by one.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §3, §6–7 | Physical wrap can evict an imaged tick that is still inside `tickCount`, even with `maxBytes = 0`. With interval 1 and `tickCount = 3`, tick 0 remains eligible at tick 3; if its space is needed, §3 evicts it although §6 says an unbounded budget evicts only by the window. | Grow the arena whenever appending would overlap any slot the window and budget rules retain. Apply logical eviction first; never use wrap itself as an eviction rule. |
| 2 | H | §3, §6 | Growth copies wrapped slots at their old offsets. After growth, appending at the end of the newest slot can collide with an older retained slot still at a higher offset, including a protected image. | Linearize retained slots during growth, update directory offsets, and place the write position after the newest slot; or specify an equivalent safe placement scheme. |
| 3 | M | §2, §6 | The cited 0.3–3 GB case relies on `b3Alloc` for an arena above `INT_MAX`. Although its parameter is `size_t`, `src/core.c` casts the allocation size to `int` when adding to the public byte counter, while `b3Free` subtracts the full size. Such an arena corrupts `b3GetByteCount` accounting (`include/box3d/base.h`). | Make correct large-allocation accounting an explicit prerequisite, or bound and revise the arena design and size claim. |
| 4 | L | §2, §7 | At interval 1, a window whose oldest image may be `tickCount` ticks before the current tick contains up to **`tickCount + 1`** slots. The “per-tick slot times the window” statement implies only `tickCount` slots. | State the inclusive slot count and use it in the capacity estimate. |

## 3. Checked, no change

- The interval-doubling overflow guard and `tickCount` cap are consistent with the stated imaging rule.
- The parent’s §6, §7.1 and §9 support the stated transfer of journal-owned blocks on undo and their release on eviction or disable.
- The parent’s §14 distinguishes `GetRestorableTick` from `Rewind`; the target’s query and restore rules are consistent with those field and result semantics.
- `src/replay.c` does double keyframe spacing under budget pressure. `src/physics_world.c` places the step’s end-event flip and migration drain after sensor work, as the capture timing requires.

## 4. Proposed edits

Define eviction solely by the window and byte-budget decisions, then grow when the resulting slots do not fit physically. Specify how growth relocates a wrapped ring and updates its directory. Correct the inclusive window count and record the required allocator-accounting fix or a supported arena limit.

## 5. Unresolved

The design must choose how to handle arena allocations above `INT_MAX` before relying on the cited multi-gigabyte case.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the doc, the parent's §14 and the live source.

- **#1 (H) arena wrap evicts inside the window: confirmed, applied.** §3's old-end bullet made
  wrapping into the oldest slot the eviction rule, protected only the minimum window, and so
  evicted a slot §7 retains under `maxBytes` = 0, which §6 says evicts by the window alone. §3 now
  says eviction is decided by the window and the budget alone, never by the arena, and the arena
  grows when the retained slots and the slot being written do not fit; §6 step 3's eviction is
  stated as the budget rule (oldest first, never into the minimum window, until the retained
  charges plus this slot's are within `maxBytes`) instead of "as far as this slot needs".
- **#2 (H) growth at the old offsets leaves the write position blocked: confirmed, applied.** A
  wrapped ring has its newest slot below its oldest; copying at the old offsets leaves the newest
  slot's end still against the oldest slot, so the appended capacity is unreachable. §6's capacity
  bullet now copies the retained slots in tick order from offset 0, rewrites the directory's
  offsets and sets the write position after the newest slot.
- **#3 (M) the allocator's byte count truncates above `INT_MAX`: confirmed, applied.** `b3Alloc`
  adds `(int)size` to `b3_byteCount` and `b3Free` subtracts `(int64_t)size` (`core.c`), so an
  arena above `INT_MAX` bytes corrupts `b3GetByteCount`. §6 states the fact and that the add takes
  the full `size_t` before an arena can exceed `INT_MAX` bytes, a fix in `core.c` the design
  requires, as it requires the const-returning getters of the parent.
- **#4 (L) the window holds `tickCount` + 1 slots: confirmed, applied.** The parent's §14 defines
  `tickCount` as the most ticks the oldest retained image may be before the current tick, so the
  window is `tickCount` + 1 slots inclusive; §7's "exceed `tickCount`" clause counted inclusive
  ticks and contradicted its own "at most `tickCount` before `historyTick`". §7 now evicts while
  the oldest retained imaged tick is more than `tickCount` before `historyTick` and names the slot
  count; §2's formula multiplies by `tickCount` + 1 and the 60-tick example says 61 slots, still
  0.3 to 3 GB (61 × 5 to 51 MB).

Raw log: the reviewer read no round record or changelog.

## Status

Four findings, two High, one Medium, one Low; two on §6 again (#2 and #3 on the capacity bullet,
#1 on §3 and §6 step 3), the fourth round running on §6 and the first after the U-S4 cut of the
budget bullet. U-S4 says pause and report when a section recurs after a cut; the series is
stopped for the owner. None of the four is on the cut text itself: #1 and #2 are the old-end and
growth rules, which no earlier round had examined, #3 is a source fact, #4 is the window count.
Not converged.

Tripwire (U-T): no mechanism added; the arena-wrap eviction rule is removed, eviction being
decided by the window and budget alone (U-T1). §6 four rounds running; #2 and #3 are on the
growth sentence round 1's fix wrote, #4 on the formula round 3's fix wrote, #1 on text no round
had touched (U-T2); the reviewer read no round record. Doc length 182 -> 188 lines; added lines
per fix: #1 two, #2 one, #3 two, #4 one (U-T3). Under the split check (U-T6).

Round 4: 4 findings (2H-1M-1L), 4 applied, 0 declined
Series total: 13 findings (4H-6M-3L) across 4 rounds
Findings: 5 (1H-3M-1L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 4 (2H-1M-1L)
