---
title: Review round 40 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round39.md
mode: full pass
---

# Round 40

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:23:28+10:00 to 23:29:45+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1356e-af25-7ac1-a7a6-5e257c373699`.

## Review report (Codex final message)

### 1. Summary

The proposed split accounts for the cited tree readers, proxy-key writers, moved-bit paths, and the rebuild, CCD, and callback ordering hazards. I found no defect in sections 1–4 or 8 that warrants a design change.

### 2. Findings

None.

### 3. Checked, no change

- Verified the four-tree pair passes, static compound handling, and moved-bit clearing against `src/broad_phase.c` and `src/dynamic_tree.c`.
- Traced sleep, wake, shape creation, proxy reset, body writes, joint marks, and contact creation through the cited source.
- Checked the tree-task join, CCD queries, sensor queries, Explode, and public tree traversals against the proposed migration timing.
- Checked the height-trigger reasoning and the churn table’s method and arithmetic. The scratch harness’s measured values were not independently reproduced.

### 4. Proposed edits

None.

### 5. Unresolved

None within scope.

### 6. Verdict

CONVERGED

## Post-review verification (Claude)

No findings. No change.

## Status

- Verdict: CONVERGED. Fifth clean round at full scope (27, 33, 36 and 38 were overturned by
  their confirmation rounds). Confirmation round next (U-P9), document unchanged.
- Round 40: 0 findings (H0-M0-L0).
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0 -> 0/0/0 -> 0/1/1 -> 0/0/0
- Tripwire: no fixes this round. Reviewer read of round records or changelog: none in the raw
  log. Doc length: 398 -> 402 lines at revision 37; unchanged for the confirmation round.
