---
title: Review round 16 - implementation prompt, awake and non-awake dynamic trees (confirmation)
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round15.md
mode: confirmation round (U-P9), same prompt and scope as round 11, doc unchanged since round 15
---

# Round 16

## Exact request sent to Codex

Identical to round 11 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T11:30:27+10:00 to 11:35:32+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13808-4240-7032-97bb-7f702c9c70f0`.

## Review report (Codex final message)

### 1. Summary

I found no substantiated defect in the implementation prompt against the canonical design and the cited source. One recording edge case is unresolved in the design and is noted separately.

### 2. Findings

None.

### 3. Checked, no change

- Verified the proxy key, tree readers, sleep and wake paths, rebuild timing, and snapshot tree loops against the live source.
- Checked the named fail-before tests and their expected checks against the test runner and validation paths.
- Checked the golden-hash policy against query-spawn’s inputs and the measurement commands against the benchmark runner.

### 4. Proposed edits

None.

### 5. Unresolved

The design does not specify what happens if a query callback wakes a body and then calls `b3World_StartRecording` before the migration queue drains. The source permits recording while the world is unlocked; its snapshot would capture the awake solver set with the proxy still in the non-awake tree. This needs an owner decision under the prompt’s §9 stop condition.

### 6. Verdict: CONVERGED

## Post-review verification (Claude)

No findings. No change to the document.

Unresolved item, for the owner (U-P5): a query callback that wakes a body and then calls
`b3World_StartRecording` before the outermost exit drains the queue would snapshot the body in the
awake set with its proxy still in the non-awake tree. A question about the design's B3 (which
public calls raise the depth) and the recording path, not about this document; this document's §9
already stops on a decision the design is silent on.

Design-level items collected across the series, all for the owner, none acted on:

- Round 7: the design's §2 says "one PR forked from `main`" while its §1 hands the branch to this
  document; the owner confirmed `master`; a one-word amendment to the design's §2 would close it.
- Round 8: between the B2 commit and B4, `b3DynamicTree_Rebuild` with `fullBuild` false retains an
  unmarked tree whole, so the non-awake tree's height is not bounded in that intermediate state.
- Round 3: callback-time `b3DestroyBody` and `b3Body_SetType` under a held tree view are tested on
  a single sleeping leaf only; the design's B3 states the unguarded `b3Body_SetTransform` case.
- Round 16: recording started from a query callback before the drain, above.

## Status

- Verdict: CONVERGED. Clean round 15 confirmed by clean round 16 at full scope on the same
  document, revision 15 (U-P9, U-S8). The gate (U-G1, AC-1 to AC-4) re-run at revision 15: the
  additions since round 1 are one assertion of a construction-guaranteed invariant, two test
  fixtures, a human attribution procedure (§5) and the Provenance section; it holds. The document
  is marked Canonical as the next edit (U-I5).
- Round 16: 0 findings (H0-M0-L0).
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> (paused, owner
  direction, revision 9) -> 0/0/0 -> 0/1/1 -> 1/1/0 -> 0/1/0 -> 0/1/1 -> 0/1/0 -> 0/2/1 -> 0/0/0 ->
  SETTLED 0/0/0
- Series: 16 rounds, 47 findings (H8-M30-L9), 45 applied, 2 declined (both the branch, recorded in
  §10).
- Tripwire: no fixes this round. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 766 lines, unchanged.
