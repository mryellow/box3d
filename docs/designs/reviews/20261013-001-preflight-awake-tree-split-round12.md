---
title: Review round 12 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round11.md
mode: full pass (against revision 11; see Status)
---

# Round 12

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T19:51:59+10:00 to 19:56:49+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s pair passes, tree readers, and stated memory arithmetic agree with the live source. The proposed migration queue has three concrete failures: it can drain during a tree walk, it can retain a migration for a proxy that has since been replaced, and the new walk lock prevents one wake path the document promises.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2, B3 | A public query holds `world->locked`, but a callback can call `b3Body_SetLinearVelocity` or `b3Body_ApplyForce`. `b3WakeBodyWithLock` sets the lock to true and then false (`src/body.c`). Under the proposed drain-on-unlock rule, that inner unlock drains migrations while the outer `b3DynamicTree_Query` still holds the node array (`src/dynamic_tree.c`). | Make traversal protection nest-safe. Drain only after the outermost walk has ended. |
| 2 | H | 2, B3 | The queued item contains only shape id and direction. `b3Body_SetType` wakes a sleeping body, then destroys and recreates its shape proxies before releasing its lock (`src/body.c`). The queued wake would act on a replaced proxy, potentially migrating the new static or kinematic proxy into the awake dynamic tree. | Migrate immediately when no tree task or walk is active, or cancel and reconcile queued items when their source proxy is destroyed or replaced. |
| 3 | M | 2, B3 | The document says an overlap callback can wake through `b3Body_SetAwake` while public walks hold the lock. That API calls `b3GetUnlockedWorld`, which returns `NULL` for a locked world (`src/body.c`, `src/physics_world.c`); the wake cannot occur. | Specify and implement a supported deferred wake path for callbacks, or remove this claim and adjust the callback test. |

## 3. Checked, no change

- The two-bit proxy key can encode a fourth tree; the identified fixed-index readers, sensor queries, CCD queries, and broad-phase walks need the stated updates (`src/broad_phase.h`, `src/physics_world.c`, `src/sensor.c`, `src/solver.c`).
- The proposed self and cross passes cover moved sleeping proxies against awake, sleeping, static, and kinematic neighbours. Contact creation already places a pair with neither body awake in the disabled set (`src/broad_phase.c`, `src/contact.c`).
- Rebuild compacts a nonempty unordered awake tree; an empty tree returns before rebuilding. The roughly 960 KB and 240 KB image examples follow from 32-byte nodes, 4-byte parents, and 24-byte proxy slots (`src/dynamic_tree.c`).
- The rewind treatment correctly distinguishes awake-tree imaging from non-awake proxy identity and, without the tree-order changes, physical layout restoration (rewind design §§5.4, 7.4; tree-order preflight §1).

## 4. Proposed edits

Revise B3 to define traversal depth and the exact drain point. Add a rule for queued migrations whose source proxy is destroyed or replaced inside the same locked operation, using `b3Body_SetType` as the concrete example. Correct the `b3Body_SetAwake` callback claim and its test.

## 5. Unresolved

None within this review’s scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

The script that was to write revision 12 failed on its first replacement and wrote nothing, while
the changelog entry for revision 12 was appended and the round was sent. The reviewer therefore
saw revision 11. Its three findings are round 11's findings 1 to 3, re-raised because the target
had not changed, and each is resolved by the revision 12 edits recorded in round 11's
verification, now applied after this record. No new finding. The changelog's revision 12 entry
stands as the description of those edits.

## Status

- Verdict: CHANGES_PROPOSED against revision 11. Not converged (U-S8).
- Round 12: 3 findings (H2-M1-L0), 3 applied (as round 11's 1 to 3), 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> 2/1/0 -> pending
- Tripwire: no fix applied between rounds 11 and 12 (author error, U-R9 would have caught it: the
  diff was not read before sending). Reviewer read of round records or changelog: none in the raw
  log. Doc length: 369 lines, unchanged.
