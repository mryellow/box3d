---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 21), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 21

## Review report (Codex final message)

## Summary

The hot-image and cold-journal approach is broadly consistent with the source, but the specified tree restore cannot yet guarantee a valid broad phase. Shape lifetime changes need explicit proxy handling, and §7.4's restore bullets conflict when a body is disabled at the target tick. I also found a lost proxy property, a callback input omitted from the caller contract, and an O(world) restore pass.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §7.4 | The journal lists shape record writes but no proxy create/destroy operation for replay. In `src/shape.c`, destroying a shape removes its tree proxy; copying the shape record back cannot recreate that proxy. Rewinding past a creation likewise leaves a live proxy for a shape that no longer exists. | Define proxy presence reconciliation for every shape whose lifetime changed, before updating proxy properties. |
| 2 | High | §7.4 | The first restore bullet removes a proxy when its body is disabled at T, yet says subsequent bullets operate on an existing proxy. A disabled body can also change type (`src/body.c`), and `b3ResetProxy` can update its shape while no proxy exists (`src/shape.c`). Those entries can make later move, category, type, or reset bullets act on an absent proxy. | Make restored proxy presence the gate for all later operations; combine applicable changes into one final action per shape. |
| 3 | High | §5.2, §7.4 | A proxy's category bits can differ from `shape->filter` after `b3Shape_SetFilter(..., false)` (`src/shape.c`). Disabling a body or destroying a shape discards that proxy (`src/body.c`, `src/shape.c`), but the listed journal entries do not preserve its category on destruction. Recreating it at T can therefore use the wrong bits. | Save the proxy's actual category bits before every proxy destruction that a retained restore might reverse, including disable and shape destruction. |
| 4 | Medium | §4, §9 | §9 requires checking **every shape id** for a debug handle and generation change. That makes restore O(world), contrary to §4's stated restore bound, even when few shapes changed. | Track affected shape ids during the journal walk and check handles only for those ids. |
| 5 | Medium | §10 | The caller contract says rewind undoes every setter except callback configuration. `b3World_SetUserData` exists (`src/physics_world.c`) and is neither imaged nor journaled. A callback that reads it through the public getter can see P's value while replaying from T. This independently contradicts the supplied earlier resolution's assertion that no setter exists. | Restore world userData, or explicitly require the caller to reinstall its T-time value and replay later changes, as with callback configuration. |

## Checked, no change

- Awake body sims and states are stored in dense solver-set arrays; gathering awake contact records separately matches the storage in `src/solver_set.h`, `src/contact.h`, and `src/physics_world.h`.
- `b3ResetProxy` can recreate a proxy without changing its bounds or body type, supporting a distinct reset entry (`src/shape.c`).
- The current closest-ray callback overwrites its result on each reported hit, so the proposed tie-break addresses a real traversal-order dependence (`src/physics_world.c`).
- Sensor overlap results are sorted and deduplicated before event comparison (`src/sensor.c`).
- Id allocation uses a free-list pop or a bump index, as the journal design accounts for (`src/id_pool.c`).

## Proposed edits

1. In §7.1 and §7.4, specify how replay identifies shape lifetime changes and reconciles tree presence and `proxyKey` before any proxy property operation.
2. Rewrite §7.4 as a per-shape decision: determine whether a proxy must exist at T; if absent, stop; if present, create or retain it in T's tree, then apply T's bounds and category, any required reset, and moved membership. Add tests combining disable with type change and proxy reset in both rewind directions.
3. Add actual proxy category bits to reversible proxy-destruction state, and test an unsynced filter followed by disable or destruction and rewind.
4. Replace §9's full shape-id sweep with a list of slots whose shape generation or existence changed.
5. Amend §10's setter contract to cover `b3World_SetUserData` explicitly.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-29T05:50:14+10:00, end 05:52:55+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 3 minutes. Codex ran read-only; the working tree was
untouched by it going in (round 20's edits were already applied to the working file, confirmed via
`git status` before the call). The curated declined-findings list used for rounds 18–20 (ten
items — nine from rounds 1–7 plus the round-14 pool-`nextIndex` item) was reconstructed from each
round file's own Post-review verification section (the original prompt text itself was never
persisted verbatim, per WORKFLOW.md) and included in the prompt; Codex did not re-raise eight of
them, but finding #5 independently re-derives and disputes declined item 3 (world `userData` has
no setter) — checked below with the same rigor as a new finding, per WORKFLOW.md's rule for a
disputed decline.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, §7.4's own restore
bullets were re-read for interaction between the disabled-set reconciliation bullet round 19/20
added and the pre-existing fat-AABB/category/body-type/reset bullet that follows it — specifically
whether a shape whose proxy the first bullet destroys or creates could still be operated on by a
later bullet written to assume a continuously-live proxy. This traced to a real, unresolved
interaction (a shape with both a disabled-transfer entry and a proxy-reset entry in the same walked
range: the reset clause says "destroy the live proxy and recreate it," but the disabled-transfer
bullet may have already destroyed it with nothing live to destroy, or the shape may be disabled at
T and should have no proxy at all — the reset clause has no disabled-at-T gate). This matches
Codex's own finding #2 below exactly; noted here rather than pre-emptively patched, since the
correct fix needed to be verified against actual source behavior (which it now has been) rather
than guessed at before Codex's independent pass.

All five findings verified directly against source:

- **#1 and #2 (the journal/restore design has no explicit proxy create/destroy step for a shape
  whose existence itself changes across the walked range, and the existing disabled-set proxy
  bullet's "subsequent bullets operate on an existing proxy" assumption is violated by the
  body-type and proxy-reset clauses that follow it), CONFIRMED, applied together as one fix.**
  Traced `b3CreateShapeInternal` (`shape.c`): sets `shape->proxyKey = B3_NULL_INDEX` unconditionally,
  then calls `b3CreateShapeProxy` only `if (body->setIndex != b3_disabledSet)` — confirming proxy
  creation is a distinct broad-phase-tree operation, never implied by the shape record's own bytes.
  Traced `b3DestroyShapeInternal` (`shape.c`): calls `b3DestroyShapeProxy` explicitly before the
  record itself is freed — again a separate tree operation. Neither is reachable from a plain
  "record write" undo/redo (copy old/new bytes), which only touches the shape struct's own fields,
  confirming §7.4's existing "proxies created or destroyed after T are handled by the journaled
  shape create/destroy" was an unsubstantiated assertion, not a specified mechanism — finding #1
  is real. Re-verified finding #2's exact scenario against source: `b3ResetProxy` (`shape.c`) has
  an explicit `if (shape->proxyKey != B3_NULL_INDEX) { ... } else { only recompute AABB }` branch,
  confirming the live engine itself already treats "no live proxy" as a distinct, valid state for
  a shape whose filter changes while disabled — so the design's restore algorithm must do the same,
  and its reset/body-type clauses, as worded before this round, did not.

  Fixed both by replacing the two-bullet block with a single ordered two-part procedure: a
  **Presence** bullet, extended to also cover a shape's own creation/destruction (closing finding
  #1 with no new journal entry — shape existence at T is already answerable from the shape record's
  own already-restored bytes plus the id pool's already-restored alloc state, both restored earlier
  in §9 by the time §7.4 runs, the same "already implied by an earlier restore step" pattern round
  19 used), followed by a **Properties** bullet explicitly scoped to shapes whose proxy the
  Presence bullet left untouched (was live before and stays live after) — so a shape the Presence
  bullet just created or destroyed can never reach a Properties clause that assumes a continuously-
  live proxy. This also directly resolves finding #3 (below) as a consequence of the same
  restructuring, rather than needing its own separate fix.

- **#3 (a live proxy's category bits, once diverged from `shape->filter` via an
  `invokeContacts=false` filter change, is not preserved across a proxy destroy at disable or shape
  destroy, so recreating the proxy later can use the wrong bits), CONFIRMED as a real risk in the
  old two-bullet wording, but resolved by the #1/#2 restructuring rather than needing a new
  reversible-state field.** Traced `b3CreateShapeProxy` (`shape.c`) precisely: every call site that
  creates a fresh proxy — initial creation, `b3ResetProxy`'s destroy-and-recreate branch, and (per
  round 19's own tracing) `b3Body_Enable` — passes `shape->filter.categoryBits` directly, **never**
  any separately-tracked "proxy categoryBits" value. The journaled categoryBits entry (§5.2) exists
  solely to track the one case where a live proxy *persists* through an `invokeContacts=false`
  change and its bits genuinely diverge from the shape record — not to serve as a source of truth
  for a freshly (re)created proxy. The old bullet 1's "create one... with T's journaled category
  bits" was therefore already subtly wrong in principle (it would have used the divergence-tracking
  value even for a fresh creation, where the correct source is `shape->filter.categoryBits`,
  already independently restored). The rewritten Presence bullet creates using T's restored
  `shape->filter` directly, matching live engine behavior exactly, and the Properties bullet's
  categoryBits clause is now correctly scoped to only the continuously-live-proxy divergence case
  the journaled value actually covers. No new journal entry needed.

- **#4 (§9 step 5's debug-handle reconciliation checks every shape id in the world, an O(world)
  pass that contradicts the ring's O(awake + journal) restore-cost model), CONFIRMED, applied.**
  Confirmed in `shape.c` that `shape->generation` is incremented only inside `b3CreateShapeInternal`
  (`shape->generation += 1`), itself already an unconditionally-journaled structural write (§7.1's
  general rule) — so a shape whose record was not touched during the walked range provably has an
  unchanged generation, by the same "already implied by an earlier restore step" reasoning used for
  finding #1. Fixed by scoping the sweep to shapes with a journaled record in the walked range,
  dropping the O(world) requirement to O(journal) — the identical fix pattern round 20's own
  finding #4 used for the disabled-set proxy scan.

- **#5 (world `b3World_SetUserData` exists, contradicting the previously-declined finding that
  claimed no setter exists, and `Rewind` neither images nor journals it, unlike the caller-contract
  treatment given to callback configuration), CONFIRMED — and it independently overturns a stale
  declined item, applied.** `grep`-confirmed `b3World_SetUserData`/`b3World_GetUserData` exist in
  both `include/box3d/box3d.h` and `src/physics_world.c` — the round-3/round-1 decline's "no setter
  found" claim is factually wrong against current source (whether it was wrong at the time or the
  setter was added later doesn't matter; it's wrong now, and this declined item is retired from the
  list carried into future rounds). Confirmed `world->userData` (`physics_world.h`) is written only
  at creation and by this setter, and read only by the paired getter — never read internally by any
  simulation code path (`grep`-checked every `userData` occurrence in `physics_world.c`) — so unlike
  callback configuration (a function pointer selecting *behavior*, needing caller replay at the
  correct point), it is inert opaque data with none of that ordering complexity, exactly the same
  shape as body/shape/joint `userData`, which §13 already notes are carried naturally by their own
  per-record images. Chose imaging over the caller-contract-exception treatment Codex proposed as
  the smaller, more consistent fix: added `userData` to §5.1's "world scalars" row (flat-copied
  every tick like every other scalar field) rather than adding a fifth exception alongside the four
  callback setters. Caught a second-order consequence of this choice before finalizing: §12's full
  state hash says "world scalars and flags" unqualified, and a raw pointer value is not portable
  across processes — hashing it would break the cross-worker (§12 test 5) and cross-platform (§3)
  bit-exactness the hash exists to verify. Fixed by excluding `userData` from §12's hash
  enumeration explicitly, the same treatment body/shape/joint `userData` already receives there
  (present in the image, absent from the itemized hash list) — self-caught before it became a
  finding for a later round to catch instead.

## Status

Round 21 of an ongoing series (rounds 1–20 committed, most recently as commit 197055e). All five
findings genuine and applied (two of them, #1 and #2, sharing one combined fix; #3 resolved as a
consequence of that same fix rather than needing independent treatment) — a sixth consecutive
full-scope round with zero declines (after rounds 18, 19, 20), and the fourth round in a row to
find a genuine defect in the tree-proxy-lifecycle mechanism specifically (after rounds 19 and 20's
own findings there), each time a different facet of the same underlying pattern: §7.4's restore
bullets were extended incrementally, one proxy-lifecycle-affecting event at a time (body-type
change, then proxy-reset, then disable/enable), each addition implicitly assuming the ones already
there had gotten the "does a live proxy exist" question right — they hadn't, fully, until this
round's restructuring made proxy presence a single up-front decision every later clause is now
provably scoped against. Finding #5 is notable for a different reason: it's the first finding in
this series (across 21 rounds) to directly overturn a previously-declined finding's *factual*
premise via independent source citation, exactly the scenario WORKFLOW.md's re-verification rule
for disputed declines exists to catch — the declined-findings list carried into round 22 drops this
item. Given the proxy-lifecycle mechanism has now needed correction in four consecutive rounds and
this round's fix was a structural rewrite (not another bolted-on special case), another full-scope
round is warranted, with attention to whether the new Presence/Properties split itself introduces
any fresh gap the way round 19's own fix did for round 20 to find.
