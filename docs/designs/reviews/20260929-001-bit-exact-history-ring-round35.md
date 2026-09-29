---

title: Codex review — bit-exact history ring, round 35
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. First attempt hit the
  540 s shell-level timeout (exit code 124); finalized by resuming the same session.
session: 01a0ebd5-e7a7-7322-847b-1d27ffe5f2a0

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 35

## Review report (Codex final message)

## Summary

The proposed history model is broadly consistent with the source paths I checked, but the document needs three corrections before its restore and verification claims are precise. This was a read-only review; I did not run the proposed implementation.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§5.1, 9, 12 | The world-scalar image omits `inv_h` and `inv_dt`. The current serializer preserves both (`src/world_snapshot.c`); `inv_h` affects public joint force getters (`src/distance_joint.c`, `src/revolute_joint.c` and other joint files), and `inv_dt` affects contact-force debug output (`src/physics_world.c`). After rewinding across steps with different time steps, those results would reflect P rather than T, contrary to §9’s restored-world claim. | Image and restore both values, and specify whether the full-state hash covers these observable values. |
| 2 | Low | §§5.3, 12 | The explanation for excluding `nameId` says IDs follow first-use order, and §5.3 calls the cache deduplicated by content. In fact, `nameId` is a hash of the string; a hash collision logs a warning and returns the existing ID even for different content (`src/name_cache.c`). | Describe hash-based IDs accurately and state how name collisions are handled under the history contract. |
| 3 | Low | §9 | The document says handles created after T fail their *generation check* after rewind. Depending on the slot, validity can fail earlier because its index is out of range or its slot is free (`src/physics_world.c`). The useful guarantee is invalidity, not that particular check. | Say future handles are invalid after rewind; remove the claimed failure mechanism. |

## Checked, no change

- The existing recording hash covers transforms and velocities, and `ScrubBackward` compares hashes after seeks (`src/recording.c`, `test/test_recording.c`).
- Sensor processing swaps overlap buffers every step and sorts visitor results; imaging `overlaps2` is consistent with the next-step state described in §§5–6 (`src/sensor.c`).
- Tree proxy IDs have their own free list, and broad-phase create/destroy wrap world-tree proxy operations (`src/dynamic_tree.c`, `src/broad_phase.c`).
- The CCD sensor cap and running-fraction gate, and explosion’s traversal-order impulse accumulation, support the order-dependence analysis in §7.4 (`src/solver.c`, `src/physics_world.c`).
- The awake and sleeping solver-set partition and graph-colour storage match the state inventory (`src/solver_set.h`, `src/constraint_graph.h`).

## Proposed edits

Add `inv_h` and `inv_dt` to the restore inventory; correct the name-cache description and handle-validity wording; add a variable-time-step rewind case to §12 that checks joint force getters immediately after restore.

## Unresolved / disagreements

The numerical performance baseline cited from `docs/designs/reviews/` remains unverified because that directory was excluded from this review. The proposed accessors, journal, and ring do not yet exist in the source, so their performance and completeness remain implementation questions.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** Attempt 1: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout, raw output to the
session scratchpad. Started 2026-09-29T16:24:18+10:00, ended 16:33:18+10:00, exit code 124 (timed out
after doing its research). Attempt 2: `codex exec --sandbox read-only -m gpt-6-sol -c
model_reasoning_effort="medium" resume 01a0ebd5-e7a7-7322-847b-1d27ffe5f2a0` with a finalize prompt
(report from the research already done, no further tool calls), same timeouts and redirection, started
16:33:31, ended 16:33:58, exit code 0. Codex ran read-only; `git status --short` (excluding review files)
before and after was identical and `src/` and `include/` have no diff, so the working tree was untouched
by Codex. Fresh full-scope prompt (target doc, source tree and `docs/`, no read of `docs/designs/reviews/`,
file-only citations by design), no declined-findings list: every earlier decline was one where the doc
was right and now says why. The final message above is the resumed session's, unchanged.

Before the round, Claude rewrote §5.2's compile-time-boundary text from the record types outward
(records, owned blocks, sensors, world scalars, containers) and checked each rule against every sibling
case (step 2), fixing these gaps directly: (a) the hot accessor asserted an awake owner, but the sensor
pass (`sensor.c`, a parallel-for over every entry of `world->sensors`) and the CCD sensor-hit write
(`solver.c`) touch the arrays of sensors on static, sleeping and disabled bodies, so the sensor view has
no owner assertion; (b) "declared pointer-to-const" does not hold for array-valued owned blocks, since a
const `b3Array` (`container.h`) still holds a writable `data` pointer, so those use a const-data array type of
the same layout; (c) the narrow phase (`contact.c`) allocates and frees an awake contact's manifold block in
place, so the hot accessor's contact access covers that; (d) the hot accessor lists a shape's `fatAABBs`
entry and graph colour arrays; (e) world scalars had only a list of setters that must use
`b3World_WriteScalars`, so they are now one struct type held by pointer-to-const, with host configuration
outside it, and §9 step 3 no longer names fields to skip; (f) record arrays live inside a slot pool but
their storage header was said to be private to the write accessor's file, so the container's own file now
holds every function that produces a writable element pointer.

**Findings verified against the doc and source.**

1. **Applied.** `world->inv_h` and `world->inv_dt` (`physics_world.h`) are set at the start of each step
   (`physics_world.c`) and are not in §5.1's scalar row. `inv_h` multiplies impulses in the public joint force
   and torque getters of every joint file, and `inv_dt` is read by the contact-force debug draw, so after a
   rewind across steps of different length they would report P's values. The step never reads either before
   setting it, so replay is unaffected; only between-step observations are. §5.1's row and §5.2's scalar
   list now include both (the hash covers world scalars, so it covers them too), and §12 test 2's churn adds
   steps of different time steps followed by a rewind and a joint force getter read before any step.
   `stepIndex` (incremented, never read outside the serializer) and `maxCapacity` (a public high-water
   statistic) are counters, §5.3's class, and were not added.
2. **Applied; reverses the rationale round 34's fix wrote.** `b3AddName` (`name_cache.c`) sets `id` to
   `b3Hash32` of the string, so a `nameId` is a content hash, not an allocation-order id, and the cache is
   deduplicated by that hash. The stated reason for excluding `nameId` from the hash was false. Since the
   cache is append-only and unrestored, a colliding pair reads back as the first name added live and after
   a rewind alike, so hashing `nameId` covers what the getters return. §5.3 now says that and §12 hashes
   `nameId` instead of the name string.
3. **Applied.** `b3Body_IsValid` checks world, index range (`bodies.count < index1`, which a bump-alloc undo
   makes true) and a freed slot (`setIndex == B3_NULL_INDEX`) before it compares generation, so a handle
   created after T can fail any of them. §9 now says such handles are invalid and names no mechanism.

3 applied, 0 declined. The status line's finding sequence and round count in the target doc were updated
to include this round.

## Status

Finding count is 3 (0H-1M-2L), down from 6 and back in the band of rounds 18 to 31, with no High for the
first time since round 31 (rounds 32 to 34 had 2, 2 and 1). None of the three touches §5.2, whose
boundary text was rewritten before this round and was the source of the Highs in rounds 32 to 34; that is
one clean pass over it, not evidence the shape is right (AC-5). Finding 2 is a consequence of round 34's own
fix, whose rationale about id assignment order was wrong; it engaged the doc's stated reason with source
evidence, so the reversal was applied and the old rationale removed. Findings 1 and 3 are older text: an
unlisted world field and an imprecise validity claim. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3,
4, 3, 3, 3, 3, 4, 4, 6, 3) are flat, not falling; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 35: 3 findings (0H-1M-2L), 3 applied, 0 declined`

`Series total: 161 findings (51H-88M-22L) across 35 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L)`
