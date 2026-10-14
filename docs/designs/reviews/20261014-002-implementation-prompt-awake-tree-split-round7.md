---
title: Review round 7 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round6.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 7
---

# Round 7

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T09:10:37+10:00 to 09:15:46+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13788-3bc9-78f0-a9ea-294882930eb7`.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the live tree, but one B2 test cannot pass with its stated fixture, and the golden-hash policy can accept an unexplained state change. Its fail-before instructions also misclassify a regression test.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §4 B2; §6 | `SleepingWriterTest` calls `b3Shape_SetFilter` with the current filter. `src/shape.c` returns before `b3ResetProxy`. Even with a changed filter, the fixture’s sleeping–static contact is non-touching in the disabled set, and `src/contact.c` wakes bodies only when a destroyed contact was touching. The asserted wake and code-2 key therefore cannot pass after B2. | Use a changed filter to test reset while the body remains asleep. Use a previously touching contact that has gone to sleep for the separate wake-on-reset assertion; update the §6 evidence. |
| 2 | M | §5 | The tie rule permits four QuerySpawn pins to change when closest rays select different shapes at the same fraction. `shared/determinism.c` uses the fraction and normal, not the hit shape ID; if those values match, the ray gives the same spawn input. This rule could accept an unrelated state-hash defect. | Permit downstream pin changes only when the tied result changes an observable input, and trace the first resulting divergence. |
| 3 | L | §4 B2; §3 | The B2 test introduction says every new test fails before the commit at its named check, but `SleepingExplodeTest` explicitly has no failing assertion and serves as a base-comparison regression test. | Exclude it from the fail-before requirement and specify its base and branch hash comparison as the evidence. |

### 3. Checked, no change

- The code-3 key format, fourth-tree capacity, guards, counters, and snapshot loops match the stated B0 mechanics.
- The cited tree readers, pair tasks, sensor pass, CCD query, and Explode path match the §2 inventory.
- The sleep and wake writers, tree-task join, and callback lock behavior support the B2/B3 migration timing.
- The dynamic-tree rebuild and moved-bit behavior support the B4 height-trigger mechanism.

### 4. Proposed edits

Revise `SleepingWriterTest` and its §6 mapping, tighten the §5 tie criterion, and label `SleepingExplodeTest` consistently as a regression comparison throughout the fail-before procedure.

### 5. Unresolved

Canonical §1 delegates branch setup to this prompt, while canonical §2 specifies a PR forked from `main`. The prompt chooses `master` and no PR; the design does not establish which branch instruction takes precedence.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (H).** `b3Shape_SetFilter` (`src/shape.c`) returns before `b3ResetProxy` when the
   filter is unchanged, and destroys the proxy only when the category bits are unchanged;
   `b3DestroyContact` (`src/contact.c`) wakes only when the contact was touching; a box created
   asleep on the ground has a non-touching contact in the disabled set (`src/solver_set.c`). Fix:
   the first box falls asleep after touching, the setter changes the mask bits only, and the reset
   of a sleeping body with no touching contact is asserted to keep code 3.
2. **Adopted (M).** `QuerySpawnOnce` (`shared/determinism.c`) reads the closest hit's fraction and
   normal, never its shape. Fix: a tie permits the four pins only when the normals differ.
3. **Adopted (L).** The B2 tests intro excepted nothing. Fix: it excepts `SleepingExplodeTest`.

Unresolved item (branch): the design's §1 hands the branch to this document and its §2 says
"forked from `main`"; this document's §10 records the decision and its reasons (U-P13). The
reviewer's point that the design does not say which of its own sentences wins is a question about
the design's text, for the owner (U-P5): a one-word amendment to the design's §2 would close it.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 7: 3 findings (H1-M1-L1), 3 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> 1/1/1 -> paused (U-S4)
- Tripwire: fixes add or remove a named mechanism: no. Clustering: §5 produced a finding for the
  third round running (5, 6, 7), each a consequence of the previous round's fix to the same tie
  rule, after a rewrite in round 6 that did not grow it; §4's tests produced a finding in every
  round, this one a consequence of round 6's new test. Each fix this round is a clause or a
  rewritten fixture of the same size. Reviewer read of round records or changelog: none in the raw
  log. Doc length: 739 -> 744 lines.
- **Paused (U-S4).** Findings in §5 three rounds running after a cut, and in §4's tests seven
  rounds running. Reported to the owner with: the §5 tie rule's history (owner-judged stop, then a
  defined tie, then order-only and tie-only pins, then normals), the Unresolved branch sentence in
  the design's §2, and the recommendation to resume at the same scope once the owner has read §5
  and §10.
