---
title: Review round 2 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round1.md
mode: full pass
---

# Round 2

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T18:08:07+10:00 to 18:16:26+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split is workable in principle, but the document misses a sleeping-body shape-creation path and specifies moved-bit behavior that the current proxy-creation API cannot provide. It also overstates when results and tree work remain unchanged. I recommend revising those claims before implementation.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2, B2 | Shape creation on a sleeping dynamic body currently chooses `body->type`, not its solver set (`src/shape.c`). Leaving that path unchanged puts the new proxy in the awake tree, contradicting B2’s membership rule. The claim that the enable path chooses by set is also false, although enable transfers the body to the awake set first (`src/body.c`). | Make shape creation select the non-awake code for sleeping dynamic bodies; state which paths actually require a change. |
| 2 | M | 2, B2 | B2 says a migrated leaf with a clear moved bit stays clear, but `b3BroadPhase_CreateProxy` marks every non-static creation moved (`src/broad_phase.c`). The proposed migration cannot preserve that bit through the current API. | Add an explicit migration creation mode that takes the old leaf’s moved bit; retain normal marking for other creates. |
| 3 | M | 1, 2 B2 | Marking every woken proxy moved changes contact timing beyond the bullet and explosion cases named in §1. For example, create an overlapping static shape without forced pair creation, then wake an otherwise unmoved sleeping dynamic body: the split searches that pair on the next step, while the current wake touches no tree (`src/broad_phase.c`, `src/shape.c`, `src/solver_set.c`). | State this behavior change, or preserve the pre-wake moved state and justify how newly needed pairs are found. |
| 4 | M | Opening, 1, 4 | “Tree work … proportional to the awake population” is too strong. A moved awake proxy whose bounds overlap a large non-awake tree makes the awake/non-awake cross pass visit that sleeping population, even when no sleeping proxy was written (`src/broad_phase.c`). | Limit the claim to awake-tree gather, rebuild and refit; describe cross-query cost separately. |
| 5 | M | 4 | The scratch method counts awake-state differences at scans, not necessarily proxy migrations. `StepSleep` can wake an island before a step, and the solver can sleep it at step end; an asleep-to-awake-to-asleep interval has two migrations but no endpoint flip (`shared/benchmarks.c`, `src/solver.c`). This method cannot establish the table’s claimed move totals. | Count sleep and wake transitions, or proxy removals and inserts, in the harness; label the present figures as endpoint flips if retained. |
| 6 | L | 5.2, 5.4 | §5.2 permits the restored non-awake layout to differ from T, while §5.4 says the state hash covers both trees. A hash of physical nodes or free pairs would then fail despite the intended layout-independent restore. | Specify a logical tree hash—membership, proxy identities, AABBs and moved state—for that case; reserve a physical-layout hash for the byte-journal case. |
| 7 | L | 2 B0, 5.3 | The tree-reader and writer inventory is incomplete. `b3ValidateNoMoved` still loops over the three body types (`src/broad_phase.c`), and §5.3’s claim that rebuilds and tree writes go through `broad_phase.c` is false for the direct calls in `src/physics_world.c` and `src/solver.c`. | Include the fourth tree in moved-state validation and name the actual journaling hook or required wrapper for each direct tree write. |

## 3. Checked, no change

- The two low proxy-key bits can encode a fourth tree while the body-type count remains three (`src/broad_phase.h`, `include/box3d/types.h`).
- The proposed self and cross pair passes cover awake, non-awake, static and kinematic neighbors when either relevant root is moved (`src/broad_phase.c`).
- Contact creation can place a pair of non-awake bodies in the disabled set; the pair set culls existing contacts (`src/contact.c`, `src/broad_phase.c`).
- Joining the rebuild before begin-touch wake processing addresses the identified tree-write race. Bullet CCD and explosion each need the additional non-awake traversal (`src/physics_world.c`, `src/solver.c`, `src/island.c`).
- The awake image’s stated contents and roughly 100 B per proxy-slot estimate agree with the tree fields and element sizes (`src/dynamic_tree.c`, `include/box3d/types.h`).

## 4. Proposed edits

Revise B2’s shape-creation path and proxy-creation API; qualify §1’s behavior and scaling claims; measure migrations directly for §4; and define the tree hash and journaling boundaries in §5.

## 5. Unresolved

The scratch harness was unavailable, so the table’s numeric rows could not be independently reproduced from the cited source.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3CreateShapeInternal` (`shape.c`) passes `body->type` as the proxy type, and
   `b3Body_Enable` (`body.c`) transfers the body to the awake set and then creates proxies with
   `body->type`. Fix: B2 states that shape creation selects the tree from the body's set (a
   sleeping dynamic body gets code 3) and that enable needs no change because the body is awake
   when its proxies are created.
2. **Adopted (M).** `b3BroadPhase_CreateProxy` computes the moved mark itself; only
   `b3CreateTreeProxyInternal` takes one. Fix: B2 names a migration entry point in
   `broad_phase.c` that takes the old leaf's moved bit; every other create keeps today's marking.
3. **Adopted (M).** A static shape created without forced pair creation over a sleeping body makes
   no pair today until that body's proxy moves; a wake that marks every proxy moved would find it
   on the next step, a timing change. Fix: migration carries the moved bit in both directions, so a
   wake re-finds nothing, exactly as today, and the sentence about re-finding the woken island's
   pairs is deleted. Its contacts persist in the sleeping set.
4. **Adopted (M).** The awake-against-non-awake cross pass visits every sleeping node the moved
   awake proxy's box overlaps, as the single tree's self pass does today. Fix: the claim is
   limited to gather, rebuild and refit; the cross pass costs what the single tree's self pass
   costs for the same overlaps.
5. **Declined (M).** `b3WakeSolverSet` sets `body->sleepTime = 0.0f` (`solver_set.c`), sleep
   requires `sleepTime >= B3_TIME_TO_SLEEP` (0.5 s, `constants.h`), the timer advances one time
   step per `b3World_Step` (`solver.c`), and the sleep pass is the last thing the solve does. An
   island cannot sleep and wake, or wake and sleep, inside one step, so the awake state at
   consecutive scans sees every transition and the endpoint flips are the migrations. Reasoning
   added to §4 and to a new Provenance section (U-C4, U-V10).
6. **Adopted (L).** With a derived non-awake layout the physical bytes legitimately differ from T.
   Fix: 5.4 says the state hash is logical for that tree (membership, proxy ids, AABBs, moved
   bits) and physical only under the byte journal.
7. **Adopted (L).** `b3ValidateNoMoved` loops over `b3_bodyTypeCount`; `b3World_RebuildStaticTree`
   (`physics_world.c`), `b3DynamicTree_Refit` and `b3DynamicTree_EnlargeProxy` (`solver.c`) write
   trees directly. Fix: B0 adds the validation loop; 5.3 names the direct writers and that the
   rewind design's section 7.4 moves them into `broad_phase.c`.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 2: 7 findings (H1-M4-L2), 6 applied, 1 declined (finding 5, reasoning in Provenance).
- Findings: 4/3/0 -> 1/4/2 -> pending
- Tripwire: fixes add or remove a named mechanism: one entry point named (migration create taking
  the moved bit); the "woken pairs re-found" rule removed. Clustering: B2 produced findings in both
  rounds, and findings 2 and 3 follow from round 1's moved-bit fix (U-T2); the fix this round
  removes the rule rather than adding one. Reviewer read of round records or changelog: none in the
  raw log. Doc length: 256 -> 277 lines; round 1's fixes added 21 lines.
