---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 35), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 35

## Review report (Codex final message)

## Summary

The design's state inventory and replay approach are broadly consistent with the source. I found one implementation gap: the proposed ring layout does not say where journal entries are written while a tick is in progress.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | Medium | Sections 4, 6, 7.3, 8 | A slot is laid out as header, image, then journal, and section 6 reserves the slot at the *end* of `b3World_Step`. But journal hooks must append entries during the step and during API calls before it; `physics_world.c` performs structural work before the proposed capture point. The image size is known only at capture, so the document does not define where those entries live or how they become the slot's trailing journal segment. | Specify the in-progress segment's storage and how it is finalized into a slot, including inter-step API calls and arena growth. |

## Checked, no change

- The existing recording hash covers body transforms and velocities, so the document correctly limits what `ScrubBackward` demonstrates (`recording.c`, `test/test_recording.c`).
- Zero-time steps still run pair and sensor work while skipping `b3Solve`; the separate `historyTick` rule is warranted (`physics_world.c`, `solver.c`).
- Sensor overlap changes are detected by the sensor task's own bit, independent of the owning body's awake state (`sensor.c`).
- Proxy category bits can diverge from a shape's current filter, supporting their separate journal entry (`shape.c`, `broad_phase.c`).
- Shape geometry setters recreate proxies, while `b3Shape_SetHull` can return without doing so when the installed hull is unchanged (`shape.c`).

## Proposed edits

1. In section 8, define a provisional journal segment that exists before the next step and accepts inter-step and in-step writes. State whether it uses a separate growable buffer that is appended at capture, or a journal-first provisional slot whose finalized layout differs from the current header/image/journal description. Update sections 4 and 6 to match, and account for its allocated capacity in `bytesUsed`.

## Unresolved / disagreements

The document and source do not determine which provisional storage layout is preferable; either can resolve finding 1.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T08:48:58+10:00, end
2026-09-29T08:52:06+10:00, `EXIT_CODE 0`, returned inline in about 3 minutes 8 seconds. The raw log
had the final `## Review report` block duplicated once (a known `codex exec` streaming artifact,
identical content both times); one copy is kept above. Codex ran read-only; the raw log has no
write/patch attempt of any kind, and the only working-tree changes present are this session's own
round 34 edits plus this round's own post-review edits accounted for below — confirming Codex made
no file changes itself.

**Declined-findings list used for the prompt.** Unchanged from round 34 (round 34 found 0 new
declines — its one finding was applied): §11.1's citation into `docs/designs/reviews/`, §7.4's CCD
"Change:" paragraph's two equivalent phrasings, and §12's unqualified "pool state" wording already
covering `nextIndex`. Codex did not re-raise or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Checked §7.1's manifold-block
trigger list ("create, destroy, sleep, wake, merge, split, transfer") against source: grepped
`b3CreateContact`/`b3DestroyContact` (`contact.c`) and the wake/sleep/merge/split transition code in
`solver_set.c` (its `contactIndices` array manipulation at each transition). All six named transitions
correspond to real code paths; no seventh transition or missing case found. This check did not
anticipate this round's actual finding, which is about physical write-ordering in the ring rather
than a missing case in an enumerated list.

The finding verified directly against the doc's own text:

- **(§8 described a slot's physical byte layout as "a header, the image, the journal segment,"
  written in that order, while §6 step 1 only reserved a slot "at the end of `b3World_Step`," sized
  from awake counts known only then — but journal hooks (§7.3) append entries throughout the step,
  and §4 says a segment also holds "the API calls between step t−1 and step t," which happen before
  step t starts; the doc never said where those entries physically live before the slot housing them
  exists, nor how they end up as that slot's trailing journal region once it does), CONFIRMED,
  applied.** Re-read §4, §6, §7.1, §7.3, and §8 together: §7.3 already uses the phrase "the currently
  open segment" as something a hook writes into mid-step, and §9's restore procedure already refers
  to "the open segment described in §4" as something that can hold entries before a step completes —
  so the design already assumed a segment could be written to before the slot's own reservation, it
  just never said what the segment's storage was during that window, and stated the slot's finished
  byte order in a sequence (header, image, journal) that is exactly backwards from the order its own
  three parts actually become known and available to write (journal bytes accumulate continuously
  from the moment the *previous* capture's step 4 closes; the header and image are producible only
  once, at the current step's end, when awake counts are finally known). This is a genuine gap in a
  design that is otherwise precise about byte-level mechanics elsewhere (§7.1's entry-kind table,
  §7.2's cost model) — an implementer would have to invent the missing storage answer themselves, and
  a wrong invention (e.g., buffering the journal in a separate scratch allocation, when the intent
  was to append it directly into the same arena the image lands in) would silently break the `maxBytes`
  accounting §8 already goes to some length to get right. Fixed by re-describing §8's slot layout to
  match the actual write-order chronology (journal segment first, growing continuously; header and
  image appended once, after it, at step end) rather than the order the three parts were previously
  just listed in, and by rewording §6 step 1 and the "journal segment's size is not known up front"
  bullet (both already gesturing at this without stating it) to say so explicitly: the journal room
  for tick t is the same still-growing region journal hooks have been appending to since tick t−1's
  own capture closed, and the header/image append lands immediately after it, not before.

## Status

Round 35 of an ongoing series (rounds 1–34 committed). One Medium finding, genuine, applied — but
unlike rounds 32–34's pattern (a missing name in an enumerated function list), this round's finding
is the first in several rounds to be a genuine missing-mechanism gap rather than a missing-citation
gap: the doc's stated slot byte-order was backwards relative to when its three parts actually become
writable, not merely under-enumerated. The fix required reconciling several sections that already
half-implied the correct model (§7.3's "currently open segment," §9's "open segment described in
§4") without any of them stating where that segment's storage lives before its owning slot exists —
so this is also the first round to resolve a finding primarily by tying together phrasing already
scattered across multiple sections, rather than by adding a wholly new fact. Not a consequence of
round 34's own fix (round 34 touched §5.2's proxy categoryBits row, disjoint from §4/§6/§7.3/§8) and
does not reopen ground the doc's own rationale already covered — this is new territory, the ring's
own physical write-ordering, which no prior round had examined at this level of mechanical detail.

Per the user's instruction, this batch was capped at 2 rounds (34–35) or sooner on `CONVERGED`.
Round 35's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, and the batch limit is now reached, so
this session stops here per that instruction; the series remains open, and whenever review next
resumes it should continue at the same full scope as every round so far, not a narrower one —
especially given this round found a genuine mechanism gap in the ring's core write path, an area
several dozen prior rounds had reviewed without surfacing this particular ordering problem.

Round 35: 1 finding (0H-1M-0L), 1 applied, 0 declined
Series total: 116 findings (42H-57M-17L) across 35 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L) -> 2 (0H-1M-1L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L) -> 1 (0H-1M-0L)
