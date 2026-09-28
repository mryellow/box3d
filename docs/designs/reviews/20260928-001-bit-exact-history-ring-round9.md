---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 9), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 9

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the source, but the proposed tree reconstruction misses one simulation-affecting state: a proxy's category bits can differ from its shape's current filter.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.4 | `b3Shape_SetFilter(..., invokeContacts=false)` updates the shape filter without updating its existing tree proxy. The proposed restore derives category bits from the restored shape record, so it can give that proxy different category bits from those it had at tick T. Broad-phase queries use proxy category bits; replay can therefore discover different pairs. (`src/shape.c`, `src/broad_phase.c`, `src/dynamic_tree.c`) | Preserve proxy category bits as distinct history state, including a baseline when history starts, or change and test the setter's behavior so proxy and shape category bits cannot diverge. |

## Checked, no change

- Awake contact records can be gathered from colour arrays and the awake set's contact indices. (`src/constraint_graph.c`, `src/solver_set.c`, `src/contact.h`)
- The sensor task sorts and deduplicates overlaps, then signals changed overlap sets through its event bitset. (`src/sensor.c`)
- The serializer captures the six id pools, solver sets, contacts with manifold data, sensors, islands, trees, and graph colours for Phase 0. (`src/world_snapshot.c`)
- Dynamic and kinematic tree rebuilds clear moved flags; the step's broad-phase update clears static tree moved flags. (`src/dynamic_tree.c`, `src/broad_phase.c`)

## Proposed edits

In §5, classify each proxy's category bits separately from `shape->filter.categoryBits`. In §§6–9, specify how capture, journal, and restore preserve the proxy value when the two differ. Add a replay test that changes category bits with `invokeContacts=false`, later changes the proxy, then rewinds to the intervening tick.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:18:12+10:00, end 20:21:39+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 4 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–7 (nine items) was included in the prompt; Codex did not
re-raise or dispute any of them.

The finding was independently re-verified against source (via a forked sub-agent):

- **#1 (a tree proxy's `categoryBits` can diverge from its shape's `filter.categoryBits` and the
  design's restore algorithm has no independent state to recover the diverged value from),
  CONFIRMED, applied.** Confirmed `b3Shape_SetFilter( shapeId, filter, invokeContacts )`
  (`shape.c`) unconditionally sets `shape->filter = filter`, but only resyncs the tree proxy's own
  `categoryBits` field (via `b3ResetProxy` → `b3DynamicTree_SetCategoryBits`) when
  `invokeContacts` is `true` — confirmed no other call site resyncs it. This is a deliberate,
  persistent engine feature (an adjacent comment notes filter changes are allowed to lag sensor
  overlaps too), not transient state the engine self-heals before the next step. It is genuinely
  simulation-affecting, not a traversal-order/performance detail like tree layout:
  `b3TestCategory( proxy->categoryBits, maskBits, ... )` gates pair admission in every tree query
  path (`dynamic_tree.c`) — a binary filter decision on whether a pair is discovered at all,
  unlike traversal order, which pair-sorting already makes irrelevant to results. §5.1/§5.2 never
  tracked the proxy's own `categoryBits` as state distinct from `shape->filter`, so §7.4's restore
  rule ("sync category bits from the restored shape record") could only read the wrong,
  potentially-resynced value whenever an `invokeContacts=false` call had diverged the two before
  T. Fixed by adding proxy `categoryBits` to §5.2's cold inventory as its own journaled field
  (written at proxy creation and by `b3ResetProxy` only when `invokeContacts=true`), having §7.4's
  restore read that journaled value instead of deriving it from the shape record, and adding it to
  §12's full-hash inventory and cold-hash guard list so a future missed write site fails a test.

## Status

Round 9 of an ongoing series (rounds 1–8 committed). One finding, genuine, applied — a real gap in
the state inventory itself (a whole field of simulation-affecting state never classified anywhere
in §5), distinct in kind from rounds 5–8's pattern of pointer-safety bugs in an otherwise-complete
inventory. This is a reminder that the inventory audit itself, not just the mechanisms built on
top of it, can still have blind spots this far into the series. Finding rate (8, 8, 8, 7, 8, 3, 1,
2, 1 applied) stays low but non-zero, and the finding classes are diversifying (restore pointer
safety, hash coverage, caller-contract completeness, now inventory completeness) rather than
repeating one exhausted category — which argues for continuing rather than treating the low raw
count as convergence. Another full-scope round is warranted.
