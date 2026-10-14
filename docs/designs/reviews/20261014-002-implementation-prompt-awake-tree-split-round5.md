---
title: Review round 5 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round4.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 5
---

# Round 5

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T08:56:39+10:00 to 09:00:00+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a1377b-7228-7661-87d6-e0215e3b5c9e`.

## Review report (Codex final message)

### 1. Summary

The prompt largely tracks the canonical design and the live code, but its golden-hash rule can stop a change the design permits, and one measurement can miss the sleep events it is meant to time. I also found one false source-inventory claim.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§5, 9 | The policy stops on any changed ray value or `QUERY_SPAWN_HASH`. The design’s §1 permits tied closest-hit casts to change with tree layout. In `shared/determinism.c`, a changed tied ray normal changes the spawn position and can therefore change the state hash even when the engine is correct. | Define evidence for a tied closest hit and allow its documented query and downstream state-hash changes. Keep unexplained differences as stops. |
| 2 | M | §7 | A drop in awake-body count does not identify every sleep step. `StepSleep` in `shared/benchmarks.c` can wake a pyramid before the step while another sleeps during it, leaving the count unchanged. The requested “step after each sleep” profile comparison can omit the event. | Track per-body awake transitions or migration counts in the scratch harness, then select the following two steps from those events. |
| 3 | L | §2 | “Every destroy goes through `b3DestroyShapeProxy`” is false. `b3ResetProxy` in `src/shape.c` calls `b3BroadPhase_DestroyProxy` directly when replacing a proxy. | List that direct route in the inventory and remove the universal claim. |

### 3. Checked, no change

- Verified the four-code key capacity, tree allocation, snapshot loops, counters, and named tree readers against the cited source.
- Traced sleep and wake set writes, the tree-task join, the mid-step drain point, CCD candidate handling, and Explode’s collect-and-sort path.
- Checked the named B0–B4 fail-before mechanisms against the test runner and current call paths; found no further concrete failure in those procedures.
- Verified the benchmark scene list and the one-worker inline tree task used by the `pairs` profile measurement.

### 4. Proposed edits

Revise §§5 and 9 for the permitted tied-hit case; use transition-level sleep evidence in §7; correct the `b3ResetProxy` route in §2.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the design (U-V2).

1. **Adopted (H).** `QuerySpawnOnce` (`shared/determinism.c`) places the spawn at the closest ray
   hit's point offset along its normal, so a tied closest hit that resolves to another shape moves
   the spawn and changes `QUERY_SPAWN_HASH`, `QUERY_SPAWN_SLEEP_STEP` and `QUERY_SPAWN_HIT_COUNT`
   (`test/test_determinism.c` pins all four). Design §1 names tied closest-hit casts as a permitted
   difference. Reversal check (U-V4): the earlier policy made a differing ray value a stop for the
   owner; the fresh evidence is that §9 then treated the downstream pins as defects, so a case the
   design settles would need the owner to overrule a stop condition. Fix: §5 defines the tie
   evidence (every earlier query equal; at the first differing query, different closest shapes at
   the same fraction), permits the four pins to change on it, and keeps everything unexplained a
   stop; §9 says "a golden changes without a §5 cause".
2. **Adopted (M).** `StepSleep` (`shared/benchmarks.c`) wakes one sleeping pyramid per step, so a
   wake and a sleep on one step leave the awake count unchanged. Fix: a sleep step is one where a
   live body's `b3Body_IsAwake` goes from true to false.
3. **Adopted (L).** `b3ResetProxy` (`src/shape.c`) calls `b3BroadPhase_DestroyProxy` and
   `b3BroadPhase_CreateProxy` directly when replacing a proxy. Fix: the inventory names that route
   and says every other destroy goes through `b3DestroyShapeProxy`.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 5: 3 findings (H1-M1-L1), 3 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no (a policy and a harness rule). Clustering:
  finding 1 is in §5, which had a finding in rounds 1, 2 and now 5, each time on the same policy;
  finding 2 is a consequence of round 4's fix 4 (the awake-count marker it chose). Each fix is a
  clause or a rewritten paragraph of the same size, no narrative. Reviewer read of round records
  or changelog: none in the raw log. Doc length: 718 -> 723 lines. Above the split-check length
  (U-T6): still one decision; no split.
