---

title: Codex review — bit-exact history ring, round 18
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 18

## Review report (Codex final message)

## Summary

The source supports the design’s central premise: the recording system can restore a whole world and reproduce the current state hash. The proposed hot image and cold journal approach is coherent, but two cost claims need correction and the proposed full state hash needs a more precise geometry rule.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §5.4 | The claim that tree rebuilding gives the engine an existing per-step O(proxies) cost is too broad. broad_phase.c conditionally rebuilds the dynamic and kinematic trees; physics_world.c rebuilds the static tree through an explicit API. This matters most to the claimed `large_world` cost. | State which trees rebuild during a step, under what condition, and keep the ring’s added cost separate from engine cost. |
| 2 | Medium | §11.1 | The copy sizes, timing shares, and 650 MB figure are attributed to a review outside the permitted evidence set. The benchmark files do not establish those serializer measurements. They are presented as a measured basis for the performance recommendation without reproducible support in this document. | Include the measurement method and raw results in this design, or label the figures as external, unverified estimates and gate the recommendation on §12’s measurements. |
| 3 | Medium | §12.1 | “Content hashes” for borrowed hull, mesh, height-field, and compound geometry are underspecified for a field-wise, cross-platform state hash. These types contain packed data and, for compounds, a tree with pointers (types.h). Hashing their existing byte representations would conflict with the stated exclusion of addresses and padding. | Specify canonical fields and ordering for each geometry type, including how compound tree pointers and derived layout are excluded. Test equal geometry built on the target platforms. |

## Checked, no change

- test_recording.c does compare hashes after backward seeks; the existing recording.c hash covers transforms and velocities, so the design correctly limits what that test proves.
- sensor.c sorts and deduplicates overlap results, and its next step uses `overlaps2` as the prior overlap set.
- solver.c confirms the stated CCD traversal dependence: it passes a running fraction to TOI and caps sensor hits at eight.
- physics_world.c confirms that explosion impulses are currently applied in query traversal order.
- world_snapshot.c clears object `userData` in serialized images, as the phase 0 caveat says.

## Proposed edits

Correct the tree cost explanation, make the performance baseline reproducible within the design, and define canonical geometry hashing before treating the full state hash as a cross-platform oracle.

## Unresolved / disagreements

I did not run the proposed implementation or tests; this is a source review of a draft. The order-changing CCD, sensor, and explosion changes remain explicit design decisions for the solver owner.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:17:53+10:00, ended 13:21:55+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Fresh full-scope prompt with no reference to earlier rounds and no declined-findings list
(the declines of rounds 14 to 17 were each answered in the doc). Before the round the uncommitted
diff was read in full and found intact, the round-17 edits were checked against AC-1 through AC-4
with no failure, and the mechanism sections were re-read for sibling-case gaps with none found. The
final message above has its source links reduced to plain file names, per the citation rules;
nothing else is changed.

**Findings verified against source.**

1. **Applied.** `b3UpdateTreesTask` (`broad_phase.c`) rebuilds only the dynamic and kinematic trees,
   and `b3DynamicTree_Rebuild` (`dynamic_tree.c`) returns early unless the root is moved or the tree
   is not DFS-ordered. The static tree is rebuilt only by `b3World_RebuildStaticTree`
   (`physics_world.c`). §5.4 said the rebuild copies every retained subtree whenever anything moved,
   which read as covering all three trees; it now names the two trees and the condition.
2. **Declined; doc sharpened.** Every figure in §11.1 is in the repo's perf review
   (`20260927-001-client-prediction-rollback-perf.md`: trees100 0.52 ms and 4.7 ms, junkyard 25.4 ms
   and 228 ms, `large_world` 650 MB/tick), so the doc's numbers are reproducible from a file in the
   repo. Codex could not see it only because the prompt excluded the reviews directory. What the
   finding gets right is that §11.1 read as a measured basis for this design's own cost. §11.1 now
   names the file, says the figures are the serializer baseline, and says §12 test 5 gates this
   design's cost claims.
3. **Applied.** Only `b3HashHullData` exists in the source; `b3MeshData` and `b3HeightFieldData` each
   store a `hash` computed over the whole blob with the field zeroed (`height_field.c`), but
   `b3CompoundData` has no hash field and holds a `b3DynamicTree` with fixed-up pointers. §12 test 1's
   "compound content hash" was therefore undefined. It now names the stored `hash` for hulls, meshes
   and height fields, and defines the compound's over its version, materials and instance arrays in
   stored order, excluding the tree.

2 applied, 1 declined.

## Status

Finding count fell to 3 (0H-3M-0L), the third round in a row with no High in four. Finding 1 is an
over-broad source claim (like round 17's finding 3), finding 2 is a reviewer-scope artifact, and
finding 3 is a real gap in test 1's hash definition. None was a consequence of round 17's fixes and
none reopened stated rationale. Findings still come from source mismatches and unstated definitions,
not only wording; the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 18: 3 findings (0H-3M-0L), 2 applied, 1 declined`

`Series total: 105 findings (40H-57M-8L) across 18 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L)`
