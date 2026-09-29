---
title: Codex review — rewind-only history ring, round 18
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 18

## Review report (Codex final message)

## Summary

The rewind-only approach is plausible, and the source supports its core sleep, sensor, and recording claims. I found three changes needed before the document’s completeness and tree-layout claims hold.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.2, §7.1 | The record-write rule contradicts itself. §7.1 requires every structural record write to be journaled, while §5.2 permits a structural write to an already-awake record through the unjournaled hot accessor. This leaves the claimed compile-time boundary ambiguous. | Choose one rule and state which accessor every structural path must use, including writes to awake records. |
| 2 | Medium | §7.4, §13 | The tree-writer migration list is incomplete. joint.c marks proxies moved, and physics_world.c rebuilds the static tree outside `broad_phase.c`. Both conflict with §7.4’s proposed boundary that places every world-tree writer there. | Include these paths in phase 1’s migration and compile-time check. |
| 3 | Low | §7.4, §10 | Tree layout is externally observable beyond callback order. b3World_GetBounds returns tree-root bounds; EnlargeProxy can leave ancestors enlarged. Reconstructing the tree can therefore change that query’s result immediately after rewind. | State that `GetBounds` may change after rewind, or preserve its reported bounds if identical query results are required. |

## Checked, no change

- The existing recording hash covers body transforms and velocities, and ScrubBackward checks those hashes after backward seeks.
- Sensor processing swaps overlap buffers, clears hits, and sorts and deduplicates current overlaps as described.
- Solver-set wake and sleep paths move bodies, contacts, joints, and islands between sets as the design’s transition analysis assumes.
- Proxy IDs use a per-tree free list; the design correctly treats identity separately from tree layout.
- The FAQ says there is currently no rollback mechanism.

## Proposed edits

Resolve the structural-write rule, add the two omitted tree writers to the migration worklist, and clarify the post-rewind `GetBounds` contract.

## Unresolved / disagreements

The document’s performance baseline cites a file under `docs/designs/reviews/`. I did not read it under the review constraint, so those measurements remain unverified here.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 17 with no curated decline list. Session 01a0ef25-f039-7b23-bd37-1362624fe388. Start 2026-09-30T07:50:34+10:00, end 07:55:02, exit code 0 (captured in its own variable), no resume needed. `git status` shows no change Codex could have made. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), applied.** §5.2 said a structural function writes through its write accessor and then allowed a structural write through the hot accessor to an awake owner's record, against §7.1's "journaled unconditionally". The second clause is deleted: a structural function never uses the hot accessor. The journal is still correct, since the structural entry over-journals awake records harmlessly and the walk undoes it in reverse order.
- **#2 (Medium), applied in part.** `joint.c` calls `b3BroadPhase_MarkProxyMovedSerial`, a `broad_phase.c` function, so it is already inside the boundary and needs no change. `b3World_RebuildStaticTree` in `physics_world.c` calls `b3DynamicTree_Rebuild` on the static tree directly, which §7.4's "every function that writes one is a function in that file" omitted. §7.4 now moves that rebuild into `broad_phase.c` with solver.c's enlarge and refit.
- **#3 (Low), applied.** `b3World_GetBounds` returns each tree's `b3DynamicTree_GetRootBounds`, and an enlarged proxy leaves its ancestors enlarged, so the result follows layout. §10's query bullet now lists it beside traversal order.

## Status

Three findings, two Medium and one Low, from three the round before. Categories: a self-contradicting write rule (#1), an incomplete boundary enumeration (#2, the tree-writer list inside §7.4, a completeness-by-listing symptom AC-2 warns about; the boundary is still enforced by the pointer-to-const type, which is what the compiler checks), an observable layout-dependent query (#3). None was a consequence of round 17's fixes. None reopened stated rationale. A clean round has not occurred, and the series has not converged. Next: another fresh full-scope round 19 at Sol/Medium, same brief. Nothing committed.

Round 18: 3 findings (0H-2M-1L), 3 applied, 0 declined
Series total: 61 findings (8H-34M-19L) across 18 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L)
