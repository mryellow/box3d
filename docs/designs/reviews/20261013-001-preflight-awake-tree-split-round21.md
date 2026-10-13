---
title: Review round 21 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round20.md
mode: full pass
---

# Round 21

## Exact request sent to Codex

Identical to round 20 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T21:11:31+10:00 to 21:16:19+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s pair passes, moved-bit handling, and step drain order are consistent with the cited source. Two parts of section 2 need to be made explicit before implementation: one public tree reader is missing from the reader list, and the proposed counter does not expose the sleeping tree’s height as section 4 requires.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | 2, B1–B3 | `b3World_CollideMover` in `src/physics_world.c` walks the trees by fixed index and invokes a host callback. It is absent from B1’s overlap-and-cast reader list. B3’s general traversal rule would cover its callback, but the implementation list could leave its fourth-tree query and traversal guard out. | Name `b3World_CollideMover` explicitly in B1 and B3. |
| 2 | M | 2, B0; 4 | B4 says `b3World_GetCounters` reports the non-awake tree’s root height. The existing `treeHeight` in `include/box3d/types.h` is the maximum of dynamic and kinematic heights (`src/physics_world.c`); including a fourth tree in that maximum cannot report its height separately. | Specify a separate sleeping-tree height counter and add `include/box3d/types.h` to B0’s files, or specify a different way to report that measurement. |

## 3. Checked, no change

- The proposed self and cross passes cover the existing dynamic pair combinations, including moved sleeping proxies and static compounds (`src/broad_phase.c`, `src/contact.c`).
- Carrying each leaf’s moved bit preserves pending pair discovery across migration; the awake-tree rebuild can compact its node array without marking an unmoved surviving leaf (`src/dynamic_tree.c`).
- The proposed drain after the tree-task join precedes finalize and bullet CCD; the sleep pass follows CCD (`src/solver.c`, `src/physics_world.c`).
- Proxy-key writers, sensor queries, explosion traversal, and the cited tree-order changes support the stated behavior (`src/shape.c`, `src/body.c`, `src/joint.c`, `src/sensor.c`, `src/physics_world.c`; cited design §§2 A1–A3).
- The benchmark scene logic supports the stated sleep/wake churn mechanism. I found no source-based error in what the described endpoint scan counts (`shared/benchmarks.c`, `src/solver_set.c`, `src/solver.c`).

## 4. Proposed edits

Add `b3World_CollideMover` to B1’s fourth-tree readers and B3’s guarded public traversals. In B0, define how B4 obtains the non-awake tree’s height, including any `b3Counters` field and header change.

## 5. Unresolved

None.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (M).** `b3World_CollideMover` and `b3World_CastMover` (`physics_world.c`) loop over
   the trees by index and run a host callback from the walk. B1's "overlap and cast queries" and
   B3's "public queries" cover them in words but not by name. Fix: both named in B1 and B3.
2. **Adopted (M).** `b3World_GetCounters` sets `treeHeight` to the maximum of the dynamic and
   kinematic heights and `staticTreeHeight` separately (`physics_world.c`). Fix: B0 adds a
   `sleepingTreeHeight` field to `b3Counters` (`types.h` joins B0's files), and B4 refers to it.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 21: 2 findings (H0-M2-L0), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> 0/3/1 -> 0/2/1 -> 1/1/0 -> 1/0/1 -> 1/1/1 -> 1/1/0 -> 0/0/1 -> 0/1/0 -> 0/2/0 -> pending
- Tripwire: fixes add or remove a named mechanism: one counter field. Clustering: finding 2 follows
  from round 20's fix (U-T2); finding 1 is a reader-list gap of the kind rounds 1, 2 and 6 found,
  now the eighth reader named. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 301 -> 308 lines.
