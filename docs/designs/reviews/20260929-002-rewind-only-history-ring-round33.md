---
title: Codex review - rewind-only history ring, round 33
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round32.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 32
---

# Round 33

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T14:59:01+10:00 to 2026-10-15T15:07:29+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13ded-91e9-70e1-a616-3838daa2c186`.

## Review report (Codex final message)

## 1. Summary

The rewind model accounts for the current split dynamic trees and sorted candidate paths. Two changes are needed: the sim journaling rule is unsafe at existing detach sites, and the stated capture cost omits a valid tick-0 static-tree case.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §5.2, §7.1, §9 | `b3Body_Write` and `b3Joint_Write` are specified to read a non-awake sim at the record’s old set coordinates. Current destroy paths remove that sim before their final record writes; `b3TransferBody` can also swap another sim into the old slot first (`src/body.c`, `src/joint.c`, `src/solver_set.c`). A direct migration to these accessors would read out of bounds or journal the wrong sim. | Specify a pre-detach capture point, or make post-detach record writes record-only and restore sims through the journaled array entries. State the required entry order. |
| 2 | M | §2, §5.1, §6, §8 | The “never proportional to static bodies” and `large_world` “few KB per tick” claims omit tick 0. `CreateLargeWorld` creates one million static shapes with contact creation forced (`shared/benchmarks.c`); enabling history before the first step images their moved-proxy IDs, requiring an O(1M) traversal and about 4 MB for that list. The design acknowledges a nonempty tick-0 static list but does not include it among requirement 2’s exceptions. | Add pending static moved proxies as a capture-cost term, qualify the `large_world` estimate for enable-before-first-step, and account for this slot under the stated budget policy. |

## 3. Checked, no change

- Pair creation, continuous candidates, and explosion candidates use the stated deterministic ordering (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The awake/sleeping tree split, parked marked leaves, migration queue, and sleeping-tree height trigger match §7.4 (`src/broad_phase.c`, `src/dynamic_tree.c`).
- The proxy-ID undo order, moved-bit restoration, and backward-only ring eviction rules are internally consistent.
- The §14 correction example applies a correction at the end of its server tick, consistent with §10’s input timing.

## 4. Proposed edits

1. Revise §5.2 and §7.1 so a record accessor never fetches a sim after its array slot has been removed or swapped; show the corresponding §9 reverse-undo order.
2. Revise §2, §5.1, §6, and §8 to include the tick-0 static moved-list cost.

## 5. Unresolved

The design must choose between capturing detached sims before removal and relying on journaled array undo for those transitions.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Checked against the live source; no fix has been folded, because the round stops under U-S3 and
U-S4 (below).

- **#1 (H) sim read at stale coordinates: confirmed in source, not folded.** `b3DestroyBody`
  removeswaps the body's sim (and state, if awake) out of its set and then writes the swapped body's
  `localIndex` and the destroyed body's own `setIndex`, `localIndex` and `id` (`body.c`);
  `b3TransferBody` emplaces the sim into the target set, removeswaps it out of the source, and only
  then writes the record's coordinates (`solver_set.c`). A record entry that reads the sim at the
  record's coordinates at those writes reads a slot that is gone or holds another body's sim. Round
  31's clause (entry holds the sim only when the coordinates name a non-awake set) does not cover
  these sites, since the coordinates still name a non-awake set. Two ways out, a design choice
  (U-S3): (a) capture a sim before it leaves its slot, at one named point inside each detach site;
  (b) a structural record entry never carries the sim, a sim moves only through the journaled array
  push and removeswap entries (which hold its bytes), and an in-place write of a non-awake sim gets
  its own entry kind (tag, id, old sim bytes) made by the setter entry alone. (b) removes the
  coordinate read from the structural path entirely and matches AC-2 (one choke point per write
  kind); (a) adds a capture point to each detach site, the list AC-2 forbids.
- **#2 (M) tick-0 static moved list omitted from the cost terms: not verified this round.** §5.1
  already states the list is non-empty only in tick 0's image (§8); the claim that
  `CreateLargeWorld` marks a million static proxies, and the 4 MB figure, are unchecked
  (`shared/benchmarks.c`).

Raw log: the reviewer read no round record or changelog.

## Status

Stopped; not converged. Two findings, one High and one Medium. Reasons for stopping, per the
instruction for this series and Part A: (1) findings in the same area three rounds running (U-S4):
rounds 31, 32 and 33 each found the sim and state payload of a body or joint record entry in §5.2
and §7.1, and each fix was a clause that the next round broke (U-T5 does not apply: these are new
facts, not re-raised findings); (2) #1 forks the design (U-S3, two viable directions with the
same user-visible behaviour and different shapes of the accessor and entry set). Owner decision
needed on (a) or (b) in the verification section above; the author's recommendation is (b).

Tripwire (U-T): no fixes folded this round (U-T1); findings clustered in the §5.2 write accessor
and §7.1 record-write text three rounds running (U-T2), the third a consequence of round 31's
clause, which the detach sites in `body.c` and `solver_set.c` contradict; the reviewer read no
round record. Doc length 1415 -> 1415 (U-T3). Over the split check (U-T6): still one decision; no
split.

Round 33: 2 findings (1H-1M-0L), 0 applied, 0 declined
Series total: 92 findings (16H-47M-29L) across 33 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L)
