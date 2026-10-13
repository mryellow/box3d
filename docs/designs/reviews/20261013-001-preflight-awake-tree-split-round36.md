---
title: Review round 36 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round35.md
mode: full pass
---

# Round 36

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:01:57+10:00 to 23:05:28+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a1355a-fc41-70c3-828b-887778ae04e0`.

## Review report (Codex final message)

1. **Summary**  
The proposed split is consistent with the cited source paths. I found no defect in the proxy migration, pair passes, moved-bit handling, or stated step ordering within the requested scope.

2. **Findings**

None.

3. **Checked, no change**

- Verified the current tree readers and proxy-key users in `src/broad_phase.c`, `src/solver.c`, `src/sensor.c`, and `src/physics_world.c` against B0–B3.
- Verified sleep and wake set transfers, contact placement, and proxy writers in the cited body, shape, joint, contact, and island source.
- Verified the rebuild, refit, moved-bit, height, and stack claims against `src/dynamic_tree.c` and `src/dynamic_tree.h`.
- Checked the churn table’s stated counting method against the cited benchmark scene definitions and sleep-timer behavior.

4. **Proposed edits**

None.

5. **Unresolved**

None within scope.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

No findings. No change.

## Status

- Verdict: CONVERGED. Third clean round at full scope (27 and 33 were overturned by their
  confirmation rounds). Confirmation round next (U-P9), document unchanged.
- Round 36: 0 findings (H0-M0-L0).
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0
- Tripwire: no fixes this round. Reviewer read of round records or changelog: none in the raw
  log. Doc length: 390 -> 391 lines at revision 35; unchanged for the confirmation round.
