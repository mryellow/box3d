---
title: Codex review — rewind-only history ring, round 7
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Attempt 1 killed by the shell timeout, resumed once (see Post-review verification).
session: 01a0ecd8-f8f8-7ac0-9643-a10b176feb72
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 7

## Review report (Codex final message)

## Summary

The design covers the main state and transition paths, but two specifications need correction before implementation: the handoff of owned blocks during destruction, and the proposed proxy-pool hash normalization.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.2, 7.1 | The proposed pool-free entry takes ownership of a destroyed shape’s materials and a destroyed contact’s manifolds and mesh cache. In the source, shape.c and body.c free shape allocations before `b3FreeId`; contact.c frees manifolds and the cache before its `b3FreeId`. If the journal call is placed only inside the proposed slot-pool free mutator, those blocks are already gone. Destroying a touching contact after T would therefore leave `Rewind(T)` without its manifold block to reattach. | Specify a destruction handoff before each block is freed or cleared, and the required source-order changes for both direct shape destruction and body destruction. Make the pool-free entry’s ownership transfer atomic with that path. |
| 2 | Medium | §§7.4, 12.1 | The hash rule drops an ascending free-list suffix ending at proxy capacity, then hashes the first id *after* that suffix. dynamic_tree.c grows capacity by appending ascending free ids. A tree with capacity 16 and free suffix `3…15` can rewind after growth to capacity 24 with suffix `3…23`; both hand out the same subsequent ids, but the specified “after” values are 16 and 24, producing different hashes for equivalent allocation behavior. | Define the canonical hash using the start of the discarded suffix, or another capacity-independent representation of the future allocation sequence. Add a restore-and-hash test for this case. |

## Checked, no change

- The recording hash covers transforms and velocities, and `ScrubBackward` checks replayed hashes; the design correctly limits what that test demonstrates.
- The awake set, sleeping sets, graph arrays, sensor overlap swap, and end-event flip match the source paths described.
- The source confirms the separate tree proxy free lists, the CCD traversal dependence, and the explosion callback’s traversal-ordered wake and impulse writes.
- The treatment of `stepIndex`, world host configuration, borrowed geometry, and name-hash collisions matches the inspected code and stated caller contract.

## Proposed edits

Specify the pre-free ownership transfer and call ordering for shape and contact destruction in §§5.2 and 7.1. Correct the proxy-pool hash canonicalization in §12.1 and add its capacity-growth case to verification.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 6. Start 2026-09-29T21:07:17+10:00, end 21:16:17, exit code 124. Attempt 2, resume of session 01a0ecd8-f8f8-7ac0-9643-a10b176feb72 with `-m gpt-6-sol -c model_reasoning_effort="high"` before `resume` and a finalize-only prompt, start 21:16:22, end 21:16:48, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. The target doc had no uncommitted diff before the round. Codex's markdown file links were reduced to bare file names when copied here.

- **#1 (High), applied.** In source, `b3DestroyShapeAllocations` (`shape.c`, called from `b3DestroyShapeInternal` and from `b3DestroyBody`'s shape loop in `body.c`) frees the `materials` block and clears the pointer and count before `b3FreeId`; `b3DestroyContact` (`contact.c`) frees the manifold block and destroys the mesh triangle cache before its `b3FreeId`. A pool free entry made inside the slot pool's free mutator would therefore find nothing to take. §5.2 now says those destroy paths leave the blocks on the record and the slot pool's free moves them into the pool free entry before freeing the id, which is what §7.1's pool free payload already assumed. The choke point stays the one function (the slot pool's free), so this is not a call-site list.
- **#2 (Medium), applied.** `b3DynamicTree_CreateProxy` (`dynamic_tree.c`) grows capacity by appending ids from the old capacity, so a free list whose trailing ascending run ends at the last capacity slot has the same allocation sequence at capacity 16 and 24, but §12 test 1's canonical form hashed "the first id the allocator would hand out after" the dropped run, which is the capacity (16 versus 24). It now hashes the first id of the dropped run, or the capacity when there is no such run, which is 16 in both. Test 2's existing clause (a step that grows a tree's proxy capacity followed by a rewind across it) compares the hash across exactly this case, so no test clause was added.

## Status

Two findings, one High. #1 is a new category (destroy-path ordering against the pool free entry; earlier rounds audited what each entry holds, not the order in which the source frees it against the entry's call point). #2 is a consequence of an earlier round's proxy-pool hash rule (round 1 to 4 era text), not of round 6's fix. Neither reopened settled rationale. Counts 2 -> 2 after 3; the trend is flat and findings are still in new categories, so the series is not near a clean round. Next: another fresh full-scope round 8, same brief.

`Round 7: 2 findings (1H-1M-0L), 2 applied, 0 declined`

`Series total: 21 findings (7H-11M-3L) across 7 rounds`

`Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L)`
