---
title: Codex review — preflight snapshot userData, round 3
target: docs/designs/20261011-003-preflight-snapshot-userdata.md
date: 2026-10-11
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
previous: 20261011-003-preflight-snapshot-userdata-round2.md
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed without resume.
---

# Codex review — docs/designs/20261011-003-preflight-snapshot-userdata.md — round 3

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

The proposed opt-in path matches the current body, shape, and joint scrub sites and keeps recording restores under a separate, always-scrubbing policy. I found no defect in the document’s stated approach to pointer preservation, free slots, restore symmetry, or recording compatibility.

## Findings

None.

## Checked, no change

- The serializer and deserializer currently clear body, shape, and joint `userData`; the proposed flag covers both sides. [src/world_snapshot.c](/home/user/box3d/src/world_snapshot.c)
- Freed records retain stale `userData`, and the proposed live-slot checks match their ID sentinels. [src/body.c](/home/user/box3d/src/body.c), [src/shape.c](/home/user/box3d/src/shape.c), [src/joint.c](/home/user/box3d/src/joint.c)
- Recording definitions use pointer placeholders, replay restores them as `NULL`, and recording snapshot callers can remain on the scrubbed path. [src/recording.c](/home/user/box3d/src/recording.c), [src/recording_replay.c](/home/user/box3d/src/recording_replay.c)
- World `userData` is outside the image; body and joint event arrays copy `userData` at step time; the state hash does not include it. [src/world_snapshot.c](/home/user/box3d/src/world_snapshot.c), [src/solver.c](/home/user/box3d/src/solver.c), [src/recording.c](/home/user/box3d/src/recording.c)

## Proposed edits

None.

## Unresolved / disagreements

Section 6 leaves policy and API placement to the owner. I found no source-based disagreement with the recommended P1, flag-off design.

## Verdict: CONVERGED

## Post-review verification (Claude)

Invocation: same command and prompt as round 1, foreground, stdin from /dev/null, timeout 540. Start 2026-10-11T08:29:19+10:00, end 2026-10-11T08:30:37+10:00, exit code 0. Header confirms gpt-6-sol / high.

No findings; nothing to verify or apply. The Checked list was spot-read against the code in rounds 1 and 2.

## Status

Tripwire: no fixes, so no mechanism added or removed; no clustering; raw log shows no reads of round records or the changelog. Doc length 177 -> 177, no fix added lines.

`Round 3: 0 findings (0H-0M-0L), 0 applied, 0 declined`

`Series total: 4 findings (0H-1M-3L) across 3 rounds`

`Findings: 2 (0H-1M-1L) -> 2 (0H-0M-2L) -> 0 (0H-0M-0L)`

Clean round, not yet converged: a confirmation round at the same scope is next (U-P9).
