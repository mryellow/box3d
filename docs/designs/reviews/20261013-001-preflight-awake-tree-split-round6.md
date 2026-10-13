---
title: Review round 6 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round5.md
mode: full pass
---

# Round 6

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T18:44:27+10:00 to 18:51:27+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s pair-pass coverage and moved-bit migration are consistent with the live source. I found two callback traversal hazards, an incorrect rewind journal coverage claim, and two places where the document’s stated behavior is inconsistent.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2, B3 | `b3World_Draw` also traverses trees while invoking host callbacks through `DrawQueryCallback`. With the world unlocked, a callback can wake a sleeping body and migrate a leaf during that traversal (`src/physics_world.c`). B3 protects overlap and cast queries but misses this reader. | Apply the chosen traversal guard or deferred migration rule to the debug-draw tree walk too. |
| 2 | H | 2, B3 | Holding `world->locked` does **not** reject `b3Body_SetTransform`: it checks the lock only with `B3_ASSERT`, which disappears in release builds (`src/body.c`, `include/box3d/base.h`). A query callback can still move a proxy while `b3DynamicTree_Query` holds its node array. | Add a runtime lock check to `SetTransform`, or defer its proxy write during traversal. |
| 3 | M | 5.2, 5.4 | The physical journal records proxy slots and free-list scalars, while §5.4 says the journal **gains** those entries alongside the existing proxy-identity entries. Reversing both for one non-awake create or destroy would undo its pool change twice. | Specify which entry owns proxy allocation and removal in the physical-journal branch, and suppress the other undo path. |
| 4 | M | 5.3 | The claim that moved-bit writes go through `broad_phase.c` is false: `b3BroadPhase_MarkProxyMovedSerial` and `b3BroadPhase_MarkProxyMoved` are inline writers in `src/broad_phase.h`, called from `src/joint.c` and `src/solver.c`. | Move these wrappers into the journal choke point, or explicitly account for their direct writes. |
| 5 | M | 1, 2 B3, 3 | Without the tree-order changes, B3 collects explosion hits from two trees but does not sort the combined list. Wake and impulse order can therefore change, as §1 acknowledges, so §3’s unconditional “same wake set and velocities as `main`” assertion is too strong. | Make that assertion conditional on A3, or state the narrower expected result without it. |

## 3. Checked, no change

- The proposed pair passes cover the relevant awake, non-awake, static, and kinematic combinations when a root is marked moved (`src/broad_phase.c`).
- Sleep and wake currently leave proxies untouched; preserving a leaf’s moved bit during migration preserves pending pair discovery (`src/solver_set.c`, `src/dynamic_tree.c`).
- The cited fixed-index readers, capacity and memory accounting, sensor query, CCD query, and explosion query require the updates identified in B0–B3.
- The awake-tree image fields and approximate 100 B per proxy-slot estimate match the tree representation. The section 4 churn counts are consistent with the stated endpoint-scan method and the benchmark scene definitions.

## 4. Proposed edits

Revise B3 to cover debug draw and enforce the `SetTransform` lock at runtime. Clarify the mutually exclusive rewind undo paths in §5.2–5.4, correct the moved-bit writer inventory in §5.3, and qualify the explosion result in §3.

## 5. Unresolved

None beyond the findings above.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3World_Draw` queries each tree with `DrawQueryCallback` and sets no lock
   (`physics_world.c`). Fix: B3's traversal rule covers the debug-draw walk.
2. **Adopted (H).** `b3Body_SetAwake` gets the world through `b3GetUnlockedWorld`, which refuses a
   locked world at runtime; `b3Body_SetTransform` uses `b3GetWorld` and only asserts
   (`body.c`). Fix: B3 says the lock rejects `SetAwake` at runtime and `SetTransform` only in a
   validation build, so a `SetTransform` from a callback stays today's hazard, unchanged by the
   split.
3. **Adopted (M).** Under the byte journal a non-awake create or destroy would be undone twice,
   once by the proxy-identity entry and once by the bytes. Fix: 5.2 states the byte journal
   subsumes the proxy-identity entries for the trees it covers, and 5.4 says the journal gains the
   byte entries in place of, not beside, those.
4. **Adopted (M).** `b3BroadPhase_MarkProxyMovedSerial` and `b3BroadPhase_MarkProxyMoved` are
   inline in `broad_phase.h`, called from `joint.c` and `solver.c`. Fix: 5.3 names them and says
   they move out of line under the byte journal.
5. **Adopted (M).** Without A3's sort the combined explosion list is in traversal order. Fix: the
   §3 explosion test asserts equality with `main` with the tree-order changes landed, and the same
   wake set without them.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 6: 5 findings (H2-M3-L0), 5 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B3 again (rounds 1, 3, 5, 6),
  and findings 1 and 2 follow from round 5's lock fix (U-T2): the fix named two readers and one
  guard and missed a third reader and the guard's build dependence. Reviewer read of round records
  or changelog: none in the raw log. Doc length: 311 -> 328 lines; round 5's fixes added 17 lines,
  of which 5.4's id-sequence paragraph and the Provenance bullet are 11.
