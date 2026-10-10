---
title: Codex review — preflight snapshot userData, round 2
target: docs/designs/20261011-003-preflight-snapshot-userdata.md
date: 2026-10-11
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
previous: 20261011-003-preflight-snapshot-userdata-round1.md
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed without resume.
---

# Codex review — docs/designs/20261011-003-preflight-snapshot-userdata.md — round 2

## Exact request sent to Codex

```
Review the design document docs/designs/20261011-003-preflight-snapshot-userdata.md against the source it cites (src/world_snapshot.c, src/world_snapshot.h, src/recording.c, src/recording_replay.c, src/solver.c, src/physics_world.c, include/). Read-only; do not modify anything.

Scope: the document's own content only. Pressure-test the design: does what it decides hold against the live code, is each claim about current behaviour true, are the decisions internally consistent, and could the stated approach produce a wrong result (leaked pointer, broken recording compatibility, snapshot/restore asymmetry, free-slot leakage, determinism)? Completeness means completeness of what the document states. Do not review any changelog, any "what changed" narrative, or any round record or finding list. Do not ask whether tests exist or whether a test list is complete. Do not check index or bookkeeping. Existing settled primitives (the snapshot serializer, recording format, userShape carry-over) are tools, not review targets: judge what the design adds and which question each primitive answers. Citations follow the repo format (source by path only, docs by section number); a missing line number is not a finding.

Output exactly this shape:
1. ## Summary (2-3 sentences)
2. ## Findings: a table with columns `# | Severity | Section | Issue | Proposed action`, real defects only, severity H/M/L, write "None." if empty.
3. ## Checked, no change: bullets of claims you verified.
4. ## Proposed edits
5. ## Unresolved / disagreements
6. ## Verdict: one line, CONVERGED or CHANGES_PROPOSED.
```

## Review report (Codex final message)

## Summary

The proposed preservation path fits the current serializer and replay call sites: recording callers can continue scrubbing pointers, and live-slot checks can keep stale pointers out of free slots. I found two low-severity inconsistencies in the proposed public API contract.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §3, §6 | “On by default later is a one-line change” does not hold for the proposed required `flags` argument. `b3_snapshotNone` explicitly means no preservation; changing what zero means would change existing callers’ results and leave no opt-out without another flag. | Remove the one-line claim. Treat off as the defined behavior of zero, or design an explicit opt-out before promising a future default change. |
| 2 | L | §3 | Unknown flags are promised a `NULL` return and a debug assertion. The default `B3_ASSERT` handler traps, so a debug caller will not receive `NULL` (`include/box3d/base.h`, `src/core.c`). | Either return `NULL` without asserting for this public input, or state that the return applies only when assertions do not trap. |

## Checked, no change

- Body, shape, and joint `userData` is scrubbed on both serialization and restore (`src/world_snapshot.c`).
- The stated live-slot tests match the sparse records; destroyed records can retain stale `userData` (`src/body.c`, `src/shape.c`, `src/joint.c`).
- Recording seed and keyframe callers use the shared serializer, and player restores use the shared reader; keeping their new argument false preserves pointer scrubbing (`src/recording.c`, `src/recording_replay.c`).
- World `userData` is outside the serialized world configuration, and the current state hash does not include object `userData` (`src/world_snapshot.c`, `src/recording.c`).
- Body and joint events copy `userData` during stepping and remain buffered until the next step clears them (`src/solver.c`, `src/physics_world.c`).

## Proposed edits

Correct the future-default claim in §3 and make the unknown-flag return contract consistent with debug behavior.

## Unresolved / disagreements

The document leaves P1 versus P3 to the owner. I found no source-code conflict with the recommended P1 policy.

## Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: same command and prompt as round 1, foreground, stdin from /dev/null, timeout 540. Start 2026-10-11T08:26:16+10:00, end 2026-10-11T08:28:48+10:00, exit code 0. Header confirms gpt-6-sol / high.

- **#1 (L), applied.** The flags argument is zero for "off" (`b3_snapshotNone`), so a later on-by-default change would alter existing callers' results and need a separate opt-out flag. §3 no longer calls it a one-line change; §6 item 2 says an opt-out flag would be needed.
- **#2 (L), applied.** `B3_ASSERT` calls `b3InternalAssert`, whose default handler hits `B3_BREAKPOINT` (`src/core.c`), so a debug caller never receives the `NULL`. §3 now returns `NULL` without asserting; the flags are caller input.

## Status

Tripwire: fixes added no named mechanism (one claim removed, one assert removed). Findings did not cluster; both are §3 API-contract wording, neither a consequence of round 1's fixes. The raw log shows no reads of round records or the changelog. Doc length 177 -> 177 lines, fixes net 0. Under the 400-line split-check.

`Round 2: 2 findings (0H-0M-2L), 2 applied, 0 declined`

`Series total: 4 findings (0H-1M-3L) across 2 rounds`

`Findings: 2 (0H-1M-1L) -> 2 (0H-0M-2L)`

Not converged.
