---
title: Codex review - rewind-only history ring, round 36
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round35.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 35
---

# Round 36

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T16:58:59+10:00 to 2026-10-15T17:05:46+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13e5b-6a06-72a0-8397-3cd3bfc3ad13`.

## Review report (Codex final message)

## 1. Summary

The current source supports the design’s main tree premise: dynamic proxies are split between awake and sleeping trees, parked leaves use a migration queue, and continuous and explosion candidates are sorted. I found three defects in the proposed cold state write boundary, ring eviction rule, and restore callback guard.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §5.2, §7.1, §9 | `fatAABBs[id]` is cold state for static and sleeping shapes, but the specified `b3Shape_Write` accepts and journals only a `b3Shape` record. No named accessor or journal entry writes the separate AABB array. For example, `b3Body_SetTransform` updates a static shape’s `fatAABBs` entry (`src/body.c`); after rewind, §7.4’s tree pass would use that later AABB instead of T’s. | Define a journaled fat-AABB write route, or make the shape accessor atomically write and undo both values. Route every cold AABB mutation through it. |
| 2 | H | §8 | Wrap eviction says to remove the oldest image and subsequent non-imaged slots through the next image. When only one image exists, there is no next image: with interval 4, ticks 1–3 have journal-only slots. Evicting tick 0 then leaves no rewind anchor, while the rule gives no instruction to grow or force an image. | Explicitly preserve the sole image and all segments after it until another image is committed; grow capacity or admit an overrun when needed. |
| 3 | M | §9, §10 | The claim that locking the world refuses API calls from `destroyDebugShape` is false for current mutators. `b3World_SetGravity` and `b3World_SetUserData` use getters that do not check `world->locked` (`src/physics_world.c`). A callback can therefore write during a partially completed restore despite the claimed guard. | Add an effective restore-time guard to those APIs, or state this solely as a caller precondition and remove the refusal guarantee. |

## 3. Checked, no change

- The awake/sleeping tree split, parked-leaf migration queue, bulk placement, and sleeping-tree height rebuild match `src/broad_phase.c` and `src/dynamic_tree.c`.
- Pair keys, continuous candidates, and explosion hits are sorted as claimed in `src/broad_phase.c`, `src/solver.c`, and `src/physics_world.c`.
- Sensor processing swaps overlap buffers and clears hits as assumed in §5.3 (`src/sensor.c`).
- The recording hash currently covers transforms and velocities, and the serializer omits the migration queue, as §1 and §13 state (`src/recording.c`, `src/world_snapshot.c`).

## 4. Proposed edits

Specify the fat-AABB accessor and undo payload in §5.2 and §7.1, including its use by §9’s tree pass. Add the sole-image case to §8’s eviction algorithm. Correct §9’s lock guarantee and align §10’s callback contract with the actual guard.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) no journaled route for a cold `fatAABBs` entry: confirmed as an unstated rule, applied.**
  `b3Body_SetTransform` writes a static shape's `fatAABBs` entry (`body.c`), §7.4's restore pass
  moves each listed static and kinematic proxy to its entry, and §5.2 names `shapes[id]` with
  `fatAABBs` as one record type, but the accessor took "the record" and the record-write entry held
  "old bytes" with no mention of the entry. Fix: `b3Shape_Write` takes the record and its `fatAABBs`
  entry together; the shape's record entry holds both and one undo restores both (§5.2, §7.1).
- **#2 (H) eviction with a sole image: confirmed as an unstated rule, applied.** The old-end bullet
  evicts the oldest slot and the non-imaged slots after it "up to the next imaged slot" with no case
  for a ring holding one image; the budget bullet admits a minimum window of two imaged ticks by
  growing the arena, which implies the writer never wraps into it, but the eviction bullet did not say
  so. Fix: the old-end bullet states that the writer never wraps into a slot of the minimum window
  and the arena grows instead, so a young ring's sole image is retained until a second is captured.
- **#3 (M) the lock does not refuse every world-level call: confirmed, applied.** `b3World_SetGravity`
  and `b3World_SetUserData` take the world through `b3GetWorldFromId`, which has no lock check
  (`physics_world.c`). §9's sentence, narrowed to "world-level" calls after an earlier round, was
  still false for those. Fix: §9 says which entries the lock refuses (those that take an unlocked
  world) and that the rest are unguarded; the callback's contract (do not call the API) is unchanged.
  Not a reversal of the recorded decision against enforcing the lock on every mutator (§17): no
  enforcement is added, only the claim corrected (U-V4).

Raw log: the reviewer read no round record or changelog.

## Status

Three findings, two High and one Medium, from one in round 35. Categories: two rules the doc
relied on without stating (#1 the shape entry's AABB, #2 the minimum window against eviction) and an
overstated guard (#3). §14, which rounds 34 and 35 found, drew nothing. Not converged. Next: round
37, same prompt, same scope, when the owner sends it.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings did not cluster in a section that
produced findings last round (§5.2/§7.1, §8 and §9 against §14); §9's lock sentence was found in
rounds 26 and 27 and now 36, not consecutive; none is a consequence of round 35's fix (U-T2); the
reviewer read no round record. Doc length 1433 -> 1436 lines; added lines per fix: #1 one, #2
three, #3 zero (U-T3). Over the split check (U-T6): still one decision; no split.

Round 36: 3 findings (2H-1M-0L), 3 applied, 0 declined
Series total: 98 findings (18H-50M-30L) across 36 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L)
