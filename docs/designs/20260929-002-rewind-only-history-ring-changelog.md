# Changelog: 20260929-002 rewind-only history ring

## Revisions 1 to 21 (2026-09-29 to 2026-09-30)

- Derived from `20260927-002-bit-exact-history-ring.md` by removing forward scrub. Twenty-one
  review rounds against source as of `5643cd8`, recorded in `docs/designs/reviews/`; the series
  converged at round 21. Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L)
  -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L)
  -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L)
  -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L). The tallies
  lived in the front matter until revision 22.

## Revision 22 (2026-10-15)

Reconciled with the tree changes on `master` through `55a40d1` (U-I1): the sorted continuous
candidates and explosion order (`20261012-001`, `20261012-005`, `20261013-005`), the awake and
sleeping dynamic trees (`20261013-001`), the parked leaf and `b3Body_SetTransform` wake
(`20261015-001`), the unrotated and bulk migration inserts. Folds in
`20261013-002-awake-tree-split-rewind.md`, now Closed. Reopens the doc at full scope: status is
the bare word Draft and the finding tallies moved here.

- Front matter: status, code baseline.
- §2 requirement 1, §3, §4 table and cost line: tree layout is not simulation-affecting on
  today's engine; the awake tree and the migration queue are hot; the static, kinematic and
  sleeping layouts are derived.
- §5.1: rows for the awake tree, the migration queue and the kinematic tree's moved proxies
  replace the moved-proxies row; the awake tree's bytes join the per-tick estimate.
- §5.2 table: proxy-id row names the single and bulk create choke points and the two journaled
  trees.
- §5.4 rewritten for four trees and the two rebuild rules.
- §6: the awake tree and the queue are flat copies; the moved-proxy gather is the kinematic
  tree's.
- §7.1: proxy entries name their trees and carry the leaf's box; the sleep path's proxy creates
  are entries. §7.2: one entry per migrated shape, churn measured in the preflight.
- §7.4 rewritten as "Trees": which tree holds what, the awake tree image with the queue, the
  derived trees' restore pass and moved-bit restore, leaf box equals fat AABB entry, proxy
  identity with the bulk create, height-triggered rebuilds, phase-1 kinematic image. The three
  order changes, the CCD two-pass sensor collection and the explode description are gone, since
  the engine now sorts.
- §9 step 6: the step's proxy-tree check. §11.3: order changes removed from the engine-change
  list.
- §12: test 1 hashes four proxy pools and the queue; test 2 adds the parked, shape-on-sleeping,
  bulk and height-trigger cases; test 5 reports the sleeping tree's rebuild; test 7 is written
  for the four trees.
- §13: phase 1 images the awake tree and the kinematic tree, journals the static and sleeping
  trees; phase 2 derives the kinematic tree.
- §16: open question on the order changes removed, the rest renumbered.
- §17 Provenance added: awake tree imaged, sleeping tree derived, pool growth, no height bound,
  queue imaged.

## Revision 23 (2026-10-15)

Owner decisions on four of §16's five open questions, folded in (U-S5); the doc stays Draft at
full scope, unreviewed since revision 22.

- §7.3: journaling runs only while history is enabled; a disabled world pays one null check per
  structural write.
- §12 test 1: `b3World_ComputeStateHash` is public, not "worth exposing".
- §16: questions 1 to 4 removed (hash public; journaling enabled with history; no return to the
  present without re-stepping, the forward journal being `20260927-002`'s own design; the
  recording player keeps its own ring). Question 5 is now question 1, with the per-record mark
  assigned to measurement after phase 1 under test 5 rather than built in phase 1.
- §17: four Provenance bullets carry the four decisions and their reasons.

## Revision 24 (2026-10-15, after round 22)

Round 22, the first against the reconciled doc: 6 findings (2H-3M-1L), all applied.

- §5 hot row, §5.1, §6, §7.4: the moved-proxy list covers the kinematic and static trees. A
  static leaf is marked by between-step API calls and cleared by the next pair update, so the
  static list is empty after any step and non-empty only in tick 0's image, which
  `b3World_EnableHistory` takes after such calls. Rewind(0) previously cleared those marks and
  the first replayed step could miss pairs.
- §7.4: the tree pass skips a listed shape id at or beyond the restored shape array's length,
  which an undone bump allocation truncated away, without reading it.
- §3, §10: the engine runs SIMD width 4, or 8 under AVX2, and `test_determinism.c` holds the two
  bit-identical; the per-world `b3World_EnableSSE2Fallback` width and the process-wide width are
  host configuration, left by `Rewind` and outside the hash. "Width-4 SIMD everywhere" removed.
- §2 requirement 2: the awake tree's proxy array is its second exception, imaged to a capacity
  that never shrinks, 24 B per slot of the high-water mark, matching what §7.4 already said.
- §5.2: the compile-time claim is narrowed to element and count assignment; the world's container
  pointers are assignable but have no producer outside the container's file.
- §1, §8: `src/recording_replay.c` is `src/replay.c`. The same stale path fixed in
  `20260927-002` and `20260929-003`.

## Revision 25 (2026-10-15, owner request)

Retained memory stated alongside per-tick cost, so the ring's RAM is a stated figure and not
only its capture time.

- §2 requirement 2: the awake tree's high-water term is retained once per imaged tick in the
  window. Requirement 5: retained memory is the per-tick slot times the imaged ticks in the
  window; on the all-awake stress scenes a 60-tick window at interval 1 is 0.3 to 3 GB, and
  `maxBytes` is what holds it to hundreds of megabytes at the price of a wider interval.
- §5.1: the retained total is the per-tick image times the imaged ticks.
- §12 test 5: reports retained bytes and the effective interval at a 60-tick window, under an
  unbounded budget and under one the window does not fit; acceptance adds `bytesUsed` within
  `maxBytes` except in §8's overrun cases.
