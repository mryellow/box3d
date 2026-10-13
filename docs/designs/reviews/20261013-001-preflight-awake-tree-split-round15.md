---
title: Review round 15 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round14.md
mode: full pass
session: 01a134c4-0c8d-7c73-8b0d-ab04e5fb208f (resumed)
---

# Round 15

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 124 (work timeout after a long trace of real reads),
2026-10-13T20:17:05+10:00 to 20:26:05+10:00. Resumed per the workflow with `-m gpt-6-sol -c
model_reasoning_effort="high" --sandbox read-only resume <session>` and the instruction to finalize
from the research already done with no further tool calls; exit 0, 20:26:19 to 20:26:45. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high` on both calls.

## Review report (Codex final message)

## 1. Summary

The proposed tree membership, pair passes, and migration timing largely match the live source. Two defects remain: the non-awake tree can outgrow fixed traversal stacks, and the rewind journal estimate treats migration counts as though they covered other non-awake tree writes.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 4, B4 | B4 allows unbounded non-awake tree height and proposes measuring it. Traversals in `src/dynamic_tree.c` and pair walks in `src/broad_phase.c` use 512-entry stacks; when full, release builds skip branches. Sustained churn can therefore omit query hits or collision pairs. | Provide a correctness guarantee: grow the traversal stacks, or enforce a tree bound before they can overflow. Height reporting alone is insufficient. |
| 2 | M | 5.2 | The claim that section 4 gives proxy moves per tick understates the physical journal workload. Its endpoint scan counts awake-state flips; it misses writes to a sleeping proxy that do not wake it, including `b3Body_SetTransform` (`src/body.c`) and proxy resets (`src/shape.c`). Those writes still mutate and journal the non-awake tree. | Label section 4 as migration churn only, and size the physical journal using all non-awake create, destroy, move, and moved-bit writes. |

## 3. Checked no change

- The two-bit proxy key has a fourth code available; the body-type range remains three (`src/broad_phase.h`, `include/box3d/types.h`).
- The proposed self and cross passes cover the current dynamic pair combinations, including moved sleeping proxies and static compounds (`src/broad_phase.c`, `src/contact.c`).
- Sleep and wake currently leave proxies untouched; carrying a leaf’s moved bit across migration preserves that behavior (`src/solver_set.c`, `src/solver.c`).
- Joining the tree task before draining queued wakes places newly awake proxies before bullet CCD (`src/physics_world.c`, `src/solver.c`).
- The section 5.1 image-size arithmetic matches the 32-byte node, 4-byte parent, and 24-byte proxy slot sizes (`src/dynamic_tree.c`).

## 4. Proposed edits

- In B4, specify how every non-awake traversal remains complete when the tree grows deep.
- In sections 4 and 5.2, distinguish measured sleep/wake migrations from the full set of writes charged to the physical journal.

## 5. Unresolved

The worst-case non-awake tree shape under sustained churn, and the total physical journal volume from non-migration writes, remain unmeasured.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted in part (H).** `B3_TREE_STACK_SIZE` is 512 and every traversal and pair walk skips a
   branch when its stack is full in a release build (`dynamic_tree.h`, `dynamic_tree.c`,
   `broad_phase.c`). The exposure is not new: the static tree is maintained today exactly as B4
   maintains the non-awake tree, by rotation-balanced inserts (`b3CreateTreeProxyInternal` rotates)
   with no step rebuild, and relies on its height staying far below the stacks. Fix: B4 states
   that the non-awake tree inherits the static tree's reliance and that the height measurement
   under churn is what confirms it. Growing the stacks or bounding the tree is declined as a change
   to a settled primitive the static tree already depends on (U-P11); reasoning in Provenance.
2. **Adopted (M).** The §4 harness counts awake-state flips; a `b3Body_SetTransform` or
   `b3ResetProxy` on a sleeping body writes the non-awake tree without one. Fix: §4 is labelled
   migration churn, and 5.2 says the byte journal is also charged for every write in B2's list,
   which §4 does not measure.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 15: 2 findings (H1-M1-L0), 2 applied (1 in part), 0 declined outright.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: both findings land on text
  added in rounds 3 and 14 (the height measurement and the moves-per-tick reference); neither is a
  consequence of a fix, both are qualifications. Reviewer read of round records or changelog: none
  in the raw log. Doc length: 386 -> 392 lines.
