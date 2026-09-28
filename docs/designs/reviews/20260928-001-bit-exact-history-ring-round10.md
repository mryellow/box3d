---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 10), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 10

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the source. I found one ownership gap that breaks the promised forward scrub when a hull is replaced.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §9 | The hull rule retains the **old** hull when a write would drop its last reference, but says nothing about retaining the **new** hull for redo. After `Rewind` undoes a hull replacement, the new hull can lose its last reference and be freed. A later forward scrub would then restore a dangling shape pointer. `b3Shape_SetHull` acquires the replacement and releases the old hull; the database frees a hull when its reference count reaches zero (`shape.c`, `physics_world.c`). | Keep both hull values needed by a reversible entry alive until that entry is evicted, and define reference transfers for undo and redo. |

## Checked, no change

- The sleep and wake paths transfer body sims, contacts, joints, and islands between solver sets as the inventory describes (`solver_set.c`).
- Sensor processing swaps its overlap arrays each step; `overlaps2` is the current overlap set used by sensor destruction and queries (`sensor.c`, `shape.c`).
- The existing tree move operation does not mark a proxy moved, so §7.4's separate moved-bit restoration is necessary (`dynamic_tree.c`).
- The current CCD query uses a running fraction and caps sensor hits during traversal, as §7.4 states (`solver.c`).
- The id pool uses a free array and bump index, matching the proposed pool journal distinction (`id_pool.c`).

## Proposed edits

In §7.1, specify how a hull journal entry retains both the before and after hulls while forward scrub remains available. Describe when the shape and entry acquire or release each reference on undo, redo, branch discard, and eviction. Add a §12 test that replaces a uniquely referenced hull, rewinds, scrubs forward, and repeats after shape destruction.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:24:37+10:00, end 20:27:38+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 4 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–7 (nine items) was included in the prompt; Codex did not
re-raise or dispute any of them.

The finding was independently re-verified against source (via a forked sub-agent):

- **#1 (hull journal protection is one-sided — only the old hull is kept alive, not the new one),
  CONFIRMED, applied.** Traced `b3Shape_SetHull` (`shape.c`): it acquires the new hull first
  (`b3AddHullToDatabase`) then releases the old one (`b3RemoveHullFromDatabase`, freeing it via
  `b3DestroyHull` in `physics_world.c` if its count hits zero) — a genuine two-sided refcount
  operation. §7.1's hull text protected only the old hull's side ("a journaled write whose forward
  direction would drop the last reference takes a reference on the hull data... holds it until the
  entry is evicted"), with no protection for the new hull's reference once undo has run. Unlike
  the other special journal kinds (set create/destroy, island arrays, manifold block), each of
  which is symmetric by construction, hull replacement was handled only in one-sided prose. The
  failure sequence is real and reachable through the design's own stated mechanics: a hull
  replacement at tick 10 installs hull B in place of hull A (A's last reference); `Rewind` back
  past tick 10 (undo, §9 step 2's reverse walk) restores `shape->hull = A` and, absent the fix,
  would drop B's now-unheld reference, freeing it if nothing else references it; a later `Rewind`
  forward past tick 10 again (redo, §9 step 2's forward walk — an ordinary scrub, not requiring
  re-stepping) then reinstalls B's raw pointer from the journal's "new bytes," now dangling. Fixed
  by making the extra-reference protection symmetric: both the old and new hull get an extra
  reference for as long as the entry is retained, released only on eviction, regardless of which
  direction (undo or redo) the entry is walked.

## Status

Round 10 of an ongoing series (rounds 1–9 committed). One finding, genuine, applied — the fourth
round to find a pointer/reference-lifetime bug in the restore or journal-undo mechanism (after
rounds 5, 8 for manifolds/island arrays/triangleCache/sensor arrays), each time in a structure the
prior rounds' fixes for the *same class of bug* didn't happen to cover. This round's instance is
different in shape from the others: not a missing exclusion in a "copy the rest of the record"
clause, but an asymmetric protection window in an already-special-cased mechanism (the hull
refcount protection round 4 introduced was itself only ever checked for the undo direction, never
the redo direction, across five subsequent rounds). Finding rate (8, 8, 8, 7, 8, 3, 1, 2, 1, 1
applied) has stayed at or below 2 applied findings for five consecutive rounds now, a longer low
stretch than the series has shown before round 6's spike. Given the recurring pattern of
one-sided/asymmetric protection being the last few rounds' actual defect shape, one more
full-scope round is warranted with specific attention to any other mechanism in the doc that
states a rule for only one of two directions (undo/redo, forward/backward, old/new) by default,
before treating the series as converged.
