---
title: Codex review — rewind-only history ring, round 19
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 19

## Review report (Codex final message)

## Summary

The rewind and undo model is broadly consistent with the source paths checked. I found one API example that can silently lose a correction and one query-result caveat that needs to be stated more precisely.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §14, API sketch | `GetRestorableTick` returns the newest image when given a future tick. In the sample correction flow, `serverTick > now` therefore makes `Rewind` succeed, but neither branch applies the correction. | Return `UINT64_MAX` for a future tick, or guard `serverTick > now` in the example and specify how callers handle it. |
| 2 | Low | §7.4, §10 | The query caveat covers callback order and bounds, but a layout change can also change **which shape** a closest cast returns when hits tie. `b3World_CastRayClosest` overwrites its result on each accepted hit, and the tree cast accepts a fraction equal to the current maximum. | State explicitly that tied closest-hit identity is outside the rewind guarantee; add a tied-hit case to the query-contract tests or choose a stable tie-break. |

## Checked, no change

- `ScrubBackward` supports the document’s limited claim about restoring and replaying its body-state hash; `b3HashWorldState` does not cover the proposed full-state contract.
- The source supports the need to handle CCD sensor selection and `b3World_Explode` traversal order before treating tree layout as derived.
- The sensor pass swaps and clears overlap buffers each step, while `overlaps2` carries the overlap state needed at the next step boundary.
- The documented distinction between proxy identity and tree layout matches the tree’s LIFO proxy free list and the shape’s stored proxy key.
- The sleep and wake cases listed in §7.1 have consistent undo paths under the proposed rule: image awake state at T, journal structural changes, and record pre-wake state for cold data that will become hot.

## Proposed edits

Apply the two actions in the findings table and add a closest-cast tie case to §12’s verification plan.

## Unresolved / disagreements

The document’s §16 decisions about accepting the order changes and supporting forward scrub remain open product choices. I found no additional source-based disagreement with its rewind-only recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 18 with no curated decline list. Session 01a0ef2e-5523-7ef0-a9db-72b118f9835b. Start 2026-09-30T07:59:45+10:00, end 08:06:26, exit code 0, no resume needed. `git status` shows no change Codex could have made. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (Medium), declined as an API change, sample sharpened.** The third fresh reviewer (rounds 16, 17, 19) to land on `GetRestorableTick` with a future tick. This round's argument is concrete about the sample: with `serverTick > now`, `Rewind` succeeds and no branch applies the correction. That is a real gap in the sample, though not in the API: `GetRestorableTick` applies one rule to every argument (newest imaged tick at or before it), and a future server tick is not a rewind at all, since that tick has not been simulated. The sample's comment now says so and says the correction is applied when that tick is stepped. The return value stays.
- **#2 (Low), applied.** `b3RayCastClosestFcn` overwrites its result on each accepted hit and the tree's cast accepts a value equal to `maxFraction` (`dynamic_tree.c`, checked), so which of several equal-fraction hits a closest cast reports follows traversal order. §10's query bullet now lists it. No tie-break is added: it is a query result, not state, and query order is already declared out of the contract. The suggested tie test is not added for the same reason.

## Status

Two findings, one Medium and one Low, from three the round before. Categories: a repeat API-convention finding that this time exposed an ambiguity in the sample (#1), an observable layout-dependent query result of the same kind as round 18's #3 (#2). Neither was a consequence of round 18's fixes, though #2 extends round 18's GetBounds sentence. #1 reopened ground the doc's rationale covered; it engaged the sample but not the stated rule. A clean round has not occurred, and the series has not converged. Next: another fresh full-scope round 20 at Sol/Medium, same brief. Nothing committed.

Round 19: 2 findings (0H-1M-1L), 1 applied, 1 declined
Series total: 63 findings (8H-35M-20L) across 19 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L)
