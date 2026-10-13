---
title: Review round 11 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round10.md
mode: full pass
---

# Round 11

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T19:42:41+10:00 to 19:50:06+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s tree and pair-pass plan is mostly consistent with the live source. Four migration and rewind defects need resolution before implementation; there is also one incorrect claim about the existing rebuild API.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2, B3 | A query holds the proposed lock while a callback calls `b3Body_SetLinearVelocity`. Its wake helper sets and clears `world->locked` itself (`src/body.c`). The proposed unlock drain can therefore migrate a leaf while `b3DynamicTree_Query` still traverses its node array; validation builds also hit the helper’s lock assertion. | Make locking aware of nested calls and drain only after the outer traversal ends. Update the wake helper’s assertion and lock behavior. |
| 2 | H | 2, B3 | `b3Body_SetType` holds the lock, wakes a sleeping set, then destroys and recreates its shape proxies before unlocking (`src/body.c`). A queued `(shape id, direction)` wake migration is stale when it drains and can target the replacement proxy in the wrong tree. | Drain or cancel migrations before proxy replacement, or reconcile each queued item against the shape’s final type, set, and proxy key. |
| 3 | M | 2, B3 | The document says a query callback can wake through `b3Body_SetAwake`, but the proposed traversal lock makes that call fail its `b3GetUnlockedWorld` check (`src/body.c`, `src/physics_world.c`). The promised callback wake would not occur. | Specify and implement a callback-safe wake path, or state the changed API behavior and remove the callback-wake claim. |
| 4 | H | 5.2–5.4 | Proxy-identity entries alone cannot restore non-awake moved bits. If a sleeping proxy is unmarked at T, wakes later, and its removal is undone with an ordinary insert, that insert marks it moved (`src/broad_phase.c`). The tree then differs from T and the next pair search can differ. The cited rewind design captures moved leaves separately (§5.4, §7.4). | Retain and extend that moved-leaf capture and restore rule to the non-awake tree in the tree-order branch; state when its bits are cleared and re-marked. |
| 5 | L | 2, B4 | `b3World_RebuildStaticTree` is declared in `src/physics_world.h`, not `include/box3d/box3d.h`; it is not currently a public API beside which to place the proposed public sleeping-tree rebuild. | Correct the API description and specify whether the static rebuild is also being exposed. |

## 3. Checked, no change

- The two proxy-key bits can encode a fourth tree without changing the key format (`src/broad_phase.h`).
- The proposed self and cross passes cover the pair classes the current dynamic-tree passes discover (`src/broad_phase.c`, `src/contact.c`).
- Moving a non-awake proxy through the existing remove and insert path updates ancestor bounds without a step refit (`src/dynamic_tree.c`).
- The awake-tree image fields and the 960 KB to 240 KB example agree with the tree’s 32-byte nodes, 4-byte parents, and 24-byte proxy slots (`src/dynamic_tree.c`).
- The `sleep` and `trees100` mean move counts are arithmetically consistent with section 4’s stated shape counts and steps. I found no demonstrated error in the scratch harness method.

## 4. Proposed edits

Specify outermost-unlock semantics in B3, including callback wake behavior and invalidation of queued migrations when a proxy is replaced. In §5.2–5.4, distinguish proxy identity from moved-bit restoration and carry the cited ring design’s moved-leaf rule over to the fourth tree. Correct B4’s description of the static rebuild API.

## 5. Unresolved

The reported churn samples were not independently reproducible from the document; no defect in their stated method was established.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3WakeBodyWithLock` sets and clears `world->locked` around the wake
   (`body.c`), so a drain on unlock would run inside the callback, mid-walk, and its assertion
   fails under a held lock. Together with findings 2 and 3 this shows `world->locked` is the wrong
   instrument. Fix: B3 queues while a traversal-depth counter is non-zero or the step is running;
   the counter is raised and lowered by one entry and exit pair around every tree walk that runs
   host callbacks; the drain is at the outermost exit and after the step's tree join. The world
   lock is untouched.
2. **Adopted (H).** `b3Body_SetType` wakes and then destroys and recreates the shape proxies
   (`body.c`), so a queued `(shape id, direction)` is stale. Fix: the queue holds body ids with
   generation; the drain places every proxy of a live body in the tree its set and type imply and
   does nothing for one already there, so a replaced proxy is simply found in place.
3. **Adopted (M).** `b3Body_SetAwake` and `b3DestroyBody` get the world through
   `b3GetUnlockedWorld` (`body.c`); a lock held during public walks would refuse them from a
   callback, which hosts do today. Fix: the traversal counter of finding 1 leaves the lock clear,
   so those calls keep working and their migrations queue.
4. **Adopted (H).** An ordinary non-awake create marks the leaf moved (B2), so undoing a wake's
   removal with it marks a proxy that was unmarked at T, and the next pair update searches it.
   Fix: 5.2's tree-order branch keeps the rewind design's moved-bit restore (its section 7.4:
   clear every tree's moved bits, then mark image T's moved list) for the non-awake tree, and 5.4
   says so.
5. **Adopted (L).** `b3World_RebuildStaticTree` is declared in `physics_world.h`, not in
   `box3d.h`. Fix: B4's sleeping-tree rebuild sits beside it there; exposing either is not part of
   this change.

## Owner decision received with this round (U-S5)

Decision 1: run the non-awake passes; do not change what `b3Body_SetTransform` does on a sleeping
body. Removed from §6; B1 states it; reason in Provenance.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 11: 5 findings (H3-M1-L1), 5 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> 3/1/1 -> pending
- Tripwire: fixes add or remove a named mechanism: the lock-and-unlock pair of revision 11 is
  replaced by a traversal entry and exit pair; the queue's key changes from shape to body.
  Clustering: B3 again (rounds 5, 6, 10, 11); findings 1 to 3 are consequences of round 10's fix
  choosing the world lock (U-T2). Reviewer read of round records or changelog: none in the raw
  log. Doc length: 362 -> 369 lines. Past the criteria review threshold (B8): the recurring shape
  is "one guard for an open-ended set of readers"; the fix each time has been to move the rule to
  the one choke point the readers share.
