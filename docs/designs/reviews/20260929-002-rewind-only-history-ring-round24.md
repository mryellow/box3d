---
title: Codex review - rewind-only history ring, round 24
target: docs/designs/20260929-002-rewind-only-history-ring.md
date: 2026-10-15
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20260929-002-rewind-only-history-ring-round23.md
mode: full pass, same prompt and scope as round 22 (U-P4), on revision 26
---

# Round 24

## Exact request sent to Codex

Identical to round 22 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-15T13:04:27+10:00 to 13:13:20+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`, session
`01a13d84-b0b8-7fa1-9230-b04834f64e7f`.

## Review report (Codex final message)

## 1. Summary

The undo and image scheme accounts for the current four-tree broad phase, including parked sleeping proxies and sorted collision candidates. The main defect is a retained-memory bound that can grow with sleeping-proxy churn; the document also understates a restore cost and omits current write sites from its migration worklist.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | §§2, 5.1, 7.4 | The awake-tree image is not bounded by “the most that was awake at once.” Creating shapes on sleeping dynamic bodies parks their marked proxies in the awake tree (`src/shape.c`, `src/broad_phase.c`). Its proxy capacity never shrinks; ordinary proxy destruction can also leave `nodeEnd` high because rebuilding an empty tree returns immediately (`src/dynamic_tree.c`). Later ticks can therefore keep copying large arrays with few or no awake bodies, beyond the stated 24 B per slot exception. | Specify how the tree is compacted or imaged sparsely while preserving proxy IDs, then revise the cost bound and budget examples. |
| 2 | M | §§4, 7.4, 14 | The overview and API sketch give a restore cost bounded by awake state, journal work and per-proxy tree operations. Undoing an insert can trigger a full sleeping-tree rebuild, proportional to the sleeping tree (`src/broad_phase.c`). §7.4 acknowledges this, so the published restore bounds contradict it. | Include the possible full rebuild in both restore cost statements. |
| 3 | L | §5.2 | The “current direct-write callers” list for `shapes[id]` omits `b3PlaceBodies`: its single and bulk placement paths assign `shape->proxyKey`, including for sleeping shapes (`src/broad_phase.c`). The proposed type boundary should catch these writes, but the migration worklist is incomplete. | Add both placement paths and route their assignments through `b3Shape_Write`. |
| 4 | L | §10 | “`userData` is restored to its value at T” under Host handles conflicts with the same section’s rule that `world->userData` remains at its current value. The intended per-object distinction is unstated there. | Say explicitly that body, shape and joint `userData` is restored, while world `userData` is host configuration. |

## 3. Checked, no change

- Pair creation is sorted by shape-pair key; continuous candidates use sweep order with a shape-id tie-break; explosions apply hits in shape-id order (`src/broad_phase.c`, `src/solver.c`, `src/physics_world.c`).
- The current broad phase has separate awake and sleeping dynamic trees. Marked sleeping proxies can remain parked in the awake tree until the next pair update and drain (`src/broad_phase.c`).
- Reverse-order proxy create/destroy undo is consistent with the tree’s LIFO proxy free list, including bulk creation (`src/dynamic_tree.c`).
- The recording hash covers transforms and velocities, so the document correctly treats it as narrower than the proposed full-state hash (`src/recording.c`).

## 4. Proposed edits

Resolve finding 1 before retaining the §2 cost requirement. Apply the wording and worklist corrections in findings 2–4 across the cited sections.

## 5. Unresolved

The design still needs a concrete awake-tree compaction or sparse-image rule that preserves proxy identity after large parked-proxy bursts.

## 6. Verdict: CHANGES_PROPOSED

## Post-review verification (Claude)

Every finding checked against the live source and the doc.

- **#1 (H) awake-tree image bound: confirmed in part, wording applied, compaction declined.**
  `b3CreateShapeInternal` queues a sleeping body's new shape with its marked leaf parked in the
  awake tree (`shape.c`, `b3BroadPhase_MigrateBody`), so the tree's proxy count and high-water
  mark include parked leaves; `b3DynamicTree_Rebuild` returns at once for an empty tree and
  `proxyCapacity` never shrinks (`dynamic_tree.c`). §7.4 already states the mark is a high-water
  term but named only what was awake. Fix: §2 and §7.4 say awake proxies and parked leaves held at
  once. The proposed compaction is declined: it renumbers proxy ids, which `shape->proxyKey` and the
  free list's id sequence depend on, and the term is already a named exception to requirement 2
  (U-V5, no new machinery); reasoning added to §17 (U-V10). Not a fork: the doc's decision (image
  the tree whole) is unchanged.
- **#2 (M) restore cost omits the sleeping-tree rebuild: confirmed, applied.** §7.4 states an undone
  wake insert can fire the sleeping tree's height-triggered full rebuild inside `Rewind`; the §4
  cost line and the §14 comment now say so.
- **#3 (L) `b3PlaceBodies` assigns `shape->proxyKey`: confirmed, applied.** Single (`broad_phase.c`
  create path) and bulk placement both assign it, for sleeping shapes too. §5.2's `shapes[id]` row
  names it.
- **#4 (L) `userData` contradiction: confirmed, applied.** §10 says body, shape and joint
  `userData` is restored and `world->userData` is host configuration (§2).
- **Unresolved item** (awake-tree compaction) is #1's declined alternative.

Raw log: the reviewer read no round record or changelog (checked: none opened).

## Status

Four findings, one High, one Medium, two Low, from three in round 23. Categories: a bound that
omitted parked leaves (#1), a cost statement missing a case §7.4 already states (#2), an incomplete
migration worklist (#3, the same class as round 23 #1, in the sibling row), a wording conflict
(#4). Not converged. Next: round 25, same prompt, same scope.

Tripwire (U-T): no mechanism added or removed (U-T1). Findings in §7.4/§2 awake-tree text: round 22
and now round 24, not round 23; not three running. #3 repeats round 23 #1's class in the `shapes[id]`
row, not a consequence of that fix (U-T2); the reviewer read no round record. Doc length 1390 -> 1395
lines; added lines per fix: #1 five (one Provenance bullet), #2 zero, #3 zero, #4 one (U-T3). Over
the split check (U-T6): still one decision; no split.

Round 24: 4 findings (1H-1M-2L), 4 applied, 0 declined
Series total: 80 findings (11H-42M-27L) across 24 rounds
Findings: 4 (1H-2M-1L) -> 5 (1H-3M-1L) -> 4 (3H-1M-0L) -> 1 (0H-1M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (0H-1M-0L) -> 2 (0H-1M-1L) -> 6 (0H-3M-3L) -> 5 (0H-4M-1L) -> 5 (0H-4M-1L) -> 3 (0H-2M-1L) -> 5 (0H-3M-2L) -> 4 (0H-2M-2L) -> 3 (1H-0M-2L) -> 3 (0H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-1M-1L) -> 4 (0H-2M-2L) -> 0 (0H-0M-0L) -> 6 (2H-3M-1L) -> 3 (0H-1M-2L) -> 4 (1H-1M-2L)
