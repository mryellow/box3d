---
title: Review round 2 - island-scoped rollback facts
target: 20261012-001-island-scoped-rollback-facts.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261012-001-island-scoped-rollback-facts-round1.md
mode: full pass
---

# Round 2

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T17:31:34+10:00 to 17:37:39+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`. The last copy of the final message is
below.

## Review report (Codex final message)

## 1. Summary

Section 10 identifies the main tree-order risks, but its proposed split leaves a gap in pair discovery and an incomplete undo path when a cold tree grows. Both can affect the bit-exact rewind goal. The reported tree counters also need to account for the fourth tree.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 10, pair discovery | The listed passes omit non-awake dynamic versus static and kinematic pairs, and treat the non-awake self pass as optional. Today, moving a sleeping proxy can discover each of those pairs; the filter does not reject a pair merely because both bodies sleep (`src/broad_phase.c`, `src/contact.c`). A sleeping `b3Body_SetTransform` can therefore produce a contact today that the described split would miss. | State every pass needed to preserve current pair discovery, including non-awake self and non-awake versus static/kinematic passes. |
| 2 | H | 10.2 | The physical journal records old array bytes and scalars, but growth replaces the `nodes`, `parents`, or `proxies` allocation (`src/dynamic_tree.c`). Replaying bytes backward cannot restore a cold tree’s allocation at T. Restoring an old capacity scalar against a later allocation also gives `b3Free` the wrong block size; retaining the later proxy capacity with T’s free-list head changes subsequent proxy IDs. | Specify undo entries or ownership handling for allocation growth, with matching capacity and free-list restoration. |
| 3 | L | 10, membership readers | Fixed-index readers are missing from the fourth-tree inventory. `b3World_GetCounters`, step maximum-capacity tracking, the memory report, and broad-phase validation address the existing trees directly (`src/physics_world.c`, `src/broad_phase.c`); the new tree would be omitted from their results. | Add these readers to the migration inventory and define their fourth-tree accounting. |

## 3. Checked, no change

- Sleep and wake currently leave proxies in their body-type trees; the proxy key has room for a fourth tree code (`src/solver_set.c`, `src/broad_phase.h`).
- CCD and explosion have the stated layout-dependent behavior. Pair keys and sensor overlaps are sorted as described (`src/solver.c`, `src/physics_world.c`, `src/broad_phase.c`, `src/sensor.c`).
- The stated moved-bit, rebuild, and proxy-identity mechanics match the source and rewind-only design §7.4.
- The section 10.5 means and worst-tick byte ranges are consistent with the table and its stated counting method; I found no demonstrated error in that method for these measured scenes.
- The earlier sections’ island, solver-order, and rewind-only claims are consistent with the cited source and cited design sections.

## 4. Proposed edits

Revise the pair-pass paragraph in section 10 and the allocation-undo description in 10.2. Add the fixed-index readers to section 10’s reader inventory.

## 5. Unresolved

The split has no implementation, so its restore path cannot yet be checked by execution. Whether kinematic proxies join the split remains an open question in the document.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3CreateContact` (`contact.c`) puts a contact whose bodies both sleep into the
   disabled set; nothing in `b3ShouldCreatePair` rejects a sleeping pair. Today a sleeping proxy
   moved by `b3Body_SetTransform` is in the dynamic tree, so the self pass and both cross passes
   find its new overlaps against sleeping, awake, static and kinematic proxies. Fix: the §10
   pair-discovery bullet states every pass that reproduces this (awake self; awake against
   non-awake, static and kinematic; and, when the non-awake root is marked moved, non-awake self
   and non-awake against static and kinematic), and that waking the body on `SetTransform` is the
   engine's alternative.
2. **Adopted (H).** `b3AllocateSiblingPair` and `b3AllocateProxy` grow by `B3_GROW_ZERO`, which
   frees the old block with its old size, and `b3Free` passes that size to a user free function
   (`core.c`). Node indices are unchanged by growth and `nodeEnd` is journaled, so node capacity is
   not state; proxy growth builds the free list from the old capacity, so proxy capacity is state
   (the same reason §10.1 gives for the image). Fix: §10.2 gains a growth entry (tree, array, old
   capacity) whose undo reallocates to the old capacity, and says which capacity is state and why.
3. **Adopted (L).** `b3World_GetCounters`, the step's capacity tracking, the memory report and
   `b3ValidateBroadPhase` address trees by fixed index (`physics_world.c`, `broad_phase.c`). Fix:
   named in the membership-readers bullet.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 2: 3 findings (H2-M0-L1), 3 applied, 0 declined.
- Findings: 3/2/2 -> 2/0/1 -> pending
- Tripwire: fixes add or remove a named mechanism: one journal entry kind added (growth). Clustering:
  findings 1 and 3 land in the §10 bullets round 1 added (pair discovery, membership readers), and
  finding 1 is a consequence of round 1's fix stating the cross pass incompletely (U-T2). Reviewer
  read of round records or changelog: none in the raw log. Doc length: 351 -> 372 lines; round 1's
  fixes added 21 lines, two bullets and four clause edits.
