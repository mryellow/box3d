---
title: Review round 6 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round5.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 6
---

# Round 6

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T09:02:01+10:00 to 09:07:31+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13780-5bec-7521-8060-d005227649da`.

## Review report (Codex final message)

### 1. Summary

The prompt tracks the canonical design’s tree and migration mechanics closely, but its branch instruction conflicts with the design’s delivery requirement. Its golden-hash rules and two evidence claims also need correction before an implementing agent can rely on them.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §1 | The canonical design §2 requires one PR forked from `main`. This prompt makes commits on `master` the deliverable and moves any PR to a later cherry-pick outside its scope. Those instructions cannot both be satisfied. | Align the branch and PR instruction with canonical §2; report any prerequisite conflict instead of redefining the deliverable. |
| 2 | H | §5 | An overlap callback reorder alone changes `QUERY_SPAWN_QUERY_HASH`, but the policy permits updating `QUERY_SPAWN_HASH`, `QUERY_SPAWN_SLEEP_STEP`, and `QUERY_SPAWN_HIT_COUNT` too. In `shared/determinism.c`, spawning uses the overlap **count**, not callback order. This would accept an unrelated state defect. | Permit only the query hash for an order-only difference. Require a demonstrated tied-ray change and its downstream effects before changing state or hit-count pins. |
| 3 | M | §5 | The callback-order proof requires a sleeping body in the differing overlap. The canonical design §1 permits layout-dependent order generally; migrations can reorder a query returning only awake bodies. The rule could reject a correct query-hash change. | Accept a same-set callback permutation without requiring a sleeping hit. |
| 4 | M | §4, `WakeUnderRebuildTest` | The fixture says each `b3Body_SetTransform` marks the third box’s proxy. `src/body.c` moves the proxy only when the new AABB leaves its current fat AABB. A short move can fail the fixture’s root-mark check before the promised code-3 fail-before check. | Specify successive transforms that leave the fat AABB, and check the mark before using the fixture as fail-before evidence. |
| 5 | M | §6 | The “every other writer of a sleeping proxy” entry cites transform tests and the general suite. Those do not demonstrate shape creation or reset on a sleeping body; `test/test_body.c` and `test/test_shape.c` do not exercise a sleep transition. The evidence is narrower than the row claims. | Add focused evidence for those writers, or state that their coverage is by source inspection and name the paths inspected. |

### 3. Checked, no change

- Verified the two-bit proxy key, three existing guards, tree capacities, and snapshot tree loops against the live source.
- Verified the pair-pass inputs, contact set selection, sleep and wake set writes, and the need to combine both dynamic CCD queries into one sorted pass.
- Verified the B0, B1, and B2 named fail-before mechanisms and the test runner’s entry-name filter.

### 4. Proposed edits

Apply the actions in findings 1–5, then make §3’s fail-before procedure name the revised `WakeUnderRebuildTest` movement fixture and §6’s revised writer evidence.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source and the design (U-V2).

1. **Declined (H).** The design's §1 says the first-branch choice is "the Implementation Prompt,
   which also sets up the branch", so the branch is this document's decision; the design's §3
   tests run under the A0 helper, which exists only on `master` (`test/test_tree_order.c`), so a
   fork from `main` could not run them; and the owner decided the four commits are the deliverable
   with an upstream PR as a later cherry-pick (U-S5). The reasoning was in §1 but not in Provenance,
   which is why it was re-raised (U-T5); it is now a §10 entry (U-C4, U-V10).
2. **Adopted (H).** `QuerySpawnOnce` (`shared/determinism.c`) uses the overlap count, not the
   callback order, so an order-only difference changes `QUERY_SPAWN_QUERY_HASH` alone. Fix: §5
   permits that pin alone for callback order and the four pins for a tie.
3. **Adopted (M).** The awake tree's layout differs from the single tree's (bodies that slept and
   woke are reinserted), so an all-awake overlap can reorder. Fix: the order proof needs a same-set
   permutation, not a sleeping body; §2's fact names both causes.
4. **Adopted (M).** `b3Body_SetTransform` (`src/body.c`) calls `b3BroadPhase_MoveProxy` only when
   the new AABB leaves the fat AABB. Fix: the third box moves by more than its extent plus margin.
5. **Adopted (M).** `test/test_body.c` and `test/test_shape.c` never put a body to sleep, and
   `b3CreateShapeInternal` (`src/shape.c`) wakes nothing while `b3Shape_SetFilter` destroys
   contacts with a wake before `b3ResetProxy` recreates the proxy. Fix: a B2 test,
   `SleepingWriterTest`, for creation (fails before: creation selects by body type) and
   `b3ResetProxy`; `b3Body_Disable` and `b3Body_SetType` are cited as inspected paths.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 6: 5 findings (H2-M3-L0), 4 applied, 1 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> 1/1/1 -> 2/3/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no (one new test). Clustering: §5 produced
  findings in rounds 1, 2, 5 and 6; findings 2 and 3 are consequences of round 5's fix 1 (its
  permitted-pins sentence was too wide and its order proof too narrow). §5 has had four fixes on
  one policy; the paragraph is rewritten, not grown (U-S4 watch: next §5 finding pauses for
  report). Reviewer read of round records or changelog: a `git log` line names an unrelated
  commit; no read of round records or the changelog. Doc length: 723 -> 739 lines, of which the
  new test adds ten and the Provenance entry five. Above the split-check length (U-T6): still one
  decision; no split.
