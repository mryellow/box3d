# Changelog: the broad-phase trees under a rewind-only world history

## Revision 1 (2026-10-15)

Split from `docs/designs/20260929-002-rewind-only-history-ring.md` §5.4 and §7.4 after that
series' round 39, under its split check: the trees decide their own thing, were the most churned
section (findings in rounds 22 to 25, 28, 34, 36) and carry the most source. Moved verbatim where
the text was reviewed: §5.4 and §7.4 as §3 to §9, the parent's §12 tree-order test as §10, its
§16 bullet-enlarge question as §11, and its seven tree decisions from §17 as §12. Citations of the
parent's sections are by path and `§N`. Requirements 1 to 3 are new text restating what the moved
sections already required.

## Revision 2 (2026-10-15, after round 1)

Round 1: 2 findings (0H-1M-1L), 2 applied, 0 declined.

- §7: the serializer's tree load becomes a load function of `broad_phase.c`; the claim that every
  shape write follows the proxy operation is narrowed to the `proxyKey` write.

## Revision 3 (2026-10-15, after round 2)

Round 2: 2 findings (0H-1M-1L), 2 applied, 0 declined.

- §9: the static and sleeping trees' proxy entries and restore pass are required from phase 1;
  phase 0 restores all four trees from the serialized image.
- §5: the awake-tree image is the serializer's fields less the fixed `version`.

## Revision 4 (2026-10-15, after round 3)

Round 3: 2 findings (0H-0M-2L), 2 applied, 0 declined.

- §9: proxy entries for the static and sleeping trees, the restore pass for the static tree only.
- §8, §12: no physics result reads the sleeping tree's layout; the counters are diagnostics.

## Revision 5 (2026-10-15, after round 4)

Round 4: 1 findings (0H-0M-1L), 1 applied, 0 declined.

- §6: the in-place move of a sleeping-tree leaf is named as `b3Body_SetTransform` from a query
  callback; the move entry covers every such move.

## Round 5 (2026-10-15, no revision)

Round 5: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The first clean round; the confirmation round follows.

## Revision 6 (2026-10-15, after round 6)

Round 6: 1 findings (0H-0M-1L), 1 applied, 0 declined.

- §7: the proxy choke points' callers are stated as the setters through `b3ResetProxy` and
  `b3Body_SetType` through `b3DestroyShapeProxy`/`b3CreateShapeProxy`.

## Round 7 (2026-10-15, no revision)

Round 7: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. Clean; the confirmation round follows.

## Revision 7 (2026-10-15, after round 8)

Round 8: 1 findings (0H-0M-1L), 1 applied, 0 declined.

- §5: `nodeEnd` compacts in a rebuild of a non-empty tree or in the reset of an emptied tree.

## Round 9 (2026-10-15, no revision)

Round 9: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. Clean; the confirmation round follows.

## Round 10 (2026-10-15, no revision)

Round 10: 0 findings (0H-0M-0L), 0 applied, 0 declined.

- No change to the design. The confirmation round of round 9, clean; the series is settled at
  revision 7 and the status word remains Draft.
