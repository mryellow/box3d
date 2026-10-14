---
title: Review round 9 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round8.md
mode: confirmation round (U-P9), same prompt and scope, doc unchanged since round 8
---

# Round 9

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T10:48:58+10:00 to 10:53:06+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a137e2-4882-7753-aacc-ed2b0c016bda`.

## Review report (Codex final message)

### 1. Summary

The prompt’s implementation sequence and the checked tree, solver, and test paths largely match the binding design and live source. Two edits are needed: the golden-hash policy can accept an incorrect ray result, and §2 states the `b3Body_SetType` path too broadly.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §5 | The tie rule requires both selected shapes to appear in the all-hits log at an equal fraction, but does not require that fraction to be the **nearest positive hit**. It could therefore permit a missed closer hit. The log also omits the closest query’s returned fraction, which `shared/determinism.c` hashes; a changed fraction could be attributed to an overlap-order permutation. | Log the closest hit’s flag and fraction. Permit a tie only when both selected shapes have the nearest positive fraction in the all-hits lists, and require every other observation at the first divergent query to match. |
| 2 | L | §2 | The claim that `b3Body_SetType` destroys contacts, wakes the body, and recreates every proxy is unconditional. In `src/body.c`, a disabled body takes an early path that does none of those operations. | Qualify the claim as applying to enabled bodies; state that disabled bodies have no proxies to migrate. |

### 3. Checked, no change

- Verified the B0 guards, capacity allocation, snapshot loops, counters, and tree-reader inventory against the cited source.
- Verified the B1 pair fixture reaches the stated fail-before `b3ValidateNoMoved` check.
- Verified the B2/B3 wake, sleep, tree-task join, callback drain, CCD candidate ordering, and Explode collection paths.
- Verified the B4 height trigger and moved-bit checks against the tree primitives.

### 4. Proposed edits

Apply the two changes in the findings table before handing the prompt to an implementing agent.

### 5. Unresolved

None.

### 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (M).** The tie clause required the two shapes at one equal fraction in both all-hits
   lists but not that the fraction be the nearest, so a closer hit missed on one side could pass as
   a tie; and the closest hit's fraction, which `QuerySpawnOnce` hashes, was not in the log. Fix:
   the log carries the closest hit's flag and fraction; the tied fraction must be the least
   positive fraction in both lists; the overlap set and cast fraction at that query must be equal.
2. **Adopted (L).** `b3Body_SetType` (`src/body.c`) returns after recording the type when the body
   is in the disabled set. Fix: the §2 claim is stated for enabled bodies, with the disabled path
   named.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. The confirmation round did not confirm; not converged (U-S8).
- Round 9: 2 findings (H0-M1-L1), 2 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5 again, a consequence of the
  revision 9 restatement (its tie clause omitted the nearest-hit condition); the fix is one clause,
  the paragraph does not grow. §2 had its last finding in round 5. Reviewer read of round records
  or changelog: none in the raw log. Doc length: 753 -> 756 lines.
