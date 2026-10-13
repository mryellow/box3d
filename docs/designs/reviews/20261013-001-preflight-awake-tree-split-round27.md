---
title: Review round 27 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round26.md
mode: full pass
---

# Round 27

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T22:04:10+10:00 to 22:08:22+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13526-1763-7db2-a6db-7328a46f10bb`.

## Review report (Codex final message)

1. **Summary**  
The proposed split is consistent with the cited source. I found no defect in the pair passes, proxy migration, moved-bit handling, or traversal ordering within the requested scope.

2. **Findings**

None.

3. **Checked, no change**

- Fixed-index and loop-based tree readers, including queries, sensors, CCD, explosions, bounds, drawing, counters, and memory reporting.
- Proxy-key writers and the static, kinematic, awake, and non-awake pair combinations.
- Sleep and wake timing against the tree task, finalize stage, refit, and sensor pass.
- Moved-bit preservation, rebuild behavior, and the section 4 table’s arithmetic and endpoint-scan reasoning.

4. **Proposed edits**

None.

5. **Unresolved**

The scratch-harness measurements were not independently reproduced.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

No findings. The Unresolved item (the scratch-harness measurements were not reproduced by the
reviewer) is the same note rounds 22 to 24 carried; the method is stated in section 4 and a
finding about it must name what the method would get wrong (prompt scope). No change.

## Status

- Verdict: CONVERGED. First clean round at full scope. Confirmation round next (U-P9).
- Round 27: 0 findings (H0-M0-L0).
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0
- Tripwire: no fixes this round. Reviewer read of round records or changelog: none in the raw
  log. Doc length: 358 -> 367 lines at revision 28; unchanged for the confirmation round.
