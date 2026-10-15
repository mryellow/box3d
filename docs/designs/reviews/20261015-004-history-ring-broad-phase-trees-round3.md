---
title: Codex review - the broad-phase trees under a rewind-only world history, round 3
target: docs/designs/20261015-004-history-ring-broad-phase-trees.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261015-004-history-ring-broad-phase-trees-round2.md
mode: full pass, same prompt and scope as round 1, on revision 3
---

# Round 3

## Exact request sent to Codex

Identical to round 2 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T18:59:03+10:00 to 2026-10-15T19:02:14+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ec9-53ed-79f3-a0e6-ebe8a2d8ed15`.

## Review report (Codex final message)

## 1. Summary

The awake-tree image, proxy journal entries, and restore order cover the leaf boxes, proxy ids, moved bits, and queued migrations I traced in the live four-tree code. I found two documentation defects, neither of which shows a first-replayed-step pair mismatch.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | L | §9 | It says the restore pass is required for both the static and sleeping trees in phase 1. §6 says the pass does not touch the sleeping tree; its leaves return through proxy undo entries. | Say both trees need proxy entries, while only the static tree needs the restore pass in phase 1. |
| 2 | L | §§8, 12 | “Nothing reads” the sleeping tree’s layout or a different height-triggered rebuild is too broad. The height trigger reads it, and `b3World_GetCounters` exposes its height and rebuild count (`src/broad_phase.c`, `src/physics_world.c`). | Limit the claim to physics results and state that these diagnostic counters can differ after rewind. |

## 3. Checked, no change

- The live code has four trees, parked marked leaves, a persistent migration queue, and bulk placement (`src/broad_phase.c`, `src/broad_phase.h`).
- Proxy create and destroy paths, including bulk migration, have the proposed choke points. Current direct world-tree writers outside `src/broad_phase.c` are the ones §7 calls for moving.
- Reverse proxy undo preserves ids and free-list order; destroy payloads preserve sleeping leaf boxes that can differ from `fatAABBs` (`src/dynamic_tree.c`, `src/solver.c`).
- The static and kinematic leaf-box pass and moved-bit restoration fit the parent’s §9 order.

## 4. Proposed edits

Make the two wording changes in the findings. No change to the tree allocation or restore design is proposed.

## 5. Unresolved

None.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (L) §9 required the restore pass for the sleeping tree: confirmed, applied.** §6 restores
  the sleeping tree's leaves from entries alone and the pass does not touch it; §9's sentence, from
  the round 2 fix, said both trees needed the pass. §9 now says proxy entries for both and the pass
  for the static tree.
- **#2 (L) "nothing reads" the sleeping tree's layout: confirmed, applied.** The height trigger
  reads it, and `b3World_GetCounters` reports `sleepingTreeHeight` and `sleepingTreeRebuildCount`
  (`physics_world.c`, `types.h`). §8 and §12 now say no physics result reads it and name the
  counters as diagnostics that can differ after a rewind.

Raw log: the reviewer read no round record or changelog.

## Status

Two Low findings, from two in round 2. #1 is a consequence of round 2's §9 fix, which overstated
what phase 1 needs for the sleeping tree; #2 is an overbroad word in two places. Not converged.
Next: round 4, same prompt, same scope. §9 has now produced a finding in rounds 2 and 3; a third
would trip U-S4.

Tripwire (U-T): no mechanism added or removed (U-T1). §9 clustered two rounds running, #1 a
consequence of the previous fix (U-T2); the reviewer read no round record. Doc length 269 -> 269
lines; added lines per fix: #1 zero, #2 two (U-T3). Under the split check (U-T6).

Round 3: 2 findings (0H-0M-2L), 2 applied, 0 declined
Series total: 6 findings (0H-2M-4L) across 3 rounds
Findings: 2 (0H-1M-1L) -> 2 (0H-1M-1L) -> 2 (0H-0M-2L)
