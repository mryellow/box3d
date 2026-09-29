---
title: Codex review — rewind-only history ring, round 16
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-30
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md — round 16

## Review report (Codex final message)

## Summary

The rewind design is broadly consistent with the source, but it omits state that crosses a step boundary. A rewind after CCD can therefore change the next step’s sensor events.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.1, 5.3, 6, 9, 12 | The design images sensor `overlaps2` but treats `hits` as scratch. CCD appends hits near the end of a step; the next sensor pass consumes them. Rewinding to the step that generated a hit loses it, so replayed overlaps and events can differ. The existing serializer preserves all three arrays. | Image and restore each sensor’s pending `hits`; include them in the state hash and add a rewind test whose target is immediately after CCD generated a hit. |
| 2 | Low | §§8, 14 | The `tickCount` API comment says the window can extend by an extra capture interval. §8 instead evicts while the oldest image is more than `tickCount` ticks behind, provided a later image exists; with the effective interval capped at `tickCount`, that extra interval cannot occur under the stated rule. | Make the API comment and eviction rule describe the same bound. |
| 3 | Low | §14 | `GetRestorableTick` maps a request later than `currentTick` to the newest image, while `Rewind` rejects a future tick. A caller using the helper without the example’s `serverTick <= currentTick` precondition can silently rewind to the wrong tick. | Return `UINT64_MAX` for a future request, or make the precondition explicit in the API contract. |

## Checked, no change

- The recording test does restore and re-step a four-box scene; its hash covers transforms and velocities, as the design states.
- The id pools use a LIFO free list and bump index.
- Contact creation and destruction update body contact edges, including neighbouring contact records (contact.c).
- The static tree is outside the normal dynamic and kinematic rebuild task. Proxy ids have a separate free list (dynamic_tree.c).
- The proposed CCD and explosion order changes address real traversal order dependencies (solver.c, physics_world.c).

## Proposed edits

Correct the sensor state inventory, capture, restore, hash, and tests for pending `hits`. Then align the two API comments with the ring rules.

## Unresolved / disagreements

The performance figures cite a prior review under `docs/designs/reviews/`; I did not read it, as requested. Source inspection cannot establish the proposed cost targets or whether the solver owner accepts the changed CCD and explosion results. No files were modified.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: attempt 1, `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540`, tool timeout 600000 ms, the same fresh full-scope prompt as round 15 with no curated decline list. Session 01a0eefe-7c84-7f70-b1f6-565716d1e8f9. Start 2026-09-30T07:07:29+10:00, end 07:14:05, exit code 0, no resume needed. `git status` shows no change Codex could have made. Codex's markdown file links were reduced to their link text when copied here.

- **#1 (High), declined, rationale added to the doc.** `sensor->hits` is appended in `b3Solve` and the continuous sweeps (`solver.c`) and consumed and cleared by `b3OverlapSensors`, which `b3World_Step` (`physics_world.c`) calls unconditionally after `b3Solve` in the same step. So `hits` is empty at every step boundary, and a rewind target is always a boundary. The serializer preserves it because it does not assume a step boundary. When the world has no sensors `b3OverlapSensors` returns early, and then no hit can be pushed since there is no sensor to receive one. §5.3 now states that `hits` is empty at every boundary.
- **#2 (Low), applied.** With the effective interval at most `tickCount` (asserted since round 15), the retained oldest image is never more than `tickCount` behind `historyTick`, so §14's "up to one interval more" and §8's "plus the interval to the next image" were wrong. Both now state the actual bound.
- **#3 (Low), declined, rationale added to the doc.** `b3World_GetRestorableTick` applies one rule, newest imaged tick at or before the argument, to every argument; returning the newest image for a future tick is that rule, not a special case, and the caller contract already says the request must not be later than `currentTick`. Special-casing the future would add a branch with a different answer. §14's comment now says the rule is uniform.

## Status

Three findings, one High and two Low, from four the round before. Categories: a state-crossing-the-boundary claim that the step ordering refutes (#1, declined), an internal inconsistency between two window-bound statements left by round 15's assertion (#2, a consequence of round 15's fix), an API convention (#3, declined). #1 and #3 asked the doc for rationale it lacked; both now have it. No finding reopened rationale the doc already stated. A clean round has not occurred, and the series has not converged. Next: another fresh full-scope round 17, same brief. Nothing committed; the user will say when.

Round 16: 3 findings (1H-0M-2L), 1 applied, 2 declined
Series total: 55 findings (8H-31M-16L) across 16 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L)
