---
title: Review round 2 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round1.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 2
---

# Round 2

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T08:35:25+10:00 to 08:39:44+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13768-04df-7df1-b030-c1b62e126af2`.

## Review report (Codex final message)

### 1. Summary

The prompt is close to the canonical design, but several instructions give the implementer conflicting commit boundaries or evidence that can pass without proving the assigned requirement. The most consequential gaps are the B0 tree walks, the moved-bit fixture, and the pending-rebuild test.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 B0; §6 | B0 defers the public query loops to B1, while §6 claims B0 makes every tree walk use the four-tree count. Canonical design §2 B0 also assigns that rule to B0. | Move those loop-bound changes into B0 and adjust B1’s file list and mapping. |
| 2 | M | §4 B4; §6 | `SleepingTreeHeightTest` creates the overlapping static shape *after* moving the sleeping proxies. A default static shape creation marks its proxy moved (`src/shape.c`, `src/types.c`), so the contact can be found even if the sleeping proxy’s moved bit was lost in the rebuild. The test does not prove the §6 moved-bit claim. | Create and step the static neighbour before the sleeping moves, or inspect the sleeping leaf’s moved bit directly after the rebuild. |
| 3 | M | §4 B2/B3; §6 | `WakeDuringTraversalTest` checks a moved root before each step, but that only ensures a rebuild is requested. With the built-in scheduler, the tree task may finish before the collide wake. The test can pass with an unsafe immediate migration. | Use a controlled task schedule that establishes the rebuild is still active at the wake, and assert that migration remains queued until the join. |
| 4 | M | §6 | The live-body drain entry cites `b3Body_SetType` and `b3DestroyBody` “from callbacks in `test_world.c`.” That file calls them between world operations; it has no query callback exercising a queued wake followed by replacement, destruction, or slot reuse. | Add an independently runnable callback fixture for those queue cases, or remove the claimed suite evidence. |
| 5 | M | §5; §9 | The golden policy requires every closest-ray and cast value to remain equal and stops on any resulting state-golden change. Canonical design §1 explicitly permits tied closest-hit casts to see layout order; `shared/determinism.c` hashes the closest ray’s normal and uses it for spawning. The policy can therefore reject a correct tied-hit outcome. | Log hit identity and tie fractions; handle a verified tie under the design’s stated exception before applying the stop rule. |
| 6 | L | §3; §4 B2/B3 | The fail-before instructions run `WakeDuringTraversalTest` as one entry. Its first case fails on B1 and returns, so that run cannot supply the promised inside-callback failure for case 2. | Give the cases separate entry names or specify separate scratch runs that reach each assertion. |
| 7 | L | §6 | “Nine loops” is the wrong count for the listed B1 public-walk loops: Draw plus the seven named overlap, cast, and mover queries is eight (`src/physics_world.c`). | Say eight, or name the ninth walk intended. |

### 3. Checked, no change

- The proxy key has a spare fourth code, and the three guards, tree capacities, and snapshot tree loops are described accurately (`src/broad_phase.h`, `src/broad_phase.c`, `src/shape.c`, `src/world_snapshot.c`).
- The current pair update, wake/sleep set writes, step join, CCD query order, and Explode collection and sort match the prompt’s source inventory (`src/broad_phase.c`, `src/solver_set.c`, `src/solver.c`, `src/physics_world.c`).
- The runner’s entry-name filter, Debug validation configuration, and benchmark scene names and lengths support the stated commands (`test/main.c`, `test/test_macros.h`, `CMakeLists.txt`, `benchmark/main.c`).

### 4. Proposed edits

Apply the actions in findings 1–7, then make §6 cite only evidence the revised tests actually produce. In particular, record a separate fail-before result for each traversal case and an unmarked-static-neighbour result for the B4 moved-bit check.

### 5. Unresolved

Whether the `QuerySpawn` fixture actually contains a tied closest hit is not established by the prompt’s proposed logging.

### 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the design (U-V2).

1. **Adopted in part (M).** The design's B1 names the public overlap and cast queries,
   `b3World_CollideMover`, `b3World_CastMover`, root bounds and debug draw as the membership
   readers that include the fourth tree, so the increment follows the design; the §6 row claiming
   B0 covers "every tree walk" was the defect. Fix: the row names the walks the design's B0 names.
   Moving the loops into B0 is declined; the reason is in the doc's Provenance (§10).
2. **Adopted (M).** `b3TestPair` (`src/broad_phase.c`) passes a node pair when either side is
   marked, and a shape created with the default def has `invokeContactCreation` true
   (`src/types.c`), which `b3CreateShapeInternal` passes as `forcePairCreation`, so a static box's
   own mark finds the pair. Fix: subtest 2 asserts the moved leaf's bit directly after each
   rebuild and creates the static box with `invokeContactCreation` false.
3. **Adopted (M).** `world->userTreeTask` is set at the enqueue (`src/broad_phase.c`) and cleared at
   the join (`src/solver.c`, `src/physics_world.c`), so it marks the rebuild as outstanding
   deterministically. Fix: the placement asserts it is null, the design's §3 "no tree write
   during the rebuild (validation build)"; the test names that assertion as its evidence. An
   assertion of a construction-guaranteed invariant, not control flow (AC-4).
4. **Adopted (M).** `test/test_world.c` calls `b3Body_SetType` and `b3DestroyBody` between world
   calls, never from a query callback. Fix: two regression cases in the callback test (destroy
   then create into the reused slot; `b3Body_SetType`), marked as passing before the commit in §6.
5. **Adopted (M).** Design §1 permits tied closest-hit casts to differ, and `shared/determinism.c`
   folds the closest ray's fraction and normal into the hash. Fix: a differing ray or cast value
   is a stop that reports every candidate's fraction for the owner to judge against the tie
   exception. The reviewer's Unresolved item (whether the scene has a tie) is the owner's to
   judge from that report, not a claim the doc makes.
6. **Adopted (L).** `ENSURE` returns at the first failure (`test/test_macros.h`), so one entry
   shows one fail-before. Fix: `WakeUnderRebuildTest` and `WakeFromCallbackTest`.
7. **Adopted (L).** Draw plus seven queries is eight. Fix: "eight loops".

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 2: 7 findings (H0-M5-L2), 7 applied (one in part), 1 declined in part.
- Findings: 2/5/1 -> 0/5/2 -> pending
- Tripwire: fixes add or remove a named mechanism: one assertion in the placement (design §3
  names it); no state, guard or branch. Clustering: findings 2, 3, 4 and 6 are in §4's tests, where
  round 1's findings 2, 3 and 4 also were; none is a consequence of a round 1 fix. Reviewer read
  of round records or changelog: none in the raw log. Doc length: 676 -> 705 lines; the fixes add
  three lines (1, with the Provenance section at five), three (2), three (3), eleven (4), four
  (5), one net (6), none (7). Above the split-check length (U-T6): still one decision; no split.
