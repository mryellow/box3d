---

title: Codex review — bit-exact history ring, round 5
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 5

## Review report (Codex final message)

## Summary

The recording test supports the document’s starting point: a backward seek can reproduce the existing body-state hash. The proposed ring does not yet establish its broader bit-exact claim. I found gaps in pointer restoration, journal coverage, and timeline branching that could make a retained tick restore incorrectly.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §9, §7.1 | Restore resizes a contact’s manifold block and then copies the saved `b3Contact` record, which contains a raw `manifolds` pointer. That copy can replace the resized block’s pointer with an address from the captured timeline. Shape records likewise contain `materials` and geometry pointers. See `contact.h` and `shape.h`. | Define pointer ownership and relocation for each saved record. Restore pointees first, then copy scalar fields and bind live pointers; test repeated backward and forward scrubs across allocation and destruction. |
| 2 | High | §5.2, §7.1, §7.3 | A journal entry inside array `push` cannot capture values filled afterward through `b3Array_Emplace`. This occurs for sleeping body sims, joint sims, islands, and sensors. Some structural paths also change array elements or `count` directly. The claimed container boundary therefore does not ensure complete journal entries. See `container.h`, `solver_set.c`, and `body.c`. | Specify journaled operations that receive completed values, plus explicit tracked edits for existing elements and length changes. Make direct access impossible or identify and migrate every exceptional path. |
| 3 | High | §8, §9, §14 | Stepping after rewind says it overwrites future slots, but the first new step may leave later images and journal segments from the old timeline available, especially when `captureInterval > 1`. A subsequent forward scrub could combine two branches. | Invalidate and release **all** future slots and their owned blocks before the first mutation or step on a new branch. Define how `GetRestorableTick` reports the shortened window. |
| 4 | Medium | §10, §12 | The proposed hash excludes host data, while custom filter and `preSolve` callbacks can consult host context or object `userData` and affect physics. A matching engine-state hash alone is therefore not a complete replay oracle under the stated callback contract. See `sensor.c`, `contact.c`, and `physics_world.c`. | Require equivalent callback-visible host state during replay and let validation incorporate a caller-supplied state hash, or narrow the advertised coverage of `ComputeStateHash`. |
| 5 | Medium | §8, §14 | `maxBytes` and `bytesUsed` charge ring slots and journal-owned blocks, but the reusable staging buffer and retained arena capacity also occupy memory. An unusually large churn tick can leave a large staging allocation after its slot is evicted. | Define whether these allocations count toward reported memory and the budget; report capacity separately if the budget intentionally applies only to retained history. |
| 6 | Low | Front matter, §11 | The document names HEAD `5643cd8`, while the current checkout is `9a3be21`. Its performance table points to a review file that this independent pass was instructed not to read, so those measured figures remain unverified here. | Refresh the source revision and put the benchmark method or independently reproducible results in an allowed location before using the figures as acceptance evidence. |

## Checked, no change

- `test_recording.c` does compare hashes after backward seeks; `recording.c` confirms that hash covers body transforms and velocities, rather than the full proposed state.
- `sensor.c` processes sensors each step and sorts and deduplicates overlaps.
- `solver.c` confirms the documented CCD dependence on the running fraction and the eight-hit sensor cap. `physics_world.c` confirms explosion effects currently occur during tree traversal.
- `id_pool.c` uses a LIFO free array followed by a bump index. `dynamic_tree.c` has a separate proxy free list.
- `docs/simulation.md` documents determinism across worker counts and 64-bit platforms, subject to build settings.

## Proposed edits

Resolve findings 1–3 in the state model and restore sequence before treating the hot image and cold journal as the recommended implementation. Add focused tests for pointer-bearing objects, post-`Emplace` writes, and forward scrub after a branched rewind. Clarify the callback contract, memory accounting, and benchmark basis.

## Unresolved / disagreements

The proposed CCD, sensor-hit, and explosion ordering changes need solver-owner review because they can change results. The cross-platform bit-exact claim for the **new** restore path remains to be demonstrated by execution.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only`, model `gpt-6-sol`, reasoning effort `medium`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T11:32:44+10:00, ended 11:36:42+10:00, exit code 0 — no resume needed. Codex ran
read-only; the working tree was untouched by the call. Same prompt as rounds 3 and 4, no
declined-findings list (round 4's two declines were answered in the doc). The final message's file
links are reduced to plain backticked names, with no other change. Before the round, `git diff`
of the target was empty and the round-4 additions were checked against AC-1 through AC-4: the
single "move into the awake set" functions are choke points, not lists, and the restore list is
produced by the restore write function itself, so neither fails a gate.

**Findings verified against source.**

1. **Applied.** Confirmed `contact.h` holds a raw `manifolds` pointer and a `triangleCache`
   array, and `shape.h` holds `materials` and geometry pointers. §9 step 3 resized the block and
   then copied the whole record, which would overwrite the live pointer with the captured one.
   Image copies now skip pointer fields; a shape's material block is journaled whatever the
   owner's wake state, because an image carries no pointees (the previous rule left an awake
   shape's reallocated material block uncovered).
2. **Applied.** Confirmed `b3Array_Emplace` followed by field-by-field fill is how `solver_set.c`,
   `body.c`, `island.c`, `sensor.c`, `shape.c` and `joint.c` populate these arrays, and that
   `island.c` and `body.c` write `count` directly (island link arrays cleared, decremented). A
   journal entry made at push time cannot hold values filled afterward. §5.2's container type now
   takes completed values, has no `Emplace` and no writable `count`, and gains clear and set
   operations; §7.1's array entry covers both. The awake set's in-step element access goes through
   a separate hot accessor that asserts the world is inside a step.
3. **Applied.** Confirmed §8's "overwrites slots T+1… the branch is implicit" leaves stale
   slots readable: with `captureInterval` above 1 a slot after the first replayed tick keeps its
   old timeline, and a forward scrub across both mixes branches. Rewind still leaves later slots
   intact, so forward scrub works until the first mutation; the first journal entry or capture
   after a rewind truncates the ring after T and frees owned blocks.
4. **Applied.** The hash covers engine state only, and §10 already requires callbacks to be
   functions of their inputs; host state they read is the caller's. Stated in §10 that a lockstep
   caller combines the engine hash with its own host-state hash.
5. **Applied.** The staging buffer and arena capacity are real memory outside §8's charge.
   `bytesUsed` is defined as retained history and a separate `capacityBytes` reports arena plus
   staging capacity.
6. **Declined.** `git log -- src` shows `5643cd8` is the newest commit touching `src`; everything
   since is docs, so the doc's revision is current. The performance figures come from design 001's
   one-off perf pass, which §11.1 names as its source; a from-scratch round is told not to read
   `docs/designs/reviews/`, so it cannot check them, but that is the workflow's rule and not a
   defect in the doc.

5 applied, 1 declined.

## Status

Finding count fell to 6 (3H-2M-1L) from 9, and it is the smallest so far. Findings 1 and 2 are
defects in earlier rounds' fixes: the whole-record copy in §9 step 3 (round 3's edit) and the
completed-value container API (this series' AC-2 rewrite). Finding 3 is older ground the doc never
examined. Findings 4 and 5 are contract and accounting clarifications; 6 was mostly noise. None
reopened a stated rationale. The Highs are all in the journal/restore mechanics, the same area as
every prior round, which is where the remaining risk sits.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 5: 6 findings (3H-2M-1L), 5 applied, 1 declined`

`Series total: 46 findings (26H-19M-1L) across 5 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L)`
