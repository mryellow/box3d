---

title: History ring budget, window and eviction for box3d
status: Draft
  Split from docs/designs/20260929-002-rewind-only-history-ring.md, which decides the per-tick image, the undo journal, the restore and the
  public API, and cites this document for the ring that holds them. This document decides the ring
  alone: its slots, its two ends, which ticks are imaged and restorable, the byte budget and how it
  degrades, the retained window, what a slot is charged, and enable and disable. The document is
  self-contained except where it cites that one.
  Code references are against source as of 55a40d1.
date: 2026-10-15

---

# History ring budget, window and eviction for box3d

## 1. What this decides

`docs/designs/20260929-002-rewind-only-history-ring.md` produces, at the end of every step, a journal segment for the tick and, on an imaged
tick, a hot image (its §6), and restores a tick by undoing segments back to an image (its §9).
This document decides where those bytes live, which ticks are kept, which are evicted, and what
happens when they do not fit. Its public surface is the history definition, info and restorable-tick
query of that document's §14; the semantics of every field there are decided here.

## 2. Requirements

1. **Restore never rejected** for any restorable tick: an imaged tick inside the retained window
   and at or before the current tick, every tick when the capture interval is 1. The ring never
   evicts a tick a caller can still name as restorable except by the window and budget rules below,
   and never leaves a retained segment without an image to walk back to.
2. **Budgeted memory with graceful degradation.** A byte budget widens the capture interval
   instead of failing. The budget is a target under normal churn, not a hard ceiling, and any
   overrun is limited to the retained minimum window's own slots: a single tick whose own image
   and journal segment exceed the budget is still admitted rather than rejected, since rejecting
   it would violate requirement 1. `maxBytes` bounds retained history; arena and staging capacity
   are reported separately (§6). Retained memory is the image charge of every imaged tick in the
   window plus the journal, header and directory charge of every retained tick and the blocks
   their entries own (§6), not the per-tick figure alone; at interval 1 it is the per-tick slot
   times the window: on the all-awake stress scenes (5 to 51 MB per
   image, `docs/designs/20260929-002-rewind-only-history-ring.md` §5.1) a 60-tick window at interval 1 holds 0.3 to 3 GB, so on those scenes
   `maxBytes` is what keeps the ring in the hundreds of megabytes, at the price of a wider interval
   and fewer restorable ticks.
3. **Nothing to invalidate.** Every retained tick is on the current timeline, by construction
   (§4); no slot carries a timeline mark and no write path notices a rewind.

## 3. Slots and the two ends

One circular byte arena per world. Slots are variable-length regions written in tick order: a
header, the image, the journal segment. A slot directory maps tick → offset. The ring changes at
its two ends only:

- **The new end.** Capture appends the slot for the next tick. `Rewind(T)` removes every slot
  after T and moves the write position back to the end of slot T.
- **The old end.** When the writer wraps into the oldest slot, that slot is evicted (the blocks
  its entries own are freed), together with the non-imaged slots after it up to the next imaged
  slot, which no rewind can reach, and the retained window shrinks by those ticks. The writer never
  wraps into a slot of the minimum window (§6); the arena grows instead, so a young ring's sole image and the segments after
  it are retained until a second image is captured. Segments are only
  walked from the current tick back to an imaged tick T, so the segments of ticks at or before
  the oldest retained imaged tick are never read; `oldestImageTick` is that imaged tick, and
  every segment after it is retained.

`world->historyTick` names the last completed tick, which is always the newest slot in the ring.
Capture increments it, then writes slot `historyTick`. Rewind sets it to T.

## 4. Rewind discards, by construction

The slots after T describe a timeline the caller is
replacing, and nothing in this design can return to them, so `Rewind` removes them as part of the
same call, after undoing their segments. Removing them is moving the write position: their images
are flat bytes, and their entries own no block once undone (`docs/designs/20260929-002-rewind-only-history-ring.md` §7.1). Every retained tick is
therefore always on the current timeline, without any write path having to notice that a rewind
happened and without any slot carrying a mark of which timeline it belongs to.

## 5. Capture interval and restorable ticks

`captureInterval = K` images every K-th tick; journal segments are still recorded every tick
because they are needed to walk back to an image. Restorable ticks are the imaged ones;
`b3World_GetRestorableTick(T)` returns the newest imaged tick ≤ T, and the caller replays from
there. K divides image bytes and image-capture cost by K; journal bytes are per-tick regardless
of K, so total ring memory falls by less than K, by an amount that depends on the scene's
image-to-journal ratio. K trades that against up to K−1 extra replayed ticks. K=1 is the default.
A tick is imaged when at least the effective interval has passed since the last imaged tick
(tick 0 is imaged), so retained imaged ticks need not be multiples of the interval.

## 6. Budget, charge and capacity

- `maxBytes` is the target for retained bytes; 0 is unbounded, and then nothing in this bullet
  applies and only the window bound (§7) evicts. At each capture, in order:
  1. The window bound (§7) is applied.
  2. The minimum window is the newest imaged tick and every slot after it through this tick, plus
     the next older imaged tick and the slots between the two when the window bound retains it.
  3. This tick's slot is sized at the effective interval, and the charges of the minimum window,
     that slot counted once, are summed. When the sum is within `maxBytes`, slots are evicted from
     the old end only as far as this slot needs, and it is written.
  4. Otherwise the effective interval becomes `min(2 × interval, tickCount)` (tested as
     `interval ≤ tickCount − interval`, so it cannot overflow), as the recording player's keyframe
     ring doubles (`replay.c`); the slot is sized again, as a journal segment alone when the new
     interval says no image is due; slots are evicted as in step 3; and the slot is written even
     when the sum is still over `maxBytes`.
  A tick is imaged when at least the effective interval has passed since the last imaged tick
  (tick 0 is imaged). Slots already retained are untouched by widening, the effective interval is
  never narrowed again, and a rewind leaves it as it is; `b3World_GetHistoryInfo` reports it with
  the window. Widening changes how often an image is taken, not any one tick's own slot, so a tick
  whose own slot exceeds `maxBytes`, and a minimum window whose slots each fit but not together,
  are written over budget, growing the arena for as long as those slots are retained;
  `bytesUsed` reports the overrun. `maxBytes` is a target held under normal churn, not a ceiling.
- A slot's charge against `maxBytes`, and its share of `bytesUsed`, is its header, its slot-directory entry, its image and
  journal bytes plus the byte size of every heap block its journal entries own (a destroyed
  sleeping set's, island's or contact's arrays, a destroyed shape's `materials` block, a destroyed
  sensor's arrays, a hull); each owning entry records that size. The charge is computed once, at
  capture, and a slot is evicted whole, so the oldest retained slot's own journal segment, which
  no rewind reads, stays retained and charged until then and a charge never changes while its slot
  is retained (releasing it early would change a charge mid-life and walk its entries when the slot
  before it is evicted); blocks owned by entries still in the staging buffer (destroys made since the
  last step) are charged when that segment closes. Until then `b3HistoryInfo`'s `pendingBytes`
  reports the staging buffer's used bytes plus the bytes of the blocks its entries own, outside
  `bytesUsed` and outside the budget, held until the next step closes the segment or a rewind undoes
  them. Evicting the slot frees the blocks and releases the charge; removing the slot in a
  rewind releases the charge, the blocks having gone back to the world.
- Capacity growth (a scene grows) allocates the larger arena, copies the retained slots at their
  offsets and frees the old one, through `b3Alloc` and `b3Free` (`core.c`), which take `size_t`
  sizes (`b3GrowAlloc` takes `int` sizes and is not used); the old and new arenas coexist during
  the copy, and `arenaCapacityBytes` reports the new one. Rare after warm-up.
- Once the arena has warmed up, the ring slot is carved from existing capacity and nothing is
  allocated for it: its size is only known at step end
  (image bytes from the `docs/designs/20260929-002-rewind-only-history-ring.md` §6 count, journal bytes from the staging buffer's final size, §6),
  and reservation happens then. The per-tick journal staging buffer (`docs/designs/20260929-002-rewind-only-history-ring.md` §6) can itself grow during
  the step on an unusually large burst of structural churn; it is reused tick to tick and grows
  rarely once warmed up to the scene's typical churn, the same amortized sense as the arena's own
  capacity growth above, but it is not claimed to be allocation-free the way the ring slot is. Its
  capacity is outside `maxBytes`, `bytesUsed` counts retained history and not arena capacity, and
  both capacities are reported separately (`b3HistoryInfo`).

## 7. Window

`tickCount` bounds the window independently of `maxBytes`: after each capture,
slots are evicted from the oldest end while the ticks from the oldest retained imaged tick to
`historyTick` exceed `tickCount` and a later imaged tick remains, so an unbounded byte budget
still retains only the ticks from the oldest retained imaged tick, at most `tickCount` before `historyTick`, to `historyTick`.

## 8. Enable and disable

`b3World_EnableHistory` is called at a step boundary, with no broad-phase traversal open, and with no end events queued by API calls since
the last step (it asserts this; tick 0's image carries no event state, and `Rewind(0)` clears both
end-event buffers). It sets `historyTick` to 0 and images the world as it stands as the slot for
tick 0, with an empty journal segment; `Rewind(0)` restores that world, and the ticks after it count
steps from there.

`b3World_DisableHistory` frees every slot, the blocks their entries own, the blocks owned by
entries still in the staging buffer, the arena and the staging buffer. `b3DestroyWorld` does the
same first, before it tears down any structure those blocks were allocated from (the manifold block
allocators, the hull database).

## 9. Verification

1. **Ring transitions.** Interval widening under `maxBytes`, eviction, a single slot over budget,
   a minimum window over budget, a sole image under eviction pressure, and removal on rewind: a
   rewind to each retained imaged tick; a rewind followed by steps and a second rewind into the
   re-stepped ticks; two rewinds with no step between them; `Rewind(P)` after a force, torque or
   impulse call on an awake body; and a rewind with no step since a create or destroy, a setter on
   a sleeping body, and a heap-owning change (sensor destruction, hull release). After each, the
   restorable ticks are exactly the retained imaged ticks at or before `historyTick`, `bytesUsed`
   is the sum of the retained slots' charges, a request for a tick after `historyTick` returns
   `b3_historyTickUnavailable`, every heap block is either live in the world or owned by an entry
   of a retained slot or of the staging buffer, and the block allocators' live allocation counts
   (not their retained capacity) return to baseline after `DisableHistory` and after
   `b3DestroyWorld` with history enabled.
2. **Budget acceptance.** On each benchmark scene of `docs/designs/20260929-002-rewind-only-history-ring.md` §12 test 5, under an unbounded
   budget and under a budget the 60-tick window does not fit: `bytesUsed` within `maxBytes` except
   in §6's overrun cases, with the retained window and effective interval reported per scene.

## 10. Provenance

- **A slot's charge is computed once, at capture, and a slot is evicted whole.** Releasing an
  entry's block early would change a charge mid-life and walk the slot's entries when the slot
  before it is evicted; the oldest slot's never-read segment is the price, and it is one slot.
- **The effective interval is never narrowed again.** Narrowing on falling pressure would make
  the restorable set depend on the ring's past, which a caller cannot predict; the caller that
  wants the narrower interval back disables and re-enables history.
- **The minimum window is admitted over budget rather than refused.** Refusing it would reject a
  restorable tick (requirement 1); the overrun is bounded by those slots and reported.
