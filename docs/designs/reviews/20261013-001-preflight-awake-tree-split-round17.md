---
title: Review round 17 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round16.md
mode: full pass
---

# Round 17

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T20:36:28+10:00 to 20:42:15+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s tree membership, pair passes, and main reader changes match the cited source. I found one release-blocking staging hazard and two defects in the rewind section’s journal description.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §2, B2–B3 | B0–B2 form the first PR, but B2 migrates proxies immediately. A wake during `b3Collide` can then write the dynamic tree while its rebuild task is running; `b3World_Explode` can migrate a leaf during its tree query. B3 describes both hazards but arrives in the second PR (`src/broad_phase.c`, `src/solver.c`, `src/island.c`, `src/physics_world.c`). | Land B3’s deferral and drain with B2, or keep migration inactive until B3 lands. |
| 2 | M | §5.2 | The byte-journal estimate counts roughly one tree-height path of 36-byte writes per proxy move. `b3DynamicTree_MoveProxyInternal` removes **and** inserts the leaf, updating ancestor paths in both operations; insert rotations add writes (`src/dynamic_tree.c`). The stated 0.5–1 KB at 10,000 proxies is therefore an optimistic journal budget. | Count both paths and rotation writes; measure bytes per move under the §4 churn. |
| 3 | L | §§5.2, 5.4 | The byte-journal branch says proxy growth is undone by reallocating to the old capacity. §5.4 then says a rewind leaves T’s free list followed by slots from discarded growth. Those slots cannot remain in the byte-journal branch. | Make §5.4’s appended-slot explanation specific to the proxy-identity journal branch; state that byte-journal undo restores the old capacity. |

## 3. Checked, no change

- The two-bit proxy key has a fourth code available; the body-type enum remains at three (`src/broad_phase.h`, `include/box3d/types.h`).
- The proposed self and cross pair passes cover awake, sleeping, kinematic, and static neighbours, including static compounds (`src/broad_phase.c`).
- The fixed-index CCD and explosion reads, sensor queries, public tree walks, counters, and capacity tracking need the changes identified in B0–B3 (`src/solver.c`, `src/sensor.c`, `src/physics_world.c`).
- Carrying a migrated leaf’s moved bit preserves pending pair discovery; ordinary non-static proxy creation marks it moved (`src/dynamic_tree.c`, `src/broad_phase.c`).
- The §4 shape totals and means are consistent with the stated endpoint-count method and the scene definitions in `shared/benchmarks.c`.
- The awake-image size arithmetic in §5.1 is consistent with the tree node, parent, and proxy sizes (`src/dynamic_tree.c`).

## 4. Proposed edits

Move the traversal-depth queue and drain into the first migration PR. Revise §5.2’s byte estimate to include removal, insertion, and rotations. Qualify §5.4’s growth explanation by journal branch.

## 5. Unresolved

The revised byte-journal size needs measurement under churn; the source establishes the missing writes but not their frequency.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** §2 put B0 to B2 in the first PR and B3 in the second, so the first PR would
   migrate during the collide's wakes and Explode's traversal with no drain. Fix: B0 to B3 are the
   first PR; B4 is the second.
2. **Adopted (M).** In the journaled tree a sleep is one insert and a wake one remove, since the
   awake side is imaged, so a migration is one path; a `b3Body_SetTransform` on a sleeping body is
   `b3DynamicTree_MoveProxyInternal`, a remove and an insert, two paths; insert rotations add
   swaps on the path (`dynamic_tree.c`). Fix: 5.2 states per-path cost, the one-path and two-path
   cases, and that bytes per move are measured under §4's churn.
3. **Adopted (L).** 5.4's appended-slots sentence holds for the identity-journal branch; the byte
   journal reallocates to the old capacity. Fix: qualified.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 17: 3 findings (H1-M1-L1), 3 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: finding 1 is a consequence of
  round 10's B3 (the PR split predates it and was not revisited, U-T2); findings 2 and 3 are
  qualifications of 5.2 and 5.4 text from rounds 1 and 5. Reviewer read of round records or
  changelog: none in the raw log. Doc length: 402 -> 407 lines.
