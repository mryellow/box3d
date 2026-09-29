---
title: Codex review — bit-exact history ring, round 42
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Completed in one attempt, no resume.
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 42

## Review report (Codex final message)

## Summary

I reviewed the design against `src/` and `include/` without modifying files or reading `docs/designs/reviews/`. The hot-image and cold-journal approach is broadly consistent with the source. I found one gap in its correctness guarantee and two smaller inconsistencies.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §2.7, §5.2, §7.1 | The claimed compiler-enforced journal completeness does not follow from the proposed hot accessor. It returns a writable record pointer after checking that the owner is awake, but C cannot restrict subsequent writes to hot fields or prevent a structural write through that pointer. A structural write during a step can therefore bypass the journal even though §7.1 requires it unconditionally. | Keep structural fields behind narrow mutators, or limit writable hot access to specific fields or types. Revise the “verified by construction” claim to match the boundary actually enforced. |
| 2 | Medium | §2.1, §5.3, §10 | §2 calls callback order the one named bit-exactness exception, while §5.3 also excludes the string returned for colliding name hashes. `src/name_cache.c` retains the first string for an id across rewinds. A pure callback that reads a name can therefore see a string left by a discarded branch. | State the name-collision exclusion alongside the callback contract, or restore the relevant name-cache mapping. Add a forced-collision branch test. |
| 3 | Low | §4, §7.4, §14 | Restore cost counts re-marking the moved proxies as linear in their number. `b3DynamicTree_MarkProxyMovedSerial` walks each proxy’s ancestors (`src/dynamic_tree.c`), so this part also depends on tree height. | Include the ancestor-walk cost in the restore estimate and benchmark it with many moved proxies. |

## Checked, no change

- `b3HashWorldState` hashes body poses and velocities only; the design correctly calls for a wider hash.
- The source supports the separate treatment of proxy identity and tree layout. Proxy create/destroy goes through the broad-phase wrappers, while `b3ResetProxy` can recreate a proxy.
- The CCD sensor cap and running solid fraction, explosion traversal order, sensor overlap sorting, and owned contact, shape, and sensor blocks match the problems the design identifies.
- The phase-0 serializer caveats about scrubbed `userData` and retained `userShape` handles match `src/world_snapshot.c`.
- I found no additional sibling-case gap in the stated awake/sleep transition rules for body and joint arrays, shape records, contact blocks, sensors, or moved proxies.

## Proposed edits

Tighten the §5.2 accessor design and its proof claim; reconcile the name-collision contract across §§2, 5, and 10; correct the moved-proxy restore cost.

## Unresolved / disagreements

The performance baseline and cost targets remain unverified in this review because their cited review file was excluded by the request. The proposed design still needs its §12 implementation tests before its wider bit-exact claim can be established.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Invocation: `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`, foreground, stdin from `/dev/null`, `timeout 540` with the tool timeout at 600000 ms, the same prompt as round 41. Start 2026-09-29T17:55:05+10:00, end 2026-09-29T18:01:39+10:00, exit code 0. `git status --short` before and after is identical, so Codex touched nothing. Before the run, §12's new generation clause was narrowed to the record arrays that carry a generation (bodies, shapes, contacts, joints), since islands and solver sets have none.

1. **Declined as a defect; §5.2 sharpened.** The hot accessor was never meant to restrict fields: it asserts the record's owner is awake, and §7.1's rule already says every write to an awake owner's record is unjournaled whatever the field, because the tick's image covers it. Structural writes that matter to the journal (create, destroy, link, transitions) reach records that are not awake or are entering or leaving the awake set, and the transitions journal the pre-wake and final bytes (§7.1). A structural field written through the hot pointer on an awake record is therefore covered without an entry, so nothing can bypass the journal in a way that loses state. The finding restated a field-level boundary without engaging that. §5.2 now says the accessor's boundary is ownership, not field, and why that is sufficient. Requirement 7's wording is about cold structures, which the accessor never reaches.
2. **Applied, test proposal declined.** §5.3 excludes the string for colliding names from the contract while §2.1 called callback order the one named exception. `name_cache.c` keeps the first string for an id across rewinds. §2.1 now names both exceptions. A forced-collision test is not added: the string is outside the contract, and a test would assert the excluded behaviour.
3. **Applied.** `b3DynamicTree_MarkProxyMovedSerial` (`dynamic_tree.c`) walks a proxy's ancestors to the root, so each re-marked proxy costs the tree height. §7.4's cost now reads (restored shape records + moved proxies) · log n, and §4's restore cost, which had omitted moved proxies, says the same.

2 applied, 1 declined. The status line's finding sequence and round count in the target doc were updated. AC-1 to AC-4 were applied before the round and no shape changed against them: the ownership boundary is a type-level rule, not a caller list or a check that changes control flow.

## Status

Finding count is 3 (1H-1M-1L), up from 2, with the first High since round 40. Finding 1 reopened the hot accessor ground of round 32's finding 2 and did not engage the doc's stated rule that awake-owner writes are unjournaled; §5.2 now states that rule at the accessor. Findings 2 and 3 are older wording and omissions, not consequences of round 41's fixes. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4, 6, 3, 5, 2, 4, 3, 4, 2, 3) are flat, not falling; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 42: 3 findings (1H-1M-1L), 2 applied, 1 declined`

`Series total: 184 findings (53H-103M-28L) across 42 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L) -> 6 (1H-4M-1L) -> 3 (0H-1M-2L) -> 5 (0H-3M-2L) -> 2 (0H-2M-0L) -> 4 (0H-2M-2L) -> 3 (0H-2M-1L) -> 4 (1H-3M-0L) -> 2 (0H-2M-0L) -> 3 (1H-1M-1L)`
