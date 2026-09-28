---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 18), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 18

## Review report (Codex final message)

## Summary

The design accounts for the main hot state, cold state, and tree reconstruction paths in the current source. Two journal entry formats need more detail to guarantee exact backward and forward scrubbing: graph-colour bitsets can grow, and cold dense arrays use swap removal.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.1 | A cold dense-array entry specifies an owner or handle, or an old length and popped bytes. That is insufficient to undo a removal from the middle: `b3Array_RemoveSwap` replaces the removed slot with the last element. Island links and non-awake solver-set arrays use this operation (`src/container.h`, `src/island.c`, `src/solver_set.c`). Restoring length and the popped element alone does not restore the removed slot or its order. | Specify an indexed swap-remove entry containing the removal index and the overwritten element bytes, as well as the former last element and length. Define undo and redo for append and swap-remove separately. |
| 2 | Medium | §5.2, §7.1, §12 | The colour `bodySet` entry records only a set or clear operation. `b3SetBitGrow` can increase `blockCount`, which `b3GetBit` and `b3ClearBit` subsequently consult (`src/constraint_graph.c`, `src/bitset.h`, `src/bitset.c`). The journal does not say how that logical length is restored when scrubbing across growth; the proposed full cold-state hash would also see a different bitset state. | Journal old and new `blockCount` with growth, or specify a canonical logical-length reconstruction during restore. State how redo grows the bitset before applying a high-index bit. |

## Checked, no change

- Awake set and graph-colour arrays are dense, and their order is retained by the proposed flat image (`src/solver_set.c`, `src/constraint_graph.c`).
- Sensor `eventBits` is set when the sorted overlap contents change, matching the conditional overlap journal rule (`src/sensor.c`).
- A filter change with `invokeContacts=false` can leave proxy category bits different from the shape filter, so tracking proxy category bits separately is warranted (`src/shape.c`).
- Id allocation uses a free list and a bump cursor; both matter to replayed ids (`src/id_pool.c`).

## Proposed edits

1. In §7.1, split "dense cold arrays" into append and swap-remove formats. For swap-remove, name the array and owner, removal index, old length, removed element bytes, and former last element bytes; give the exact inverse operations.
2. In §7.1, extend "bitset set/clear" to cover logical-length changes. In §12, include that length in the cold-hash comparison and test a rewind and forward scrub across the first use of a new bitset block.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — reasoning
effort resolved to `medium`), foreground, stdin from `/dev/null`, shell-level `timeout 540`, Bash
tool `timeout: 600000`. Start 2026-09-28T22:01:19+10:00, end 22:04:09+10:00, `EXIT_CODE 0` — no
resume needed, returned inline in under 3 minutes. Raw log had the final message duplicated (a
`codex exec` streaming artifact); one copy is kept above. Codex ran read-only; the working tree was
untouched by it going in (round 17's edits were already applied to the working file, confirmed via
`git status` before the call). The curated declined-findings list from rounds 1–7 (nine items,
sharpened per rounds 6/14/15's corrections, plus the round-14 pool-`nextIndex` item — ten items
total) was included in the prompt; Codex did not re-raise or dispute any of them.

Before writing the prompt, per WORKFLOW.md step 2's pre-check requirement, every pointer-typed
field of `b3Shape`, `b3Body`, `b3Joint`, `b3Contact`, `b3Island`, and `b3Sensor` was audited against
the "generic record copy overwrites a live pointer" hazard class round 17 flagged as open (its
finding #1, `userShape`, was the sixth instance of this class found across the series). Traced each
struct directly: `b3Shape.materials` (heap array, ownership-transfer), `.hull` (refcounted
database, round 2/10/14/15's mechanism), `.userShape` (round 17's own fix), `.userData` and
`b3Body.userData`/`b3Joint.userData` (caller-owned scalars the engine never frees, safe to copy
verbatim), `.mesh.data`/`.heightField`/`.compound` (caller-owned, §10's alive-and-unchanged
contract), `b3Contact.manifolds` and `meshContact.triangleCache` (round 5/8's exclusions),
`b3Island.bodies`/`.contacts`/`.joints` (round 5/11's dense-cold-array treatment), and
`b3Sensor.hits`/`.overlaps1`/`.overlaps2` (round 8's ownership-transfer). No unguarded
wholesale-copy path remained; no doc edit was needed from this pass, and it surfaced no new
finding for Codex to independently re-derive.

Both findings verified directly against source:

- **#1 (the "dense cold arrays" entry's payload — "old length, popped element bytes" — models
  removal as always coming from the tail, but `b3Array_RemoveSwap` removes from an arbitrary index
  and swaps the array's current last element into the freed slot), CONFIRMED, applied.** Traced
  every call site (`body.c`, `constraint_graph.c`, `island.c`, `physics_world.c`, `shape.c`,
  `contact.c`, `joint.c`, `solver_set.c`, `sensor.c`): each passes the removed element's own
  `localIndex` (or equivalent), not necessarily the array's last occupied slot —
  `b3RemoveBody`/`b3DestroyBody` (`body.c`) is a representative case, removing `body->localIndex`
  from `set->bodySims` and, when a swap occurs (`b3RemoveHelper`'s return is not `B3_NULL_INDEX`),
  updating the *moved* body's own `localIndex` to the freed slot — confirming §5.2's existing
  swap-compaction paragraph ("the moved element's own record write... restores its content, but
  not the array's length or which slot it occupies") is correct about what the moved element's own
  record write covers, but the entry kind it points to for the rest never named the removal index
  at all. Read `b3RemoveHelper` (`container.h`) precisely: on a non-tail removal it
  `memcpy`s the array's *current* last element into the freed index and decrements count; on a
  tail removal (`index == *count` after decrement) it does nothing but truncate. Confirmed the
  minimal correct fix does not need a separately-stored "former last element" payload: since
  journal entries for a given array are walked strictly in order and mutate live memory in place
  (§9), an undo can read the swapped-in bytes directly from the *live* array at the removal index
  (exactly what the swap placed there) and move them to the new last slot, before overwriting the
  removal index with the entry's own stored (genuinely lost, must be journaled) removed-element
  bytes; a redo needs only the index, since it is simply re-executing the same swap-remove against
  whatever is currently live. Fixed the "dense cold arrays" row to store `(removal index, old
  length, removed element bytes)` and describe undo/redo as the swap-then-restore / re-swap
  sequence above, instead of the tail-only "popped element" framing.

- **#2 (a graph colour's `bodySet` bitset's `blockCount` only ever grows, never shrinks, over a
  world's life, and the journal's plain "bitset set/clear" entry plus §12's hash/cold-hash-guard
  wording don't say how a rewind that crosses a growth point is compared), CONFIRMED, applied —
  for a narrower reason than the summary implies.** Traced `b3CreateGraph` (`constraint_graph.c`):
  each colour's `bodySet` is sized to the *world's initial* `bodyCapacity` at graph creation, not
  grown incrementally from empty; `b3SetBitGrow` (`bitset.h`/`bitset.c`) only fires later, when a
  body id from subsequent world growth exceeds that initial sizing, and confirmed (by grepping
  every call site) it is never called again after creation with a *smaller* count — `blockCount` is
  monotonically non-decreasing for the life of the colour. Traced `b3GetBit`/`b3ClearBit`
  (`bitset.h`): both already treat any bit index at or beyond the live `blockCount` as unset,
  and `b3GrowBitSet` zero-fills new blocks — so a "too-large" `blockCount` left in place after a
  rewind (nothing shrinks it back) cannot corrupt what any bit query returns for a given body id;
  the two possible states are functionally indistinguishable at the level of `b3GetBit`. The actual
  defect is narrower than "restore leaves the wrong logical bits": it is that §12's full-state hash
  and cold-hash guard, as worded, don't say whether they compare raw bitset bytes (in which case
  two functionally-identical bitsets that grew by different amounts would hash unequal — a false
  positive that would fail requirement 7's validation-build guard on a document that is otherwise
  correct) or logical bit values. Fixed by specifying, in both §12 point 1 and point 4, that
  `bodySet` bitsets are compared by logical bit value up to the body id pool's capacity, not raw
  block count — this needs no journal-entry change at all, since growth is already reversible
  no-op-equivalent at the bit-query level once the hash stops depending on `blockCount`'s exact
  value. Declined the "journal old and new `blockCount` with growth" half of Codex's proposed
  action as unnecessary: it would add a new entry kind and per-growth journal writes to fix a
  problem that a two-clause hash-comparison rule already fully resolves without touching the
  journal at all.

## Status

Round 18 of an ongoing series (rounds 1–17 committed). Both findings genuine and applied, with
finding #2 applied for a narrower mechanism (a hash-comparison rule) than either Codex's summary or
its literal proposed action suggested — the underlying defect traced to source was real, but its
correct fix was smaller than "journal blockCount changes," which would have been unnecessary
journal-format churn for the same reason the pre-round-18 doc already didn't need one. Finding #1
is the series' second distinct defect in the "dense cold arrays" mechanism round 11 introduced
(after round 11 itself, which added the mechanism to generalize the island-array entry kind) — the
mechanism's *existence* was right, but its payload silently assumed tail-only removal the whole
time, missed by six full-scope rounds (12–17) that touched adjacent territory without re-deriving
this specific entry kind's payload against `b3Array_RemoveSwap`'s actual semantics. The pre-prompt
self-check (auditing every pointer-typed struct field against the class of hazard round 17's own
finding belonged to) found nothing further wrong, which is itself informative: that hazard class
appears to be fully closed now across six prior instances, and this round's two findings are in a
different area (journal-entry payload completeness, hash-comparison normalization) than the
pointer-lifetime bugs that dominated rounds 5–17. Given a full-scope round immediately after
round 17 (2 findings, 0 declines) still found two more genuine, source-confirmed, previously-missed
defects — including a six-round-old gap in a mechanism several rounds have built on top of without
re-deriving — another full-scope round is warranted, not a narrower one, with attention to whether
any other entry kind's payload silently assumes a special case (tail-only, single-element, etc.)
of a more general engine operation the way "dense cold arrays" did here.
