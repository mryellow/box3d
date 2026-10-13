---
title: Review round 3 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round2.md
mode: full pass
session: 01a13457-7403-7852-9ac7-5ee44c5a427d (resumed)
---

# Round 3

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 124 (work timeout after a long trace of real reads),
2026-10-13T18:18:28+10:00 to 18:27:28+10:00. Resumed per the workflow with `-m gpt-6-sol -c
model_reasoning_effort="high" --sandbox read-only resume <session>` and the instruction to finalize
from the research already done with no further tool calls; exit 0, 18:27:41 to 18:28:21. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high` on both calls.

## Review report (Codex final message)

## 1. Summary

The split’s pair-pass and migration approach is plausible, but the document leaves a correctness gate and fixed-tree paths insufficiently specified. Its rewind cost bound and two behavior and complexity claims also need correction.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 5.1, B0 | The awake-tree image is **not bounded by the most proxies awake at once** under the current allocation rule. `b3CreateBroadPhase` starts the dynamic tree at `capacity->dynamicShapeCount`, and 5.1 images every proxy slot. A large capacity hint can therefore make captures proportional to the anticipated dynamic population even when only a few bodies are awake (`src/broad_phase.c`, `src/dynamic_tree.c`). | Specify a small initial awake-tree capacity and assign the capacity hint appropriately; revise the bound. |
| 2 | M | B1 | The listed passes do not explicitly change `b3UpdateBroadPhasePairs`’s early `needUpdate` gate. It currently checks the static, kinematic and dynamic trees only; a write that moves only a non-awake proxy would return before the proposed passes or moved-bit clear (`src/broad_phase.c`). | For the pass-based option, add the non-awake root’s moved bit to that gate. |
| 3 | M | 1, 5.2 | Section 1 says results become independent of tree membership after the tree-order changes, while 5.2 acknowledges that stateful custom filters and `preSolve` callbacks still see layout-dependent call order. A stateful filter can change which contacts are accepted (`src/broad_phase.c`, `src/solver.c`). | Qualify section 1’s result and golden-hash claims with the callback exception. |
| 4 | L | B0–B1 | The fixed-tree inventory misses the fourth tree’s construction and destruction paths, and the step’s separate recording-bounds walk still loops over `b3_bodyTypeCount` (`src/broad_phase.c`, `src/physics_world.c`). Left as specified, the tree can be left uninitialised or unfreed, and recorded bounds omit sleeping shapes. | Name these paths explicitly and use a proxy-tree count for all four-tree walks. |
| 5 | L | 4, B4 | “O(log n) each” is not guaranteed for a move. Sibling search follows a tree path, removal does not rebalance, and insertion rotations enforce no height bound; with no routine non-awake rebuild, cost is O(tree height) (`src/dynamic_tree.c`). | State the height-dependent bound and measure height under churn. |

## 3. Checked, no change

- The proxy key has an unused fourth two-bit code (`src/broad_phase.h`).
- Carrying moved bits across sleep and wake preserves pending pair searches; non-static proxy creation marks new leaves (`src/dynamic_tree.c`, `src/broad_phase.c`).
- The proposed self and cross passes cover awake, non-awake, static and kinematic neighbours once entered; contact creation can place a sleeping–sleeping contact in the disabled set (`src/broad_phase.c`, `src/contact.c`).
- The rebuild/wake race and explosion traversal mutation described in B3 follow from the current step and wake paths (`src/physics_world.c`, `src/solver.c`, `src/solver_set.c`).
- The section 4 move totals checked against scene shape counts and step counts are arithmetically consistent (`shared/benchmarks.c`). I found no defect in the stated endpoint-scan method.
- The roughly 100-byte tree-image estimate per occupied proxy slot is consistent with the node, parent and proxy sizes (`include/box3d/types.h`).

## 4. Proposed edits

Apply the five actions in the findings table. In B1, state the early gate alongside the pass list so a non-awake-only move is an explicit case.

## 5. Unresolved

None within the review scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3CreateBroadPhase` creates the dynamic tree with `capacity->dynamicShapeCount`
   and `b3DynamicTree_Create` allocates `proxies` to that capacity and threads the free list through
   every slot (`broad_phase.c`, `dynamic_tree.c`). Imaging `proxies` to capacity is therefore
   O(capacity hint) before anything is awake. Fix: B0 gives the capacity hint to the non-awake tree
   and starts the awake tree at the 16-slot floor; 5.1's bound becomes the awake tree's high-water
   mark, which starts at that floor.
2. **Adopted (M).** `b3UpdateBroadPhasePairs` returns early unless the static root is moved or the
   kinematic or dynamic tree needs a rebuild. Fix: B1 adds the non-awake root's moved bit to that
   gate.
3. **Adopted (M).** §1 claimed results independent of membership with the tree-order changes; 5.2
   already carries the stateful-callback exception. Fix: §1 qualified.
4. **Adopted (L).** `b3CreateBroadPhase` has `_Static_assert( b3_bodyTypeCount == 3 )`,
   `b3DestroyBroadPhase` and the recording-bounds walk loop over `b3_bodyTypeCount`
   (`broad_phase.c`, `physics_world.c`). Fix: B0 names them and states that every tree walk uses a
   proxy-tree count of four.
5. **Adopted (L).** `b3RemoveLeaf` does not rebalance, and insert rotations bound nothing;
   `b3FindBestSibling` follows a path. Fix: §4 states O(height) per operation and that the PR
   measures the non-awake tree's height under churn.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 3: 5 findings (H1-M2-L2), 5 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B0's fixed-index inventory drew
  findings in all three rounds (U-S4 would fire again; owner directed the series to convergence,
  U-S5), and finding 1 is a consequence of round 1's high-water fix stating the bound incompletely
  (U-T2). Reviewer read of round records or changelog: none in the raw log. Doc length: 277 -> 298
  lines; round 2's fixes added 21 lines, of which the Provenance section is 10.
