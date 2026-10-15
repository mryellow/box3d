---
title: Codex review - history ring budget, window and eviction, round 1
target: docs/designs/20261015-003-history-ring-budget-and-eviction.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: none (round 1)
mode: full pass, round 1 of the ring doc, on revision 1 (split from the parent's section 8)
---

# Round 1

## Exact request sent to Codex

```
Review the design document docs/designs/20261015-003-history-ring-budget-and-eviction.md as an adversarial design reviewer for the Box3D physics engine. It decides the ring that holds a rewind-only world history: per-tick slots of image plus journal segment, the ring's two ends, which ticks are imaged and restorable, a byte budget that widens the capture interval instead of failing, what a slot is charged, the retained window, and enabling and disabling history. The image, the journal, the restore and the public API are decided by docs/designs/20260929-002-rewind-only-history-ring.md, which this document cites.

Scope: the document's own content; docs/designs/20260929-002-rewind-only-history-ring.md only where this document cites it (its sections 6, 7.1, 9, 12 and 14, which are settled for this review: judge whether this document's rules are consistent with them, not whether they are right); and the source it cites (src/replay.c, src/recording.c, src/physics_world.c, src/core.c, include/box3d/, test/test_recording.c). Citations follow the project format: source by path only, docs by section number; a missing line number is not a finding. Read the live source and check every claim the document makes about it.

Pressure-test the design. Find real defects: a budget, eviction, interval or window rule that can reject a restorable tick, evict the only image a retained segment walks back to, or leave a segment with no image before it; a charge that can double-count, under-count or change while its slot is retained; a block an entry owns that can be freed while a retained tick or the staging buffer still needs it, or leaked at eviction, disable or world destruction; a rule under which two of the document's own sections disagree, or under which the document and the cited sections of the parent disagree, including the field semantics of the parent's section 14; a claim about the recording player's keyframe ring or the allocators that the source does not support; a wrong or inconsistent number. Treat existing primitives (the recording serializer and player, the allocators) as settled tools. Decisions recorded in section 10 are settled unless you have fresh source evidence that their stated reason no longer holds.

Out of scope: the changelog, any history or "what changed" narrative, any citation into review records, index or bookkeeping, whether tests exist or a coverage table is complete, and the parent document's own rules except as this document relies on them. Do not review or follow any other design document except where this one cites it. Do not read docs/designs/reviews/ or any changelog.

Output exactly this shape:
1. Summary (2-3 sentences)
2. Findings: a table with columns `# | Severity | Section | Issue | Proposed action` (Severity H/M/L; real defects only; write "None." if empty)
3. Checked, no change: bullet list of what you verified and found correct
4. Proposed edits
5. Unresolved
6. Verdict: one line, `CONVERGED` or `CHANGES_PROPOSED`
```

Run: foreground, `timeout 540`, exit 0, 2026-10-15T17:38:37+10:00 to 2026-10-15T17:42:35+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13e7f-adb8-7d12-bea6-cbbb5a30e1fa`.

## Review report (Codex final message)

## 1. Summary

The ring’s ownership and rewind rules are largely consistent with the cited parent sections. The budget procedure needs changes before implementation: it can evict an image that would fit after interval widening, and its charge omits the slot directory.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §6 | Evicting down to two images **before** widening can discard a restorable tick unnecessarily. For example, with a 100-byte budget, two 40-byte image slots fit. If the next candidate costs 80 bytes with its image but 5 bytes as a journal-only slot, evicting the oldest image first leaves one image; widening first would retain both images and the new slot at 85 bytes. | Decide the candidate’s effective interval and slot size before committing budget evictions, then evict only what that final slot requires. |
| 2 | M | §§3, 6 | The stated minimum window is “two imaged ticks with the segments between them.” It omits mandatory journal slots **after** the newer image through `historyTick`. Those slots must remain to satisfy §3’s newest-slot and segment-retention rules, so the minimum-window and overrun rules disagree. | Define the protected minimum as the two images, every slot between them, and every slot through the current tick. Include that suffix in budget and arena-growth decisions. |
| 3 | M | §§3, 6 | A separate tick-to-offset slot directory needs storage per retained slot, but the slot charge includes only its header, image, journal, and owned blocks. Thus `bytesUsed` and the budget omit retained history metadata, with no separate directory-capacity field. | Charge each slot for its directory entry; report any unused directory capacity separately if it is allocated. |
| 4 | L | §6 | “Doubles … and never past `tickCount`” gives no consistent result when doubling crosses a non-power-of-two limit, such as interval 2 with `tickCount` 3. Stopping at 2 and advancing to 3 retain different image sets. | Specify `effectiveInterval = min(2 × effectiveInterval, tickCount)`, with overflow-safe arithmetic. |
| 5 | M | §6 | The described arena “realloc” is unsupported as an existing allocator operation. `src/core.c` offers `b3GrowAlloc`, which allocates, copies, and frees using **int-sized** byte counts; the document’s own roughly 3 GB example can exceed that range. | Specify a `size_t`-safe arena growth path using the project allocator hooks, and account for its temporary old-plus-new allocation during growth. |

## 3. Checked, no change

- The cited parent §§6, 7.1, and 9 support the staging, ownership-transfer, pending-undo, and rewind-discard rules. The future-tick lookup in §5 is consistent with parent §14; the unavailable result in §9 applies to `Rewind`.
- `src/replay.c` does double its keyframe interval under budget pressure, as claimed.
- `src/physics_world.c` tears down the hull database and manifold allocators during world destruction; freeing history-owned blocks before that teardown is the correct order.
- The 5–51 MB image estimate gives approximately 0.3–3 GB for 60 images.

## 4. Proposed edits

Revise §6 to size and choose the current slot before committing evictions, charge directory entries, define the capped interval update, and specify a `size_t`-safe growth operation. Revise §§3 and 6 to include the journal tail through `historyTick` in the minimum window.

## 5. Unresolved

None.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) eviction before widening: confirmed, applied.** §6 widened only "when a capture would
  not fit even after evicting down to a minimum window", so the evictions ran first and a tick that
  would have fit as a journal-only slot had already cost an image. The rule now sizes the slot,
  decides the interval against what the minimum window would retain, sizes the slot again, and
  only then evicts, as far as the final slot needs.
- **#2 (M) the minimum window omitted the tail: confirmed, applied.** §3 retains every segment after
  the oldest retained image and keeps `historyTick` as the newest slot, so the slots after the newer
  image through the current tick are never evictable; the window definition said "two imaged ticks
  with the segments between them". §3 and §6 now define it as the two newest imaged ticks and every
  slot from the older of them through `historyTick`.
- **#3 (M) the slot directory was not charged: confirmed, applied.** §3 names a tick → offset
  directory; the charge listed header, image, journal and owned blocks. The charge now includes the
  slot's directory entry.
- **#4 (L) doubling past `tickCount`: confirmed, applied.** `min(2 × interval, tickCount)`, tested
  without overflow.
- **#5 (M) "realloc" and `b3GrowAlloc`: confirmed, applied.** `b3GrowAlloc( void*, int, int )`
  takes `int` sizes; `b3Alloc( size_t )` and `b3Free( void*, size_t )` take `size_t` (`core.c`).
  Growth is now stated as allocate, copy, free through those two, with the old and new arenas
  coexisting during the copy.
- Also removed: a sentence on when a tick is imaged that the split had left in both §5 and §6.

Raw log: the reviewer read no round record or changelog.

## Status

Five findings, one High, three Medium and one Low, on the first round of the split-out doc. All
on §6's budget procedure and §3's window definition: the moved text had been reviewed inside the
parent for fifteen rounds without these being found, and a four-minute round on 178 lines found
them at once. Not converged. Next: round 2, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed; the budget procedure's order of operations is
restated (U-T1). First round, nothing to cluster against (U-T2); the reviewer read no round
record. Doc length 178 -> 181 lines; added lines per fix: #1 three, #2 one, #3 one, #4 zero, #5
two, one duplicated sentence removed (U-T3). Under the split check (U-T6).

Round 1: 5 findings (1H-3M-1L), 5 applied, 0 declined
Series total: 5 findings (1H-3M-1L) across 1 round
Findings: 5 (1H-3M-1L)
