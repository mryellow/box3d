---
title: Codex review — preflight snapshot userData, round 4
target: docs/designs/20261011-003-preflight-snapshot-userdata.md
date: 2026-10-11
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
previous: 20261011-003-preflight-snapshot-userdata-round3.md
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed without resume.
---

# Codex review — docs/designs/20261011-003-preflight-snapshot-userdata.md — round 4

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

The proposed opt-in pointer policy holds against the cited code. The live-slot checks and reader-side flag control address free-slot leakage and recording safety; I found no defect in the document’s stated approach.

## Findings

None.

## Checked, no change

- The serializer and reader currently scrub body, shape, and joint `userData` (`src/world_snapshot.c`).
- Recording seeds and replay keyframes use those functions, while recorded creation definitions use zero placeholders and replay as `NULL` (`src/recording.c`, `src/recording_replay.c`).
- Destroyed body, shape, and joint slots retain their old `userData`, so the proposed live-slot check is necessary (`src/body.c`, `src/shape.c`, `src/joint.c`).
- World `userData` is outside the image; body move and joint event arrays retain their last-step copies until the next step (`src/world_snapshot.c`, `src/physics_world.c`, `src/solver.c`).
- `b3HashWorldState` does not hash `userData` (`src/recording.c`).

## Proposed edits

None.

## Unresolved / disagreements

Section 6 leaves P1 versus P3 and the placement of the `flags` parameter to the owner. The U2 steps are written for P1, so that choice must be settled before U2 is implemented.

## Verdict

CONVERGED

## Post-review verification (Claude)

Invocation: same command and prompt as round 1, foreground, stdin from /dev/null, timeout 540. Start 2026-10-11T08:30:44+10:00, end 2026-10-11T08:31:34+10:00, exit code 0. Header confirms gpt-6-sol / high.

No findings; nothing to verify or apply. U-G1 author check against AC-1..AC-4: no shadow structure or side table (live records read at point of use, AC-1); one flag at one choke point (the internal argument, set only by the public snapshot path), not a callers list (AC-2); no reconciliation of snapshot and world (AC-3); free slots are NULL by liveness construction, no validator or retry (AC-4). Pass.

## Status

Tripwire: no fixes; no mechanism added or removed; no clustering; raw log shows no reads of round records or the changelog. Doc length 177 -> 177.

`Round 4: 0 findings (0H-0M-0L), 0 applied, 0 declined`

`Series total: 4 findings (0H-1M-3L) across 4 rounds`

`Findings: 2 (0H-1M-1L) -> 2 (0H-0M-2L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L)`

`4 rounds: 0/1/1 -> 0/0/2 -> 0/0/0 -> SETTLED 0/0/0`

Converged (U-S8): a clean round followed by a clean confirmation round at full scope. Status stays Draft: §6 holds three owner decisions.
