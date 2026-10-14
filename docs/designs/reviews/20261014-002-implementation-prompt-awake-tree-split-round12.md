---
title: Review round 12 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round11.md
mode: full pass, same prompt and scope as round 11 (U-P4), on revision 12
---

# Round 12

## Exact request sent to Codex

Identical to round 11 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T11:06:32+10:00 to 11:11:46+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a137f2-5cc0-7670-b824-cda0f2a138c4`.

## Review report (Codex final message)

### 1. Summary

The four-increment plan largely matches the canonical design and the live code. I found one false source claim and one gap in the golden-hash rule that could let a ray-cast regression pass.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5 | At the first query with a permitted tied closest hit, the rule checks that the winning fraction is minimal but does not explicitly require the complete all-hits lists to contain the same shape IDs and fractions. It could therefore accept a missing or altered farther hit as a layout effect. `shared/determinism.c`, `src/physics_world.c` | Require equal all-hits multisets at that query; permit only their callback order and the tied closest-hit choice to differ. |
| 2 | L | §2 | “`test/test_ccd_layout.c` runs with sleep off” is false. Its corridor worlds disable sleep, but its sensor scene uses `b3DefaultWorldDef`, which enables sleep (`test/test_ccd_layout.c`, `src/types.c`). | State the narrower fact and retain the conclusion that this one-step sensor fixture needs no change. |

### 3. Checked, no change

- The proxy-key format, fourth-tree capacity plan, guards, and snapshot tree loops match `src/broad_phase.h`, `src/broad_phase.c`, `src/dynamic_tree.c`, and `src/world_snapshot.c`.
- The pair-update order, wake and sleep writers, task join, and proposed drain points match `src/broad_phase.c`, `src/solver_set.c`, `src/solver.c`, and `src/physics_world.c`.
- The specified fail-before checks for B0 through B4 are consistent with the preceding commits’ described behavior. The test runner, benchmark options, and profile access cited in the prompt are present.

### 4. Proposed edits

- In §5, add an explicit all-hits multiset comparison at the first differing query.
- In §2, distinguish the sleep-disabled corridor fixtures from the sensor fixture’s default world.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (M).** The tie clause compared the closest hit and the query's overlap set and cast
   fraction, not the all-hits lists themselves, so a farther hit missing on one side would pass.
   Fix: the two all-hits lists must be the same set of (shape id, fraction).
2. **Adopted (L).** `test/test_ccd_layout.c` disables sleep in its corridor worlds and builds its
   sensor scene from `b3DefaultWorldDef`, which enables sleep (`src/types.c`); that scene steps
   once. Fix: §2 states both and the conclusion (nothing sleeps there) stands.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 12: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5 for the fifth round running
  (9 to 12), each a one-clause tightening of the same attribution proof toward "every logged value
  equal but for callback order and the tied choice"; the clause added this round completes that
  statement. §2's finding is a fact correction. Reviewer read of round records or changelog: none
  in the raw log. Doc length: 758 -> 760 lines.
