---
title: Review round 26 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round25.md
mode: full pass
---

# Round 26

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:58:53+10:00 to 22:02:39+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13521-41a1-78a1-8335-5d459ccbe913`.

## Review report (Codex final message)

### 1. Summary

The split covers the main tree readers and preserves the existing pair-set and moved-bit model. I found a false cost condition, a traversal hazard introduced by the proposed height rebuild, and an incorrect stack-depth explanation.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | Opening; 2 B1 | “No write to a sleeping body’s proxy” does not imply awake-only pair work. `b3Joint_SetCollideConnected` in `src/joint.c` can mark a sleeping proxy moved without writing it; B1 then gathers its non-awake siblings. | Qualify the cost claim by moved marks as well as proxy writes. |
| 2 | M | 2 B3–B4 | B4 says non-awake inserts occur only outside traversals, but `b3Body_SetTransform` in `src/body.c` can move a sleeping proxy from a query callback. B3 explicitly leaves that path unguarded. B4 adds a possible full rebuild during that walk, while `b3DynamicTree_Query` in `src/dynamic_tree.c` retains its node-array pointer. | Defer the height-triggered rebuild until the outer traversal exits, or specify a guard for this write path; correct the traversal-safety claim. |
| 3 | L | 2 B4 | The cross-pair stack does not grow by three entries per level of the *deeper* tree. In `src/broad_phase.c`, reaching a leaf on either side switches to `b3CollideProxyAndSubtree`, which uses a separate stack. The “near 170” explanation therefore overstates this walk’s depth for a shallow awake tree. | Describe the two stack limits separately and retain the threshold on its actual conservative basis. |

### 3. Checked, no change

- The proxy key has two tree-code bits; the body-type count is three (`src/broad_phase.h`, `include/box3d/types.h`).
- Sleep and wake currently leave proxies in place; contact creation can place a sleeping–sleeping contact in the disabled set (`src/solver_set.c`, `src/contact.c`).
- Pair gathering, rebuild, and refit behave as described for moved marks and `nodeEnd` (`src/broad_phase.c`, `src/dynamic_tree.c`).
- The cited fixed-index readers, sensor queries, CCD bullet queries, explosion query, and end-of-step ordering are present as described.
- The cited tree-order preflight’s CCD and explosion changes support the stated layout-order caveat (cited design, sections 1–2).

### 4. Proposed edits

Apply the actions in findings 1–3 to the opening cost claim and B3–B4. In B4, also specify where the promised triggered-rebuild count is recorded: B0 adds `sleepingTreeHeight`, but no rebuild-count field.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** `b3Joint_SetCollideConnected` (`joint.c`) calls
   `b3BroadPhase_MarkProxyMovedSerial` on the joint's bodies' shapes without waking them, and B2
   already lists it as a mark on a sleeping proxy. The intro's cost condition named writes only.
   Fix: "no write to or moved mark on a sleeping body's proxy".
2. **Adopted (M).** B3 leaves `b3Body_SetTransform` from a query callback as today's unguarded
   write, and revision 27's B4 claimed every non-awake insert happens outside a traversal. Fix:
   the insert-time height test is skipped while the traversal depth is nonzero, and the drain at
   the outermost exit tests once after its own inserts; the callback move stays today's hazard
   without a rebuild added to it. B4's traversal claim corrected.
3. **Adopted (L).** `b3VisitPair` (`broad_phase.c`) hands a leaf on either side to
   `b3CollideProxyAndSubtree`, which walks the other subtree with its own stack, one net entry per
   level. The pair stack grows by up to three per level only while both sides are internal, so
   its depth is bounded by the shallower tree. Fix: B4 describes the two stacks; the threshold's
   basis is the single-walk limit on the non-awake side, with the pair-stack limit reached only
   when both trees are deep.
4. **Adopted (proposed edit).** The rebuild count B4 cites had no field. Fix: B0 adds
   `sleepingTreeRebuildCount` to `b3Counters` (`types.h`), a world counter the rebuild function
   increments, diagnostic like the other counters.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 26: 3 findings (H0-M2-L1), 3 applied plus one proposed edit, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1
- Tripwire: fixes add or remove a named mechanism: no; a counter field and a depth-gated test
  on an existing choke point. Clustering: B4 (findings 2, 3), seventh round; finding 2 is a
  consequence of revision 27's move of the test to the insert; finding 3 corrects a number
  revision 25 introduced; finding 1 is the intro's cost claim, first raised. Reviewer read of
  round records or changelog: none in the raw log. Doc length: 345 -> 358 lines at revision 27.
