---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 36), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 36

## Review report (Codex final message)

## Summary

The source supports the design’s broad hot/cold split, including its treatment of awake contacts, sensor overlaps, and dense solver sets. I found two gaps in derived-tree restore, one verification gap, and two smaller inconsistencies in the memory and caller-visible-state contracts. These leave the proposed bit-exact guarantee unproven.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §§5.2, 6, 7.4, 9 | Restore can lack a proxy’s **historical category bits**. `b3Shape_SetFilter(..., false)` changes the shape filter while leaving the live proxy category unchanged. The enable-time image does not capture proxy categories, and proxy destruction does not journal the category being removed. Rewinding across a disable or destruction can therefore require a category value that neither the image nor journal holds. `shape.c`, `body.c`, `dynamic_tree.c` | Capture each existing proxy’s category at history enable, and journal its old category whenever a proxy is removed or replaced. Define how restore reads that value when recreating a proxy. |
| 2 | High | §§2, 7.4, 10 | The document identifies an unresolved closest-ray tie for mesh and height-field shapes, yet still promises bit-exact replay with unchanged inputs. Their internal casts can suppress a hit tied with the current limit before the proposed shape-id tie-break sees it; a caller can use that result as later input. `mesh.c`, `height_field.c`, `physics_world.c` | Make the internal casts report tied hits so the shape-id rule can decide them, or narrow the bit-exact contract explicitly until that change is implemented. |
| 3 | Medium | §§9, 12 | The proposed hash and validators can miss an incorrect tree-leaf AABB. The hash includes shape bounds but not the corresponding tree-leaf bounds; `b3DynamicTree_Validate` checks tree consistency without comparing leaves to world shapes. A missed `MoveProxy` could pass restore-hash equality and validation while changing later queries or pair discovery. `dynamic_tree.c`, `broad_phase.c`, `physics_world.c` | Add a restore test or validator that checks every live shape’s proxy presence, tree type, user shape id, category, and leaf AABB against its restored state. |
| 4 | Medium | §8 | The budget counts **allocated arena capacity**, but widening the capture interval and evicting slots do not release capacity already reserved by amortized growth. Usage can remain above `maxBytes` after the exceptional minimum window no longer requires it. `container.h`, `recording_replay.c` | Specify arena shrinking or compaction after eviction, or state that `maxBytes` is a high-water allocation target rather than a continuing cap. Test the budget after a temporary large capture. |
| 5 | Low | §§5.3, 10 | `maxCapacity` is persistent, caller-visible high-water state, rather than a counter reset before each use. The inventory omits it, so rewind can report capacity reached only on the discarded future timeline. `physics_world.c`, `physics_world.h`, `world_snapshot.c` | Image and restore `maxCapacity`, or disclose in the caller contract that this diagnostic retains future high-water values. |

## Checked, no change

- Awake contacts are found through the awake contact-index array and graph colors; their records and manifold data are used by the narrow phase. `physics_world.c`, `contact.c`, `constraint_graph.c`
- Sensor overlap changes are compared by visitor id and generation, and the sensor task sets its change bit when those logical contents differ. `sensor.c`
- Sleeping-set wake and sleep paths transfer dense arrays and update referencing body, contact, joint, and island records, matching the journal inventory’s need for both array and record entries. `solver_set.c`
- Broad-phase pair keys are sorted before contact creation, and the explicit tree moved bits feed the next pair update. `broad_phase.c`, `dynamic_tree.c`
- The separate `historyTick` rationale is sound: a zero-time step still runs pair and sensor work, while the solver advances `stepIndex` only when it runs. `physics_world.c`, `solver.c`

## Proposed edits

1. Add enable-time proxy-category state and a removal/replacement journal rule to §§5–7; update §9’s recreation procedure to use it.
2. Resolve the mesh and height-field tied-hit behavior before claiming the §2 guarantee; update §7.4 and §10 together.
3. Extend §12 with a world-to-tree leaf check that exercises rewind in both directions.
4. Give §8 an explicit capacity-reclamation rule and a test for recovery after a temporary memory spike.
5. Classify `maxCapacity` in §5 and specify its post-rewind behavior in §10.

## Unresolved / disagreements

No additional issue required resolution outside the document and source. I did not run code; this is a source-based design review.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. **First attempt was misconfigured**: the Bash tool call
omitted the explicit `timeout: 600000` parameter, defaulted to 120000ms, and the harness
auto-backgrounded it — per WORKFLOW.md this is proof of misconfiguration, not a benign "still
running" state, so it was killed immediately via `TaskStop` without waiting for its notification,
and the call was reissued from scratch with `timeout: 600000` set. The reissued call: start
2026-09-29T09:05:46+10:00, end 2026-09-29T09:10:38+10:00, `EXIT_CODE 0`, returned inline in about 4
minutes 52 seconds. The raw log had the final `## Review report` block duplicated once (known
`codex exec` streaming artifact, identical content both times); one copy is kept above. Codex ran
read-only; the raw log has no `apply_patch`/write/patch tool call of any kind, and `git status`
after the run shows only this session's own doc edits — confirming Codex made no file changes
itself.

**Declined-findings list used for the prompt.** Unchanged from rounds 34–35 (round 35 found 0 new
declines — its one finding was applied): §11.1's citation into `docs/designs/reviews/`, §7.4's CCD
"Change:" paragraph's two equivalent phrasings, and §12's unqualified "pool state" wording already
covering `nextIndex`. Codex did not re-raise or dispute any of the three. The list is unchanged
going into round 37: none of this round's five findings were Codex misreading the source or
relitigating settled ground (see below), so none is added to it.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Two checks, neither of
which anticipated this round's actual findings:
- Checked whether other hot-while-awake, cold-while-asleep fields besides `shapes[id].aabb`/
  `fatAABBs` need the same wake/sleep checkpoint treatment §5.2 gives that pair. `bodies[id]`,
  `bodySims`/`bodyStates`, `jointSims`, `contactIndices`, `islandSims` are all part of a solver
  set's own dense arrays, so a wake/sleep transition's append/swap-remove dense-cold-array entry
  (§5.2's "Swap-compaction on removal" paragraph, §7.1's dense-cold-arrays entry kind) already
  captures the moved element's content as part of the array move itself — unlike `aabb`/
  `fatAABBs`, which live on the shape record, not the solver set's arrays, and so need the
  separate checkpoint. No gap found.
- Checked §12 point 4's cold-hash guard enumeration against every row of §5.2's journaled-structure
  table for coverage. Every row maps onto the guard's list (solver sets → "sleeping sets"; bodies/
  bodySims/bodyStates/contacts/jointSims → "non-awake records"; islands and link arrays; shape/
  joint fields journaled regardless of awake state; non-awake shape bounds; pools; pair set;
  colour bitsets; tree proxy `categoryBits`; sensor existence; sensor overlaps), with the one
  documented exception (tree proxy reset, excluded and separately covered per §12 point 4's own
  text) already called out. No gap found.

Codex's actual findings were about a different axis entirely — proxy-category recoverability across
a plain (non-recreating) proxy destroy, and validation/documentation completeness — which these two
checks did not target.

The findings verified directly against the doc and source:

- **(Finding 1: proxy `categoryBits` unrecoverable across a plain destroy), CONFIRMED, applied.**
  Traced every caller of `b3DestroyShapeProxy` (`shape.c:1016`): `b3ResetProxy` with
  `destroyProxy=true` (`shape.c:1365`, already journaled per §5.2's existing categoryBits row,
  since it immediately recreates the proxy), plus three call sites that destroy without recreating
  — `b3DestroyBody` (`body.c:412`, permanent shape+body destroy), `b3DestroyShapeInternal`
  (`shape.c:509`, single-shape destroy), `b3Body_Disable` (`body.c:2236`, transfer to the disabled
  set). None of these three is in §5.2's categoryBits row's trigger list, which names only
  `b3CreateShapeProxy` and `b3ResetProxy(destroyProxy=true)`. Constructed the failure: a proxy is
  created (category A = filter at creation, outside or before the retained window); an
  `invokeContacts=false` `b3Shape_SetFilter` call changes `shape->filter.categoryBits` to B without
  triggering a reset, so the live proxy's category silently diverges to a stale A (this divergence
  is exactly what the existing row's own text already anticipates, and it is already
  correctly journaled via the generic shape-record entry for `filter`); at tick T (inside the
  window) the proxy is still live with the diverged category A; later the shape is destroyed via
  one of the three uncovered paths, with no categoryBits journal entry. Rewinding to T now needs
  the proxy re-created with category A (§7.4's Presence bullet, since T predates the destroy), but
  the only available source is `shape->filter.categoryBits` at T, which is B, not A — a genuine
  bit-exactness violation, since the recreated proxy's tree category would be wrong and could
  change pair discovery on the corrected timeline. This is a real mechanism gap, not a
  documentation gap: no existing row or bullet covers it, and it survives even a `filter` change
  that predates history being enabled at all. Fixed by extending §5.2's categoryBits row to also
  journal on these three destroy-without-recreate sites (old value only, since §7.4's Presence
  bullet — not this field — governs whether the proxy exists at all after a plain destroy), so a
  backward walk across such a destroy recovers the pre-destroy category the same way it recovers
  any other record field's pre-write value.
- **(Finding 3: no direct check that §7.4's tree reconstruction actually ran for every shape),
  CONFIRMED, applied.** §12 point 1 confirmed to hash `shape->aabb`/`fatAABBs` but never a tree
  leaf's own bounds (by §5.4's explicit design choice — trees are derived precisely so the ring
  doesn't pay to image them, and duplicating their bytes into the hash would partially undo that).
  A missed `b3DynamicTree_MoveProxy` or `SetCategoryBits` call in §7.4's reconstruction would leave
  a stale leaf that only affects the state hash indirectly, through whatever later broad-phase pair
  or query result it happens to change — a replayed range with no query that happens to touch the
  stale region would pass tests 2 and 3 (§12) despite the bug. This is a genuine verification-plan
  gap, distinct from the tree layout itself being simulation-irrelevant (§7.4 already establishes
  that; the gap is about testing whether the *algorithm* ran correctly, not about whether tree
  layout matters). Fixed by adding a direct validation-build assertion (§9 step 6) comparing every
  live proxy's tree-leaf bounds and category against the shape's own just-restored values.
- **(Finding 5: `maxCapacity` omitted from the doc's inventory, no stated rewind behavior),
  CONFIRMED, applied — as a rationale gap, not a mechanism defect.** Confirmed `world->maxCapacity`
  (`physics_world.c:1080`, updated every step; `physics_world.c:2282`,
  `b3World_GetMaxCapacity`) is a monotonic high-water mark over shape/body/contact counts, never
  read by any simulation code — it is diagnostic, the same category as `userData` and
  `nodeVisits`/`leafVisits`, both of which the doc already explicitly excludes from restore with
  stated reasoning (§5.1, §10). `maxCapacity` was the one member of that category the doc never
  actually named, so a fresh reviewer had no way to know its omission from §5.1's imaged-scalars
  row was deliberate rather than an oversight. Per WORKFLOW.md step 5's rule that required
  rationale is a fix like any other: added an explicit `b3World_GetMaxCapacity` bullet to §10
  alongside the existing `nodeVisits`/`leafVisits` one, stating the same non-simulation-affecting
  reasoning and its caller-visible consequence (a post-correction peak can reflect ticks the new
  timeline never reaches).

Two findings revisited ground the doc's rationale already covers, and neither cites new evidence
disputing it (this round's reversal-check, WORKFLOW.md step 5's third bullet); both were declined
in substance but their re-raise showed the existing rationale wasn't reachable enough from where a
fresh reviewer would look, so both are fixed as pointer/clarification additions rather than left
unaddressed or added to the carried-forward declined list (WORKFLOW.md step 2's first decline
case: the doc was right and should have said so more visibly):

- **(Finding 2: mesh/height-field ray-tie leaves req 1's bit-exactness "unproven"), substance
  declined, rationale sharpened.** §7.4 and §10 already state this exact residual gap in detail —
  §7.4: "a residual order dependence on top of the two §10 already documents, not eliminated by
  this change"; §10: "a caller using its result as a later input inherits that same residual order
  dependence" — using the identical caller-contract pattern (a callback or query result the caller
  must not feed back into simulation input) already applied to the two other documented residual
  order-dependences in the same section. Codex's finding restates the gap without disputing this
  existing scoping or citing evidence it's insufficient. The doc was right that req 1 holds under
  the caller-contract conditions §10 states, but requirement 1 itself, read in isolation, doesn't
  point there — sharpened by adding a direct cross-reference from requirement 1 to §10's scoping,
  so the connection is visible at the point the guarantee is first promised rather than only
  discoverable by separately reading §7.4 and §10 in full.
- **(Finding 4: arena capacity never shrinks after a spike), substance declined, rationale
  sharpened.** §8 already states capacity growth is realloc-based and "rare after warm-up," and §2
  requirement 5 already accepts the minimum window's storage exceeding `maxBytes` as a named
  exception — both already establish that this is an amortized-growth arena whose usage is a
  high-water figure by design, not a promise of shrink-after-spike; nothing in requirement 5 is
  about post-spike compaction, only about not rejecting a capture under pressure. Codex's finding
  doesn't dispute this, it asks the design to go further (add compaction) without engaging why the
  existing accepted-overage language was judged sufficient. The doc was right not to promise
  compaction, but never said outright that capacity is monotonic absent an explicit disable/
  re-enable cycle — sharpened by stating that directly next to the existing accounting text, with
  the recovery path (`b3World_DisableHistory`/`b3World_EnableHistory`) named.

## Status

Round 36 of an ongoing series (rounds 1–35 committed). Five findings, all genuine in the sense
that each identified something the doc didn't already say adequately — two (1, 3) were real
mechanism/verification gaps requiring new design content, not just a missing sentence, breaking
the pattern of rounds 32–35 (missing citations or a single ordering gap): finding 1 in particular
is the second round in a row (after round 35's journal-storage-timing gap) to surface a genuine
"how does this actually work" question in a section — the ring's tree/proxy reconstruction — that
many dozens of prior rounds have reviewed without finding it, suggesting the derived-tree
machinery (§7.4) rewards continued full-scope scrutiny rather than being settled. The other three
(2, 4, 5) were rationale/documentation-completeness gaps the doc's own governing rule (WORKFLOW.md
step 5) treats as fixes rather than no-ops: two were the doc having the right answer without a
visible pointer to it (declined in substance, rationale sharpened, per the reversal-check), one
was an omission from an established exclusion pattern (`userData`, `nodeVisits`/`leafVisits`) the
doc had simply never extended to a fourth field that fits it. None of the five is a consequence of
round 35's own fix (round 35 touched §4/§6/§7.3/§8's journal-segment write-ordering, disjoint from
proxy categories, tree-leaf validation, arena capacity semantics, and `maxCapacity`) and none
reopens ground the doc's own rationale already covered in a way that failed the reversal-check —
findings 2 and 4 reopened ground the rationale covers, but neither disputed it with new evidence,
so both stayed declined in substance while the doc gained a sharper pointer, per WORKFLOW.md's
rule that a reversal-check failure should still prompt sharpening when the re-raise shows the
existing text wasn't reachable enough.

Per the user's instruction, this batch is capped at 2 rounds (36–37) or sooner on `CONVERGED`.
Round 36's verdict is `CHANGES_PROPOSED`, so round 37 runs next, at the same full scope as every
round so far.

Round 36: 5 findings (2H-2M-1L), 5 applied, 0 declined
Series total: 121 findings (44H-59M-18L) across 36 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L) -> 5 (2H-2M-1L)
