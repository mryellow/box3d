---

title: Codex review — bit-exact history ring, round 39
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 39

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the source tree, but three contract and test details need revision. This was a read-only review; I did not run the proposed tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | §§2, 10, 12 | The bit-exact claim names callback order as its sole exception, yet `b3World_SetUserData` survives rewind. A pure callback that reads that value can therefore produce a different result after replay. The caller contract assigns host state to the caller, but requirement 1 does not state that qualification. | State explicitly that callers must restore or replay host state read by callbacks, including world `userData`; test a changed value that affects a callback result. |
| 2 | Medium | §§8, 14 | `tickCount` permits 1, while the budget rule assumes a minimum window of “say two imaged ticks.” The required eviction and interval-widening behavior is undefined when those rules conflict. | Define the minimum window for each valid `tickCount`, including 1, and specify which limit wins. |
| 3 | Low | §§7.4, 12 | The filter-change examples blur two source behaviors. In [shape.c](/home/user/box3d/src/shape.c), `SetFilter` can leave the proxy untouched when `invokeContacts` is false; when true, its `destroyProxy` comparison occurs after assigning the new filter and therefore requests recreation even for a mask-only change. A filter change alone does not move a proxy to another body-type tree. | Separate the no-proxy filter case from the body-type cross-tree case in the text and tests; account for the current `SetFilter` behavior. |

## Checked, no change

- The existing serializer covers the major world structures, while [recording.c](/home/user/box3d/src/recording.c) hashes only body poses and velocities. The document correctly limits what `ScrubBackward` establishes.
- The source supports the distinction between world proxy identity and tree layout, and between awake contact records and their owned manifold blocks.
- The design accounts for sensor overlap capture, moved proxies that outlive wakefulness, heap ownership during undo and redo, and phase 1’s temporary cost for dynamic and kinematic trees.
- Clearing step T’s events on rewind is consistent with the stated API contract.

## Proposed edits

Clarify the callback host-state obligation in §§2 and 10, make the `tickCount = 1` budget behavior explicit in §§8 and 14, and split the filter and body-type fixtures in §§7.4 and 12.

## Unresolved / disagreements

The document leaves approval of the three order changes and the forward-scrub requirement as open decisions. I found no further source-backed reason to reject the overall recommendation.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, stdin from
`/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout, raw output to the session
scratchpad. Started 2026-09-29T17:17:38+10:00, ended 17:22:34+10:00, exit code 0; no resume. Codex ran
read-only; `src/` and `include/` have no diff. Fresh full-scope prompt (target doc, source tree, no read of
`docs/designs/reviews/` or git history, file-only citations by design), no declined-findings list: no round
38 finding was declined. The final message above is the run's, one copy kept.

Before the round, Claude read the uncommitted diff of the target doc (round 38's four edits and status line;
not mangled), re-read the mechanism sections and checked round 38's bitset fix against its sibling journaled
containers: the pair set's add (`contact.c`, only for a key the pair query found absent) and remove (only for
a live contact's key) are always real changes, so its self-inverse entry needs no old value. No gap found.

**Findings verified against the doc and source.**

1. **Applied.** §10 already put host state a callback reads with the caller, but named only what `userData`
   points to; `world->userData`'s own value survives a rewind (§10) and a callback can read it, so §10 now
   names the value and says the caller sets or replays it per tick. Requirement 1's exception is about
   simulation-affecting bytes, and host configuration is not one (§10), so §2 is unchanged.
2. **Applied in part.** The two limits do not conflict: §8's `tickCount` eviction runs while a later imaged
   tick remains, so it can leave one imaged tick (interval 2, `tickCount` 1), and the minimum window of two
   imaged ticks is where the byte budget stops evicting and widens. §8's text said "say two imaged ticks"
   without saying `tickCount` is a separate limit; it now says `tickCount` can leave fewer and never widens
   the interval. No change to the design.
3. **Applied.** `b3Shape_SetFilter` (`shape.c`) returns before writing when the filter is unchanged, writes the
   filter and touches the proxy only when `invokeContacts` is set, and calls `b3ResetProxy`, which recreates
   the proxy in the tree of the shape's body type; only a body-type change moves it to another tree. Round 37's
   churn item said "a filter or body-type change ... in a different tree", which blurred the two. §12 test 2
   now lists the three cases separately: no proxy touch (`invokeContacts` false), a filter change that
   recreates the proxy in its tree, and a body-type change that recreates it in a different tree. This one was
   a consequence of the immediately preceding round's own fix (round 37's churn edit, Claude's).

3 applied (one in part), 0 declined. The status line's finding sequence and round count in the target doc
were updated to include this round. AC-1 to AC-4 were applied to the doc before the round and no shape
changed against them.

## Status

Finding count is 3 (0H-2M-1L), down from 4, still with no High (rounds 35 to 39). Finding 3 was a consequence
of round 37's own fix; findings 1 and 2 are older §10 and §8 wording. No finding contradicted stated
rationale, and finding 2 was a misreading of two limits the doc had not separated, now sharpened. The counts
since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5, 2, 4, 3) are flat, not falling;
not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 39: 3 findings (0H-2M-1L), 3 applied, 0 declined`

`Series total: 175 findings (51H-97M-27L) across 39 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L) -> 4 (0H-2M-2L) -> 3 (0H-2M-1L)`
