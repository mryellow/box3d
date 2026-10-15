# Changelog: history ring budget, window and eviction

## Revision 1 (2026-10-15)

Split from `docs/designs/20260929-002-rewind-only-history-ring.md` §8 after that series' round
36, under its split check: the ring (slots, two ends, interval, budget, charge, window, enable and
disable) decides its own thing and had drawn findings in rounds 22, 26, 33 and 36. Moved here
verbatim where the text was reviewed, with the parent's requirement 5, its §6 capture-interval
paragraph, its §12 ring-transitions test and test 5's budget acceptance; the sole-image case joins
the transitions test. Citations of the parent's sections are by path and `§N`. A Provenance section
records the three decisions the moved text already gave reasons for.

## Revision 2 (2026-10-15, after round 1)

Round 1: 5 findings (1H-3M-1L), 5 applied, 0 declined.

- §6: the slot is sized and the interval decided before any eviction; eviction then goes only as
  far as the final slot needs. The interval becomes `min(2 × interval, tickCount)`. A slot's charge
  includes its directory entry. Arena growth is allocate, copy, free through `b3Alloc`/`b3Free`.
- §3, §6: the minimum window is the two newest imaged ticks and every slot from the older of them
  through `historyTick`.
- §5/§6: the duplicated sentence on when a tick is imaged kept in §5 only.
