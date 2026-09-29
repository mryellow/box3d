---
title: Codex review — bit-exact history ring, round 41
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 41

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the current source. Two parts of the specification need tightening before the full-state hash can serve as the proposed restore oracle. This was a read-only source review; I did not run the proposed tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§5.1–5.2, 12 | The proposed `b3WorldScalars` inventory names enable flags, gravity and thresholds, but does not explicitly account for simulation inputs such as `restitutionIterations`, `maxLinearSpeed`, `contactSpeed`, `contactHertz`, `contactDampingRatio` and `contactRecycleDistance` in `src/physics_world.h`. The image and hash rules depend on this struct being complete. | Enumerate every simulation scalar in the proposed struct and state that its image and hash cover every field. |
| 2 | Medium | §§9, 12 | §9 requires free-slot generations to be restored, but §12 does not explicitly require them in the full-state hash. A hash of live sparse records and pool free-list order could miss a wrong generation in a free slot until that id is allocated again. Generation fields reside in the sparse records in `src/body.h`, `src/shape.h`, `src/contact.h` and the joint records. | Specify that the hash includes the generation of every allocated slot, including free slots, and test a rewind followed by reuse of a free id. |

## Checked, no change

- `b3HashWorldState` covers the limited body state described in §1 (`src/recording.c`); the document correctly assigns wider coverage to the proposed hash.
- The six world id pools and the trees’ separate proxy free lists match the inventory (`src/physics_world.h`, `src/dynamic_tree.c`).
- `b3DynamicTree_ClearMoved` descends moved nodes, as the capture and restore rules assume (`src/dynamic_tree.c`).
- The identified CCD sensor cap and explosion traversal dependencies are present in `src/solver.c` and `src/physics_world.c`.
- Sensor overlap arrays swap each step, supporting the treatment of `overlaps2` as the retained overlap state (`src/sensor.c`).

## Proposed edits

Make the world-scalar list exhaustive in §§5.1–5.2, then use that same list in §12’s hash rule. Add an explicit free-slot generation clause and reuse test to §12.

## Unresolved / disagreements

The proposed cost and replay guarantees still require the execution gates in §12. I found no disagreement with the stated source revision, 64-bit platform guarantee, existing-hash scope, or moved-node traversal.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540` with the tool timeout at 600000 ms. Start 2026-09-29T17:47:06+10:00, end 2026-09-29T17:52:34+10:00, exit code 0. `git status --short` before and after the run is identical, so Codex touched nothing. The prompt was a fresh, full-scope brief that excluded `docs/designs/reviews/` and carried a short curated context list (the doc's source revision, the engine's documented 64-bit cross-platform determinism, §1's scoping of the existing hash, `b3DynamicTree_ClearMoved` descending only moved nodes). Codex's report agreed with all four.

Before the round, the uncommitted diff from round 40 was read in full and was intact. Re-reading the mechanism sections and checking each rule against its siblings found three gaps, fixed directly:

- §7.1's pair set entry called add and remove self-inverse without saying why; `b3CreateContact` adds a key only after pair discovery found it absent and `b3DestroyContact` removes one its contact put there, so no add or remove is a no-op (`contact.c`, `broad_phase.c`, `table.c`'s `b3AddKey` returns whether the key existed), unlike the bitset case round 40 fixed. The entry now states this.
- §8's slot charge listed a destroyed set's, island's and contact's arrays, a sensor's arrays and a hull, but not a destroyed shape's `materials` block, which rides in the shape's pool free entry (§7.1). Added.
- §12 test 2's churn list named hull, compound and material cases but not sensor shape creation and destruction, the other structure whose heap arrays move into a journal entry. Added.

1. **Applied.** `physics_world.h` holds `restitutionIterations`, `maxLinearSpeed`, `contactSpeed`, `contactHertz`, `contactDampingRatio`, `contactRecycleDistance`, `enableRestitutionPropagation`, `hitEventThreshold` and `restitutionThreshold` beside `gravity` and the enable flags, and the contact and joint code read them each step. §5.1's row said only "enable flags, gravity, thresholds", so which fields the scalar struct held was not stated. §5.1 now names every field and §5.2 says the struct is every field that row images, so the hash's "world scalars" clause covers exactly the same set. Host configuration (callbacks, contexts, `userData`, `workerCount`, task callbacks) and scratch (`stepIndex`, `maxCapacity`, profile, counters) stay out, as before.
2. **Applied.** `generation` lives in the body, shape, contact and joint records and survives a free (`body.c`, `joint.c` and `contact.c` increment it on destroy, and the id pool does not touch it), so a wrong free-slot generation would pass a hash of live records until that id was allocated again. §9 already restores free-slot generations, and §12's hash now includes the generation of every slot of each sparse record array, free slots included. Test 3's replay recreates handles in the same order, so a mismatch also shows there.

2 applied, 0 declined. The status line's finding sequence and round count in the target doc were updated to include this round. AC-1 to AC-4 were applied to the doc before the round and no shape changed against them; finding 1 lists fields of a struct the design defines and finding 2 adds a hashed field, neither adds a shadow structure, a caller list, a reconciliation or a control-flow check.

## Status

Finding count is 2 (0H-2M-0L), down from 4, with no High. Both findings are older gaps in the hash and scalar inventory, not consequences of round 40's fixes, and neither reopened rationale the doc already states. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5, 2, 4, 3, 4, 2) are flat, not falling; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 41: 2 findings (0H-2M-0L), 2 applied, 0 declined`

`Series total: 181 findings (52H-102M-27L) across 41 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L) -> 4 (0H-2M-2L) -> 3 (0H-2M-1L) -> 4 (1H-3M-0L) -> 2 (0H-2M-0L)`
