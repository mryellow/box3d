---
title: Codex review — bit-exact history ring, round 43
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 43

## Review report (Codex final message)

## Summary

I found no actionable discrepancy between the design and the reviewed `src/` and `include/` code. The recommendation is coherent: image awake state, journal cold mutations, and rebuild tree layout after the proposed order changes. This was a read-only source review; it did not execute the proposed implementation or its tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|

No findings.

## Checked, no change

- The existing recording hash covers body transforms and velocities; the design correctly requires a wider state hash.
- The hot, cold, scratch, and derived classifications account for awake records, owned blocks, sensors, pools, moved proxies, and tree identity.
- The undo and redo rules cover the listed create, destroy, sleep, wake, swap removal, and heap ownership cases.
- The tree order changes address the source’s CCD, sensor hit, and explosion traversal dependencies.
- The phase boundaries and cost claims distinguish the temporary raw tree images in phase 1 from the phase 2 awake proportional target.
- The caller contract distinguishes replayed engine inputs from host configuration and borrowed geometry.

## Proposed edits

None.

## Unresolved / disagreements

The decision questions already listed in §16 remain open. I found no additional disagreement.

## Verdict

CONVERGED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540` with the tool timeout at 600000 ms, the same full-scope prompt as rounds 41 and 42. Start 2026-09-29T18:03:53+10:00, end 2026-09-29T18:10:15+10:00, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. Round 42's edits were the only doc changes since the previous round, and none of them adds a structure.

There were no findings to verify. Before recording `CONVERGED`, AC-1 to AC-4 were applied to the doc as it stands:

- **AC-1.** The ring's image and journal are the design's own record of past ticks, not a copy of a live population read in place of the live structure; the restore-time shape list is built and consumed within one restore and is never kept. No shadow structure.
- **AC-2.** §5.2's caller table is a migration worklist. Completeness comes from const-qualified and incomplete types, so a bypass fails to compile, and §13 makes that a phase gate.
- **AC-3.** Tree layout is derived from the fat AABBs at restore, not reconciled against a second live structure; proxy identity is journaled with the pools it matches.
- **AC-4.** The assertions (replayed proxy id matches, world inside a step or a rewind) document invariants and are not control flow. The setter entry's awake-owner branch is the journal rule itself (§7.1), not a validity gate.

No shape violates them. The target doc's `status:` was updated to record convergence.

## Status

Finding count is 0 (0H-0M-0L), down from 3. The verdict was `CONVERGED`, from a fresh full-scope round, which is the only thing that closes the series. It says this round found nothing wrong with what the doc currently claims; it is not architectural endorsement, and any later revision or further round runs at the same full scope. The counts since round 17 were flat until this round, so the convergence rests on one clean round and is worth a confirming fresh round if the user wants one.

`Round 43: 0 findings (0H-0M-0L), 0 applied, 0 declined`

`Series total: 184 findings (53H-103M-28L) across 43 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L) -> 4 (0H-2M-2L) -> 3 (0H-2M-1L) -> 4 (1H-3M-0L) -> 2 (0H-2M-0L) -> 3 (1H-1M-1L) -> 0 (0H-0M-0L)`
