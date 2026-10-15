# Changelog: history ring budget, window and eviction

## Revision 1 (2026-10-15)

Split from `docs/designs/20260929-002-rewind-only-history-ring.md` §8 after that series' round
36, under its split check: the ring (slots, two ends, interval, budget, charge, window, enable and
disable) decides its own thing and had drawn findings in rounds 22, 26, 33 and 36. Moved here
verbatim where the text was reviewed, with the parent's requirement 5, its §6 capture-interval
paragraph, its §12 ring-transitions test and test 5's budget acceptance; the sole-image case joins
the transitions test. Citations of the parent's sections are by path and `§N`. A Provenance section
records the three decisions the moved text already gave reasons for.
