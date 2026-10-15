---
title: Codex review - rewind-only history ring, round 35
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round34.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 34
session: 01a13e4c-361c-7020-a37c-94a1f7b017e2, resumed once to finalize
---

# Round 35

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(11,181 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T16:42:24+10:00
to 16:51:24. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 16:51:37 to 16:52:23. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13e4c-361c-7020-a37c-94a1f7b017e2`.

## Review report (Codex final message)

## 1. Summary

The design’s state inventory and tree restore strategy account for the current awake and sleeping tree split, migration queue, and sorted candidate processing. I found one caller contract defect: the correction example can silently lose API mutations made after the latest step.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §6, §10, §14 | `Rewind` undoes the open segment of API calls made after `now`, even when rewinding to `now`. The §14 correction example replays inputs only through `now`, so those pending inputs disappear. | Have the example retain and reapply pending inputs for tick `now + 1`, or state and enforce a precondition that no such calls are pending. |

## 3. Checked, no change

- The current source has four broad-phase trees; marked sleeping leaves can remain in the awake tree, with migration order held in the queue. The proposed awake-tree image and queue capture cover that state.
- Continuous collision candidates and explosion hits are sorted as the design claims.
- The proposed proxy entries retain sleeping-tree leaf boxes and proxy IDs; moved static and kinematic proxies are captured separately.
- The cited serializer omits the migration queue and clears object `userData`, as the phase-0 discussion states.
- The identified world scalars, pool generations, sensor overlaps, owned contact blocks, and shared hull references have restoration routes in the design.

## 4. Proposed edits

Revise §14’s correction example to preserve pending calls made since the last completed step and reapply them after catch-up, before the next step. State that this is required even when `T == now`.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the doc and the source it cites.

- **#1 (M) the correction example drops the inputs pending for `now + 1`: confirmed, applied.**
  §9 says `Rewind` first undoes the live entries since the current tick, including when T equals
  it, and §14's own sentence says a call made after step `now` is an input to `now + 1` that the
  replay applies again; the example's loop ends at `now` and never reapplies them, so a caller
  copying it loses every setter, force and creation made since the last step. Fix: one line after
  the loop replays the calls made after step `now`. No precondition is added: forbidding pending
  calls would make a correction that arrives mid-frame wait for a step, and the journal already
  holds what is needed to undo them.

Raw log: the reviewer read no round record or changelog.

## Status

One Medium finding, on the §14 example. Category: an example that did not do what the sentence
beneath it says (#1). Not in a section that produced findings last round (§6/§7.4/§12-14's hash
comment); the §14 hit is a different paragraph from round 34's. Not converged. Next: round 36,
same prompt, same scope, when the owner sends it.

Tripwire (U-T): no mechanism added or removed (U-T1). §14 produced a finding in rounds 34 and 35,
two running, in different paragraphs; not a consequence of round 34's fix (U-T2); the reviewer read
no round record. Doc length 1432 -> 1432 lines; added lines per fix: #1 one (U-T3). Over the split
check (U-T6): still one decision; no split.

Round 35: 1 findings (0H-1M-0L), 1 applied, 0 declined
Series total: 95 findings (16H-49M-30L) across 35 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L)
