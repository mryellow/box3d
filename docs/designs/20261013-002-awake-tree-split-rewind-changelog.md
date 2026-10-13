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
