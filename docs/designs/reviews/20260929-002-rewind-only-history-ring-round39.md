---
title: Codex review - rewind-only history ring, round 39
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round38.md
mode: full pass, same prompt and scope as round 37, on revision 38
session: 01a13e91-c89e-72d3-8065-5252d7b3a1b7, resumed once to finalize
---

# Round 39

## Exact request sent to Codex

Identical to round 38 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(9,053 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T17:58:23+10:00
to 18:07:23. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 18:07:35 to 18:08:18. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13e91-c89e-72d3-8065-5252d7b3a1b7`.

## Review report (Codex final message)

1. **Summary**

The design’s state inventory and rewind sequence are broadly consistent with the current split-tree code and sorted candidate paths. One integration gap can break the promised whole-world restore: recording playback can replace a world while history remains enabled.

2. **Findings**

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§10, 13, 17 | The design excludes active recording from history but does not exclude a recording player. A caller can enable history on the world returned by `b3RecPlayer_GetWorldId`; `b3RecPlayer_SeekFrame` then uses `b3DeserializeIntoShell` to overwrite that world without producing journal entries or a history image. A later `Rewind` would apply retained history to a different world state. | Make history and player seeks mutually exclusive, or define a history-aware seek that resets and reimages the ring. Specify how the serializer’s direct restore writes coexist with phase 1’s restricted write access. |

3. **Checked, no change**

- The source has separate awake and sleeping dynamic trees; the migration queue and parked marked leaves are represented in the design.
- Pair creation, continuous candidates, and explosion application use the stated deterministic ordering.
- The described pool, sensor, manifold, material, and hull ownership paths match the mutations inspected.
- The API correction sketch applies an end-of-tick correction at the stated point in replay.

4. **Proposed edits**

Add the recording-player restriction and its enforcement point to §10, then carry it through the phase 1 integration in §13 and the player decision in §17.

5. **Unresolved**

The separately cited ring’s budget and eviction rules belong to its own review.

6. **Verdict:** `CHANGES_PROPOSED`

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) a recording player's seek on a world with history: confirmed, applied.**
  `b3RecPlayer_GetWorldId` returns the player's world and `b3RecPlayer_SeekFrame` restores a
  keyframe through `b3DeserializeIntoShell` (`replay.c`, `include/box3d/box3d.h`), which writes the
  whole world with no journal entry and no image; §10 excluded only an active recording. Fix: §10
  makes a player's world and history mutually exclusive, asserted at `b3World_EnableHistory` and at
  the seek. The reviewer's second point, how the serializer's direct writes survive phase 1's
  compile-time boundary, was a gap: §5.2 now states that restore's writer also serves
  `b3DeserializeIntoShell`, under an assertion widened to "inside `Rewind` or a deserialize", and
  §13 migrates the serializer's reader to it in phase 1; §17's player bullet cites the exclusion.
  The history-aware seek the reviewer offered as the alternative is declined by the existing §17
  decision that the player keeps its own ring; nothing in the finding shows that reason no longer
  holds (U-V4).

Raw log: the reviewer read no round record or changelog.

## Status

One High finding, from one in round 38. Category: an integration case the caller contract did not
exclude (#1), in §10, §13 and §17. Not in §14, which rounds 34, 35 and 38 found. Not converged.
Next: round 40, same prompt, same scope, when the owner sends it; the ring doc's series is paused
under the owner's stop condition, so both are with the owner.

Tripwire (U-T): no mechanism added; one assertion pair added, mirroring the recording exclusion
(U-T1). Not a cluster against last round (§14); not a consequence of a previous fix (U-T2); the
reviewer read no round record. Doc length 1339 -> 1348 lines; added lines per fix: #1 nine across
four sections (U-T3). Under the split check (U-T6).

Round 39: 1 findings (1H-0M-0L), 1 applied, 0 declined
Series total: 100 findings (19H-51M-30L) across 39 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L)
