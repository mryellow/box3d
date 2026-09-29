---
title: Codex review — rewind-only history ring
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort high. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260929-002-rewind-only-history-ring.md

## Review report (Codex final message)

## Summary

The rewind-only approach is coherent, but the proposed restore and ownership rules have gaps. The contact manifold issue can make a rewind use the wrong allocation size; the other findings concern memory accounting and whether the proposed container types actually enforce their ownership rules. This was a read-only source review; no tests were run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §9 | A record-write undo restores `b3Contact.manifoldCount` but leaves its live `manifolds` pointer in place. A structural write can log an awake contact before a later collision changes or frees its manifold block: contact creation rewrites neighbouring contact edges in contact.c, while collision allocates and frees manifolds there. Undo can therefore pair an old count with a block of another size—or a null pointer—before §9 resizes the image contact. `b3FreeManifolds` selects its allocator by count in physics_world.h. | Treat the pointer and allocation count as one owned-block state. Keep live allocation metadata separate during record undo, then restore both from a block entry or the target image. Add a test with a neighbouring contact creation followed by a manifold-count change. |
| 2 | Medium | §5.2, §7.1 | The proposed `b3JournaledArray` exposes `clear` and `set` for `world->sensors`, but its generic payloads and undo rules do not transfer ownership of a sensor’s three heap arrays. Only sensor push and removeswap have special ownership handling. A clear or set could leak, double-free or restore stale pointers. See §5.2, §7.1 and sensor.h. | Give the sensor array a restricted type exposing only the specified push/removeswap operations, or define ownership-safe clear/set entries. |
| 3 | Medium | §6, §8, §14 | Destroyed blocks can sit in the unclosed staging segment between steps, yet §8 charges owned blocks only when a slot is captured. `bytesUsed` omits them, and `stagingCapacityBytes` reports buffer capacity, not the blocks its entries own. A caller that pauses before stepping can retain substantial unreported memory. The destroy paths include contact.c, sensor.c and solver_set.c. | Track and report pending owned bytes as entries append; state whether they count toward the budget target. |
| 4 | Low | §5.3, §8 | Repeated rewinds can leave every distinct discarded-timeline name in `world->names`. The cache frees entries only when destroyed and does not remove names individually (name_cache.c). §5.3 excludes this growth from the ring budget, so memory can grow indefinitely even with a fixed retained window. | Document this as a separate memory cost and expose its usage, or reclaim names when no live or restorable record refers to them. |

## Checked, no change

- The existing recording hash covers transforms and velocities only; §1 correctly limits what `ScrubBackward` demonstrates (recording.c, test_recording.c).
- Sensor overlap processing runs for sensors regardless of owner sleep state, supporting the stated sensor-count cost (sensor.c).
- Proxy IDs have a separate per-tree free list, and world proxy creation and destruction pass through the broad-phase wrappers identified in §7.4 (dynamic_tree.c, broad_phase.c).
- The one-way ring consistently discards slots after rewind. Its operative journal entries specify undo payloads rather than redo payloads; forward replay appears only in the alternatives and open questions (§7–§9, §15–§16).
- §13 correctly identifies the serializer’s host `userData` limitation for phase 0 (world_snapshot.c).

## Proposed edits

Resolve finding 1 in the record and block undo rules before treating restore as allocation-safe. Restrict sensor-array operations as in finding 2, add pending ownership accounting to §8 and `b3HistoryInfo`, and state the name-cache memory policy in §5.3 or §8. Add focused verification cases for the contact sequence and sensor ownership operations.

## Unresolved / disagreements

The §11.1 performance figures cite a file under `docs/designs/reviews/`. I did not read it under the requested scope, so I could not verify those figures. The proposed implementation and full-state tests have not been executed.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="high"`, foreground, stdin from `/dev/null`, `timeout 540` with the tool timeout at 600000 ms, fresh full-scope prompt (target doc and source only; `docs/designs/reviews/` and `WORKFLOW.md` excluded). Start 2026-09-29T19:29:54+10:00, end 2026-09-29T19:37:03+10:00, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. The target doc had no uncommitted diff before the round. AC-1 to AC-4 were applied to the doc before the round: the §5.2 caller table is a migration list whose completeness is enforced by const-qualified and incomplete types, not an enumerated-callers rule, and no shadow structure, second structure to reconcile, or control-flow check was found. Codex's markdown file links were reduced to bare file names when the report was copied here.

- **#1 (High), applied, and extended to a sibling.** Verified in `contact.h`: `b3Contact` holds `manifolds` and `manifoldCount` side by side, and `b3FreeManifolds` in `physics_world.h` picks its allocator by count. `contact.c`'s create and destroy rewrite a neighbouring contact's edge keys, which the doc journals unconditionally even when that contact is awake, and a later collide can change its manifold count and block. The doc excluded only the pointer from record undo, so undo could pair an old count with a live block of another size, and the image scatter's resize and every block entry's realloc would then free with the wrong size. Applied: `manifoldCount` joins the fields record undo and the image scatter leave alone (§7.1, §9 step 3), the pool free entry carries the count with the block, and test 2 gets the neighbour-edge case. Sibling check per the workflow: `shape.h` has `materialCount` beside `materials`, the same shape of defect, so it was fixed the same way and gets its own churn case in test 2. Both were in the doc's own inventory, so this round's fix covers cases Codex did not name.
- **#2 (Medium), applied.** §5.2 gave `b3JournaledArray` clear and set for every array while only push and removeswap had an ownership rule for a sensor's three arrays. Nothing in `sensor.c`, `shape.c` or `physics_world.c` clears or sets `world->sensors` as a whole (only per-sensor arrays and world destruction), so the sensor array's type now offers only push and removeswap.
- **#3 (Medium), applied as a definition, severity overstated.** The blocks are real (`contact.c`, `sensor.c` and `solver_set.c` destroy paths put them in entries), but they are transient and bounded by the API calls of one inter-step window, and are charged at capture or returned by a rewind. The doc did not say when the charge begins, so §8 now states it and §14's `bytesUsed` comment says closed slots. No new counter was added: the budget is already a target, not a ceiling (requirement 5).
- **#4 (Low), declined; the doc's rationale sharpened.** §5.3 already says the cache is append-only and grows only with distinct names. A replay supplies the same names as the original run, so a rewind adds none; growth by names the caller invents is the engine's existing behaviour with or without a ring (`name_cache.c` never removes entries), and reclaiming names would be a change to an existing primitive, outside the scope rule. Codex did not engage the stated rationale. §5.3 now says the names are the caller's and a replay adds none.
- **Unresolved item:** Codex could not read `docs/designs/reviews/20260927-001-client-prediction-rollback-perf.md` (excluded by the prompt) so §11.1's figures were not checked by it. That is the prompt's design, not a defect in the doc; those figures are labelled a baseline and gated by §12 test 5.

## Status

First round against this document, so no trend yet. All four findings are new categories of issue: a count/pointer pairing hole in undo (with a sibling in the material block), an ownership gap in a container's operation set, and two accounting/rationale points. None is a consequence of an earlier round's fix, and none reopened rationale the doc already stated except #4, which did not engage it. Next: another fresh full-scope round, same prompt, against the edited doc.

`Round 1: 4 findings (1H-2M-1L), 3 applied, 1 declined`

`Series total: 4 findings (1H-2M-1L) across 1 rounds`

`Findings: 4 (1H-2M-1L)`
