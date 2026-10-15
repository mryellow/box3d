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

## Gate re-run before Canonical (2026-10-16, U-G4 and U-G1, no revision)

- AC-1 to AC-4 checked by the author against revision 7, the whole text, all pass. This is the
  first gate entry in this document's own changelog: revision 1 moved the parent's §5.4 and §7.4
  verbatim after that series' round 39, inside the parent's own gates, and revisions 2 to 7 are
  the edits of rounds 1 to 8 above. Code references re-read against 55a40d1, which is still the
  last commit touching `src/` and `include/`: the tree writes outside `broad_phase.c` are exactly
  the four §7 names (`solver.c` refit and enlarge, `physics_world.c` rebuild, `world_snapshot.c`
  load); every leaf-box write in the static and kinematic trees passes the shape's `fatAABBs`
  entry (`body.c`, `shape.c`, `solver.c`); proxy-pool growth appends an ascending free list only on
  an exhausted list (`dynamic_tree.c`); the migration queue is one array by id in placement order
  (`broad_phase.h`). The parent's §9 orders the tree step after the journal walk and the image
  copy, as §6 requires; its §5.2 third writer is §6's one restore-time shape writer.
- AC-1: the awake tree's image and the queue beside it are point-in-time copies the ring owns and
  discards; the proxy entries are a log; the restore list is a transient list of shape ids that
  restore's writer appends to and the pass consumes once, holding no record and kept in step with
  nothing, with no set beside it (a repeated id is moved twice); image T's moved list is part of
  the image; a leaf equalling its `fatAABBs` entry is an engine fact read at the point of use, not
  a structure added. §12 rejects the scan of body records for the parked flag. No subset, scope
  or side table over a tree, a set or a pool.
- AC-2: the four trees are reachable outside `broad_phase.c` only as a pointer-to-const, which the
  `b3DynamicTree` write functions do not accept, so a bypass does not compile; the journal call is
  inside the proxy create and destroy every path reaches, single and bulk; today's four outside
  writers move in. The §7 list of callers (shape create and destroy, `b3ResetProxy`,
  `b3Body_SetType`, the placement) describes coverage, not the mechanism. §6's "every write of a
  leaf's box writes the entry" states what the engine does today, verified above; once the writers
  are in one file the design reads it as an invariant. Observation for the Implementation Prompt,
  not a finding: the static and kinematic leaf writers can assert the box equals the entry in a
  validation build, an assertion of a construction-guaranteed invariant under AC-4, never a branch.
- AC-3: the three layouts are derived at restore from the entries and the moved list, never
  compared with a kept copy; `proxyKey` comes back by the reverse undo of the entries and §7 says
  it needs no post-restore correction; the sleeping tree is restored from entries alone; a
  kinematic tree the pass left unordered is rebuilt by the step's own rebuild, no step of `Rewind`
  reconciles it; the height and rebuild counters may differ and nothing brings them back.
- AC-4: the undo asserts the popped id is the recorded one; the pass's predicate (a proxy key, one
  of the two tree codes, an id inside the restored array) is the selection the pass is defined by
  over a list that is a superset by construction, reading the restored record, not a check that a
  guaranteed invariant held; a repeated id is moved again rather than detected; the height test an
  undone wake can fire inside `Rewind` is the engine's own; the parent's §9 step 6 checks are
  validation-build assertions. Nothing branches on a generation, a staleness mark or a confirm.
- U-G6: each of the four trees has its route stated and the routes agree across §5, §6, §7 and §9
  with the parent's §13 phases: awake imaged with no entry and no pass; static entries and pass
  from phase 1; sleeping entries and no pass; kinematic imaged raw in phase 1, entries and pass
  from phase 2. U-G7: no claim about a system the project does not own.
- Status stays Draft; Canonical is the owner's edit.
