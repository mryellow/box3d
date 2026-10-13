---
title: Review round 38 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round37.md
mode: full pass
---

# Round 38

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T23:13:09+10:00 to 23:17:53+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session `01a13565-3bf5-74f3-ba8b-27f0d1424950`.

## Review report (Codex final message)

1. **Summary**  
I checked the design’s source claims against the live cited code and pressure-tested the proposed pair passes, migrations, and step ordering. I found no substantive defect within the requested scope.

2. **Findings**  
None.

3. **Checked, no change**

- The proposed passes cover the pair classes currently found through the dynamic tree, including moved sleeping proxies and static compounds (`src/broad_phase.c`, `src/contact.c`).
- Proxy-key readers, moved-bit writers, sensor queries, CCD, Explode, world queries, counters, and validation have corresponding changes in B0–B4.
- The rebuild join precedes finalize and bullet CCD; the sleep pass occurs last in the solve (`src/solver.c`, `src/physics_world.c`).
- The height-trigger and stack discussion matches the cited tree operations. The `sleep` scene’s 2,100-shape peak also matches its ten 210-body pyramids (`src/dynamic_tree.c`, `shared/benchmarks.c`).

4. **Proposed edits**  
None.

5. **Unresolved**  
None.

6. **Verdict:** CONVERGED

## Post-review verification (Claude)

No findings. No change.

## Status

- Verdict: CONVERGED. Fourth clean round at full scope (27, 33 and 36 were overturned by their
  confirmation rounds). Confirmation round next (U-P9), document unchanged.
- Round 38: 0 findings (H0-M0-L0).
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> 1/1/2 -> 0/1/1 -> 0/1/0 -> (paused, owner decision, revision 26) -> 1/1/0 -> 0/2/1 -> 0/0/0 -> 0/1/1 -> 0/1/1 -> 0/1/1 -> 0/1/0 -> 1/1/0 -> 0/0/0 -> 1/0/0 -> 1/0/0 -> 0/0/0 -> 1/1/0 -> 0/0/0
- Tripwire: no fixes this round. Reviewer read of round records or changelog: none in the raw
  log. Doc length: 391 -> 398 lines at revision 36; unchanged for the confirmation round.
