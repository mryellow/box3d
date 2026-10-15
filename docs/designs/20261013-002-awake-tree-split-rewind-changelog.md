# Changelog: 20261013-002 awake tree split rewind

## Revision 1

- Created from §5 of `20261013-001-preflight-awake-tree-split.md` at its revision 19, split out
  under U-T6 (the preflight was over the split-check length and decided two things). Sections 5.1
  to 5.4 become §1 to §4; the two Provenance bullets about the journaled tree (proxy-pool growth,
  stack reliance) move here as §5. Cross references to the preflight use its B0 to B4 and section
  numbers. Not yet reviewed as its own doc; its text carries the preflight's rounds 1 to 19.

## Revision 2 (with the preflight's revision 26)

- §2 and §3 follow the preflight's B4 change: the rebuild entry is the height-triggered rebuild,
  paid on the step it fires; the trigger reads the restored root height so it fires on the same
  step after a rewind under the byte journal, and may fire on a different step under the
  tree-order branch, where nothing reads the layout; `b3World_RebuildSleepingTree` no longer
  exists.

## Revision 3 (with the preflight's revision 27)

- §2: the height trigger fires after an insert, not at a step; wording follows.

## Revision 4 (2026-10-15)

- Status set to Closed, pointing forward to `20260929-002-rewind-only-history-ring.md`. The
  tree-order changes (`20261012-001`, `20261012-005`) and the awake tree split (`20261013-001`,
  `20261015-001`, the bulk migration) are on `master`, so the "without them" branch of §2 and §3
  no longer applies and the "with them" branch is now the rewind design's own text: the awake
  tree image and the migration queue, the three derived trees, the proxy entries for the static
  and sleeping trees with the single and bulk create choke points, and the height-triggered
  rebuilds are in its §7.4; the size estimate is in its §5.1 and §7.2; the parked-leaf and bulk
  cases are in its §12; the two Provenance bullets are in its §17. Body unchanged (U-I6).
