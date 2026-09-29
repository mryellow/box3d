---

title: Codex review — bit-exact history ring, round 9
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 9

## Review report (Codex final message)

## Summary

The whole-world rewind approach is plausible, and several key source claims check out. Three restore and verification rules need changes before the design supports its stated contract. I made no file changes.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.3, 9, 12 | Event arrays are classified as disposable, but `b3Body.bodyMoveIndex` can still point into `world->bodyMoveEvents` after a step. After `Rewind` clears that array, an immediate `b3Body_SetAwake(id, false)` calls `b3TrySleepIsland`, which dereferences the saved index (`src/body.c`, `src/solver_set.c`, `src/solver.c`). | Reset `bodyMoveIndex` when dropping move events, exclude it from the simulation-state hash, and test `SetAwake(false)` immediately after rewind; or retain the internal move-event storage until it can no longer be read. |
| 2 | Medium | §§7.4, 12 | The proposed full hash does not explicitly include live tree-proxy category bits. They cannot always be derived from shape filters: `b3Shape_SetFilter(..., invokeContacts=false)` changes the shape filter without updating its proxy (`src/shape.c`, `src/dynamic_tree.c`). A restore test could therefore pass while broad-phase queries use different categories. | Hash each live proxy’s category bits, shape id and AABB, alongside proxy identity and moved state. Test the `invokeContacts=false` case across rewind and replay. |
| 3 | Medium | §§7.1, 9, 10 | Restore says a shape’s `userShape` keeps its live value, while the caller contract says a shape that vanishes releases that handle and one that reappears starts with null. Reusing a shape slot across timelines can leave the later shape’s renderer handle attached to the restored shape. Ordinary destruction releases it in `src/shape.c`; the proposed pool replay specifies no equivalent action. | Define an identity-aware restore step that releases handles for shapes leaving the live timeline and clears handles when a different shape identity occupies a slot. Test destroy, slot reuse, backward scrub and forward scrub. |
| 4 | Low | §8 | Interval widening cannot reduce journal bytes. Two consecutive segments can each fit `maxBytes` while their required minimum window does not, even with no images. The over-budget rule covers a *single* tick but leaves this case unspecified. | State whether the ring admits this overrun or shortens the minimum window, and add that case to the ring tests. |

## Checked, no change

- `ScrubBackward` does compare hashes after backward seeks, and the current hash covers body transforms and velocities only (`test/test_recording.c`, `src/recording.c`).
- The serializer restores substantially more world state than that hash checks; phase 0 appropriately calls for a stronger oracle (`src/world_snapshot.c`).
- Sensors are processed each step, including sensors on sleeping or static bodies (`src/sensor.c`).
- Each broad-phase tree has its own proxy free list, separate from the six world id pools (`src/dynamic_tree.c`, `src/physics_world.h`).
- The documented worker-count and 64-bit platform determinism claims match `docs/faq.md`, `docs/simulation.md`, `test/test_determinism.c`, and `CMakeLists.txt`. This review did not execute cross-platform tests.
- I treated §11.1’s performance figures as supplied context.

## Proposed edits

Revise the scratch-state rule and restore sequence for `bodyMoveIndex`; expand the hash and tests to cover tree-proxy payloads; specify renderer-handle cleanup during journal replay; and define the multi-segment budget behavior.

## Unresolved / disagreements

The document’s listed solver-order changes remain design decisions for the solver owner. I found no source evidence requiring a different overall hot-image and cold-journal architecture.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:01:02+10:00, ended 12:06:03+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` after the call showed only the target doc and `WORKFLOW.md` modified plus
the untracked round-6 to round-8 files, all from before the call, so the working tree was untouched
by Codex. Same fresh full-scope prompt and context note as rounds 6 to 8, no declined-findings list
(round 8 declined nothing). Before the round the round-8 edits were checked intact in the target
diff and against AC-1 through AC-4, with no failure.

**Findings verified against source.**

1. **Applied.** `solver_set.c` (the sleep path) does `b3Array_Get( world->bodyMoveEvents,
   body->bodyMoveIndex )` for any body whose index is not null, and `physics_world.c` clears
   `bodyMoveEvents` at the start of each step. `bodyMoveIndex` rides in the imaged `b3Body`, so
   after a rewind an image-restored index points into an array `Rewind` has just cleared, and an
   immediate `SetAwake(false)` reads out of range. §9 step 5 now nulls it on every awake body
   (non-awake bodies already hold null, set by the sleep path), and §12 excludes it from the hash
   with a test of `SetAwake(false)` right after a rewind.
2. **Applied.** `b3Shape_SetFilter` with `invokeContacts` false assigns `shape->filter` and
   leaves the proxy's category bits alone, so the proxy category is state the shape record does not
   determine. Restore stays correct because proxy category bits are fixed at proxy creation and the
   §7.4 create entry carries them; the hash did not cover them, so a broken proxy re-insertion would
   have passed the oracle. §12's hash now lists each live proxy's tree, id, category bits, shape id
   and fat AABB, and test 2's churn includes a filter change that does not touch the proxy.
3. **Applied, without an identity-aware restore step.** The finding is right that "vanishes" and
   "reappears" were not tied to a mechanism. They are now tied to the journal: an undo of a shape's
   create or a redo of its destroy releases and clears `userShape`, and bringing a shape back leaves
   it null (an ordinary destroy already nulls the slot's handle, and record copies exclude the
   field). A shape alive at both ends keeps its handle. Slot reuse needs no separate handling
   because the walk applies each create and destroy in order.
4. **Applied.** Widening the interval cannot reduce journal bytes, so a minimum window can exceed
   `maxBytes` with every slot individually under it. §8 now admits that overrun the way it admits a
   single oversized slot, and test 6 covers it.

4 applied, 0 declined.

## Status

Finding count fell to 4 (1H-2M-1L) from 5. Finding 1 is new ground, an interaction between imaged
body state and the event arrays that a from-scratch reader found through a public API call
(`SetAwake`) immediately after a rewind. Finding 3 is a consequence of round 7's finding 2 fix, whose
wording named an effect without a mechanism; finding 2 is an oracle-coverage gap, and finding 4 an
unspecified budget case adjacent to an existing rule. No finding reopened a stated rationale. The
remaining risk keeps moving to the edges (event arrays, host handles, budgeting) rather than the
journal core.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 9: 4 findings (1H-2M-1L), 4 applied, 0 declined`

`Series total: 64 findings (32H-29M-3L) across 9 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L)`
