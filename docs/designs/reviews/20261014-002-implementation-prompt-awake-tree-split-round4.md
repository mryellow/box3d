---
title: Review round 4 - implementation prompt, awake and non-awake dynamic trees
target: 20261014-002-implementation-prompt-awake-tree-split.md
date: 2026-10-14
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261014-002-implementation-prompt-awake-tree-split-round3.md
mode: full pass, same prompt and scope as round 1 (U-P4), on revision 4
---

# Round 4

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-14T08:50:04+10:00 to 08:54:43+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13775-6d86-7091-8140-4e022678fc6d`.

## Review report (Codex final message)

### 1. Summary

The four-commit plan largely follows the canonical design, and the main tree and solver mechanics match the live source. I found one test fixture with an impossible permitted input and several evidence procedures that cannot establish the claims assigned to them as written.

### 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §4 B4 | `SleepingTreeHeightTest` says to create “more proxies than the threshold.” At 65 proxies, a binary tree’s maximum height is 64, so its required `height > 64` assertion is impossible; changing geometry cannot fix that count. | Specify at least 66 proxies and a concrete count for the fixture. |
| 2 | M | §§6–7 | The node-visit counters omit both self-pass gathers: the awake scan to `nodeEnd` and the new marked descent. The proposed measurement therefore cannot show the gather cost or support the §6 marked-descent evidence. | Count gather visits separately for both self passes, or remove the node-visit measurement as evidence for gathering. |
| 3 | M | §7 | The scene-hash procedure does not define how to enumerate “every body.” The cited `HashWorldState` takes a fixed list of body IDs, while `rain` destroys and recreates bodies and `large_world` creates bodies during the run (`shared/benchmarks.c`). | Specify active-body enumeration and a stable hash order, including how freed slots are skipped. |
| 4 | M | §7 | `pairs` and `refit` timings cannot by themselves show that the saving starts on the second step after sleep. The rebuild runs in a separate tree task (`src/broad_phase.c`), outside those profile intervals, and the described harness does not record which steps contain sleep transitions. | Record sleep steps and direct gather/rebuild work or tree-task timing alongside the profiles. |
| 5 | L | §2 | The inventory says every listed world query runs with a host callback. `b3World_CastRayClosest` has no host callback parameter (`src/physics_world.c`). | Describe it as an internal callback query; retain its fourth-tree reader entry. |

### 3. Checked, no change

- The proxy-key encoding, three body types, current tree allocation, and snapshot tree loops match §2.
- The sleep and wake set writes, shape creation path, moved-bit behavior, and step join point support the proposed migration mechanics.
- The B1 direct-placement fixture can reach its stated pre-B1 `b3ValidateNoMoved` failure.
- The explosion candidate sort and the one-pass CCD candidate ordering match the prompt’s reading of the base code.
- The golden-hash exception corresponds to fields actually folded into `QUERY_SPAWN_QUERY_HASH`.

### 4. Proposed edits

Make the B4 proxy count explicit, add gather counters to §7, define active-body hash traversal, and record sleep transitions plus rebuild work for the second-step claim. Correct the callback wording in §2.

### 5. Unresolved

None.

### 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against live source (U-V2).

1. **Adopted (M).** `b3MakeInternalNode` (`src/dynamic_tree.c`) sets height to one plus the taller
   child and a leaf's height is zero, so a path of n leaves has height n - 1 and 65 leaves cannot
   exceed 64. Fix: the fixture creates the threshold plus 16 proxies, with the bound stated.
2. **Adopted (M).** The counters named `b3GatherCrossSeeds`, `b3VisitPair` and
   `b3CollideProxyAndSubtree`, none of which is a self-pass gather. Fix: per step, the pair indices
   `b3GatherMovedSiblings` scans and the nodes the non-awake marked descent visits are counted too.
3. **Adopted (M).** `StepRain` (`shared/benchmarks.c`) destroys and recreates body groups, so a
   fixed body list does not cover "every body". Fix: the harness walks `world->bodies` by index,
   skipping slots whose `setIndex` is `B3_NULL_INDEX`.
4. **Adopted (M).** `profile.pairs` times `b3UpdateBroadPhasePairs` (`src/physics_world.c`); the
   tree task it enqueues runs inline only under `b3DefaultAddTaskFcn`, the one-worker serial
   fallback, and otherwise concurrently outside that window. The harness recorded no sleep steps.
   Fix: the second-step comparison is taken at one worker, and the per-step awake body count marks
   the sleep steps.
5. **Adopted (L).** `b3World_CastRayClosest` (`include/box3d/box3d.h`) takes a filter and no
   callback. Fix: the §2 wording names its internal callback.

Checked, no change items agree with the source read for the revision.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 4: 5 findings (H0-M4-L1), 5 applied, 0 declined.
- Findings: 2/5/1 -> 0/5/2 -> 0/3/1 -> 0/4/1 -> pending
- Tripwire: fixes add or remove a named mechanism: no (scratch-only counters and a harness
  enumeration rule). Clustering: four of five findings are in §7's measurements, which had two
  findings in round 1 and none in rounds 2 and 3; §4's tests, which produced findings three rounds
  running, produced one. Finding 2 extends round 1's fix 7 (the counter it widened still omitted
  the self gathers). No finding is in a section whose fix added paragraphs; each fix is a clause.
  Reviewer read of round records or changelog: a `git status` listing names the files; no read of
  their content in the raw log. Doc length: 710 -> 718 lines; the fixes add one line (5), two (1),
  two (4) plus one (harness), three (2), two (3), less one from rewrapping. Above the split-check
  length (U-T6): still one decision; no split.
