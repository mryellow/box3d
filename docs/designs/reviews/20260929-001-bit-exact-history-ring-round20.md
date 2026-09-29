---

title: Codex review — bit-exact history ring, round 20
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 20

## Review report (Codex final message)

## Summary

The proposed state split is broadly consistent with the source. I found two specification gaps that should be resolved before implementation. I made no file changes.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §12.1 | The full-state hash includes hull database membership and refcounts, but does not specify a canonical iteration order. The database is a hash map; its bucket layout can differ after a restore even when its contents match. A field-wise hash alone does not prevent a false mismatch. | Hash hull entries in content-defined order, independent of map buckets and pointers, and test equivalent databases with different insertion histories. |
| 2 | Medium | §12.3, §12.7 | Replay exactness tests compare state hashes at every tick, while event equality is tested only in the tree-layout cases. Requirement 1 also promises exact events under ordinary API churn, sleep, and worker-count changes. The state hash deliberately excludes event arrays. | Compare event types, contents, and order at every replayed tick in the general replay and cross-worker tests. |

## Checked, no change

- `ScrubBackward` checks restored and replayed transform/velocity hashes; the document correctly limits what that establishes (test/test_recording.c, recording.c).
- The serializer clears object `userData`, as the phase-0 caveat says (world_snapshot.c).
- Sensors swap overlap arrays and evaluate all sensors each step; imaging `overlaps2` and charging for sensor overlap count matches that path (sensor.c).
- CCD’s running fraction and eight-hit cap, and explosion’s traversal-order impulse application, support the proposed order fixes (solver.c, physics_world.c).
- The six world ID pools and separate tree proxy free lists match the inventory (physics_world.h, dynamic_tree.c).

## Proposed edits

Specify canonical hull-map hashing in §12.1. Extend §12.3–4 to compare replayed events each tick, while retaining §12.7’s targeted tree-layout cases.

## Unresolved / disagreements

The §11.1 serializer measurements are attributed to a review file outside the permitted reading scope, so I did not independently verify those figures. I found no source-tree contradiction in the architectural recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:37:52+10:00, ended 13:41:29+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 and 19, no declined-findings list. Before the
round the uncommitted round-19 edits were read in the diff and found intact. The final message above
has its source links reduced to plain file names, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Applied.** The hull database is a `b3HullMap` (`hull.c`) keyed by hull pointer with content
   hashing, and the doc's own §3 says hash tables are never iterated, so §12.1's "hull database
   membership and refcounts" had no defined order. Iterating the map would hash bucket order, which
   is not state. Entries are now hashed in ascending order of each hull's stored `hash`, which the
   content-deduplicated database makes a unique key.
2. **Applied.** Requirement 1 promises events are reproduced exactly on re-stepping, and
   `b3World_ComputeStateHash` excludes event arrays, so tests 3 and 4 compared state only. Test 3 now
   also compares each replayed step's events (types, contents and order) with the original run's for
   that tick, and test 4 runs 3 for hashes and events; test 7 keeps its targeted cases.

2 applied, 0 declined.

## Status

Finding count fell to 2 (0H-2M-0L), the lowest of the series and the fourth round in five with no
High. Both findings are unstated definitions in §12 (an iteration order, a test's coverage against a
stated requirement), not source mismatches. Neither was a consequence of round 19's fixes and neither
reopened stated rationale. The series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 20: 2 findings (0H-2M-0L), 2 applied, 0 declined`

`Series total: 111 findings (41H-61M-9L) across 20 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L)`
