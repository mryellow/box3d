---

title: Codex review — bit-exact history ring, round 37
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 37

## Review report (Codex final message)

## Summary

The hot-image and cold-journal design is broadly consistent with the source, but two stated guarantees need changes. This was a read-only review; I did not run tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §13, Phase 0; §12, test 2 | Phase 0 says the geometry-replacement debug-draw test can run against the existing serializer. On restore, [world_snapshot.c](/home/user/box3d/src/world_snapshot.c:630) preserves `userShape` when shape ID and generation match, even if geometry changed. [physics_world.c](/home/user/box3d/src/physics_world.c:1315) then reuses that handle instead of rebuilding it. The phase-0 test can display stale geometry. | Clear and release the handle on a geometry-changing restore in phase 0, or defer that test clause to phase 1 and state the limitation. |
| 2 | Medium | §5.3; §10 | The claim that a callback reading a name gets the value set by the caller fails across a discarded branch with a name-hash collision. [name_cache.c](/home/user/box3d/src/name_cache.c:78) keeps the first string for an ID, while §5.3 leaves the cache untouched by rewind. A name introduced only on the discarded branch can therefore be returned for a different, colliding name on the new branch. | Journal or otherwise disambiguate name-cache entries, or explicitly exclude name-getter results from the replay contract and test the collision case. |

## Checked, no change

- The existing recording hash covers transforms and velocities, while `ScrubBackward` compares forward and restored hashes within that scope. The document correctly limits what this proves.
- The source supports the distinction between tree layout and proxy identity: broad-phase proxy creation and destruction use per-tree free lists, and pair keys are sorted before contact creation.
- The document accounts for sensor overlap updates on static and sleeping owners, and for `inv_h` and `inv_dt` being observable between steps.
- The awake-to-sleep and sleep-to-awake rules account for shape, manifold, and mesh-cache bytes that would otherwise fall out of the hot image.

## Proposed edits

Revise the phase-0 test claim and restore handling for `userShape`. Clarify the name-cache collision contract and add a branch-rewind collision test.

## Unresolved / disagreements

The document’s open questions about the three order changes, public hash API, journaling mode, forward scrub, and recording integration remain design decisions. This review does not resolve them.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, stdin from
`/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout, raw output to the session
scratchpad. Started 2026-09-29T16:52:19+10:00, ended 16:56:36+10:00, exit code 0; no resume. Codex ran
read-only; the only working-tree changes are Claude's own edits to the target doc, and `src/` and `include/`
have no diff. Fresh full-scope prompt (target doc, source tree, no read of `docs/designs/reviews/` or git
history, file-only citations by design), no declined-findings list: the one decline in round 36 was one where
the doc was right and already says why. The final message above is the run's, one copy kept.

Before the round, Claude read the whole uncommitted diff of the target doc (rounds 26 to 36's edits since the
last commit; not mangled), re-read the mechanism sections (§5.2, §7.1, §7.4, §9) and checked each rule against
its sibling cases (step 2), fixing this gap directly: §12 test 2's churn named a shape filter change that does
not touch the proxy but not the sibling case §7.4 describes, a filter or body-type change that destroys and
recreates a shape's proxy, possibly in a different tree. The churn now includes it.

**Findings verified against the doc and source.**

1. **Applied.** `b3DesShapes` (`world_snapshot.c`) saves each live shape's `userShape` before the array is
   wiped and puts it back on the restored record whenever the slot's id and generation match, with no check
   that the geometry is the same; the debug draw (`physics_world.c`) only creates a handle when
   `userShape` is null. So through the serializer a shape whose geometry was replaced after T keeps a handle
   built for the old geometry, and §12 test 2's geometry-replacement debug-draw clause would show it in
   phase 0. §13's phase 0 paragraph now defers that clause to phase 1 with the `userData` clauses, and lists
   the handle carry-over among the reader's behaviours that phase 1 replaces. No change to the design.
2. **Applied.** `b3AddName` (`name_cache.c`) returns the existing id when the hash is already in the map and
   keeps the first string, and `Rewind` leaves the cache alone, so a name added only on a discarded timeline
   is what a colliding name reads back on the new one. §5.3 said a collision reads back the first name added
   "as they do live", which holds within one timeline and not across a rewind. The `nameId` is exact and
   hashed; only the string a getter returns for a colliding name is not, and nothing in the step reads it.
   §5.3 now says so and puts that string outside the bit-exact contract. Journaling the name cache was not
   taken: it would add a structure to keep consistent with the records that name it, for a 32-bit hash
   collision on a string the step never reads.

2 applied, 0 declined. The status line's finding sequence and round count in the target doc were updated to
include this round. AC-1 to AC-4 were applied to the doc before the round and no shape changed against them:
the fix to finding 2 deliberately declines a shadow journal of the name cache (AC-1, AC-3).

## Status

Finding count is 2 (0H-2M-0L), down from 5, still with no High (rounds 35 to 37). Both findings are in older
text outside §5.2's module-boundary rules: finding 1 is phase-0 test scope, finding 2 is a claim in §5.3 that
holds within one timeline only; neither is a consequence of round 36's fixes, and neither contradicted stated
rationale. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5, 2) are flat to
slightly falling, not yet at zero; not converged, and a low count is not evidence the shape is right (AC-5).

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 37: 2 findings (0H-2M-0L), 2 applied, 0 declined`

`Series total: 168 findings (51H-93M-24L) across 37 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L)`
