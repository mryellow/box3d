---

title: Codex review — bit-exact history ring, round 29
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 29

## Review report (Codex final message)

## Summary

The whole-world approach is sound, and the document generally matches the source tree. I found two design gaps and one scope statement to correct. I made no file changes.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §11.2(4), §§6 and 10 | The proposed future incremental replay says unchanged islands’ per-tick results can be copied from the ring. With `captureInterval > 1`, those ticks have no hot image; the ring also does not retain their event arrays. The listed solver changes alone would not supply the data needed for that proposal. | State that incremental replay requires per-tick island state and a way to reproduce or retain events, or restrict the proposal to a separately specified storage mode. |
| 2 | Medium | §§7.4, 9 and 13 | Phase 1 images the dynamic and kinematic trees raw, while the general restore procedure replays proxy operations and rebuilds tree layout. It does not specify how those raw images and journal operations are ordered or which restore-time tree pass is skipped in phase 1. | Give phase 1 its own explicit tree restore sequence, including proxy identity, moved flags and the static-tree pass. |
| 3 | Low | §§5.4 and 7.4 | The text calls tree derivation a “one CCD change,” but §7.4 requires three order changes: solid CCD selection, CCD sensor hits, and explosion processing. The phase plan correctly lists all three. | Update the earlier scope statements to name all three changes. |

## Checked, no change

- [The recording hash and `ScrubBackward` test](test/test_recording.c:251) support the document’s narrower claim about existing transform and velocity replay; the document correctly says that hash is insufficient for full-state exactness.
- [The proxy allocator](src/dynamic_tree.c:137) uses a free list, and [the broad-phase wrappers](src/broad_phase.c:53) are the relevant world-proxy creation and destruction boundary.
- [CCD’s running fraction and eight-hit sensor cap](src/solver.c:313), [explosion query order](src/physics_world.c:3471), and [sensor overlap processing](src/sensor.c:185) support the identified order dependencies and sensor capture cost.
- [The serializer clears object `userData`](src/world_snapshot.c:1050), as the phase-0 caveat says. The determinism description in [simulation.md](docs/simulation.md) and floating-point contraction setting in [CMakeLists.txt](CMakeLists.txt) also align with the document.

## Proposed edits

Clarify the incremental replay data requirement, spell out phase 1’s tree restore sequence, and replace the “one CCD change” wording. No source edit is indicated by this review.

## Unresolved / disagreements

The document’s five open questions remain decisions for the engine and API owners. I found no source-based reason to reject the whole-world recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:44:01+10:00, ended 14:48:11+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 28, no declined-findings list. The final
message above is unchanged apart from absolute path prefixes on its links.

**Findings verified against the doc and source.**

1. **Applied.** §11.2 (4) proposed copying unchanged islands' per-tick results from the ring, but
   with `captureInterval` above 1 most ticks carry no image, and §6 and §8 store no event arrays.
   The proposal now states that it needs an image every tick and the copied ticks' events retained
   or reproduced.
2. **Applied.** §13 phase 1 images the kinematic and dynamic trees raw while §9 step 2 replays
   proxy operations through the journal walk and §7.4's pass rebuilds layout. The ordering was
   unstated: the walk runs against the live trees, then the image replaces each of those two trees
   wholesale, free list and moved bits included, so no tree pass runs on them. §13 now says so.
3. **Applied.** §4, §5.4 and §7.4's heading and opening described tree derivation as depending on
   one CCD change, while §7.4 requires three order changes (solid CCD min, CCD sensor hits,
   explode), as §11.3 and §13 already say. The three earlier statements now name three order changes.

3 applied, 0 declined.

## Status

Finding count is 3 (0H-2M-1L), the same as round 26, with no High. All three are scope statements
or unstated sequencing in text that predates round 28; none is a consequence of round 28's fixes
and none reopened ground the doc's rationale covered. The counts since round 17 (3, 4, 2, 4, 3, 2,
3, 2, 3, 4, 3, 3) remain flat at 2 to 4, so the series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 29: 3 findings (0H-2M-1L), 3 applied, 0 declined`

`Series total: 138 findings (46H-77M-15L) across 29 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L)`
