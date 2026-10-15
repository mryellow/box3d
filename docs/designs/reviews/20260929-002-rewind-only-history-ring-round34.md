---
title: Codex review - rewind-only history ring, round 34
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round33.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 33
session: 01a13e41-d8d0-7681-9fe4-a51d576a2504, resumed once to finalize
---

# Round 34

> **Correction (revision 41):** the "Post-review verification" and "Status" sections below are
> round 33's text, copied into this record by a scripting fault that also failed before writing
> any of round 34's three fixes to the document; the commit for this round touched only the
> changelog and this file. The exact request and the review report above are round 34's. Its
> findings were verified and applied later: #2 (`version`) in the trees doc's revision 3, #1 (the
> hash comment) and the §6 capture point in the parent's revision 41, which lapses the settled
> status rounds 40 and 41 gave revision 40. The round's tally line is counted as it stands.


## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, killed at the wrapper's limit after nine minutes of real reads
(11,446 lines of trace, no closing message; a work timeout, not a stall), 2026-10-15T16:31:01+10:00
to 16:40:01. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 16:40:26 to 16:41:10. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, session `01a13e41-d8d0-7681-9fe4-a51d576a2504`.

## Review report (Codex final message)

## 1. Summary

The awake and sleeping tree split, sorted candidate paths, and migration queue are substantially reflected in the design. I found two contract and source-description defects; neither requires changing the proposed rewind mechanism.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §12, §14 | The API promises a hash of **all simulation-affecting state**, but §12 excludes every `userData` word. §10 allows callbacks to read those values and affect the next step, so equal hashes need not mean equal replay inputs. | Describe the hash as covering engine-owned state, and state in the public API that callers must combine it with a hash of callback-visible host state. |
| 2 | L | §7.4 | The awake-tree image is described as containing the fields written by the serializer, but its listed scalars omit `b3DynamicTree.version`, which `src/world_snapshot.c` writes. The version is fixed and does not affect simulation, so the image need not include it. | State explicitly that the image omits the fixed version field. |

## 3. Checked, no change

- Verified that pair creation, continuous candidates, and explosion impulses use the current sorted paths in `src/broad_phase.c`, `src/solver.c`, and `src/physics_world.c`.
- Verified the awake and sleeping tree migration, parked marked leaves, bulk proxy placement, and queue drain against `src/broad_phase.c`; the design accounts for their proxy identity and moved-bit state.
- Checked the proposed reverse-order pool, proxy, sensor, manifold, material, island, and solver-set ownership rules against the cited mutation paths. I found no additional undo or ownership defect.
- Checked the ring’s retained-image and intervening-journal rule, including pending between-step mutations; I found no restorable-tick rejection case.

## 4. Proposed edits

Make the two changes in the findings table. Also make §6 explicit that capture occurs **after** `b3BroadPhase_EndTraversal` drains migrations: in `src/physics_world.c`, that drain follows the end-event flip.

## 5. Unresolved

None within the requested design-review scope.

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

Round 34: 2 findings (0H-1M-1L), 2 applied, 0 declined
Series total: 94 findings (16H-48M-30L) across 34 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L) -> 2 (1H-0M-1L) -> 2 (0H-1M-1L) -> 0 (0H-0M-0L) -> 0 (0H-0M-0L) -> 1 (1H-0M-0L) -> 1 (0H-1M-0L) -> 2 (1H-1M-0L) -> 2 (0H-1M-1L)
