---

title: Codex review — bit-exact history ring, round 38
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 38

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the source tree, but two write paths need clearer rules before the design can claim complete, bit-exact restoration. No files were modified.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §7.1, bitset entries | Graph removal can clear an already-clear bit for a static body ([constraint_graph.c](/home/user/box3d/src/constraint_graph.c), [solver_set.c](/home/user/box3d/src/solver_set.c)). If that call produces the specified “clear; undo by set” entry, rewind creates a bit that was absent. | Make the journaled bitset mutator record an entry only when the bit changes, or store its old value. Test static-body contact and joint removal across backward and forward scrubs. |
| 2 | Medium | §§5.2, 8, 10 | Debug draw writes `shape->userShape` outside a step ([physics_world.c](/home/user/box3d/src/physics_world.c)). The proposed writable routes offer a step-only hot accessor or a setter that journals cold shapes and truncates future history. Neither fits this host-owned handle. | Define a shape-module handle accessor that neither journals nor truncates history. Use it for debug-draw creation and handle release, and test drawing between rewind and forward scrub. |
| 3 | Low | §12.3 | The replay test says to compare every intermediate tick’s hash with the hash recorded at **P**. Intermediate ticks should be compared with their own recorded ticks. | Change the assertion to compare each replayed tick `t` with its recorded hash at `t`. |
| 4 | Low | §§9, 11–12 | Restore’s stated awake-and-journal cost omits the validation pass explicitly required in validation builds; those validators can scan wider world state. | Qualify the restore cost and cost acceptance target as release-build measurements, while retaining validation-build checks. |

## Checked, no change

- [test_recording.c](/home/user/box3d/test/test_recording.c) exercises backward keyframe scrub; [recording.c](/home/user/box3d/src/recording.c)’s current hash covers transforms and velocities, as the design states.
- [sensor.c](/home/user/box3d/src/sensor.c) processes every sensor each step and sorts and deduplicates overlaps. The sensor-count exception is justified.
- [solver.c](/home/user/box3d/src/solver.c) uses a running CCD fraction and caps continuous sensor hits at eight; [physics_world.c](/home/user/box3d/src/physics_world.c) applies explosion effects during tree traversal. The proposed order changes address real dependencies.
- [dynamic_tree.c](/home/user/box3d/src/dynamic_tree.c) has a separate proxy free list. Preserving proxy identity separately from tree layout is necessary.
- The document distinguishes simulation state from step events, host `userData`, borrowed geometry, and renderer handles.

## Proposed edits

Specify change-aware bitset journal entries and the host-handle write route; add their restore tests. Correct the intermediate-tick hash assertion and qualify the restore cost claim.

## Unresolved / disagreements

The performance baseline cites material in the excluded reviews directory, so I did not independently verify those figures. The document’s proposed benchmark gate remains necessary.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, stdin from
`/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout, raw output to the session
scratchpad. Started 2026-09-29T17:06:13+10:00, ended 17:11:36+10:00, exit code 0; no resume. Codex ran
read-only; `src/` and `include/` have no diff. Fresh full-scope prompt (target doc, source tree, no read of
`docs/designs/reviews/` or git history, file-only citations by design), no declined-findings list: no round
37 finding was declined. The final message above is the run's, one copy kept. The round 37 files and the
doc were committed before the round (`0f7cb7d`), so the doc had no uncommitted diff to read at its start;
Claude re-read the mechanism sections (§5.2, §7.1, §9) against their sibling cases and found nothing further
to fix before the run.

**Findings verified against the doc and source.**

1. **Applied.** `b3RemoveContactFromGraph` (`constraint_graph.c`) clears both bodies' bits in a colour's
   `bodySet` with a comment that this may clear a bit for a static body, and the joint removal in
   `solver_set.c` does the same; a static body's bit is never set (the add path sets only dynamic bodies'
   bits), so the clear is on an already-clear bit. §7.1's entry, "clear / set", would set that bit on undo.
   The entry now carries the bit's old value and undo and redo write a value rather than toggle.
2. **Applied.** `b3DrawShapes` (`physics_world.c`) creates `shape->userShape` when it is null, outside a
   step, and `shape.c` releases it on a geometry change. §5.2's routes were the step-only hot accessor, the
   setter entry (journals a cold owner, truncates) and restore's writer, none of which fits a host-owned
   handle written between steps. §5.2 now names one function in `shape.c` that writes it and neither
   journals nor truncates.
3. **Applied.** §12 test 3 compared every intermediate tick's hash with the hash recorded at P; each tick's
   is compared with its own recorded hash. A wording fault in older text.
4. **Applied.** §9 step 6's validators (`b3ValidateSolverSets` and the others) walk more than the awake
   state, and §12 test 5 gave no build. The cost measurements are now stated as release-build ones without
   those checks.

4 applied, 0 declined. The status line's finding sequence and round count in the target doc were updated to
include this round. AC-1 to AC-4 were applied to the doc before the round and no shape changed against them:
finding 1's fix stores the old value in the entry and adds no check that changes control flow (AC-4).

## Status

Finding count is 4 (0H-2M-2L), up from 2, still with no High (rounds 35 to 38). Finding 1 is in older §7.1
text (bitset entry) that no earlier round tested against static bodies; finding 2 is a gap in §5.2's route
list for a field round 37's fix also touched (`userShape`), so it is partly a consequence of the preceding
round's handling of the handle; findings 3 and 4 are older wording. No finding contradicted stated
rationale. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5, 2, 4) are
flat, not falling; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 38: 4 findings (0H-2M-2L), 4 applied, 0 declined`

`Series total: 172 findings (51H-95M-26L) across 38 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L) -> 4 (0H-2M-2L)`
