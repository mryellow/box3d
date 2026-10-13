---
title: Review round 39 - preflight awake tree split (confirmation)
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round38.md
mode: confirmation round (U-P9), same prompt and scope, doc unchanged since round 38
---

# Round 39

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:18:08+10:00 to 23:22:32+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13569-cec1-7a53-b0c6-9b1b37eb023f`.

## Review report (Codex final message)

### 1. Summary

The split accounts for the fixed tree readers, proxy keys, moved bits, pair passes, and the tree task join. Two claims need correction: the height check can be deferred across multiple proxy writes, and the solve is not strictly proportional to awake bodies.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §2 B3–B4 | Height checks are skipped while traversal depth is nonzero, but a public query callback can call `b3Body_SetTransform` repeatedly on sleeping bodies. `src/body.c` moves those proxies immediately. A later tree walk in the **same public call** can therefore see a non-awake tree well above 64; the assertion that a rebuild sees at most one level over the threshold is also false. `src/physics_world.c`, `src/dynamic_tree.c` | Specify how walks remain safe after deferred height checks, or defer those proxy writes. Remove the “at most one over” claim. |
| 2 | L | §1 | “Collide and solve are O(awake)” is too broad. Bullet CCD in `src/solver.c` queries dynamic proxies, including sleeping bodies, so one awake bullet can traverse work that scales with the sleeping population. | Limit the claim to awake body and contact processing; call out CCD tree traversal separately. |

### 3. Checked, no change

- The two proxy-key bits accommodate a fourth tree; the cited fixed-index readers and tree-count loops need the updates listed in B0–B3.
- The proposed self and cross passes cover moved sleeping proxies, including static compounds; clearing their moved bits after pair discovery is necessary.
- Carrying leaf moved bits through migration preserves pending pair searches. The tree-task join precedes finalize and bullet CCD, while sleep occurs after those stages.
- The churn table’s move arithmetic and its step-boundary counting rationale agree with the cited benchmark and sleep paths. The cited tree-order preflight supports the stated order-dependent exceptions.

### 4. Proposed edits

Revise B4’s height guarantee to cover writes made inside callbacks and subsequent walks in the same public call. Qualify the §1 complexity statement as described above.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M), as a qualification.** `b3Body_SetTransform` from a callback moves the proxy at
   once (`body.c`), the test is deferred while the depth is nonzero, and a public call's later
   walks and the exit rebuild see every such move. Each unrotated insert raises the height by at
   most one, the walks in a public call are single walks (`b3DynamicTree_Query`, the casts), and
   the rebuild's gather walk pushes one net per level, so a call stays within the 512-entry
   stacks up to about 430 callback moves of sleeping bodies. Fix: B4 states the height at the
   exit as at most one over the threshold plus one per such move, and that bound. The write is
   today's unguarded one (B3), and deferring it is out of this design's scope by owner decision.
2. **Adopted (L).** `b3SolveContinuous` (`solver.c`) queries both dynamic trees for a bullet,
   and the sensor pass walks trees, each a tree query, O(log of the tree plus hits). Fix:
   section 1 qualifies the O(awake) claim.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round found defects, so round 38's clean round
  does not stand (U-P9); a fresh clean round and confirmation are needed (U-S8).
- Round 39: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0 -> 0/0/0 -> 0/1/1
- Tripwire: fixes add or remove a named mechanism: no. Clustering: B4 (finding 1, consequence
  of revision 36's "at most one over" sentence); §1 (finding 2, first raised). Fourth
  confirmation round to find what a clean round missed (28, 34, 37, 39). Reviewer read of
  round records or changelog: none in the raw log. Doc length: 398 lines, unchanged this round.
