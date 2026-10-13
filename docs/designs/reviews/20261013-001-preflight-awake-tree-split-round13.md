---
title: Review round 13 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round12.md
mode: full pass
---

# Round 13

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T19:58:31+10:00 to 20:04:47+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split covers the main pair passes and proxy readers, but three paths need to be made explicit before implementation: deferred migrations conflict with validation, and both rewind branches have a tree restoration gap. The empty-tree reset wording also contradicts the stated proxy-capacity and identity rules.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | B0, B3 | B0 makes validation compare a proxy’s tree with its body’s solver set. B3 defers migration after changing that set; `b3WakeBody` and `b3TrySleepIsland` call `b3ValidateSolverSets` before the drain (`src/body.c`, `src/solver_set.c`, `src/physics_world.c`). Validation builds will reject a valid queued transition. | Make validation recognize a queued migration, or run the proxy-membership check after the drain. |
| 2 | M | §5.2, §5.4 | The “nothing more” treatment of the non-awake tree omits §7.4’s restore-time AABB move pass. `b3Body_SetTransform` can change a sleeping proxy’s leaf AABB without creating or destroying its proxy (`src/body.c`). Restoring proxy identity and moved bits alone leaves that leaf at its post-T AABB. | Explicitly retain §7.4’s restored-fat-AABB move pass for the non-awake tree, or journal its AABB writes. |
| 3 | M | B4, §5.2–5.3 | B4 adds `b3World_RebuildSleepingTree`, a direct tree writer in `src/physics_world.c`, but §5.3 routes only the existing static rebuild and solver writes through the rewind boundary. Without a rebuild entry, the no-order-changes branch cannot restore the non-awake tree’s layout after this call. | Route the new rebuild through the journal-aware broad-phase operation and name it in §5.3. |
| 4 | L | B2, §5.1, §5.4 | B2 says an emptied awake tree resets to its “initial state,” while §5.1 says `proxyCapacity` never shrinks and §5.4 relies on proxy-pool allocation order. A full reset would violate both statements. | Define the empty reset as a node-structure reset; specify that proxy capacity, slots, and free-list order are retained. |

## 3. Checked, no change

- The proposed awake/non-awake self and cross passes cover moved sleeping proxies against awake, sleeping, static, and kinematic neighbours (`src/broad_phase.c`).
- Carrying a migrated leaf’s moved bit preserves pending pair discovery; the next pair update precedes the awake-tree rebuild (`src/dynamic_tree.c`, `src/broad_phase.c`).
- The fixed-index sensor, CCD, explosion, query, draw, bounds, counters, and memory-report readers are identified in the proposed changes (`src/sensor.c`, `src/solver.c`, `src/physics_world.c`).
- The section 4 move means and the stated proxy-slot image sizes agree with the table and tree structure sizes (`src/dynamic_tree.c`, `shared/benchmarks.c`).

## 4. Proposed edits

Resolve findings 1–3 in the relevant change descriptions and rewind treatment. Narrow B2’s “initial state” wording to the node state, with the proxy-pool fields it preserves.

## 5. Unresolved

None within the requested scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** `b3WakeBody` calls `b3ValidateSolverSets` right after `b3WakeSolverSet`
   (`body.c`), before any drain. With B0's proxy-type-against-set check there, a queued migration
   fails validation. Fix: that check lives in `b3ValidateBroadPhase`, which the step runs after the
   drain, not in `b3ValidateSolverSets`.
2. **Adopted (M).** `b3Body_SetTransform` on a sleeping body moves its proxy to a new fat AABB
   without a create or destroy (`body.c`); proxy identity and moved bits do not restore the leaf.
   Fix: 5.2's tree-order branch states the non-awake tree gets exactly the rewind design's
   section 7.4 treatment (identity journal, restored-fat-AABB move pass, moved-bit restore); what
   the split removes is that treatment for the awake tree.
3. **Adopted (M).** `b3World_RebuildSleepingTree` would be a direct tree write outside
   `broad_phase.c`. Fix: B4 implements it as a broad-phase function that `physics_world.c` calls;
   5.3 names it with the static rebuild.
4. **Adopted (L).** "Reset to its initial state" would shrink `proxyCapacity` and reorder ids.
   Fix: B2 defines the reset as node state only (root empty, `nodeEnd` 2, pair free list empty,
   ordered), with proxy slots, capacity and free-list order retained.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 13: 4 findings (H0-M3-L1), 4 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no (one validation check relocated). Clustering:
  findings 1 and 4 follow from round 11's queue and round 7's reset (U-T2); finding 2 narrows a
  claim that has been cut back in rounds 2, 11 and 13 (U-T5: the 5.2 branch is now stated as
  "exactly section 7.4", which should end the series of omissions). Reviewer read of round
  records or changelog: none in the raw log. Doc length: 369 -> 378 lines.
