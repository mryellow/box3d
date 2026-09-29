---

title: Codex review — bit-exact history ring, round 12
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 12

## Review report (Codex final message)

## Summary

The whole-world image and journal approach is sound in principle, and the document identifies several important source dependencies correctly. Two restore paths need a more complete ownership design, and the correction example needs a tick-boundary fix.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1 | The journal says destroyed islands transfer their three heap arrays to an entry, but no island entry or pool payload carries them. `b3DestroyIsland` (`src/island.c`) frees those arrays; the split path also detaches and frees them outside that function. Undo across a merge or split could therefore restore a record with lost arrays. | Specify an island ownership entry and route split’s detached arrays through it. Test backward and forward scrub across merge and split. |
| 2 | Medium | §6, §8, §14 | The correction example applies a correction **after** stepping `serverTick`. That tick’s image has already closed; the correction belongs to the next segment. A later rewind to `serverTick` restores the uncorrected image, despite the example treating the correction as part of that tick. | State the tick-boundary convention and apply the correction before the next step, or explicitly recapture the corrected tick. |
| 3 | Medium | §7.4, §13 | §7.4 says proxy identity needs journaling “either way” under the raw-tree fallback, while phase 1 says a raw tree image already includes the proxy free list and needs no proxy journal yet. | Qualify the §7.4 statement: identity journaling is required when raw tree images are removed. |
| 4 | Medium | §5.1, §10, §12 | “Every API call” is to be undone, but `b3World_SetUserData` (`src/physics_world.c`) changes a world-owned host pointer whose restore policy is unspecified. A callback can read that pointer through the public getter; the proposed hash excludes host values. | Explicitly restore and separately verify world `userData`, or put it under the caller’s stable host-state contract. |

## Checked, no change

- The existing state hash (`src/recording.c`) covers transforms and velocities only; `ScrubBackward` (`test/test_recording.c`) supports the document’s narrower demonstrated claim.
- The FAQ’s rollback statement and the cross-platform and worker-count determinism descriptions match `docs/faq.md`, `docs/simulation.md`, and `CMakeLists.txt`.
- Sensor processing swaps its overlap buffers, then uses the prior `overlaps2` contents as `overlaps1`; imaging `overlaps2` is consistent with the next step’s comparison.
- The described CCD sensor cap and running-fraction dependence, explosion query-order dependence, and separate tree proxy free lists are present in the source.
- The phase-0 serializer clears body, shape, and joint `userData`, as the document warns.

## Proposed edits

Define island block ownership and split handling in §7.1, fix the §14 correction sequence and tick convention, reconcile the two proxy-identity statements, and specify world `userData` behavior with a focused restore test.

## Unresolved / disagreements

The document’s listed solver-order changes remain an explicit design decision for the solver owner. I did not remeasure the supplied §11.1 figures or execute tests; this was a read-only source review.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T12:26:43+10:00, ended 12:30:13+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status` before and after the call showed the same set (the target doc and
`WORKFLOW.md` modified, the untracked round-6 to round-11 files), so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 6 to 11, no declined-findings list (round 11
declined nothing). Before the round the round-11 edits were checked intact and against AC-1 through
AC-4 with no failure. The Codex final message above has its line-numbered source links reduced to
plain paths, per the citation rules; nothing else is changed.

**Findings verified against source.**

1. **Applied.** `b3DestroyIsland` (`island.c`) frees the island's `bodies`, `contacts` and `joints`
   arrays; the merge path reaches it for the small island, and `b3SplitIsland` nulls the base
   island's three arrays "so b3DestroyIsland won't free them", calls it, then frees the detached
   arrays itself. §7.1's ownership-transfer prose and the round-10 rule ("for every owner — set,
   island, ...") said islands transfer their arrays, but no table row carried the handles, so the
   entry format was missing. §7.1 now has an island create/destroy row with the three handles and
   sizes, `b3DestroyIsland` as the only destroyer, and split handing its detached arrays to that
   entry rather than freeing them. Undo order (reverse) reattaches the arrays at destroy-time
   content before any earlier link-array entry is undone, so the contents match.
2. **Applied as a clarification; the doc's convention was already right.** §4 and §6 already define
   a call between step t−1 and step t as part of tick t's segment, and §10 says a rewind undoes
   every setter made after T, so the example's correction after step `serverTick` is an input to the
   next tick and `Rewind(serverTick)` correctly restores the uncorrected image. Nothing needed
   recapturing. §14 now states the convention next to the example, since the example was the one
   place a reader could take it the other way.
3. **Applied; the two statements did conflict, and the fix is broader than Codex proposed.** The
   raw-image fallback covers only the kinematic and dynamic trees (§7.4); the static tree is never
   imaged, so its proxy ids have no image to ride in and need journaling from phase 1. The proxy
   entry is one entry shared by all three trees, so §13 now puts it in phase 1 for all three,
   removes it from phase 2's list, and §7.4's closing sentence says why it is needed either way.
4. **Applied, as a policy.** `b3World_SetUserData` assigns `world->userData` (`physics_world.c`),
   and the world scalar struct is imaged, so the field would have been restored by accident. It is a
   host pointer of the same class as the callback pointers §10 already leaves unrestored, so §9 step
   3 now excludes callback pointers and `userData` from the world scalar copy, §10 names
   `world->userData` beside the function pointers, and test 2 includes a `b3World_SetUserData` call
   whose value must survive each rewind.

4 applied, 0 declined.

## Status

Finding count held at 4 (1H-3M-0L) for a fourth round. Finding 1 is a real completeness gap in the
entry inventory; it sits next to round 10's finding 3 (pool entries missing ownership handles), the
same class one structure over, so the ownership-handle sweep across owners was incomplete after
round 10. Finding 3 is an inconsistency between §7.4 and §13 that predates the series. Finding 4 is
adjacent to round 11's `userData` finding. Finding 2 asked for a convention the doc already stated.
No finding reopened a stated rationale, and none was a consequence of round 11's fixes. The count
is flat and the findings are still mechanism-level, so the series is not converging yet.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 12: 4 findings (1H-3M-0L), 4 applied, 0 declined`

`Series total: 76 findings (35H-38M-3L) across 12 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L)`
