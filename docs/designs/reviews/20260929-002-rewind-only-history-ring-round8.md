---
title: Codex review — rewind-only history ring, round 8
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Attempt 1 killed by the shell timeout, resumed once (see Post-review verification).
session: 01a0ece4-f60e-7d43-9d6d-2e89a5c86660
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 8

## Review report (Codex final message)

## Summary

The rewind and ownership rules are largely consistent with the inspected source. One source-writer inventory is incomplete. This was a read-only review; no files were changed.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2, §7.3 | The table of “current direct-write callers, to migrate” omits cross-module writes. shape.c changes a body’s `headShapeId` and `shapeCount` during shape creation and destruction; joint.c changes both bodies’ joint-list heads and counts during joint creation and destruction. Following the stated migration list leaves those writes outside the body accessor: the proposed opaque types would then fail the build. If direct access remained, rewinding a shape or joint created on a static body would leave a list pointing to an undone object. | Add these callers and their affected records to the inventory, then require them to use the body write accessor before the compile gate is considered complete. |

## Checked, no change

- The recording test checks backward restore by comparing the existing transform and velocity hash; the document correctly limits what that demonstrates.
- Contact manifold, mesh cache, shape material, sensor array, sleeping set, and island array ownership paths were checked against their create and destroy paths.
- The proposed handling of pending mutations between steps, sensor overlap swapping, and moved proxy bits is consistent with the inspected paths.
- The source supports the stated tree traversal dependencies in CCD and explosion processing.

## Proposed edits

Correct the §5.2 migration table and make §7.3’s completion criterion explicitly cover writes to a record made from another structure’s module.

## Unresolved / disagreements

The cited benchmark figures were not independently checked: their cited review is outside the files permitted for this review. No source-level disagreement remains on the finding above.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as rounds 6 and 7. Start 2026-09-29T21:20:23+10:00, end 21:29:23, exit code 124. Attempt 2, resume of session 01a0ece4-f60e-7d43-9d6d-2e89a5c86660 with `-m gpt-6-sol -c model_reasoning_effort="high"` before `resume` and a finalize-only prompt, start 21:29:28, end 21:30:13, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. The target doc had a diff (round 7's fixes, uncommitted) that was read and was not mangled. Codex's markdown file links were reduced to bare file names when copied here.

- **#1 (Medium), applied.** A grep over `src/` for writes to `headShapeId`, `shapeCount`, `headJointKey` and `jointCount` finds them in `body.c` (creation), `shape.c` (`b3CreateShapeInternal` and `b3DestroyShapeInternal`) and `joint.c` (`b3CreateJoint` and `b3DestroyJointInternal`, both bodies). The §5.2 table listed those functions only in the shapes and joints rows, so the bodies row, which is the migration list for the body record's write accessor, omitted two files' writes. The row now lists them with the fields they write. This is a correction to a migration list, not a completeness rule: the const-qualified and incomplete record types are what make a missed writer a compile error (§5.2), and the row says which callers the one-time migration redirects.
- **Unresolved item:** the perf-review file was excluded from Codex's scope by the prompt, so the §11.1 baseline figures were not checked by Codex; they are quoted from that review and gated by §12 test 5.

## Status

One finding, Medium, down from two. It is a new category (an incomplete migration row, not an entry, ordering or hash issue) and no consequence of round 7's fixes; nothing reopened settled rationale. Counts are now 2 -> 2 -> 1 with no High in the last round's counterpart list except round 7's; the trend is turning down but a clean round has not occurred. Next: another fresh full-scope round 9, same brief.

`Round 8: 1 findings (0H-1M-0L), 1 applied, 0 declined`

`Series total: 22 findings (7H-12M-3L) across 8 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L)`
