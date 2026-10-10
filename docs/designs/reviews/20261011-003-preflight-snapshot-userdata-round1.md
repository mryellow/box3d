---
title: Codex review — preflight snapshot userData, round 1
target: docs/designs/20261011-003-preflight-snapshot-userdata.md
date: 2026-10-11
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
previous: none
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed without resume.
---

# Codex review — docs/designs/20261011-003-preflight-snapshot-userdata.md — round 1

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

The opt-in P1 policy is consistent with the inspected snapshot, recording, and event paths. Two details in §4 need correction: the free-slot guidance cites the wrong liveness check for bodies and joints, and a restore check calls a getter with an invalid ID.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 U2 | The cited liveness checks include `c->contactId == i`, which applies to contacts. The body and joint writers currently scrub every slot without a liveness check; freed records can retain `userData`. Simply making those scrubs conditional could write stale pointers into flagged snapshots. The stated free-slot check would miss this because creating a body overwrites its `userData` (`src/world_snapshot.c`, `src/body.c`, `src/joint.c`). | Specify `body.id == i`, `shape.id == i`, and `joint.jointId == i` for the writer, with free slots scrubbed on restore. Check the free-slot image or restored record before reuse. |
| 2 | L | §4 U2 | Check 3 says to read `NULL` through the getter for a body created after T. Its ID is invalid after restore, and the getter requires a valid ID (`src/body.c`, `src/physics_world.c`). | Check `b3Body_IsValid(id) == false` instead. |

## Checked, no change

- The current serializer and reader both scrub body, shape, and joint `userData` (`src/world_snapshot.c`).
- Recording creation, keyframe capture, and player restore use those shared paths; recording definition placeholders replay as `NULL` (`src/recording.c`, `src/recording_replay.c`).
- World `userData` is outside the image, while body and joint event pointers are copied into separate arrays at step time (`src/world_snapshot.c`, `src/solver.c`, `src/physics_world.c`).
- The current state hash excludes `userData`, and the existing `userShape` carry-over follows slot and generation (`src/recording.c`, `src/world_snapshot.c`).

## Proposed edits

Correct the U2 liveness predicates and free-slot check, then replace the invalid-ID getter check with an ID-validity check.

## Unresolved / disagreements

§6 leaves P1 versus P3, the default, and API placement to the owner. I found no source-driven objection to the recommended P1 policy.

## Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec -m gpt-6-sol -c model_reasoning_effort="high" --sandbox read-only`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms. Start 2026-10-11T08:14:59+10:00, end 08:16:12, exit code 0. Header confirms model `gpt-6-sol`, effort `high`. `git status --short` after the run shows only the author's pre-round edits to the target, so Codex touched nothing. Before the round the author fixed two mangled spots in the target (a duplicated "Three candidates" sentence in §3 and an overlong line in the intro); no content change.

- **#1 (M), applied.** `b3SerializeWorld` and the joint writer in `src/world_snapshot.c` copy every body and joint slot with no liveness test, and `b3DestroyBody` sets `id = B3_NULL_INDEX` without clearing `userData`, so a freed record can hold a stale pointer. `c->contactId == i` is the contact test, not a body or joint one. §4 U2 now names `body.id == i`, `shape.id == i`, `joint.jointId == i` and states the body/joint writers have no test today. Test 5 was rewritten: the old wording (create a body in the slot and read its pointer) could not detect the defect because create overwrites `userData`; it now checks the image and the restored record directly.
- **#2 (L), applied.** `b3GetBodyFullId` asserts `b3Body_IsValid`, so the getter cannot be called on a body gone after restore. Test 3 now checks `b3Body_IsValid` is false.

## Status

Tripwire: fixes added no named mechanism; they corrected a cited predicate and two test wordings. No clustering (first round), no finding was a consequence of an earlier fix. The raw log was not grepped for reads of round records or a changelog (none exist for this target). Doc length 178 -> 177 lines (after the pre-round mangling fix); fix #1 added 1 line, #2 and test 5 changed in place. Under the 400-line split-check.

`Round 1: 2 findings (0H-1M-1L), 2 applied, 0 declined`

`Series total: 2 findings (0H-1M-1L) across 1 round`

`Findings: 2 (0H-1M-1L)`

Not converged: a fresh full-scope round against the edited doc is next (U-S8).
