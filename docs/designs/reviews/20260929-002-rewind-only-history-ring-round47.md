---
title: Codex review - rewind-only history ring, round 47
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round46.md
mode: confirmation round (U-P9) of round 46, same prompt and scope as round 40, on revision 46
---

# Round 47

## Exact request sent to Codex

Identical to round 46 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T20:51:06+10:00 to 2026-10-15T21:00:05+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13f2f-ea45-7b72-a4a2-ad0e05d9e4e3`.

## Review report (Codex final message)

## 1. Summary

The hot image, cold journal, and restore sequence are coherent against the current source, including the awake/sleeping tree split and sorted continuous and explosion candidates. I found one inconsistency in how the document describes its phase-0 serializer path.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §§1, 13, 15 | §15 calls phase 0 reuse of the serializer “as-is” and “correct and already tested.” §13 itself requires serializer changes for `userData` and `userShape`, a retained geometry registry, and a separate migration-queue image. `src/world_snapshot.c` currently omits that queue and host data. | Describe phase 0 as an adapted serializer path; distinguish the existing recording test from validation of the proposed rewind behavior. |

## 3. Checked, no change

- The live broad phase has separate awake and sleeping dynamic trees and a migration queue (`src/broad_phase.c`, `src/broad_phase.h`).
- Pair creation, continuous candidates, and explosion application use the stated deterministic ordering (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The inventory covers the reviewed awake records, solver arrays, sensor overlaps, world scalars, and cold pools and containers. I found no uncovered simulation state or demonstrated undo ownership error in those paths.
- The §9 restore order and §14 correction example agree with the §10 caller contract.

## 4. Proposed edits

Replace the “as-is” and “already tested” claims in §15 with the phase-0 work listed in §13.

## 5. Unresolved

None in this document’s review scope. The companion tree and ring rules retain their separate reviews.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

- **#1 (L) §15 called the serializer reuse "as-is" and "already tested": confirmed, applied.** §13's
  phase 0 adapts the serializer (the `userData` and `userShape` options, the shared geometry
  registry, the migration queue beside the image), and `world_snapshot.c` today has none of them.
  §15's rejected alternative is now keeping the adapted serializer as the product, correct with
  §13's adaptations but O(world) per tick. §1's "not reused as-is" is about the hash and stands.

Raw log: the reviewer read no round record or changelog.

## Status

One Low finding in the confirmation round, in §15, a consequence of revision 46's phase-0 change.
Fix folded; a fresh clean round and its confirmation are needed (U-S8). Not converged.

Tripwire (U-T): no mechanism added or removed (U-T1). §15 last produced a finding before round 40;
#1 follows from the owner's revision 46 edit to §13, not from a round's fix (U-T2); the reviewer
read no round record. Doc length 1169 -> 1169 lines; #1 added zero (U-T3). Over the split check
(U-T6): unchanged, one decision.

Round 47: 1 findings (0H-0M-1L), 1 applied, 0 declined
Series total: 109 findings (20H-55M-34L) across 47 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 3 (2H-1M-0L) -> 0 (0H-0M-0L) -> 1 (0H-1M-0L) -> 1 (1H-0M-0L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 3 (0H-2M-1L) -> 1 (0H-0M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 0 (0H-0M-0L) -> 1 (0H-0M-1L)
