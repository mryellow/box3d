---
title: Review round 10 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round9.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 10
---

# Round 10

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T10:54:38+10:00 to 11:00:18+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a137e7-790d-7d01-a979-6458f8cde163`.

## Review report (Codex final message)

### 1. Summary

I found two defects in the implementation prompt: its branch instruction conflicts with the canonical design, and its golden-hash procedure can reject a permitted query-order change. The checked proxy, pair, migration, and test-runner mechanics otherwise agree with the cited source.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §1 | The canonical design §2 requires one PR forked from `main`. The prompt instead starts four deliverable commits from `master` and moves any `main` PR outside its scope. The refs are distinct, and `main` lacks the A0 helper. | Specify how the tree-order prerequisite reaches `main`, then fork the four-commit PR from `main` as required by the design. |
| 2 | M | §5 | The policy permits reordered overlap callbacks but requires every other logged value to match in order. Its logging `b3World_CastRay` has a public callback whose hit order can also change with tree layout (`src/physics_world.c`). A correct split could therefore be rejected. | Compare the logged all-ray hits as a multiset of shape IDs and fractions; retain their sequence only as diagnostic evidence. |

### 3. Checked, no change

- Proxy-key width, fourth-tree guards, capacity hints, snapshot loops, and counter paths.
- Pair-pass primitives, contact set placement, moved-bit handling, and sleep/wake call sites.
- The named fail-before build mechanism and test-runner registration.
- Benchmark options, profile collection, scene list, and the query-spawn inputs.

### 4. Proposed edits

Revise §1 to follow the canonical `main` fork requirement. Revise §5’s log comparison to allow ray callback permutations while still checking hit membership, fractions, and closest-hit tie evidence.

### 5. Unresolved

The prompt does not specify how the required tree-order work and A0 helper will be available on `main` before the branch is forked.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Declined (H), re-verified (U-V7).** Same finding as round 6's first. The design's §1 hands the
   branch to this document; the owner has since confirmed in writing that the fork is `master`;
   §10 records it. The finding brings no fresh evidence that the reason no longer holds (U-V4), and
   its own text concedes `main` lacks the A0 helper. Scope check (U-P3): the prompt never said
   §10's decisions are settled, so from round 11 it does (U-P13); the Unresolved item (how the
   tree-order work reaches `main`) is the owner's, outside this document.
2. **Adopted (M).** The logging `b3World_CastRay` runs a host callback per hit across the trees, so
   its hit order is public callback order and may change with layout. Fix: the all-hits list
   compares as a set of (shape id, fraction), as the overlap already does.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 10: 2 findings (H1-M1-L0), 1 applied, 1 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5 again, a consequence of
  round 9's fix (the log it widened has its own callback order); one clause. The branch finding is
  a re-raise of a recorded decision: a Provenance gap in the prompt's scope wording, closed for
  round 11 (U-T5, U-P13). Reviewer read of round records or changelog: none in the raw log. Doc
  length: 756 -> 756 lines.
- Criteria review threshold (10 rounds without converging): reached. Reconsideration noted for
  the closing changelog entry (U-R10, B8): the §5 policy drew findings in rounds 1, 2, 5, 6, 7, 9
  and 10, each one a tightening of an attribution proof the reviewer itself proposed; no Part B
  criterion was involved, so no AC change is proposed. The pattern is Part A: a prompt that does
  not state Provenance as settled invites re-raises (U-P13 should be applied from round 1).
