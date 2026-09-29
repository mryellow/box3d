---

title: Codex review — bit-exact history ring, round 22
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 22

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the source tree, but three points need resolution before its bit-exact claim and verification plan are internally consistent. This was a read-only source review; no implementation or performance tests were run.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§2, 7.4, 10 | The document names order-dependent `preSolve` effects as its one replay exception, but CCD also calls the custom filter while traversing tree candidates (`src/solver.c`). A layout-changing restore can change that callback’s order. This conflicts with the “one named exception” claim, even though §10 separately requires callbacks to be pure. | State one consistent callback contract: either require purity for both callbacks and remove the special exception, or document both order-sensitive cases. |
| 2 | Medium | §§5.3, 9, 12 | Restore copies each sensor’s `overlaps2` and treats `overlaps1` and `hits` as scratch (`src/sensor.c`). The proposed hash says it covers “the sensor array and its overlap state,” which could include `overlaps1`. If it does, a correct rewind can fail the proposed `hash(Rewind(T)) == hash(T)` test. | Define the hash’s sensor fields explicitly. Exclude scratch arrays, and add a restore test in which the prior overlap set differs between T and P. |
| 3 | Low | §12 | Hull database entries are to be hashed in ascending order of their stored 64-bit hash. The database compares hull content as well as the hash (`src/physics_world.c`, `src/hull.c`); distinct hulls can therefore share a hash and have no specified order in the state hash. | Break hash ties with a deterministic comparison of canonical hull content. |

## Checked, no change

- `ScrubBackward` restores keyframes and checks the existing transform and velocity hash; the document correctly limits what that test demonstrates (`test/test_recording.c`, `src/recording.c`).
- The FAQ says rollback determinism is unavailable, while the recording code provides a narrower restore and replay mechanism (`docs/faq.md`, `src/world_snapshot.c`, `src/recording_replay.c`).
- Sleeping solver sets, sparse records, contact manifolds, sensor arrays, and per-tree proxy free lists exist in the forms the inventory describes (`src/solver_set.c`, `src/contact.c`, `src/sensor.c`, `src/dynamic_tree.c`).
- The identified tree-order risks are real: CCD uses a running fraction and caps sensor hits; explosion impulses are currently applied during tree traversal (`src/solver.c`, `src/physics_world.c`).
- The stated whole-world resimulation cost and the phase-1 limitation for worlds with many proxies are appropriately disclosed. The performance figures were not independently verified; I did not read the referenced review file.

## Proposed edits

Resolve findings 1–3 in §§2, 7.4, 10 and 12, and make the sensor hash field list precise enough to serve as a restore-test oracle.

## Unresolved / disagreements

The proposed capture and restore costs remain estimates until the phased implementation and §12 measurements exist. I found no source-based reason to reject the overall architecture.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:48:11+10:00, ended 13:51:31+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 21, no declined-findings list. The final
message above has its source links reduced to plain file names, per the citation rules; nothing else
is changed.

**Before the round (Claude's own pass, step 2).** Checking the §7.1 inventory's sibling cases found
that the pool alloc row and the manifold block row both said a contact's blocks ride in its alloc
entry, while `b3CreateContact` (`contact.c`) allocates the id and leaves `manifolds` null, the block
being created later in the narrow-phase update (`b3AllocateManifolds`). Both rows now say a contact's
free entry carries the handles, its alloc carries none, and alloc undo hands the entry whatever
blocks the record holds at that moment. This is not one of Codex's findings and is not counted.

**Findings verified against source.**

1. **Applied.** `b3ContinuousQueryCallback`'s filter path (`solver.c`) calls `world->customFilterFcn` for each
   tree candidate in traversal order, so a custom filter with order-dependent side effects has the
   same layout dependence as `preSolve`. Pure callbacks are unaffected, which is what §10 already
   requires; the named exception is for side effects, so it now names both callbacks, in §2 and §10.
2. **Applied.** `b3SensorTask` (`sensor.c`) swaps `overlaps1` and `overlaps2` at the start of
   each sensor's pass, so `overlaps1` at the next step is the restored `overlaps2` and is scratch,
   as §5.3 says. §12 test 1's "overlap state" was ambiguous; it now names `overlaps2`. The requested
   extra restore test is covered by test 3's event equality, which fails if `overlaps2` restores
   wrongly, so none was added.
3. **Applied.** The hull database compares by content and says it does not trust the hash
   (`b3AddHullToDatabase`, `physics_world.c`), so two distinct hulls may share a stored `hash`, and
   ordering by it leaves ties. The hash now combines each entry's `hash` and refcount and sums the
   entries' results, which needs no order.

3 applied, 0 declined.

## Status

Finding count fell to 3 (0H-2M-1L) from 4, with no High. None was a consequence of round 21's fixes:
finding 1 is a callback path the original exception omitted, finding 2 is hash wording written
before round 18, and finding 3 sits in the hull-map ordering added in round 20, so it is a
consequence of an earlier round's fix but not of the immediately preceding one. No finding reopened
ground the doc's rationale already covered. The counts since round 17 (3, 4, 2, 4, 3) hover at 2 to 4
with no clear downward trend, and the categories are still new (callback contract, hash coverage),
not only edge cases. The series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 22: 3 findings (0H-2M-1L), 3 applied, 0 declined`

`Series total: 118 findings (43H-64M-11L) across 22 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L)`
