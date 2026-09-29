# Bit-exact history ring vs rewind-only history ring: differences

Compares `20260927-002-bit-exact-history-ring.md` (call it **A**, two-way) with
`20260929-002-rewind-only-history-ring.md` (call it **B**, rewind-only).

Basis: A was read in full through its §10 and §11; B was read through §11.2 and its §11.3 onward
(§12 tests, §14 API, §15) was not read. Nothing here has been checked by execution.

## 1. In one line

Both designs keep the same architecture. B removes forward movement through history, so the journal
becomes an undo log. Most other differences follow from that one change.

## 2. Unchanged architecture

| Area | Both designs |
|---|---|
| State classes | Hot (imaged), Cold (journaled), Scratch (nothing), Derived (trees) |
| Hot image | O(awake) flat copy and gathers into a ring slot |
| Cold journal | O(events), through write accessors and journaled containers |
| Module boundary | Const-qualified records, incomplete container types, compile-time enforcement |
| Trees | Rebuilt at restore from fat AABBs; proxy identity journaled |
| Order-dependence fixes | CCD solid min, CCD sensor hits, `b3World_Explode` |
| Resimulation | Ordinary `b3World_Step` |
| Capture interval | `captureInterval = K`, doubled under a byte budget |
| Named exceptions | Callback order after restore; name-string hash collisions |

## 3. Removed in B

| # | Feature | A | B |
|---|---|---|---|
| 1 | Forward scrub | Requirement 4 "Scrubbable": restore any imaged tick, backward or forward | Requirement 4 "Rewindable": backward only. Forward scrub is a non-goal |
| 2 | Peek-and-return | Supported (rewind, inspect, scrub back) | Non-goal. The caller records its own past poses |
| 3 | Redo half of journal entries | Entries store old and new values | Entries store what undo needs only |
| 4 | Two-way block ownership | Blocks move entry <-> world in both directions (undo and redo) | One way: undo hands back, or eviction frees |
| 5 | Redo-side sleep entries | Final bytes of shapes, manifolds and mesh caches journaled when leaving the awake set | None. §7.1 argues four cases need no entry |
| 6 | Ring truncation in write paths | First write after a rewind, through any accessor or container, truncates the ring | `Rewind` removes the later slots in the same call. Accessors only append entries |
| 7 | Forward journal walk | `Rewind` has a "T > P: apply forward" branch | Walk is backward only |
| 8 | Timeline-staleness handling | Slots after T kept, so writes must invalidate them | Nothing to invalidate. The current tick is always the newest slot |

## 4. Journal entries: payload comparison

| Entry | A payload | B payload |
|---|---|---|
| Record write | tag, id, old bytes, new bytes | tag, id, old bytes |
| Pool alloc | id, pop or bump, prior `nextIndex`, blocks handed to the entry | id, pop or bump, prior `nextIndex`. Owns nothing; undo frees the live blocks |
| Pool free | id, block handles | id, block handles and counts |
| Pair set | key | key and which of add/remove |
| Bitset | colour, body id, old value | same, undo only |
| Island create | id, array handles | id only. Undo destroys the live arrays |
| Island destroy | id, array handles | same |
| Set create | set index, ownership handle | set index. Undo destroys the arrays |
| Set destroy | set index, handle | set index, handle and size for each of five arrays |
| Array push | old length, plus the pushed element's bytes | old length. For `world->sensors`, undo frees the pushed element's arrays |
| Array removeswap | old length, index, removed bytes | same, undo only |
| Array clear / set | clear: old length and bytes; set: index, old and new bytes | clear: old length and bytes; set: index and old bytes |
| Material block | old and new bytes | old bytes and `materialCount` |
| Manifold / mesh cache | old and new bytes | old bytes. Also adds a cache entry from `b3CreateContact` for mesh contacts |
| Proxy create | tree, id, fat AABB, category bits, shape id | tree, id |
| Proxy destroy | full payload | full payload |
| Hull refcount | both directions | undo only; frees the hull if the count reaches zero |

## 5. Ownership rules

| Question | A | B |
|---|---|---|
| Which entries own blocks? | Any entry, at various times | Only entries that destroy something |
| How does a block leave an entry? | Undo, redo, or eviction | Undo or eviction |
| Does a create entry own anything? | Yes (redo needs it) | No |
| Cost of discarding a segment after undo | Frees blocks it holds | Nothing: entries own no block, so no walk |
| Slot charge | Follows blocks between entry and world | Fixed at capture, never changes while retained |

## 6. Ring and Rewind behaviour

| Aspect | A | B |
|---|---|---|
| Position in ring | Can sit anywhere | Always at the head |
| After `Rewind(T)` | Slots after T remain | Slots after T removed |
| Retained ticks on other timelines | Possible until the first write | Never |
| Eviction (old end) | Oldest slot | Oldest slot plus non-imaged slots after it up to the next image |
| Budget widening | Interval doubles | Doubles, capped at `tickCount`. The tick that triggers it skips its image |
| Pending staging bytes | Not reported separately | `pendingBytes` in `b3HistoryInfo` |
| `Rewind(P)` | Not described | Undoes API calls since the last step and restores image P |
| World locked during rewind | Not stated | Locked, so `destroyDebugShape` cannot re-enter |
| Task-context bitsets on rewind | Cleared | Left alone (avoids an O(world) term) |
| `Disable/DestroyWorld` cleanup | Not stated in the parts I read | Frees slots, entry blocks, staging blocks, arena, before teardown |

## 7. Module-boundary tightening in B

| Item | A | B |
|---|---|---|
| World trees outside `broad_phase.c` | Not restricted | Pointer-to-const. Enlarge, refit and rebuild move into that file |
| `world->sensors` container | push, removeswap, clear, set | push and removeswap only |
| Per-record arrays in sets/islands | Not spelled out | Const-data array type, addressed by (array tag, owner id) |
| Migration worklist | Shorter | Adds `headShapeId`/`shapeCount`, `headJointKey`/`jointCount`, island and graph writes to contacts and joints. States the compiler, not the table, says when it is done |
| Destroy paths free blocks | Not specified | Destroy paths leave blocks on the record; the pool free moves them into its entry |
| `world` container pointers | Not stated | Plain pointers, not const, because `b3World` is a zeroed global array element |

## 8. Requirements and non-goals

| Item | A | B |
|---|---|---|
| Req 3 wording | any restorable tick in the window | also "at or before the current tick" |
| Req 4 | Scrubbable | Rewindable, one-way |
| Non-goals | Predicted-subset capture/resim | Adds forward scrub and peek-and-return |
| Host configuration | Callbacks, `userData` not restored | Stated in requirement 1 too, and the caller must reset what its callbacks read |
| Geometry freed after rewind | Only after the window passes | Also allowed once `Rewind(T)` returns, if only shapes created after T reference it |
| Query order (§10) | Overlap/ray callbacks | Adds `b3World_GetBounds` and equal-fraction hit choice |
| `world->names` | Not read by the step | Read only for log text (CCD stall diagnostics) |

## 9. Cost and effort

| Item | Effect of dropping forward scrub |
|---|---|
| Image size and capture cost | Unchanged. The same image either way |
| Journal size | Smaller (no new-value bytes), still about an order of magnitude below the image |
| Ring memory | Barely changes; the image dominates in scenes with meaningful awake state |
| Resim cost | Unchanged: N whole-world steps |
| Build and proof burden | Smaller: no redo paths, no two-way ownership, no truncate-on-write |
| Incremental replay (future) | B notes `Rewind` would have to hand removed slots' images to the replay instead of dropping them |
| Time-sliced replay | B notes the ring cannot serve old-timeline poses, so the caller renders from its own record |

## 10. Review status

| | A | B |
|---|---|---|
| Rounds | 43 | 21 |
| Final round | 0 findings | 0 findings |
| Convergence carry-over | n/a | B states A's convergence does **not** carry over: removing forward scrub changes payloads, ownership, the discard rule and the sleep entries |
| Both | "Clean round is not an endorsement of the approach"; code references against 5643cd8 | same |

## 11. Not compared

B's §11.3 onward (§12 verification, §14 API, §15 "what forward scrub would cost") was not read.
The A-side tests in §12 include forward-scrub cases; whether B's tests drop them was not checked.
