---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=high
mode: broad (round 6), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 6

## Review report (Codex final message)

## Summary

The hot image and cold journal split is broadly consistent with the source, but the proposed restore leaves two cross-step values incorrect in specific cases. The caller contract also omits tree traversal effects in the mover queries.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1, §7.4, §9 | Rewinding to a tick when a body was asleep can leave its shape bounds from a later awake tick. After wake, `solver.c` writes `shape->aabb` and `fatAABBs`; those hot writes are not journaled. The target tick has no image for the sleeping shape, and `b3WakeSolverSet` does not write its bounds (`solver_set.c`). The restored tree can therefore retain the wrong proxy bounds too. | Preserve shape bounds at the cold-to-hot boundary, or otherwise restore the last cold bounds when rewinding across wake. Test sleep → wake → move → rewind to the sleeping tick. |
| 2 | High | §5.1, §9 | §9 clears `bodyMoveEvents` but restores awake `b3Body` records containing non-null `bodyMoveIndex`. A subsequent `b3Body_SetAwake(id, false)` calls `b3TrySleepIsland`, which indexes the cleared event array through that value (`solver.c`, `body.c`, `solver_set.c`, `physics_world.c`). | Clear or rebase every restored `bodyMoveIndex` when dropping move events. Test an immediate forced sleep after rewind. |
| 3 | Low | §7.4, §10 | The traversal-order callback contract omits `b3World_CollideMover` and `b3World_CastMover`. Both invoke caller callbacks during tree traversal, so callback order can change after tree reconstruction (`physics_world.c`). | Add both APIs to the caller contract and to tests using pure, order-independent callbacks. |

## Checked, no change

- Awake body state is held in contiguous solver-set arrays, while contact records and manifolds require gathering (`solver_set.h`, `contact.h`).
- Sensor overlap changes are detected independently of the owning body's awake state (`sensor.c`).
- Proxy category bits are stored per proxy, and `b3DynamicTree_SetCategoryBits` updates the value queried by the tree (`dynamic_tree.c`).
- The closest-ray callback currently overwrites equal-fraction results, as §7.4 states (`physics_world.c`).

## Proposed edits

1. In §5–§9, specify how bounds are saved when a sleeping shape becomes hot and how a rewind restores those bounds when its target owner is asleep. Include the case in §12's restore tests.
2. In §9, state how `bodyMoveIndex` is made consistent with cleared move events; add the immediate `SetAwake(false)` test to §12.
3. In §7.4 and §10, include the two mover APIs in the traversal-order callback rule.

## Unresolved / disagreements

I disagree with declined finding 1 for the **rewind direction**. It correctly observes that a sleeping shape does not move before wake, but after wake `solver.c` can overwrite its bounds without journaling them. Rewinding from that later awake state to the earlier sleeping tick has neither a target image nor a journal entry for the old bounds.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (no `-m`/`-c` override — CLI default model/effort
for this invocation, foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash tool
`timeout: 600000`. Start 2026-09-28T19:50:44+10:00, end 19:53:53+10:00, `EXIT_CODE 0` — no resume
needed, returned inline in under 4 minutes. Raw log had the final message duplicated (a `codex
exec` streaming artifact); one copy kept above. Codex ran read-only; working tree was untouched by
it going in. The curated declined-findings list from rounds 1–5 (eight items) was included in the
prompt; Codex explicitly disputed declined item 1 (sleep→wake bounds) rather than silently
re-raising it, correctly distinguishing the rewind *direction* the original decline didn't
consider — see finding 1 below.

Each finding was independently re-verified against source (via a forked sub-agent tracing the
exact call sites) before any doc edit:

- **#1 (a body's shape `aabb`/`fatAABBs` written by the post-wake hot path has no journal entry
  to undo when rewinding past the wake to the still-sleeping tick), CONFIRMED, applied.** Traced
  `b3TrySleepIsland`/`b3WakeSolverSet` (`solver_set.c`): both move `b3BodySim`/`b3BodyState`
  between solver-set arrays as a structural write (journaled unconditionally, per §7.1), but
  neither touches `shape->aabb` or `world->fatAABBs` — confirmed no shape/AABB/proxy code in
  either function. Those fields are written only by the awake hot path (`solver.c`, every step,
  for every awake body's shapes) and by the explicit non-awake setters §5.2 already lists
  (`body.c`, `shape.c`). So a sleeping body's shape bounds are correctly frozen while asleep
  (declined finding 1 from round 1 is still correct for *that* claim), but the *first* hot
  rewrite after the body wakes again is an ordinary awake-path write, exempt from journaling by
  §7.1's general rule on the assumption that "the image covers it" — true only for ticks at or
  after that wake. A `Rewind(T)` to a tick before the wake finds no image for the shape (not
  awake at T) and no journal entry undoing that first hot write (never journaled), leaving
  `shape->aabb`/`fatAABBs` at their post-wake value instead of T's frozen value — a real hole in
  §7.1's own sufficiency claim ("a body's record is either in image T or was last written by a
  journaled event between T and P"): the first post-wake hot write is neither. Fixed by having
  the wake transition itself (`b3WakeSolverSet`, already a "wake" structural function per §7.1)
  checkpoint the woken body's shape bounds into the journal, giving the journal walk something to
  reverse back to when a later rewind crosses back over this wake boundary — the same role
  ownership-transfer already plays for solver-set-owned arrays, extended to shape bounds, which
  live in the separate world-sized `shapes[]` array and aren't covered by that transfer.

- **#2 (`bodyMoveIndex` in the restored image can point past the just-cleared move-event array),
  CONFIRMED, applied.** Traced `bodyMoveIndex` (`body.h`): set unconditionally for every awake
  body every step in `b3FinalizeBodiesTask` (`solver.c`) as an index into `world->bodyMoveEvents`,
  which §5.1 already lists as part of the imaged `b3Body` record. `bodyMoveEvents` itself is
  resized to the current awake count and cleared at the start of each `b3World_Step`
  (`physics_world.c`) and is not part of the image. After `Rewind(T)`, the image restores T's
  `bodyMoveIndex` values, sized for T's awake population, while the live `bodyMoveEvents` array is
  whatever §9 step 5 leaves it as. §14's own correction loop replays API calls (which can include
  `b3Body_SetAwake(id, false)`) before the next `b3World_Step`; `b3TrySleepIsland`
  (`solver_set.c`) indexes `bodyMoveEvents` through the stale `bodyMoveIndex` via a
  bounds-asserted array access (`container.h`) — an assertion failure in debug, undefined
  behavior in release. Fixed by having §9 step 5 also clear move events and reset every restored
  body's `bodyMoveIndex` to none, so nothing indexes the cleared array until the next step
  rewrites it.

- **#3 (`b3World_CollideMover`/`b3World_CastMover` missing from the traversal-order callback
  contract), CONFIRMED, applied.** Confirmed both exist as public API
  (`include/box3d/box3d.h`, `physics_world.c`), each taking a caller callback and querying the
  body-type trees, invoking the callback per traversal candidate — the same order-sensitive
  shape as `b3World_CastShape`/`b3World_OverlapShape`/`b3World_OverlapAABB`, which §7.4 already
  names. Neither appeared in §7.4's list or §10's "cast or overlap" wording. Added both to §7.4's
  API list and reworded §10's and §7.4's callback-order bullets from "cast or overlap" to "cast,
  overlap, or mover".

Also added "forced sleep/wake toggles" to §12.2's fuzz-churn list (previously creates, destroys,
setters on sleeping bodies, explosions), since that is the churn class exercising the new
wake-checkpoint mechanism finding #1 required — closing requirement 7 ("verified by test, not by
audit") for the new mechanism rather than leaving it to a future audit to notice the gap.

## Status

Round 6 of an ongoing series (rounds 1–5 committed or pending commit). All 3 findings were
genuine and applied — the second round in this series with zero declines (after round 5). Finding
#1 is the same class of defect as round 5's #1 (a live-state-corrupting restore-algorithm bug
surviving multiple prior full-scope rounds): round 1 declined a *related* claim about the
sleep→wake bounds transition, correctly, for the direction it was asked about (does waking need a
journal entry for bounds that haven't changed) — but never considered the *other* direction
(does a later, un-journaled awake rewrite of those same bounds break a deeper rewind past the
wake). A fresh reviewer, prompted with the declined finding as context, independently re-derived
the related-but-distinct correct claim rather than either blindly re-raising or blindly accepting
the decline — which is the review process working as designed (WORKFLOW.md's instruction to
re-verify a disputed decline "with the same rigor as any new finding", not dismiss it for having
been raised before). Findings #2 and #3 are narrower coverage gaps of the kind recent rounds have
mostly been finding (a missed restored-value invalidation, an incomplete API enumeration). Finding
rate by round (8, 8, 8, 7, 8, 3) shows round 6 lower than the round 1–5 run, but the *class* of
finding #1 (silent bit-exactness violation in the restore algorithm, not just a documentation
gap) argues against reading the lower count as convergence yet. Another full-scope round is
warranted.
