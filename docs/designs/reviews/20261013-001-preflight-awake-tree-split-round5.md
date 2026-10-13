---
title: Review round 5 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round4.md
mode: full pass
---

# Round 5

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T18:37:17+10:00 to 18:42:50+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split covers the main pair passes and the step’s rebuild race, but two paths still break its guarantees. A public query callback can migrate a proxy during tree traversal, and the proposed rewind treatment cannot restore the non-awake proxy pool after it grows.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2.3, B3 | Public overlap and cast callbacks run while `dynamic_tree.c` traverses a tree using a cached `nodes` pointer. They can call `b3Body_SetAwake` while the world is unlocked (`physics_world.c`, `body.c`). After B2, that wake removes sleeping leaves and inserts awake leaves, potentially invalidating the traversal or its cached pointer. B3 handles this pattern for explosions but misses public queries. | Defer proxy migrations until each public traversal returns, or collect query hits before invoking user callbacks. |
| 2 | H | 5.2, 5.4 | With tree-order changes landed, section 5.2 relies on the proxy-identity create/destroy journal to restore the non-awake tree. If its proxy pool is full at T, a post-T create grows it; undoing that create pushes only its ID onto the free list (`dynamic_tree.c`). The capacity and remaining new slots are not restored. After replay allocates that ID, the next allocation encounters an empty free list while `proxyCount < proxyCapacity`, violating the allocator assertion and changing future IDs. This also contradicts 5.4’s requirement that free lists return exactly. | Journal non-awake proxy-pool growth and undo its capacity and free-list changes, or restore that pool’s capacity and free list from an image. |

## 3. Checked, no change

- The four-code key fits the existing two-bit type field; the body-type range remains three (`broad_phase.h`, `broad_phase.c`).
- The proposed self and cross passes cover moved awake and sleeping proxies against the same relevant neighbour classes (`broad_phase.c`, `contact.c`).
- Sleep and wake currently touch no proxies; carrying a leaf’s moved bit preserves pending pair work (`solver_set.c`, `dynamic_tree.c`, `solver.c`).
- The stated rebuild, CCD and explosion call paths, fixed-index readers, and the awake-tree image fields match the cited source.
- Section 4’s totals and per-tick means are consistent with its stated endpoint-scan method and `shared/benchmarks.c`; I found no case in that method that would hide a within-step sleep/wake round trip.

## 4. Proposed edits

Add public query callback traversal to B3’s migration ordering rules. Add non-awake proxy-pool growth and its undo to the tree-order branch of 5.2, and align 5.4’s exact free-list claim with that mechanism.

## 5. Unresolved

None.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** The public overlap and cast entry points call `b3GetUnlockedWorldFromId` and
   never set `world->locked` for the traversal (`physics_world.c`); `b3DynamicTree_Query` caches
   `tree->nodes` for the whole walk (`dynamic_tree.c`); `b3Body_SetAwake( true )` reaches
   `b3WakeBody` -> `b3WakeSolverSet` (`body.c`). Under B2 a wake from a query callback migrates
   leaves in the tree being walked and can reallocate its node array. Fix: B3 states that the
   public queries hold the lock for the traversal, which also rejects a `b3Body_SetTransform` from
   a callback (today's unguarded remove-and-insert during a walk); deferring migrations is the
   alternative and is decision 5 for the owner.
2. **Declined (H).** Every proxy slot is either allocated or on the free list: `b3AllocateProxy`
   pops, `b3FreeProxy` pushes, and growth runs only when the list is empty and chains the new
   slots `oldCapacity..newCapacity-1` in ascending order (`dynamic_tree.c`). Undoing entries in
   reverse keeps the invariant, so the "empty free list while `proxyCount < proxyCapacity`" state
   cannot arise. After a rewind the list is T's list followed by the slots a discarded growth
   added; the original timeline, on exhausting T's list, grew and handed out those same slots in
   the same order. This is the rewind design's section 7.4 argument, which the doc cites. 5.4's
   "free lists must come back exactly" is reworded to the property that matters, the id sequence,
   and the reasoning goes in Provenance (U-C4, U-V10).

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 5: 2 findings (H2-M0-L0), 1 applied, 1 declined (finding 2, reasoning in Provenance).
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> pending
- Tripwire: fixes add or remove a named mechanism: no; B3 gains a lock rule and §6 a decision.
  Clustering: B3 drew findings in rounds 1, 3 and 5 (query coverage, then ordering), each a
  different reader of the trees. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 310 -> 311 lines.
