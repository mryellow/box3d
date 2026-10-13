---
title: Review round 19 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round18.md
mode: full pass
---

# Round 19

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T20:54:10+10:00 to 21:02:57+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s pair-pass coverage, migration ordering, and rewind treatment are consistent with the cited source. I found one defect in the churn measurement method; I did not find a demonstrated tree-state or pair-discovery failure.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §4 | The harness counts awake-state flips for every non-static body. Kinematic bodies can sleep and wake, but B2 leaves their proxies in the kinematic tree. A kinematic flip would therefore be counted as proxy moves that never occur. This challenges the method’s claim of exactness, though it does not establish that a reported row is wrong (`shared/benchmarks.c`, `src/solver_set.c`). | Count shapes only for bodies whose type is dynamic; rerun the churn table if any recorded kinematic body flipped. |

## 3. Checked, no change

- The proposed self and cross passes cover the pair classes found by the current dynamic-tree passes (`src/broad_phase.c`, `src/contact.c`).
- The queued migration and drain points account for collision-triggered wakes, the asynchronous rebuild, CCD, explosions, and public tree walks (`src/physics_world.c`, `src/solver.c`, `src/island.c`).
- The awake image and non-awake proxy-identity undo are consistent with the cited rewind rules (§5.4, §7.1, §7.4).
- The stated tree sizes and `sleep` scene mean agree with the source and table (`src/dynamic_tree.c`, `shared/benchmarks.c`).

## 4. Proposed edits

Qualify §4’s “one flipped shape” and exactness statements to **dynamic-body** flips, and filter the harness accordingly.

## 5. Unresolved

Whether any kinematic body flipped during the reported runs; the document does not provide that breakdown.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (L).** The harness skipped static bodies only. Rerun with the filter changed to
   dynamic bodies only (`b3Body_GetType( bodyId ) != b3_dynamicBody` skipped): every flip column
   is identical in every scene; the only difference is the awake-body maximum, one lower in
   `junkyard` and `washer`, which is their kinematic body. Fix: §4 states the method counts
   dynamic bodies and that the rerun matched; the table is unchanged.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 19: 1 finding (H0-M0-L1), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §4's method has drawn one
  finding in each of rounds 2, 15 and 19, each a qualification; the finding is a consequence of
  revision 9's kinematic revert (U-T2). Reviewer read of round records or changelog: none in the
  raw log. Doc length: 412 -> 418 lines.
