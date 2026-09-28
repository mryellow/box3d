---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0, gpt-6-sol, model_reasoning_effort=medium
mode: broad (round 31), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 31

## Review report (Codex final message)

## Summary

The hot image and cold journal inventory is largely consistent with the source, and the document addresses several difficult ownership and tree-order cases. I found two correctness issues in the ring rules and one gap in the proposed validation guard.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | 4, 8, 9 | Section 4 assigns API calls between ticks *t−1* and *t* to segment *t*, but section 8 puts calls after the enable-time image into that image’s tick segment. If those calls are stored in segment *T*, a later rewind from *T+1* to *T* walks segment *T+1* and leaves the calls’ cold mutations in place. The step-boundary distinction matters because `b3World_Step` performs mutations after those calls (`physics_world.c`). | Open segment *T+1* immediately after capturing the enable-time image at *T*. Apply the same rule after every completed step; keep the current segment’s entries pending until that next step closes it. |
| 2 | Medium | 2, 6, 8 | The minimum-window and interval rules cannot both hold under the stated minimum storage calculation. With a two-tick minimum and `K > 1`, each newly oldest tick must carry an image, forcing an image on successive ticks. Keeping an older image instead also retains its additional journal segment, which the “two segments plus one image” minimum budget does not count. | Define the minimum in terms of **reachable** ticks and budget all intervening segments back to their anchor image, or require an image each tick when the budget cannot retain that anchor. State when widening `K` can actually reduce storage. |
| 3 | Medium | 7.4, 12 | The cold-hash guard is claimed to catch a missed proxy-reset entry, but a reset can change only the proxy’s numeric identity while leaving bounds, category and body type unchanged (`shape.c`, `broad_phase.c`, `dynamic_tree.c`). Section 7.4 expressly excludes `shape->proxyKey` from restored record state. Hashing the listed logical cold fields therefore cannot reveal that this semantic hook was missed. | Add a direct validation check for reset-hook coverage, such as instrumentation that compares actual reset calls with emitted entries. Narrow the cold-hash guarantee to writes observable in its shadow state. |

## Checked, no change

- The existing recording hash covers transforms and velocities only; section 12 correctly calls for a stronger oracle (`recording.c`, `test/test_recording.c`).
- Zero-time-step calls still run pair, collision and sensor work while skipping the solve; a history tick separate from `stepIndex` is warranted (`physics_world.c`, `solver.c`).
- A proxy reset can recreate a proxy without a meaningful bounds change, supporting section 7.4’s separate reset entry (`shape.c`, `broad_phase.c`).
- Sensor overlap content is sorted and compared by shape identity and generation, supporting the document’s content-based treatment (`sensor.c`).

## Proposed edits

1. In section 8, replace the enable-time open-segment sentence with an explicit *next-tick* segment rule, and align sections 4 and 9 with it.
2. In section 8, specify the anchor image and all journal segments needed to guarantee the minimum reachable window, then revise the interval-widening and over-budget rules accordingly.
3. In section 12, add a proxy-reset hook coverage check and qualify what the cold hash alone proves.

## Unresolved / disagreements

The document does not specify whether interval widening may retain an anchor image older than the advertised minimum window. That choice determines the exact budget rule for finding 2.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only "<prompt>"` (CLI default model/effort — resolved to
`gpt-6-sol`, `model_reasoning_effort=medium`), foreground, stdin from `/dev/null`, shell-level
`timeout 540`, Bash tool `timeout: 600000`. Start 2026-09-29T08:16:39+10:00, end
2026-09-29T08:19:13+10:00, `EXIT_CODE 0`, returned inline in about 2 minutes 34 seconds. The raw log
had the final `## Review report` block duplicated once (a known `codex exec` streaming artifact,
identical content both times); one copy is kept above. Codex ran read-only; the raw log has no
write/patch attempt of any kind, and the only working-tree changes present are this session's own
round 30 edits plus this round's own post-review edits accounted for below — confirming Codex made
no file changes itself.

**Declined-findings list used for the prompt.** Unchanged from round 30 (round 30 found 0 new
declines — both its findings were applied): §11.1's citation into `docs/designs/reviews/`,
§7.4's CCD "Change:" paragraph's two equivalent phrasings, and §12's unqualified "pool state"
wording already covering `nextIndex`. Codex did not re-raise or dispute any of the three.

**Pre-prompt cross-case pre-check** (WORKFLOW.md step 2's final bullet). Round 30 found that §5.2's
shape/body mutator rows under-named their own function families (`Set*` only, missing `Enable*`).
Checked whether the same gap exists in §5.2's joints row, which uses different wording ("joint
setters (all joint files)", not a `Set*`-specific name). Grepped every joint file for an `Enable*`
family (`b3DistanceJoint_EnableLimit`/`EnableSpring`/`EnableMotor` and the equivalent per joint
type across `revolute_joint.c`, `prismatic_joint.c`, `wheel_joint.c`, `spherical_joint.c`) and
spot-checked `b3DistanceJoint_EnableLimit`: it writes its `b3JointSim` field unconditionally,
matching the same hot/cold pattern round 30 fixed for shapes and bodies. But §5.2's own wording for
this row was already generic ("joint setters", not "`Set*`"), so it already covers `Enable*` by not
excluding it in the first place — unlike the shape/body rows' literal `Set*` naming, which did
exclude it. No gap; no doc change needed. Recorded here as the round's pre-check rather than left
unchecked.

All three findings verified directly against source and the document's own cross-section
consistency:

- **#1 (§4's general rule assigns API calls between step *t−1* and step *t* to segment *t* — i.e.
  to the segment of the tick those calls precede, not the one they follow — but §8's enable-time
  text said calls between `b3World_EnableHistory` and the first step enter "that tick's [T's]
  still-open journal segment", even though §8 also says the enable-time capture is "the same
  capture as §6", which closes T's segment per §6 step 4 — an internal contradiction, and not
  merely a wording ambiguity: if resolved by literally keeping T's segment open past its own
  capture, §9 step 2's forward walk "for t = P+1 up to T" would include segment T, applying those
  between-calls' mutations when landing exactly on tick T even though image T was captured before
  they happened; if resolved the other way (T's segment closes, calls go nowhere), the walk that
  rewinds P=T+1 back to T only covers "P down to T+1", never touching segment T, so a
  between-calls mutation wrongly parked in T would survive a rewind to T uncorrected), CONFIRMED,
  applied.** Read `b3World_Step` (`physics_world.c`) to confirm ordinary steps perform mutations
  after any pre-step API calls, matching §4's model for the ordinary per-step case exactly; the
  enable-time case is the same shape of boundary with no source-level special case, only the
  document's own inconsistent description of it. Fixed by stating explicitly that the enable-time
  capture closes its own segment (§6 step 4, same as any tick), and that the between-calls enter
  the *next* tick's segment instead, opened as soon as the enable-time capture closes — restating
  §4's own rule at the enable boundary instead of contradicting it.
- **#2 (§8's minimum-window storage formula — "every tick's journal segment plus that one required
  image" — counts only the window's own N ticks' segments, but the "at or before" wording already
  established (round 29) lets the required anchor image sit *older* than the window itself, in
  which case the journal segments between that anchor and the window's current ticks are also
  load-bearing, physically required to bridge the gap, and not counted by the stated formula),
  CONFIRMED, applied, but not for the literal per-tick-imaging reading Codex's issue text leads
  with.** Re-derived from the doc's own "at or before" wording (§8, as fixed by round 29): a fixed
  anchor image at tick X satisfies "at or before" for every later tick's minimum-window check for
  as long as X's own slot survives eviction — X does not need refreshing every tick just because
  the window's oldest tick advances, since X stays ≤ any later oldest-tick value once it is ≤ the
  current one. Codex's own proposed action ("or require an image each tick") captures this reading
  as one branch, but the finding's own body ("each newly oldest tick must carry an image") does
  not follow from the doc's "at or before" text and was not applied as stated. What *is* a genuine
  gap, independent of that per-tick framing: whenever the anchor is allowed to trail behind the
  window (the normal case under a widened interval), the segments between the anchor's tick and
  the window's current ticks are real, physically-retained, budget-relevant state that the
  formula's "every tick in the window" phrase does not include, since those bridging ticks are
  outside the window itself. Fixed by rewording the formula to span from the anchor's tick through
  the window's newest tick (not just the window's own ticks), and by stating the "at or before"
  single-anchor persistence explicitly so a reader does not independently arrive at Codex's
  per-tick-imaging misreading.
- **#3 (§12 point 4 lists "tree proxy reset entries" as something the cold-hash guard verifies, but
  §7.4 already states a reset can leave bounds, category, and body type all unchanged while only
  `shape->proxyKey` changes, and that field is explicitly excluded from restored record state — so
  a state hash built from the guard's own listed fields cannot differ between "reset correctly
  journaled" and "reset hook missed" in exactly that case, contradicting requirement 7's claim that
  "a missed write site fails a test rather than a review"), CONFIRMED, applied.** Re-read §7.4's
  reset-entry row and §12's hash-field list side by side: confirmed no listed hashed field (not
  `categoryBits`, not bounds, not body type, not any record field) changes as a *necessary*
  consequence of a proxy reset — §7.4's own text says so directly ("without necessarily changing
  bounds, category, or body type"). Fixed by removing "tree proxy reset entries" from §12 point 4's
  coverage list (the guard cannot verify what it lists there) and adding one sentence explaining
  the gap and that reset-hook coverage needs its own separate check — the same rationale-not-fix
  content this series' step 5 already requires when a decline or a gap needs the doc to say why.
  Also amended requirement 7's own "a missed write site fails a test" claim with the same
  exception, so the requirements section doesn't overstate what §12 actually delivers.

## Status

Round 31 of an ongoing series (rounds 1–30 committed or pending commit). One High and two Medium
findings, all three genuine, all three applied — but two of the three (#1, #3) needed independent
re-derivation from the doc's own stated rules before applying, since Codex's given rationale for #2
did not survive verification in its literal form even though a real, adjacent gap did. This is a
different pattern from most recent rounds (24–30), which mostly applied Codex's findings close to
as stated: two of three findings this round required Claude to re-derive the actual defect from
first principles rather than transcribe Codex's own framing, which the series has not needed to do
this heavily in one round before. Finding #1 is the first correctness bug this series has found in
the enable-time boundary case specifically (§8's enable-time paragraph was last substantively
touched when it was first written, not by rounds 24–30's own work), and it is a genuine
bit-exactness break if implemented as the doc previously described, not a documentation-only gap —
the highest-severity finding this series has produced since the early rounds. None of the three
findings is a consequence of round 30's own fix (round 30 touched §5.2's mutator rows and §10's
name-cache paragraph; this round's findings are in §4/§8/§9 and §7.4/§12/requirement 7, disjoint
sections) and none reopens ground the doc's own rationale already covered — all three are new
territory, in a part of the design (the enable-time boundary, the minimum-window budget formula's
exact scope, and the cold-hash guard's actual coverage limits) no prior round had examined this
specifically.

Per the user's instruction, this batch was capped at 2 rounds (30–31) or sooner on `CONVERGED`.
Round 31's verdict is `CHANGES_PROPOSED`, not `CONVERGED`, and the batch limit is now reached, so
this session stops here per that instruction; the series remains open, and whenever review next
resumes it should continue at the same full scope as every round so far, not a narrower one —
especially given this round found a High-severity bug in an area (the enable-time segment
boundary) the series had not previously scrutinized this closely, which is exactly the kind of gap
a narrowed scope would have kept missing.

Round 31: 3 findings (1H-2M-0L), 3 applied, 0 declined
Series total: 111 findings (42H-53M-16L) across 31 rounds
Findings: 12 (7H-5M-0L) -> 8 (6H-2M-0L) -> 10 (3H-6M-1L) -> 8 (4H-3M-1L) -> 8 (1H-5M-2L) -> 3 (2H-0M-1L) -> 2 (0H-1M-1L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 1 (1H-0M-0L) -> 2 (1H-0M-1L) -> 2 (1H-1M-0L) -> 3 (1H-2M-0L) -> 4 (2H-1M-1L) -> 4 (1H-1M-2L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 2 (1H-1M-0L) -> 1 (1H-0M-0L) -> 4 (1H-3M-0L) -> 5 (3H-2M-0L) -> 4 (2H-1M-1L) -> 2 (1H-0M-1L) -> 2 (0H-2M-0L) -> 1 (0H-1M-0L) -> 2 (0H-2M-0L) -> 3 (0H-1M-2L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 2 (0H-2M-0L) -> 3 (1H-2M-0L)
