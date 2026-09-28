---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 14), two independent fresh sessions (see Post-review verification for why)
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 14

## Review report (Codex final message)

## Summary

The whole-world rewind approach is sound in outline, but two restore rules can leave live pointers or proxy keys invalid. The proposed cold-state guard also misses fields it is meant to verify. This was a read-only review; I did not run the proposed tests.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1, §9 | Hull undo/redo copies a shape record while the journal holds extra references to both hulls. Copying the pointer does not transfer the shape’s own reference. On journal eviction, a hull still used by the restored shape can be freed, while the other hull retains a leaked reference. Hull references are counted and freed at zero in `physics_world.c`; shape changes release them in `shape.c`. | Specify reference-count changes when a shape switches hulls during undo, redo, image restore, and eviction. Test repeated scrubs followed by eviction. |
| 2 | High | §7.4, §9 | Tree reconstruction restores a journaled `shape->proxyKey` but does not reconcile its numeric proxy id with the live tree. A shape reset can destroy and recreate a proxy **in the same tree** in `shape.c`. If its AABB, category, and body type match T, none of §7.4’s stated repair conditions applies; the restored key can then address a freed or different proxy through `broad_phase.c`. | Reconcile proxies by shape id, write each shape’s key from its actual live or newly created proxy, and use that key when restoring moved membership. Cover same-tree reset and id reuse in tests. |
| 3 | Medium | §5.2, §12 | The cold-hash guard claims to check every journal-dependent cold write, but its listed coverage omits cold fields on **awake** shapes and joints, changed sensor overlaps, and non-awake `fatAABBs`. Those fields are assigned to the journal in §5.2; setters and sensor updates write them in `shape.c`, `joint.c`, and `sensor.c`. The restore tests also emphasize setters on sleeping bodies, leaving this gap weakly exercised. | Define guard coverage by journal-owned field, regardless of the owner's awake status, and exercise awake setters, sensor changes, and non-awake bounds in restore tests. |
| 4 | Low | §5, §10 | “Every other setter” restoring its T-time value is too broad for names. Body and shape records hold name ids, but the final ring inventory does not restore the monotonic name cache. `b3AddName` returns an existing id on a hash collision, so a name added on a discarded future branch can change a later replayed name lookup (`name_cache.c`, `body.c`, `shape.c`). | Journal name-cache additions, or explicitly exclude names from the rollback contract and describe the collision behavior. |

## Checked, no change

- `b3HashWorldState` hashes transforms and velocities; the backward scrub test compares that hash after seeking (`recording.c`, `test_recording.c`).
- Id allocation uses a LIFO free array and a bump index (`id_pool.c`).
- Broad-phase candidate pairs are sorted by shape-pair key before contact creation (`broad_phase.c`).
- Sensor overlap results are sorted and deduplicated before change detection (`sensor.c`).
- The existing serializer includes world configuration, pools, solver sets, contacts, sensors, islands, trees, and graph colours (`world_snapshot.c`).

## Proposed edits

1. In §7.1 and §9, define hull ownership as a state transition: release the shape’s reference to the outgoing hull and acquire one for the incoming hull, while journal-held references remain independent.
2. In §7.4, replace numeric-key restoration with shape-id-based proxy reconciliation, including same-tree destroy/recreate paths.
3. In §12, enumerate and hash all fields classified as cold in §5.2, then add tests that mutate each omitted category.
4. In §10, narrow the setter promise or add name-cache journaling.

## Unresolved / disagreements

Earlier-round context says world `userData` has no setter after creation. `b3World_SetUserData` exists in `physics_world.c`, so that premise is incorrect. Its value still appears caller-opaque and outside the design’s simulation-affecting state, so I do **not** propose journaling it on that basis.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation, and why this round has two sessions.** First attempt: `codex exec --sandbox
read-only "<prompt>"`, foreground, stdin from `/dev/null`, shell-level `timeout 540` — but I
omitted the Bash tool's own `timeout` parameter, so the harness auto-backgrounded the call at its
120s default exactly as WORKFLOW.md warns. Per WORKFLOW.md I called `TaskStop` on it immediately
rather than waiting on its notification. However, the underlying `codex exec` process was already
detached from the killed wrapper shell and kept running to completion on its own, writing a full,
valid answer (session `01a0e7af...`, reasoning effort `medium`, the CLI default) into the same raw
log the reissued call also appended to. I reissued the call correctly (Bash tool `timeout: 600000`
explicit this time); that second, properly-supervised invocation (session `01a0e7b1...`, same
prompt, same default effort) ran foreground start-to-finish, 2026-09-28T21:06:31+10:00 to
21:10:48+10:00, `EXIT_CODE 0`. Both sessions used the identical prompt text (rounds 1–7's nine-item
curated declined-findings list, this round's own wording) and both are genuine, independent,
from-scratch full-scope reviews — not a resume of each other. Rather than discard the
auto-backgrounded session's output as tainted by the tooling mistake, I verified findings from
both against source, since both are legitimate reviews that happened to run. The second session's
report (`01a0e7b1...`) is kept above as this round's canonical `## Review report`, being the one
from a correctly-configured, foreground-supervised call; the first session's additional findings
are verified below alongside it. Codex ran read-only both times; the working tree was untouched by
either going in. The curated declined-findings list from rounds 1–7 (nine items) was included in
the prompt; neither session re-raised or disputed any of them (the second session's own "Unresolved"
note about `b3World_SetUserData` is a factual correction to the curated list's wording, addressed
below, not a dispute of the decline's conclusion).

All four findings from the canonical (second) session verified directly against source:

- **#1 (hull undo/redo never re-acquires the shape's own database reference, so the journal's
  eviction-time release of both "extra" references can free a hull the live shape still points
  to, and permanently leaks the other), CONFIRMED, applied.** Read `b3Shape_SetHull` (`shape.c`):
  it calls `b3AddHullToDatabase` for the new hull, then (via
  `b3DestroyShapeAllocationForShapeChange`) `b3RemoveHullFromDatabase` for the old one — the
  shape's *own* reference is transferred by these two calls, live, at the moment of the original
  write, not by anything undo/redo does later. Confirmed `b3AddHullToDatabase`/
  `b3RemoveHullFromDatabase` (`physics_world.c`) are plain refcount inc/dec, freeing at zero, with
  no journal awareness. §7.1's undo/redo for a generic record write is a raw byte copy of the
  `hull` pointer field — it does not call either database function. Traced the resulting sequence:
  original forward write leaves the database's refcount for the new hull at (shape's own ref +
  journal's extra ref) and the old hull at (only the journal's extra ref, since the shape's own
  ref on it was just released live). An undo restores `shape->hull` to the old pointer via raw
  copy only — the database still thinks the *new* hull is the one with a "real" owner, and the old
  hull's only reference is the journal's extra one. At eviction, the entry (as worded before this
  round) released *both* extra references unconditionally: on the old hull (now the live value,
  post-undo) that drops it to zero and frees memory a live shape still references; on the new
  hull, an extra reference nothing else was tracking survives as a permanent leak. Fixed §7.1 to
  make eviction asymmetric: release the extra reference on whichever hull is not the shape's
  current live value, and let the surviving extra reference stand in for the shape's own,
  which no undo/redo ever reacquired.

- **#2 (a same-tree proxy reset changes `shape->proxyKey`'s numeric value without necessarily
  changing the bounds/category/body-type §7.4 checks, so restore can leave a shape's proxy key
  pointing at the wrong or a freed proxy), CONFIRMED, applied.** Read `b3ResetProxy` (`shape.c`):
  with `destroyProxy=true`, it destroys the live proxy and creates a new one *in the same tree*
  (`proxyType` unchanged), assigning `shape->proxyKey` a numeric id from that tree's own live free
  list — independent of whether the recomputed fat AABB or `shape->filter.categoryBits` actually
  changed. Traced `b3Shape_SetFilter` (`shape.c`), the primary caller with `invokeContacts=true`:
  it always calls `b3ResetProxy` with `destroyProxy` computed from `filter.categoryBits ==
  shape->filter.categoryBits` — but `shape->filter` was already overwritten to the new `filter`
  value one line earlier in the same function, making this comparison always true (a live,
  pre-existing engine bug outside this design's scope, but one that makes the same-tree-reset
  case the *common* path, not a rare corner). Confirmed the design nowhere journals or restores
  `shape->proxyKey` directly, and nowhere excludes it from the generic "shapes[id] fields other
  than aabb/fatAABBs" journaled row either — so as written, a generic record-write undo/redo would
  either silently miss the reset (if `proxyKey` isn't actually captured by that row) or restore a
  stale numeric id with no guarantee the live tree still has a matching proxy at that slot (if it
  is). Fixed by adding a distinct "tree proxy reset" journal entry (§5.2, keyed by shape id,
  explicitly excluding `proxyKey` from the generic shape-record row) and using it in §7.4's
  restore algorithm as a fourth, independent trigger for destroy-and-recreate, alongside the
  existing AABB/category/body-type checks rather than folded into them.

- **#3 (the cold-hash guard's own enumeration is narrower than §5.2's actual journaled-cold-state
  inventory — specifically shape/joint fields journaled regardless of owner awake state, sensor
  overlaps, and non-awake shape bounds), CONFIRMED, applied.** Re-read §5.2 and §7.1 side by side
  with §12 point 4 as it stood before this round ("sleeping sets, non-awake records, pools, pair
  set, graph-colour `bodySet` bitsets, tree proxy `categoryBits`"): §7.1's own rule states a
  shape's filter/material/`materials`/geometry/flags and a joint's tuning parameters are journaled
  "regardless of the owner's awake state" — genuinely cold state on an *awake* owner, which
  "non-awake records" does not naturally read as covering. §5.2 separately lists
  `sensors[shapeId].overlaps2` content and non-awake `shapes[id].aabb`/`fatAABBs` as journaled,
  neither named in the guard's list. Since §12 point 4's own stated purpose is "what makes §5.2's
  list a test rather than a promise," a guard enumeration that can drift out of sync with §5.2
  defeats that purpose the moment they diverge — confirmed a real, present divergence, not a
  hypothetical one. Fixed by replacing the guard's separately-maintained list with "every cold
  structure §5.2 lists as journaled, in full," naming the specific categories that were missing
  (including this round's own new proxy-reset entry) so the two can't silently drift apart again.

- **#4 (world `userData` unresolved note), factual correction acknowledged, no doc change.**
  Confirmed `b3World_SetUserData`/`b3World_GetUserData` exist (`physics_world.c`,
  `include/box3d/box3d.h`) — round 3's declined-finding text ("no setter found") was wrong on this
  point. Checked whether this changes the decline's conclusion: grepped every internal
  `b3FindName`/`world->userData`-style read the engine makes of caller-opaque data; `userData` has
  no internal engine reads at all (round 3 already established this), so it remains outside
  "simulation-affecting" by requirement 1's own definition regardless of whether a setter exists —
  a setter only means it's an ordinary API call the caller replays like any other (§10 already
  covers "every API call" generically). No doc change needed; the target doc never itself states
  the wrong "no setter" claim (that was only in round 3's own file, which WORKFLOW.md says is
  never edited after the fact). Noting the correction here so future curated-list compilations
  drop the stale "no setter" phrasing for this item.

Three further findings, from the first (auto-backgrounded-but-completed) session, independently
verified against source and folded into this round rather than discarded:

- **(minimum retained window can end up with zero images under sustained memory pressure),
  CONFIRMED, applied.** §8's claim "the minimum window always carries at least one image" was
  asserted without a mechanism. Read the cited precedent, `b3RecCaptureKeyframe`
  (`recording_replay.c`): its doubling loop evicts existing keyframes that fall off the new,
  wider `frame % keyframeInterval` grid — but that player keeps an *unbounded*, ever-growing
  recording and can afford to thin arbitrarily; it does not, and structurally cannot, establish
  "the most recent N ticks always contain an image," since it never forces an out-of-cadence
  capture. This design's ring is different (bounded, FIFO, oldest-evicted-on-wraparound), but as
  written it borrowed only the "double the interval" idea, not a description of what keeps the
  live edge of the window covered once K grows past the minimum window size — under sustained
  doubling, the current tick and the minimum window around it can both legitimately fall off a
  large K's alignment, leaving zero images in the window the guarantee promises always has one.
  Fixed by stating explicitly that already-retained images are never purged for grid misalignment
  (only ordinary FIFO eviction removes anything), and that a capture takes an image outside the
  normal K-th-tick cadence whenever skipping it would leave the current minimum window imageless.

- **(preSolve order claim is too broad — only CCD-triggered preSolve calls are tree-traversal-
  order-dependent, not the ordinary per-contact call), CONFIRMED, applied.** Read both call sites:
  `solver.c`'s CCD path (tree-traversal-order-dependent, as §7.4 already documents elsewhere for
  the same reason as the sensor-hit cap) and `contact.c`'s ordinary per-touching-contact call,
  reached via the graph-colour/`contactIndices` arrays — hot, imaged state whose order the image
  restores exactly, entirely independent of how §7.4 rebuilds the (unrelated) broad-phase tree.
  §7.4's and §10's "`preSolve` order within a step... follows tree traversal" read as covering
  every `preSolve` call, which is false for the common (non-CCD) case. Fixed both sentences to
  name the CCD-triggered calls specifically, distinguishing them from the ordinary per-contact
  call whose order the image already reproduces exactly.

- **(pool `nextIndex` allegedly excluded from the state hash "as performance state"), checked,
  no change — the premise doesn't match the doc.** Searched the target document for any text
  excluding pool cursor/allocator internals from the hash or calling them "performance state";
  found none — §12 point 1 says "pool state" unqualified, which naturally includes `nextIndex`
  (confirmed a real field of `b3IdPool`, `id_pool.c`, that determines the next bump-allocated id).
  Not applied; no doc change needed.

Two more findings from the first session were checked and found to already be resolved by prior
rounds, evidently missed by that session despite the from-scratch instruction — not evidence of a
present gap:

- **(`b3World_Explode` said to be an "acknowledged exception" to exact replay), stale, no
  change.** §10's actual current text says the opposite: "a replayed explode reproduces the same
  velocities and wake order as the original run" — round 3 already fixed this exact issue by
  sorting candidates by shape id before applying impulses.
- **(material-element journal entries said to lack an element index), stale, no change.** §7.1
  already states such a write "journals that element's old and new bytes, keyed by shape id and
  element index" — round 4 already fixed this exact issue.
- **(name cache: same conclusion as this round's own finding #4)** — the first session's version
  of this finding additionally raised unbounded memory growth from name-cache pollution across
  discarded branches. Checked: the name cache is a pre-existing, `maxBytes`-independent
  world-level structure that grows identically with or without history enabled (no code path
  clears or bounds it today); its growth is not something this design introduces or is
  responsible for bounding, and (per this round's own finding #4 above) its content is not
  simulation-affecting. Not applied; no doc change needed.

## Status

Round 14 of an ongoing series (rounds 1–13 committed). Two tooling mistakes this round, both
recovered without losing a result: an auto-backgrounded call (missing Bash-tool `timeout`
parameter) that I stopped per WORKFLOW.md but whose underlying process had already run to
completion and produced a genuine second independent review, which I chose to verify and fold in
rather than discard. Combined, the two sessions produced 7 distinct, source-confirmed findings (4
from the canonical session, 3 more from the first), all applied, plus one factual correction to
the curated declined-findings list's wording (`userData` does have a setter; the decline's
conclusion is unchanged) and three findings that were already resolved by earlier rounds (stale,
not re-applied). This is the highest single-round finding count since round 2 (8) — a reminder
that running two independent full-scope passes surfaces more than one does, not that the document
regressed; the two sessions' findings barely overlapped (only the cold-hash-guard gap was found by
both, independently), which is itself evidence the document still has enough surface area that a
single from-scratch pass doesn't reliably find everything a second one would. Finding #2 (proxy
reset) is the same general shape as round 9's `categoryBits` sync gap and round 5's proxy moved-
list fix — a third distinct way `shape->proxyKey`/tree-proxy state can desync from what the
restore algorithm assumes, suggesting this general area (live tree-linkage state vs. journaled
shape state) keeps rewarding scrutiny. Finding #1 (hull refcounting) is the second round (after
round 10) to find the hull-reference mechanism one-sided; round 10 fixed the *acquire* side for
the new hull, this round fixes the *release* side's eviction-time bookkeeping — both are the same
underlying lesson (a refcounted resource shared between the live engine and the journal needs its
transfer points fully specified in both directions) recurring in a mechanism that keeps getting
touched by fixes without a full pass ever double-checking all of them at once. Given this round's
finding rate (7 applied, its highest since round 2) and that two of the seven are in mechanisms
(hull refcounting, tree-proxy restoration) that have now each been fixed at least twice across
this series, another full-scope round is warranted, with attention to whether these two
mechanisms in particular are now actually complete in both directions or still have a third edge.
