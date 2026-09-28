---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 15), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 15

## Review report (Codex final message)

## Summary

The whole-world rewind approach is broadly consistent with the source, but the hull reference rule can leak references, and the caller contract needs two clarifications for bit-exact replay. I found one inaccurate allocation claim in the restore procedure. I did not read `docs/designs/reviews/`.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.1 | Hull eviction retains an extra reference for the shape’s current hull, although the shape’s original reference was never released by undo or redo. A hull replacement followed by eviction can therefore leak a reference; after a rewind, the original reference may also remain on a hull the shape no longer points to. `shape.c`, `physics_world.c` | Transfer the shape’s database reference when undoing or redoing a hull pointer change. Keep journal references separate and release **both** when the entry is evicted. |
| 2 | Medium | §10 | Keeping caller-owned mesh and height-field buffers *alive* does not preserve their contents. The engine stores their pointers and reads the pointed-to geometry during collision; an in-place caller edit cannot be undone by the proposed pointer journal. The same concern applies to geometry referenced by a baked compound. `shape.c`, `mesh_contact.c`, `box3d.h` | Require referenced geometry to remain unchanged while any retained tick may use it, or retain versioned copies. |
| 3 | Low | §§7.4, 10 | Tree reconstruction can change the public `nodeVisits` and `leafVisits` fields returned by the closest-ray query, even with the proposed hit tie-break. The caller contract mentions callback traversal order but not these diagnostic results. `dynamic_tree.c`, `physics_world.c`, `types.h` | State that query visit counts may differ after rewind and must not drive replayed simulation inputs. |
| 4 | Low | §9 | The repeated “resize (not reallocate)” instruction is not guaranteed: `b3Array_Resize` reserves capacity and may grow its allocation when restoring larger awake, island-link, or triangle-cache arrays. `container.h` | Say “resize, allocating if capacity is insufficient,” and keep restore allocation separate from §8’s capture claim. |

## Checked, no change

- The existing state hash covers transforms and velocities rather than the proposed full state. `recording.c`
- Sensor overlap changes are detected from the sensor task’s own comparison, including sensors on non-awake bodies. `sensor.c`
- A proxy reset can replace `shape->proxyKey` without a corresponding change in bounds or body type, supporting the separate reset entry. `shape.c`
- The closest-ray callback currently overwrites an equal-fraction result, as §7.4 states. `physics_world.c`
- Id allocation uses a free list followed by a bump cursor, as §3 and §7.1 describe. `id_pool.c`

## Proposed edits

1. Replace §7.1’s hull eviction rule with explicit accounting for the shape’s reference and each journal-held reference, including undo, redo, eviction, and history disable.
2. Add the geometry immutability requirement to §10 beside its lifetime requirement.
3. Add query visit counts to §10’s tree-layout caveat.
4. Correct §9’s resize wording for all three array cases.

## Unresolved / disagreements

None. I did not find grounds to reopen the previously declined findings.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T21:23:34+10:00, end 21:27:18+10:00, `EXIT_CODE 0`. As with round 14, this round's raw
log inherited pre-existing, unrelated content at the top from an earlier abandoned session that
happened to reuse the same filename before this conversation started (a different, older session
id, different prompt wording); the content from my own invocation's session id onward, kept above,
is what this round's findings are drawn from, verified by locating my own prompt's distinctive
text within it. Codex ran read-only; working tree was untouched by it going in (round 14's edits
were already applied to the working file). The curated declined-findings list from rounds 1–7
(nine items, plus this round's own item 10 for the pool-`nextIndex` decline from round 14, and
item 2's and item 6's wording corrected per round 14's own findings) was included in the prompt;
Codex neither re-raised nor disputed any of them.

All four findings verified directly against source:

- **#1 (hull eviction leaks/dangles because undo and redo were only raw pointer copies, so the
  shape's own database reference never actually followed the pointer across a scrub), CONFIRMED,
  applied — corrects round 14's own fix.** Round 14 fixed the *symptom* (asymmetric release at
  eviction, keyed off the shape's current live value) without fixing the *cause* this finding
  identifies: leaving undo/redo as plain pointer copies. Traced the consequence of round 14's own
  wording through a second hull change: shape does A→B (entry1 takes extra refs on A, B), then
  B→C (entry2 takes extra refs on B, C). If entry1 evicts first, round 14's rule ("release
  whichever hull isn't the shape's *current* live value") compares against C (the shape's present
  value, two changes later) — matching neither A nor B — so entry1's eviction logic has no correct
  answer for either of its own two hulls; the rule only ever worked for the single-change case it
  was reasoned through. Confirmed `b3AddHullToDatabase` (`physics_world.c`) deduplicates by
  content via a hash-map lookup, bumping an existing entry's count rather than cloning, so calling
  it with the already-live, extra-ref-protected pointer the journal is holding is cheap and
  correct, not a fresh allocation. Fixed by making undo/redo of a hull-changing entry actually
  call `b3AddHullToDatabase`/`b3RemoveHullFromDatabase` on the shape's `hull` field, mirroring
  exactly what the original `b3Shape_SetHull` call did — so the shape's real database reference
  always tracks its current live value, scrub after scrub, the same invariant the engine
  maintains outside history — and simplifying eviction back to releasing both extra references
  unconditionally, since their only remaining job is to stop one of those real releases from ever
  hitting true zero while the entry is still retained (a floor, not a substitute for the real
  reference). This is a cleaner mechanism than round 14's, not merely a patch: it needs no
  "current live value" comparison at eviction at all, and generalizes correctly to any number of
  chained hull changes instead of only the case that motivated it.

- **#2 (caller-owned mesh/height-field/compound geometry buffers are only required to stay
  *alive*, not unchanged, so an in-place content edit silently breaks bit-exactness for any
  retained tick that references the shape through that pointer), CONFIRMED, applied.** Confirmed
  `b3Shape_SetMesh` and its siblings (`shape.c`) store only the caller's pointer, and collision
  code (`mesh_contact.c`) reads through it live at solve time — there is no engine-side copy for
  this geometry class, unlike a hull (§7.1, engine-owned refcounted database). §10's existing
  wording required only that the buffer not be freed or replaced without the pointer itself being
  journaled; it said nothing about the bytes *at* that pointer staying constant. Since a rewind to
  an earlier tick re-enters collision code that dereferences the *current* pointer value, an
  in-place caller mutation (e.g. streaming new heightfield data into the same buffer) changes what
  an earlier, supposedly-frozen retained tick collides against — the same class of hazard as
  freeing it, just not caught by a dangling-pointer crash. Fixed by extending the requirement to
  "alive and unchanged."

- **#3 (`nodeVisits`/`leafVisits` diagnostic counts on a query result can differ after a
  restore-rebuilt tree, undocumented), CONFIRMED, applied.** Confirmed these are real, public
  fields (`include/box3d/types.h`), incremented during tree traversal (`dynamic_tree.c`) and
  aggregated into query results (`physics_world.c`) — and, notably, already serialized by the
  existing recording system (`recording.c`/`recording_replay.c`), confirming the engine already
  treats them as caller-visible result data, not internal scratch. Since §7.4 already establishes
  that tree layout is a performance property restore may reconstruct differently, these counts —
  a direct function of traversal shape — can genuinely differ post-restore even though the query's
  actual *result* (which shape, which fraction) does not. Fixed by adding this caveat next to the
  existing traversal-order caveats in §10, with the same "don't feed it back into simulation
  input" framing already used there.

- **#4 (the repeated "resize (not reallocate)" instruction in §9 overstates what
  `b3Array_Resize` guarantees), CONFIRMED, applied.** Read `b3Array_Resize`/`b3Array_Reserve`
  (`container.h`): `Resize` calls `Reserve`, which reallocates via `b3GrowAlloc` whenever the
  requested count exceeds the array's *current* capacity — true on any restore that needs an
  awake/island-link/triangle-cache array larger than it has ever been before (e.g. redoing forward
  into a tick with more awake bodies than the array was last sized for). This doesn't violate any
  requirement — restore was never claimed allocation-free, only capture (already established by
  round 1's declined finding #3 and round 3's fix scoping §1's "allocation-free" claim to image
  *writes*) — it was simply an inaccurate description of the mechanism. Fixed by describing what
  `Resize` actually does (grows the allocation only when needed, never shrinks it) instead of
  asserting it never reallocates, at all three occurrences.

## Status

Round 15 of an ongoing series (rounds 1–14 committed). All four findings genuine and applied — the
highest-confidence round in a while, with zero declines and zero stale-finding noise (round 14 had
three of the latter, from the accidental second session; this round's single from-scratch session
had none). Most notably, finding #1 **overturned round 14's own fix** for the same underlying
hull-reference mechanism rather than finding a new edge of it: round 14 correctly identified the
*symptom* (eviction can free a hull the live shape still uses, or leak the other) but reasoned to
an eviction-time patch ("release whichever isn't the current live value") that only worked for a
single hull change and silently broke under a second one — round 15, reviewing the *fixed* text
from scratch with no memory of why round 14 wrote it that way, re-derived the same symptom
independently and this time traced it to the actual cause (undo/redo never transferring the real
reference), producing a strictly better mechanism. This is exactly the value a fresh, full-scope
pass has over a narrower one even on text a previous round just edited: round 14 was reasoning
about the fix it had just made, not re-deriving the mechanism from the requirements up, and missed
that its own patch didn't generalize. It's now the *third* round (after round 2's acquire-side fix
and round 10's one-sided-reference fix) to touch hull refcounting, and the first to describe a
mechanism that doesn't need special-casing at eviction — worth specifically re-checking next round
rather than assuming three rounds of fixes means it's finally complete. Findings #2–#4 are smaller,
independent gaps (a caller-contract completeness gap, a documentation-only diagnostic-field gap,
and a wording overstatement) in territory this round was the first to look at closely. Given a
round immediately after a high-finding-count round (round 14: 7 applied) still found four more
genuine, source-confirmed issues — including one serious enough to overturn the previous round's
own fix — another full-scope round is warranted, with particular attention to whether the
corrected hull mechanism (undo/redo now performing real Add/Remove calls) interacts correctly with
the other places §7.1 already special-cases a heap-owned array (materials, triangle caches,
sleeping-set/island ownership transfer) that don't get this same real-reference treatment.
