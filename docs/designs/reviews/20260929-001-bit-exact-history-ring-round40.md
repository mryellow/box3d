---

title: Codex review — bit-exact history ring, round 40
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 40

## Review report (Codex final message)

## Summary

The hot image and cold journal split is a sound direction, and the document accurately limits what the existing replay test proves. One tree restore rule can change the set of broad-phase candidates after a rewind. Three other parts need tighter specifications before the bit-exact and cost claims are reviewable.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.4, §9 | Restore moves each proxy to `fatAABBs[id]`, but those bounds need not equal the proxy’s leaf AABB at T. The bullet path in [solver.c](/home/user/box3d/src/solver.c) can update a shape’s fat AABB while retaining a larger tree AABB; [dynamic_tree.c](/home/user/box3d/src/dynamic_tree.c) then keeps that larger leaf. Shrinking it during restore can change broad-phase candidates, beyond the traversal-order differences §7.4 fixes. | Preserve the actual leaf AABB in history, or establish and test an invariant that leaf AABBs always equal the recorded fat AABBs. Add a replay case where a bullet’s tree AABB contains a newer, smaller fat AABB. |
| 2 | Medium | §7.1, §9, §12 | The instruction to copy contact records while excluding `triangleCache` needs a tagged-union rule. In [contact.h](/home/user/box3d/src/contact.h), `meshContact.triangleCache` shares storage with `convexContact.cache`; [contact.c](/home/user/box3d/src/contact.c) reads the convex cache on later steps. A blanket exclusion of that storage would lose persistent convex state. | Specify copy and hash behavior separately for convex and mesh contacts, including mesh `queryBounds`, and test both across rewind. |
| 3 | Medium | §1, §7.1, §9 | The claim that restore allocation is bounded by awake state omits cold changes. Redo or undo of a material-block replacement on a static or sleeping shape can reallocate its block; tree and journal reconstruction can also allocate in proportion to structural churn. | State the restore allocation bound in terms of awake state **and traversed cold changes**; measure both in §12’s cost tests. |
| 4 | Medium | §7.4, §12 | The proposed CCD sensor fix collects all candidates before choosing eight. The current path caps storage at eight in [solver.c](/home/user/box3d/src/solver.c); the proposal leaves temporary memory proportional to every sensor candidate in a sweep, with no stress test for that case. | Specify a bounded-memory selection method, such as a second traversal after the final solid fraction is known, and add a dense-sensor cost case. |

## Checked, no change

- [test_recording.c](/home/user/box3d/test/test_recording.c) does exercise backward seeking and hash comparison; [recording.c](/home/user/box3d/src/recording.c) hashes transforms and velocities, as the document says.
- The document accounts for sensor overlap state being updated even when the sensor’s owner is static or sleeping, and for the two end-event buffers being cleared on rewind.
- Pool free-list order, proxy IDs, owned contact blocks, shared hull lifetime, and API writes between steps all have explicit restore paths in the proposal.
- The source supports the listed CCD sensor cap and explosion traversal-order dependencies.

## Proposed edits

Apply the four actions above, then make the tree-leaf AABB case a gate for phase 1 as well as phase 2. Clarify the contact union rule in both the restore procedure and full-state hash specification.

## Unresolved / disagreements

The performance figures attributed to files under `docs/designs/reviews/` were not independently checked because those files were excluded from this review. No disagreement with the document’s stated optional API choices.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, stdin from
`/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout, raw output to the session
scratchpad. Started 2026-09-29T17:34:23+10:00, ended 17:41:49+10:00, exit code 0; no resume. Codex ran
read-only; `src/` and `include/` have no diff. Fresh full-scope prompt (target doc, source tree, no read of
`docs/designs/reviews/` or git history, file-only citations by design), no declined-findings list: no round
39 finding was declined. The final message above is the run's, one copy kept.

Before the round, Claude read the uncommitted diff of the target doc (rounds 38 and 39's edits; not mangled)
and re-read the mechanism sections against their sibling cases; no gap found before the run.

**Findings verified against the doc and source.**

1. **Applied as a missing invariant, not a defect.** The finding's premise is that a shape's fat AABB can be
   smaller than its proxy's leaf. In `solver.c` a fat AABB is rewritten only when the new shape AABB is not
   contained in the old one, so the new fat AABB is not contained in the old either; every non-bullet write
   is followed by `b3BroadPhase_MarkProxyMoved` with the new value, and a bullet's by the serial pass, which
   calls `b3DynamicTree_EnlargeProxy` whenever the leaf does not contain it, and that call replaces the leaf's
   box with the new fat AABB (`dynamic_tree.c`). Since the leaf starts equal to the fat AABB (creation, proxy
   move) and each fat write makes it equal again, at a step boundary the two are equal, and restore's move to
   the restored fat AABB reproduces the leaf. The doc did not state this, so §7.4 now does, and §12's hash
   reads each proxy's leaf AABB from the tree, so a drift would fail the exactness tests rather than pass
   unhashed. No change to the design.
2. **Applied.** `b3Contact` (`contact.h`) holds `convexContact` (a `b3ContactCache`) and `meshContact`
   (`triangleCache` and `queryBounds`) in one union, used per `b3_simMeshContact`. §9 step 3 excluded a
   contact's `triangleCache` without saying it is the mesh member; read as covering the union's storage it
   would keep a live convex cache. §9 now says that for a mesh contact the array keeps its live block and a
   convex contact's cache and a mesh contact's `queryBounds` are copied.
3. **Applied.** §1 called restore's allocation awake-proportional; undoing or redoing a material block
   entry on a cold shape reallocates too. §1 now says awake state plus, per journal entry walked, the block it
   replaces. §12 test 5's restore cost already scales with journal bytes.
4. **Applied.** `solver.c` stores at most eight sensor hits per sweep, so the doc's "collect all candidates"
   would need storage proportional to the candidate count. A fixed buffer cannot be filled during the one
   traversal, since the final solid fraction that filters candidates is known only after it. §7.4 now has a
   first traversal find the solid min and a second keep the eight smallest pairs under it in an eight-slot
   buffer, which is order-independent because it selects the smallest of a set. §12 test 7's more-than-eight
   sensor case already covers it.

4 applied, 0 declined. The status line's finding sequence and round count in the target doc were updated to
include this round. AC-1 to AC-4 were applied to the doc before the round and no shape changed against them;
finding 1's leaf-equals-fat statement is an invariant the construction guarantees (AC-4 permits stating it).

## Status

Finding count is 4 (1H-3M-0L), up from 3, and the first High since round 34. Finding 1 was a false alarm on
the design that exposed an unstated invariant it depends on; finding 4 was older §7.4 text (its collect-then-select wording), not a consequence of the immediately
preceding round's fix;
findings 2 and 3 are older wording. No finding contradicted stated rationale. The counts since round 17
(3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5, 2, 4, 3, 4) are flat, not falling; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 40: 4 findings (1H-3M-0L), 4 applied, 0 declined`

`Series total: 179 findings (52H-100M-27L) across 40 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L) -> 4 (0H-2M-2L) -> 3 (0H-2M-1L) -> 4 (1H-3M-0L)`
