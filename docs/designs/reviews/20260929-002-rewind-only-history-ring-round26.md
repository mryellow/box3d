---
title: Codex review - rewind-only history ring, round 26
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round25.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 28
---

# Round 26

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T13:23:43+10:00 to 2026-10-15T13:31:56+10:00. Header confirmed `model: gpt-6-sol`,
`reasoning effort: high`, session `01a13d96-50cc-7353-b289-03e116bfbc43`.

## Review report (Codex final message)

## 1. Summary

The design accounts for the current awake and sleeping tree split, migration queue, and sorted continuous and explosion candidates. I found one unsafe restore entry point and one false replay guarantee that need edits before the design converges.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §8–9 | “Step boundary” and `world->locked == false` do not exclude an active public query. Public queries keep a broad-phase traversal open while invoking callbacks without locking the world (`src/physics_world.c`, `src/broad_phase.c`). A callback can therefore call `Rewind`, whose tree restore frees arrays the query is traversing. Enabling history there can also capture a queued, undrained migration as tick 0. | Require `traversalDepth == 0` for enable and rewind, and specify the API behavior when called from a query callback. Capture tick 0 only after migrations drain. |
| 2 | M | §14 | “If the caller replays nothing and steps, the world follows the old timeline bit for bit” contradicts §10’s requirement to replay every API input. An omitted force, setter, creation, or destruction changes the timeline. | State the guarantee with the same API calls, time steps, and host inputs replayed. |

## 3. Checked, no change

- The four-tree split, parked marked leaves, migration queue, and bulk placement match `src/broad_phase.c`.
- Pair keys, continuous candidates, and explosion candidates are sorted as claimed (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The awake tree image and the static, kinematic, and sleeping tree proxy rules account for the current moved-bit and leaf-box behavior (`src/dynamic_tree.c`, `src/shape.c`, `src/solver.c`).
- The sensor overlap image excludes arrays whose contents are discarded before their next physics use (`src/sensor.c`).

## 4. Proposed edits

- Add an explicit no-active-traversal precondition and its enforcement to §§8–9 and the API sketch in §14.
- Correct §14’s replay sentence to require replay of the original inputs.

## 5. Unresolved

None beyond the two proposed edits.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) Rewind and EnableHistory inside a query callback: confirmed, applied; the drain-first
  sub-proposal declined.** The public queries and casts open `b3BroadPhase_BeginTraversal` and call
  the host's callback without locking the world (`physics_world.c`), and a restore frees and
  reallocates the tree arrays the walk is reading. §9 now requires `traversalDepth` zero for
  `Rewind` and for `EnableHistory`, asserted (fail closed, U-S7), and the §14 comment says so.
  Capturing tick 0 only after a drain is declined: a queued body whose leaf is marked cannot be
  placed before the next pair update has searched it (`b3PlaceBodyProxies`, `broad_phase.c`), so
  the drain would leave the same bodies queued and add a write outside a step; reasoning added to
  §17 (U-V10).
- **#2 (M) "replays nothing and steps": confirmed, applied.** §10 and §14's earlier paragraph
  require replaying the recorded API calls; the sentence now says the world follows the old timeline
  when it replays the same API calls, time steps and host inputs.

Raw log: the reviewer read no round record or changelog.

## Status

Two findings, one High and one Medium, from two in round 25. Categories: an unstated precondition
on entry (#1, the traversal state beside the lock) and an overstated guarantee in §14 (#2).
Not converged. Next: round 27, same prompt, same scope.

Tripwire (U-T): no named mechanism added or removed; one assertion precondition added (U-T1).
Findings did not cluster in a section that produced findings last round (§9 and §14 against §13 and
§7.4) (U-T2); the reviewer read no round record. Doc length 1399 -> 1404 lines; added lines per
fix: #1 five (one Provenance bullet), #2 one (U-T3). Over the split check (U-T6): still one
decision; no split.

Round 26: 2 findings (1H-1M-0L), 2 applied, 0 declined
Series total: 84 findings (13H-44M-27L) across 26 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L) -> 2 (1H-1M-0L) -> 2 (1H-1M-0L)
