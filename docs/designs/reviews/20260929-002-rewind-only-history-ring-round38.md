---
title: Codex review - rewind-only history ring, round 38
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round37.md
mode: full pass, confirmation round (U-P9) of round 37, same prompt and scope, on revision 37
session: 01a13e84-9e35-7ac2-a84e-80f0f9d097c0, resumed once to finalize
---

# Round 38

## Exact request sent to Codex

Identical to round 37 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(12,298 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T17:44:00+10:00
to 17:53:00. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 17:53:20 to 17:54:11. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13e84-9e35-7ac2-a84e-80f0f9d097c0`.

## Review report (Codex final message)

## 1. Summary

The hot image, cold journal, and split-tree restore rules are consistent with the live source I checked. The correction example has one concrete gap: it replays steps before restoring caller-owned state that callbacks may read.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §14, §10 | The example calls `Rewind(T)` and starts replay without resetting host state to its value at T. §10 requires that reset. If `world->userData` or a callback context changed after T, the first replayed callback sees the value from P and can produce a different result. | Add an explicit host-state restore at T before correction and replay, then replay host-state changes at their original ticks. |

## 3. Checked, no change

- The awake and sleeping dynamic trees, parked leaves, migration queue, and static and kinematic moved-proxy handling match `src/broad_phase.c`, `src/dynamic_tree.c`, and `src/physics_world.c`.
- Continuous candidates and explosion hits are sorted before evaluation or application, as claimed.
- The listed world scalars, sensor overlaps, pool identity, pair membership, and owned contact and shape blocks have a stated image or undo route. I found no additional simulation-affecting state gap.
- The reverse-order proxy and block-ownership rules are consistent with the mutation paths reviewed.

## 4. Proposed edits

Revise the §14 correction example to restore caller-owned callback state at T before its first replayed operation and to replay later changes at their original ticks.

## 5. Unresolved

The cited ring document’s budget and eviction rules are reserved for its separate review.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the doc.

- **#1 (M) the example replays before resetting host state: confirmed, applied.** §10 says the
  caller sets back what its callbacks read, and §12 test 3 sets each such value back to its value
  at T before the first replayed step; the §14 example did neither. One line after a successful
  `Rewind` sets the host state the callbacks read back to its value at T; later changes are API
  calls, which the loop's "replay API calls" line already covers.

Raw log: the reviewer opened no round record or changelog; five changelog filenames appear in one
directory listing of `docs/designs/`, none read.

## Status

One Medium finding on the confirmation round of round 37, so the clean round is not confirmed. The
§14 example has now drawn a finding in rounds 34, 35 and 38 (not consecutive: 36 and 37 did not),
each a line it was missing against §10; each fix was one line. Not converged. Next: round 39,
same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Not a cluster against last round (round 37
had none); not a consequence of a previous fix (U-T2); the reviewer read no round record. Doc
length 1338 -> 1339 lines; added lines per fix: #1 one (U-T3). Under the split check (U-T6).

Round 38: 1 findings (0H-1M-0L), 1 applied, 0 declined
Series total: 99 findings (18H-51M-30L) across 38 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L)
