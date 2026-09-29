---

title: Codex review — bit-exact history ring, round 16
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 16

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is plausible against the current source tree. The document correctly identifies several important sources of replay state, including moved proxy bits, proxy IDs, sensor overlaps, and heap owned contact data. Its ring contract and performance claims need tightening before the design is ready to implement. This was a read only review; I did not run the proposed tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §8, §14 | `tickCount` promises a retained window, but the ring rules specify eviction only for arena pressure or wrap. They do not say when to evict by tick count. With `maxBytes = 0`, retention and memory use have no stated bound. | Define the exact tick count eviction rule, including which journal segments remain when the oldest image is evicted. Test it with an unbounded byte budget. |
| 2 | Medium | §1, §8 | The claim that a capture slot “never allocates” conflicts with the stated arena realloc on growth. Reservation at step end still occurs inside `b3World_Step`; the staging buffer can also grow during the step. | Describe capture allocation as amortized, or specify preallocation if avoiding step time allocation is a requirement. |
| 3 | Medium | §10, §14 | “Inputs are every API call” and “every setter” are broader than the actual restore contract. World `userData` and callback registrations remain live across rewind, while body, shape, and joint `userData` are restored. A caller following the broad wording could replay host configuration under the wrong assumptions. | State the tracked API calls and live host configuration exceptions together in the caller contract. |
| 4 | Medium | §11.2 | The time sliced replay example says to perform three replay ticks “plus the live tick” per frame. A single world that is still behind the current tick cannot step that live tick without skipping intervening inputs. | Specify that incoming ticks join the replay backlog until the world catches up, and explain how rendering uses the saved old timeline poses. |
| 5 | Medium | §12 | The proposed public hash promises identical values across supported platforms, but verification lists restore, replay, and worker count tests only. The current determinism test uses a narrower state hash than the proposed field wise hash. | Add a cross platform golden hash test for the new hash, with geometry, pools, proxies, and sleeping state represented. |
| 6 | Low | Front matter | The document says its source references were checked at `5643cd8`; the reviewed tree is `6c8cb71`. That provenance no longer describes this review’s source revision. | Refresh the revision note when the source audit is updated. |

## Checked, no change

- `ScrubBackward` does seek backward and compare replayed hashes. The existing hash covers body transforms and velocities, as the document says: `test_recording.c`, `recording.c`.
- The serializer clears body and joint `userData` on restore, supporting the phase 0 limitation: `world_snapshot.c`.
- Proxy IDs have a per tree free list, and the broad phase wraps proxy creation and destruction: `dynamic_tree.c`, `broad_phase.c`.
- CCD currently uses a running fraction and caps sensor candidates at eight; the proposed order changes address real source behavior: `solver.c`.
- Sensor overlaps are sorted after tree queries, and explosions currently apply impulses during traversal: `sensor.c`, `physics_world.c`.

## Proposed edits

Define tick retention and eviction first, then align the API comments and ring tests with that rule. Revise the allocation and time sliced replay wording. Tighten the caller contract and add cross platform hash acceptance coverage.

## Unresolved / disagreements

The document leaves the three simulation changing order fixes, public hash API, forward scrub requirement, and journaling mode open for owner decisions. I found no source evidence that resolves those choices. The performance figures attributed to a review file were not independently checked because that file is outside the permitted reading scope.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T13:02:17+10:00, ended 13:06:44+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 13 to 15, no declined-findings list (round 15's one
decline was answered in the doc). Before the round the round-15 edits were checked intact and
against AC-1 through AC-4 with no failure. The final message above has its source links reduced to
plain file names, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Applied.** §8 defined eviction only for arena pressure and wrap, and §14 describes `tickCount` as
   the retained window, so with `maxBytes = 0` nothing bounded retention. §8 now evicts from the
   oldest end after each capture while the span from the oldest retained imaged tick to
   `historyTick` exceeds `tickCount` and a later imaged tick remains, independent of `maxBytes`.
2. **Applied.** §8 said the ring slot is never allocated inside the step while also saying the arena
   reallocs on growth, and reservation runs at step end inside `b3World_Step`. §1 and §8 now say
   capture allocation is amortized to zero once the arena and staging buffer have warmed up.
3. **Applied as a sharpening.** §10's callback bullet already lists the function pointers and
   `world->userData` as unrestored; the first bullet's "every setter" wording read broader. It now
   points at that exception. The callback bullet itself is unchanged.
4. **Applied.** §11.2 (3)'s "plus the live tick" cannot be done by one world still behind the current
   tick, since stepping it would skip the inputs for the ticks in between. It now says ticks that
   arrive while the world is behind join the replay backlog and are stepped in order.
5. **Applied.** §14 says the hash is the same across the platforms the determinism guarantee covers,
   and §12 had no test for that. §12 test 4 now adds a golden-value comparison across platforms,
   in the style of `test_determinism.c`, over scenes with geometry, pool and proxy churn and
   sleeping islands.
6. **Declined; doc sharpened.** `git log -1 -- src include` is `5643cd8`, so the doc's source
   revision is current and the review tree `6c8cb71` differs from it only by docs (the same check
   round 5's finding 6 recorded). The front matter said "HEAD 5643cd8", which read as the repository
   head; it now says "the last commit touching `src/` and `include/`".

5 applied, 1 declined.

## Status

Finding count rose to 6 (1H-4M-1L) from 4, and a High returned after round 15's zero. The High (1) is
a missing eviction rule that no earlier round found, the first ring-eviction finding since round 14's
finding 3, which turned out to be a non-defect in a neighbouring rule; the two are the same area
(§8), so the ring's retention rules had a real hole underneath the answered one. Findings 2 and 3 are
wording over-promises of the kind rounds 13 and 15 found. Findings 4 and 5 are new: a scenario in the
cost discussion and a missing test for a stated guarantee. Finding 6 asked for a fix the source did
not need. No finding reopened a stated rationale, and none was a consequence of round 15's fixes. The
count is bouncing between 4 and 7 with mechanism-level findings still appearing; the series is not
converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 16: 6 findings (1H-4M-1L), 5 applied, 1 declined`

`Series total: 98 findings (40H-52M-6L) across 16 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L)`
