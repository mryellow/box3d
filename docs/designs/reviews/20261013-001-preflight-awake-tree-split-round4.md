---
title: Review round 4 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round3.md
mode: full pass
---

# Round 4

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T18:29:50+10:00 to 18:36:44+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The proposed passes cover awake–awake, awake–sleeping, and sleeping–sleeping dynamic pairs, and the rewind treatment is consistent with the cited journal and moved-list design. One performance claim is broader than the proposed pair passes support.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | Opening claim; §2 B1 | The document says that, without a sleeping-proxy write, pair gathering is proportional to the awake population. B1 also runs non-awake cross passes when the static or kinematic root moves. A moving kinematic box overlapping many sleeping nodes can therefore make gathering scale with the sleeping population even though no sleeping proxy changed (`src/broad_phase.c`). | Qualify the claim to include static and kinematic cross-pass work, and state the broad-overlap case. |

## 3. Checked, no change

- Traced proxy-key creation, destruction, movement, and validation, including sleeping-body shape writes (`src/broad_phase.c`, `src/shape.c`, `src/body.c`, `src/joint.c`).
- Checked moved-bit propagation, pair gating, rebuild, refit, and incremental tree updates (`src/broad_phase.c`, `src/dynamic_tree.c`, `src/solver.c`).
- Confirmed the rebuild task can overlap the begin-touch wake path, and that the proposed join or deferral addresses the tree-write hazard (`src/physics_world.c`, `src/island.c`, `src/solver_set.c`).
- Checked the rewind claims against the cited §§5.4, 7.1, 7.2, and 7.4, including proxy identity and the captured moved list.

## 4. Proposed edits

Narrow the opening cost statement and §4’s savings statement so they account for non-awake cross passes triggered by static or kinematic movement.

## 5. Unresolved

None within the review scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** `b3GatherCrossSeeds` and `b3CollideCrossPairs` descend every node pair where
   either side is marked moved and the boxes overlap (`broad_phase.c`), so a moved kinematic or
   static box over a sleeping pile visits those sleeping nodes, in the split's non-awake cross
   pass as in today's dynamic cross pass. Fix: the intro and §4 say every cross pass costs what
   the single tree's passes cost for the same overlaps, and the saving is the gather, rebuild and
   refit of the dynamic tree.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 4: 1 finding (H0-M1-L0), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: the scaling claim in the intro
  drew a finding in rounds 2, 3 (as §4's bound) and 4; each fix narrowed it by a clause. Reviewer
  read of round records or changelog: none in the raw log. Doc length: 298 -> 310 lines; round
  3's fixes added 12 lines.
