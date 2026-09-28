---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 30), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 30

## Review report (Codex final message)

## Summary

The hot image and cold journal design is broadly consistent with the source paths checked. I found two gaps in the document’s enumerated journal write sites. Its general journaling rules would cover these writes, but section 7.3 proposes adding hooks at the sites listed in section 5.2, so the omissions matter for implementation and verification.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | 5.2, 7.3 | The shape and body mutator inventories name `Set*` APIs but omit persistent writes made by `Enable*` APIs. `b3Shape_EnableSensorEvents`, `EnableContactEvents`, `EnablePreSolveEvents`, and `EnableHitEvents` change `shape->flags`; `b3Body_EnableHitEvents` changes the flags of every owned shape. Body APIs including `b3Body_EnableContactRecycling` and `b3Body_AllowFastRotation` also change persistent body and sim flags. These fields must follow section 7.1’s journaling rule. (`shape.c`, `body.c`, `contact.c`, `solver.c`) | Enumerate these APIs in the relevant section 5.2 rows and include awake and sleeping owner cases in the cold-hash tests. |
| 2 | Medium | 5.2, 7.3 | `b3Body_SetMassData` clears `b3_relativeTransformValid` on every neighboring contact, including contacts outside the awake image. The contact-record row does not name this write site. `b3UpdateBodyMassData` has the same contact-cache invalidation path. (`body.c`, `contact.h`) | Add both mass-update paths to the contact-record row and require a journal hook for affected cold contacts; test a mass change on a body with a sleeping contact. |

## Checked, no change

- The existing backward-scrub test compares restored and replayed hashes, while `b3HashWorldState` covers a narrower set of fields than the proposed full hash. Sections 1 and 12 distinguish these claims correctly. (`test/test_recording.c`, `recording.c`)
- Broad-phase pairs are sorted by key before contact creation, supporting section 7.4’s tree-order claim for that path. (`broad_phase.c`)
- Sensor overlap lists are sorted and deduplicated, and the task signals changed overlap contents. (`sensor.c`)
- Zero-time-step calls still run pair updates and sensors while skipping `b3Solve`, supporting section 8’s separate `historyTick` counter. (`physics_world.c`)
- The Phase 0 serializer clears body, shape, and joint `userData` on restore as section 13 states. (`world_snapshot.c`)

## Proposed edits

1. In section 5.2, expand the body and shape rows to name the `Enable*` and other non-`Set*` mutators above. In section 12, add test churn for their flag changes across awake and sleeping states.
2. In section 5.2’s contact row, name contact-cache invalidation by `b3Body_SetMassData` and `b3UpdateBodyMassData`. Add a restore and replay test that checks the affected cold contact.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T08:06:50+10:00, end
2026-09-29T08:10:08+10:00, `EXIT_CODE 0`, returned inline in about 3 minutes 18 seconds. The raw log
had the final `## Review report` block duplicated once (a known `codex exec` streaming artifact,
identical content both times); one copy is kept above. Codex ran read-only; the raw log has no
write/patch attempt of any kind, and the only working-tree changes present are this session's own
pre- and post-prompt edits accounted for below — confirming Codex made no file changes itself.

**Declined-findings list used for the prompt.** Before writing the prompt, a dedicated audit of the
whole series (rounds 1–29) was run to reconstruct the curated declined-findings list, since round
29's file claimed a "corrected nine-item" list without ever enumerating it, citing round 28's
`## Status` section as its source for a count that section never states. Round 28's own
`## Post-review verification (Claude)` section (not its Status section, which round 29
misattributed) had already correctly traced the list from round 18 (10 items) through round 21 (9
items) down to a 7-item list carried by rounds 24–27 and round 28 itself, confirming the 2 items
dropped between round 21 and round 24 were dropped because the doc now answers them directly, not
lost. The audit reconfirmed that reasoning (all 4 of the doc-answerable items from round 28's list
were independently re-checked against the doc's current text and still read correctly), so 4 of
round 28's 7 items came off the list again this round for the same reason: sleep→wake AABB bounds,
proxy numeric-id reconstruction, the allocation-free-restore half of `maxBytes`, and state-hash
joint/solver-array storage order — each is now answered by the doc's own current text (§5.2/§7.1,
§7.4, §1/§9, and §9/§12 respectively). The audit also surfaced one previously-tracked decline that
had silently dropped out of the curated list at round 15 without documented reason: the name cache
(`name_cache.c`)'s unbounded growth, declined in round 14 as pre-existing, `maxBytes`-independent
engine behavior this design doesn't worsen or need to bound. Since that reasoning is correct and the
doc did not yet say so, the fix was to add it to the doc's own text (§10's name-cache paragraph,
this session, before writing the round 30 prompt) rather than keep carrying it as external context —
per WORKFLOW.md step 2's exception, this closes it out of the curated list entirely rather than
carrying it forward as a 5th item. Round 29's "nine-item" claim itself could not be substantiated by
anything in round 29's own file and is treated as a documentation error in that file (see the
audit's reconciliation, quoted in full in this session's own working notes — not reproduced here per
WORKFLOW.md's citation-not-narration rule for round files). The round 30 prompt therefore carried
exactly 3 items: §11.1's citation into `docs/designs/reviews/` (unverifiable under Codex's own
review-scope restriction, not a doc defect), §7.4's CCD "Change:" paragraph's two equivalent
phrasings, and §12's unqualified "pool state" wording covering `nextIndex`. Codex did not re-raise
or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Checked whether every
mutation site of the graph-colour `bodySet` bitsets matches §5.2's own citation
(`constraint_graph.c:107-140, 181-182, 237-268, 312-313; solver_set.c:353-354, 416-417`) by grepping
every `b3SetBit`/`b3ClearBit`/`b3SetBitGrow` call against `color->bodySet` in both files. Found one
additional call site, `constraint_graph.c:55` (`b3SetBitCountAndClear`), not in the cited ranges —
traced it to `b3CreateGraph`, the constraint graph's one-time construction at world creation, before
any state exists to journal against. Not a gap: journaling exists to make a *later* state
reconstructible from an *earlier* one, and there is no earlier state at world construction. No doc
change needed; recorded here as the round's pre-check rather than left unchecked.

Both findings verified directly against source:

- **#1 (§5.2 names the shape mutator family only as `b3Shape_Set*`, and the body mutator family only
  as `b3Body_Set*` plus a short explicit list, but neither name covers the `Enable*`/`AllowFastRotation`
  family that also writes cold `flags` fields unconditionally), CONFIRMED, applied.** Read
  `b3Shape_EnableSensorEvents`, `EnableContactEvents`, `EnablePreSolveEvents`, `EnableHitEvents`
  (`shape.c`) in full: each writes `shape->flags` unconditionally, with no awake-state check,
  matching the shapes-fields row's own "flags" entry. These four fall inside the row's already-cited
  `shape.c:1141-1684` range, so the range was correct but the named function family wasn't — fixed by
  adding `/Enable*` to the row's function-family name. Read `b3Body_AllowFastRotation` and
  `b3Body_EnableContactRecycling` (`body.c`) in full: each writes `body->flags` unconditionally and
  falls inside the bodies-row's cited `body.c:1601-2512` range — fixed the same way, appending
  `/Enable*`/`AllowFastRotation` to that row's function-family name, no range change needed. Read
  `b3Body_EnableHitEvents` (`body.c`) in full: unlike the two above, it writes every owned *shape's*
  `flags` field, not the body's own record, and its line span (`body.c:2524-2538`) falls entirely
  outside every currently-cited range in either row — fixed by adding it, with its own citation, to
  the shapes-fields row (the row matching what it actually mutates), not the bodies row (matching the
  API it's named after).
- **#2 (§5.2's `contacts[id]` row's mutator list omits `b3UpdateBodyMassData`/`b3Body_SetMassData`,
  both of which clear `b3_relativeTransformValid` on every one of the body's neighbour contacts,
  awake or not), CONFIRMED, applied.** Read both functions (`body.c:895-1055`,
  `1856-1945`) in full: both walk the body's `headContactKey` edge list unconditionally and clear the
  bit on every neighbour `b3Contact` record, with no awake-state check on the contact — a genuinely
  missed site for a non-awake neighbour. `b3UpdateBodyMassData` was already named in the *bodies[id]*
  row (its effect on the body's own record), but this second, distinct effect on neighbour *contact*
  records was not named in the contacts row, and `b3Body_SetMassData` wasn't named in either row for
  this effect. Fixed by adding both functions, with their own line citations, to the contacts row.

## Status

Round 30 of an ongoing series (rounds 1–29 committed). Two Medium findings, both genuine and
applied, in the same general territory (§5.2's mutator-site inventories) that rounds 24–27 also
found gaps in, but a specific slice those rounds didn't cover: non-`Set*`-named API families
(`Enable*`/`AllowFastRotation`) and a setter's *secondary* effect on a different structure's row
(mass-data changes touching contact records, not just the body's own). Neither finding is a
consequence of the immediately preceding round's own fix (round 29 touched §8; this round's findings
are in §5.2's existing rows, last substantively touched by round 24-27's own inventory work) and
neither reopens ground the doc's own rationale already covered — both are first-time findings in a
completeness dimension (named-function-family coverage vs. cited-line-range coverage can diverge, as
found here) the series had not previously scrutinized this specifically. This round also closed out
one previously-untracked decline (name-cache growth, recovered by this round's declined-list audit)
by writing its rationale into the doc rather than continuing to carry it as external context, and
corrected round 29's unenumerated and unsubstantiated "nine-item" claim back down via an independent
re-audit, landing at the same conclusion round 28's own audit had already reached.

Per the user's instruction, this batch is capped at 2 rounds (30–31) or sooner on `CONVERGED`. This
round's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, so round 31 follows next, at the same full
scope, per WORKFLOW.md step 8.

Round 30: 2 findings (0H-2M-0L), 2 applied, 0 declined
Series total: 108 findings (41H-51M-16L) across 30 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L)
