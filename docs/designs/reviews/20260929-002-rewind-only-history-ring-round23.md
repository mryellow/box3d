---
title: Codex review - rewind-only history ring, round 23
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round22.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 25
session: 01a13d60-ad0b-7891-981b-43248579fc0e, resumed once to finalize
---

# Round 23

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 124 after nine minutes of real reads (a work timeout, not a
stall: 11,500 lines of source reads, the last message mid-analysis), 2026-10-15T12:25:07+10:00 to
12:34:07+10:00. Resumed per the Parameters row with "finalize from the research already done, no
further tool calls", options before `resume`, exit 0, 12:34:17 to 12:34:53. Both headers confirmed
`model: gpt-6-sol`, `reasoning effort: high`, the same session id.

## Review report (Codex final message)

## 1. Summary

The hot image, undo journal, and tree split account for the main state paths I checked. I found one migration write path that the design needs to make explicit, plus two claims about current source behavior that need correction.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | M | §§5.2, 7.4 | `b3PlaceBodies` writes `b3_proxyParked` on sleeping body records in `src/broad_phase.c`. Those records are cold, but the body write migration list omits this path. Restoring the queue and proxies without restoring the flag can leave migration state inconsistent. | Require these migration flag writes to use the structural `b3Body_Write` entry point; name the path in the migration worklist. |
| 2 | L | §§2, 3, 10 | The claim that custom filter and `preSolve` calls *within a CCD sweep* follow tree traversal is false. `src/solver.c` collects candidates, sorts them by sweep key and shape id, then evaluates those callbacks. | Narrow the layout-order exception to paths that still invoke callbacks during traversal, including pair discovery and sensor queries. |
| 3 | L | §§5.1, 7.4 | The static moved-proxy writer list omits `b3Joint_SetCollideConnected`. It can choose a static body and mark its proxy in `src/joint.c`. The proposed moved-leaf capture still covers the bit. | Add the joint setter to the stated sources of static moved marks. |

## 3. Checked, no change

- Current code has separate awake and sleeping dynamic trees, a migration queue, and parked marked leaves; the design’s main tree model matches them.
- Pair creation is sorted, CCD candidates are sorted, and explosions apply in shape-id order.
- The reviewed pool, pair-set, array, and owned-block undo rules did not reveal a further concrete failure.
- The pending-segment rewind and retained-tick budget rules were internally consistent in the cases checked.

## 4. Proposed edits

Specify the `b3PlaceBodies` flag writes as journaled structural body writes. Correct the CCD callback-order exception and add `b3Joint_SetCollideConnected` to the static moved-mark inventory.

## 5. Unresolved

- `src/broad_phase.c` comments that a migrated leaf box can differ from its shape’s fat AABB, while §7.4 relies on equality at step boundaries. The inspected write paths did not establish a reachable mismatch; that invariant still needs a focused check.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source at 55a40d1 and the doc.

- **#1 (M) `b3PlaceBodies` writes the parked flag: confirmed, applied.** The drain sets and clears
  `b3_proxyParked` and `b3_proxyMoving` on each placed body's record (`broad_phase.c`); a placed
  body is sleeping, so the record is cold. Fix: §7.4 states the parked write goes through
  `b3Body_Write`'s structural entry and that the moving flag is set and cleared inside one drain,
  so it is clear at every boundary; §5.2's body worklist names `b3PlaceBodies`.
- **#2 (L) callback order within a CCD sweep: confirmed, applied.** The continuous stage collects,
  keys and sorts candidates, then evaluates them, and the custom filter and `preSolve` are called
  from the evaluation (`solver.c`), so within a sweep the order is the sort's, as the tree-order
  preflight states. The layout-dependent remainder is the custom filter in pair discovery and
  sensor overlap (`broad_phase.c`, `sensor.c`), which run during traversal, and the interleaving
  of parallel workers. Fix: §10's sentence rewritten to say exactly that.
- **#3 (L) `b3Joint_SetCollideConnected` marks a static proxy: confirmed, applied.** It marks every
  proxy of whichever joined body has fewer shapes through `b3BroadPhase_MarkProxyMovedSerial`
  (`joint.c`), and that body can be static. Fix: §5.1's static row names the three mark setters of
  the broad phase as the sources, with the API callers in parentheses, so a new caller of a setter
  is covered without a list change.
- **Unresolved item, confirmed by the author as a defect, fixed.** The reviewer could not establish a
  reachable mismatch between a migrated leaf's box and its shape's fat AABB entry. It is reachable:
  the finalize writes a smaller fat entry when the shape's AABB leaves the old one, and for a bullet
  the enlarge that follows replaces the leaf only when the leaf does not already contain the new
  entry (`solver.c`, `b3DynamicTree_EnlargeProxy` sets the leaf to the argument), so a slowing
  bullet keeps a leaf larger than its entry; the migration carries that leaf into the sleeping tree
  (`broad_phase.c`, the comment the reviewer cited). §7.4 then claimed every leaf equals its entry
  at a boundary and had the restore pass move each listed sleeping proxy to its entry, which would
  shrink such a leaf after a rewind across the body's later wake and change which cross-pass
  overlaps the first replayed step finds. For the static and kinematic trees the claim holds: their
  writers are the create, the move and the finalize mark, which replace the leaf with the entry, and
  a kinematic body is not a bullet. Fix: the sleeping tree's leaves are restored from entries alone,
  the destroy entry's recorded box for an undone migration and a new proxy move entry (tree, id,
  old box, made inside `b3BroadPhase_MoveProxy` for the sleeping tree only) for a leaf written in
  place by `b3ResetProxy`; the pass runs on the static tree in phase 1 and the kinematic tree from
  phase 2 (§7.1 table, §5.2 row, §7.4, §12 test 1's hash no longer assumes equality, test 2 gains the
  bullet case, §13). Not counted in the tally, which counts the Findings table only (U-R5). The
  engine-side alternative, a bullet enlarge that replaces the leaf whenever it differs from the entry
  as the finalize mark does, would restore the equality for every tree and retire the move entry;
  it changes pair discovery for slowing bullets and is an engine change for its own preflight.

Raw log: the reviewer read no round record or changelog.

## Status

Three findings, one Medium and two Low, from six in round 22, plus one unresolved item the author
confirmed and fixed. Categories: a cold write path the migration worklist missed (#1), a stale
statement of the callback-order exception (#2), a missed static-mark source (#3), and a false
invariant about leaf boxes (the unresolved item). #3 landed in the §5.1 row round 22 wrote, as a
consequence of that fix's enumeration; the unresolved item is in text revision 22 rewrote. Not
converged. Next: round 24, same prompt, same scope.

Tripwire (U-T): one named mechanism added, the proxy move entry for the sleeping tree, in place of
the restore pass's coverage of that tree (U-T1); nothing removed by name. Findings did not cluster
in a section that produced findings last round except §5.1 (#3, the enumeration round 22 added),
answered by naming the choke points instead of the callers (U-T2); the reviewer read no round
record. Doc length 1370 -> 1382 lines; added lines per fix: #1 four, #2 two, #3 zero, the
unresolved item six (U-T3). Over the split check (U-T6): still one decision; no split.

Round 23: 3 findings (0H-1M-2L), 3 applied, 0 declined
Series total: 76 findings (10H-41M-25L) across 23 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L)
