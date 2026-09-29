---

title: Codex review — bit-exact history ring, round 7
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 7

## Review report (Codex final message)

## Summary

The whole-world image and journal approach is plausible, but the proposed write inventory and restore rules have gaps that can leave a restored world with missing shape state or invalid host-owned pointers.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.3 | The shape write inventory omits writes outside `shape.c`. `body.c` changes shapes and fat AABBs in `b3Body_SetTransform`, changes shape flags in `b3Body_EnableHitEvents`, and directly frees shape slots in `b3DestroyBody`. `sensor.c` changes a swap-moved shape’s `sensorIndex`. These include cold-owner writes that the listed migration would miss. | Add these callers and route every affected shape and fat-AABB write through the proposed accessors. Include sleeping, static, disabled, and body-destruction cases in restore tests. |
| 2 | High | §5.1, §9, §10 | Whole-shape restore would copy `userShape`, although it is a host-owned renderer handle. `shape.c` can destroy that handle after T; restoring its old pointer could then draw through or destroy a stale handle. `world_snapshot.c` already treats these handles separately during restore. | Exclude `userShape` from images and record writes; reconcile live renderer handles by shape identity and generation, releasing handles for shapes that vanish. State the corresponding lifetime rule for `userData`. |
| 3 | Medium | §7.4, §9 | The tree fixup says to call `b3DynamicTree_MoveProxy` for every restored shape record. Disabled shapes have `proxyKey == B3_NULL_INDEX`, and `dynamic_tree.c` requires a valid proxy id. | Skip shapes without a live proxy, including records restored to a free slot. Specify how the restore list handles multiple writes to one shape. |
| 4 | Medium | §10, §12 | The borrowed-geometry contract requires mesh, height-field, and compound data to remain *valid*, but does not require their contents to remain unchanged. `shape.c` retains pointers to that data, which simulation reads after rewind. An in-place host edit could therefore change replay without any journal entry. | Require borrowed geometry to remain immutable while referenced by any retained tick, or define a recorded geometry-replacement operation. |
| 5 | Low | §8, §14 | §8 says arena and staging capacities are “reported separately,” while `b3HistoryInfo` exposes only their sum as `capacityBytes`. | Expose two capacity fields or change the prose to describe the sum. |

## Checked, no change

- `test_recording.c` does compare hashes across backward seeks; `recording.c` confirms that the existing hash covers body transforms and velocities, not full simulation state.
- `id_pool.c` uses a free list and `nextIndex`, consistent with the proposed pool journaling.
- `sensor.c` processes every sensor each step and sorts and deduplicates overlaps, supporting the stated sensor cost and ordering rationale.
- `physics_world.c` confirms that explosion callbacks currently wake bodies and accumulate impulses during tree traversal.
- `world_snapshot.c` confirms the phase-0 need for a geometry registry and host `userData` rebinding.

## Proposed edits

Revise the shape mutation inventory and accessor migration; define host-handle reconciliation in restore; guard tree fixup for absent proxies; make borrowed-geometry immutability explicit; align the capacity prose and API.

## Unresolved / disagreements

The design’s stated open decisions on solver order changes, public hash coverage, and whether forward scrub is needed remain open. I found no source evidence that resolves them.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T11:49:19+10:00, ended 11:53:42+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` after the call showed only the target doc and `WORKFLOW.md` modified plus
the untracked round-6 file, all from before the call, so the working tree was untouched by Codex.
Same fresh full-scope prompt as round 6: target doc and source only, no access to
`docs/designs/reviews/`, file-only citations stated as by design, and the same context note (the
doc's revision is the newest commit touching `src`; the section 11.1 figures come from another
design's perf pass). No new declined-findings list, since round 6 declined nothing. Before the
round the whole `git diff` of the target (round 6's edits) was read and was not mangled; the
sibling re-read found no further gap (`b3SensorTaskContext::eventBits`, the one other bitset near
sensors, is per-step scratch). The additions were rechecked against AC-1 through AC-4: the
const-qualified access is a compile-time boundary, not a check; nothing shadows a live structure.
The file links in Codex's final message are reduced to plain backticked names, with no other
change.

**Findings verified against source.**

1. **Applied.** `body.c` writes shape and fat AABB state in `b3Body_SetTransform`, writes shape
   flags in `b3Body_EnableHitEvents`, and frees shape ids in `b3DestroyBody`; `sensor.c` writes the
   swap-moved shape's `sensorIndex`. None was in §5.2's shape row. The row now lists them. The
   listing is descriptive, since the compiler-enforced boundary is what guarantees completeness.
2. **Applied.** `shape.c` calls `destroyDebugShape` on `userShape` when a shape is destroyed, and
   `world_snapshot.c` treats `userShape` separately on restore, so a whole-record copy would
   reinstate a possibly released handle. §9 step 3 excludes `userShape` from record copies, and
   §10 gains a host-handles rule for `userShape` and `userData`. Codex's proposal to reconcile
   handles by shape identity and generation was not taken: keeping the live handle and releasing it
   when the shape vanishes is the smaller design and needs no generation bookkeeping.
3. **Applied.** `shape.c` shows a shape can hold `proxyKey == B3_NULL_INDEX`, and
   `b3DynamicTree_MoveProxy` asserts a valid proxy id. §7.4's fixup pass now moves only shapes whose
   restored record holds a proxy, and says a repeated listing is idempotent.
4. **Applied.** The shape retains pointers to borrowed geometry and the simulation reads through
   them after a rewind, so an in-place edit changes replay with no journal entry. §10 now requires
   the data to stay unmodified as well as valid.
5. **Applied.** §8 said the arena and staging capacities are reported separately while
   `b3HistoryInfo` had one summed field. The struct now has two fields.

5 applied, 0 declined.

## Status

Finding count rose to 5 (2H-2M-1L) from 4, after four rounds of falling or flat counts, so the
trend is not yet monotone. Finding 1 is an incompleteness in the §5.2 inventory that earlier
rounds' fixes named but never checked against `body.c`; finding 2 is new ground (host handles);
finding 3 is a defect in §7.4's fixup pass, older than any recent fix; finding 4 and 5 are contract
and API-consistency points. Finding 5 is a consequence of round 5's fix (the split capacity
prose). No finding reopened a stated rationale. The Highs moved outside the journal core, into the
inventory and the host boundary, which suggests the mechanism is settling while its edges are not.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 7: 5 findings (2H-2M-1L), 5 applied, 0 declined`

`Series total: 55 findings (30H-23M-2L) across 7 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L)`
