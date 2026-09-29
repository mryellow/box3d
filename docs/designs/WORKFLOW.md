# Workflow: Codex review of design docs

This project uses OpenAI Codex CLI as a second reviewer for the design docs under
`docs/designs/` (e.g. `docs/designs/20260927-001-client-prediction-rollback.md`,
`docs/designs/20260927-002-bit-exact-history-ring.md`) before they're treated as settled.
This document describes how that review loop is run and where its output lives.

Only the process is described here — content decisions it produced live in the design docs
themselves, not here: design rationale in the doc's body (step 5 below), and the series'
outcome — round count, converged or not — as prose in its `status:` front matter (step 8). The
front matter is the one place a design doc refers to its own review series; the body never does.

## Acceptance criteria — hard gates

These are **prohibitions on architecture**, not review findings — contrast every other section of
this document, which governs the mechanics of a round Codex might return a finding against. They
describe shapes a design must not have, independent of whether any review round ever runs against
it or what that round would find. A design that has one of these shapes is rejected and rewritten in
that area, not defended, not priced, and not carried into a round as one more table row for step 5
to verify and apply. Apply them **before** a design's first round, and again before recording a
round's `CONVERGED` verdict as the series' close (step 8) — a clean round only means this round
found nothing wrong with what the doc currently claims (AC-5); it says nothing about whether the
shape itself is one of these, and a series that drifted since these were last checked doesn't get to
skip the recheck just because this round's Codex output came back clean.

**SCOPE.** These criteria govern what a design *adds* — the structures, hooks, and enumerations it
introduces — not a license to re-litigate an existing, already-relied-upon engine behavior a design
merely calls. If an existing primitive genuinely looks wrong, that's its own separate design
question, raised on its own, never a finding folded into review of whatever happens to call it.

**AC-1 — No shadow structure over a population the design doesn't own.** A subset, scope set, shadow
graph, or side table that duplicates entities already tracked by an existing live structure (a
solver set, the constraint graph, an island, an ID pool) is forbidden, full stop — not priced against
the cost of not having one. That population is created, destroyed, merged, split, put to sleep, and
reclassified by code the design doesn't control, and a design that freezes a copy of part of it
inherits every one of those transitions as a fact it must independently reproduce, discover, or
patch for. If a design needs "the entities relevant right now," it reads that from the live
structure at the point of use — never from a copy taken earlier.

**AC-2 — No completeness that depends on an enumerated list of callers.** A rule of the form "the
following functions must call X" is a claim that can never be checked true, only checked incomplete
one caller at a time by whoever next traces one — the compiler enforces nothing, so a missed caller
just silently doesn't happen. If a mutation must always be accompanied by some other action (a
journal write, an invalidation, a link update), the design routes the mutation through one function
that is the only legal way to perform it, so a bypass fails to compile or fails an assertion. A list
of named call sites is never a substitute for that choke point; if the operation genuinely cannot be
centralized, that is itself a finding against the surrounding code, to be fixed there, not a license
to enumerate.

**AC-3 — No reconciliation between two structures that can each change independently.** Once a
design has (in violation of AC-1) or is tempted to introduce a second structure alongside a first —
a scope over the world, a shadow graph over the live graph, a side table of staged values over live
records — logic to keep the two consistent as either one changes is not a missing piece to add, it
is the sign the second structure should not exist. The fix is never "handle this additional
transition too"; it is deleting the second structure and deriving whatever it existed to answer
directly from the first, on demand.

**AC-4 — By construction, not by check.** No validator, generation check, staleness guard,
"confirm still valid" gate, or reconciliation pass bolted onto a mechanism to catch a case its own
shape allows to go wrong. A check whose result changes what the code does — a branch, a retry, a
skip, a "not this one" — is a symptom that the structure permits the bad case at all; fix the
structure so the case cannot arise, don't add a gate in front of it. An assertion of an invariant the
construction already guarantees is fine (it documents the invariant and aborts if reality disagrees)
— it must never be control flow.

**AC-5 — A clean review round is not architectural endorsement.** Zero findings after N rounds means
this round, run at full scope, found nothing wrong with what the doc currently claims. It says
nothing about AC-1 through AC-4 — a design can satisfy every review round Codex has run against it
and still be built from a shadow structure or a call-site list that simply hasn't had its next gap
found yet. Apply AC-1 through AC-4 yourself, before sending a round and again before recording
`CONVERGED`; a falling finding count is what convergence looks like, but it is equally what a
growing enumeration looks like in between the rounds that happen to catch its gaps, so a falling
count alone is never evidence the shape is right.

**How to fail a design against these.** Name the criterion and name the structure: what population
it shadows (AC-1), what list stands in for a choke point (AC-2), what second structure it's
reconciling against a first (AC-3), or what case its check exists to catch (AC-4). Do not propose a
smaller list, a lighter-weight shadow, or a narrower reconciliation — deleting the structure that
should not exist is the correct outcome, and a design that needs one of these is priced by rewriting
it out, not by optimizing it in place.

## Where files are saved

- Every review round is written to `docs/designs/reviews/`, one file per round. A Codex round
  file is never edited after the fact; the only later edit any review file receives is the
  `Resolution` note a non-Codex review gets once addressed (see the end of this document).
- Filename: `YYYYMMDD-NNN-<slug>.md`, e.g. `20260927-002-bit-exact-history-ring-round11.md`.
  - `YYYYMMDD` — date the round was run.
  - `NNN` — a counter that increments per *target doc* review started that day, not per round
    (it does not reset when review moves to a different target on the same day — the first
    target reviewed that day is `001`, the second is `002`, and so on; all rounds against a
    given target reuse that target's number).
  - `<slug>` — the target doc's slug, with `-roundN` appended from the second round on (the
    first round's file has no `-round1` suffix).
  - A one-off review that isn't part of the Codex round series (e.g. a Claude-authored
    performance pass) gets its own descriptive suffix instead of a round number, e.g.
    `20260927-001-client-prediction-rollback-perf.md`.
- Each Codex round file's front matter records `title`, `target` (path to the doc under
  review), `date`, `reviewer` (Codex CLI version and model) and `mode` (how the round was
  run — see below). A non-Codex review file uses `reviewer` for who ran it and a
  `focus` line instead of `mode`.
  - Add a `session` (Codex session id) field only when a resume was needed (see below);
    otherwise omit it.
- Committed with `git commit`. This repo has no commit-type prefix convention — use a plain,
  descriptive message. Rounds may accumulate uncommitted while a series is actively running;
  commit them together once the series reaches a stopping point.
- `README.md` does not reference `docs/designs/` at all; it's product-facing and out of scope
  for this loop.

## Citation format

- Cite source files by path only, never by line number (`solver.c`, not `solver.c:1234`).
- Never cite the doc's own line numbers either (no "see line 42 above/below"); use the doc's own
  `§N` section numbers for self-references — stable across edits.
- When writing a Codex prompt, state that citations are file-only by design and that a missing
  line number is not a finding.
- Verify any bulk or mechanical edit across a whole doc (e.g. stripping a pattern doc-wide)
  immediately with a grep/script check that no partial or orphaned matches remain, before leaving
  it uncommitted.

## Running a round

1. Before anything else, `git diff` the target doc. If a previous session left it uncommitted,
   read the whole diff — not a preview — and check it isn't mangled: an interrupted or careless
   edit (see "Citation format" above) can look fine skimmed but break on close read. Fix any
   mangling first; don't let Codex review broken text as if it were the design.
2. Claude writes the Codex prompt: point Codex at the target doc and the source tree it
   describes. **Default to a fresh, from-scratch review**: give Codex only the target doc and
   the source, explicitly instruct it not to read `docs/designs/reviews/`, and don't carry any
   memory of previous rounds' scope or coverage into the prompt. Prior-round context anchors Codex
   on what that round already covered instead of re-checking the whole doc. The target doc itself
   must therefore carry enough settled rationale inline (status/why-not-X notes) that a
   from-scratch reviewer reconstructs the current constraints correctly without review history —
   that's what keeps Codex from relitigating settled decisions, not a prompt reminder about prior
   rounds. Step 5 is where that rationale gets written into the doc, and step 7 is where a
   round that had to relitigate settled ground gets noticed, so the gap is closed before the
   next round rather than found again by it.
   - **Exception: declined findings the doc cannot answer for itself travel with the prompt, as
     a curated list, never as a pointer to the reviews directory.** A decline is one of two
     things. Either the doc was right and should have said why — then the fix is the doc's own
     rationale (step 5), the doc now answers a fresh reviewer directly, and the finding does not
     go on the list. Or the reviewer was wrong about the source or misread the doc — the doc has
     nothing to add, so the finding goes on the list: a terse one-line restatement plus the reason
     it was declined, compiled by Claude from the round files' own Post-review verification
     sections, not by directing Codex to go read those files itself. This is handed over as plain
     context, not framed as an invitation to argue or relitigate. If a fresh reviewer
     independently lands on the same ground again — listed or in-doc — and disputes the stated
     reasoning by citing source directly, that citation gets checked against the actual source
     with the same rigor as any new finding (step 5's reversal-check): a repeat's content needs
     re-verification every time it recurs, not just a check that it was raised and declined
     before.
   - **Every round is the same full, broad pass — first round, last round, and every round
     between**: correctness, soundness of the recommendation, internal consistency, gaps/risks,
     clarity, and completeness. There is no narrowed or reduced-scope round, at any point in a
     series. A fresh reviewer given the full brief is what catches a defect a narrower brief would
     have excluded from consideration by definition — cutting what a reviewer is asked to look at
     once a round or two comes back clean is exactly how a real gap stops getting checked for.
     Record `mode` as simply the run mechanics (model, resume session if any).
   - There is no fixed round count and no round number that by itself justifies stopping or
     narrowing scope. Some designs converge in a handful of rounds; others take many more.
     Treat any number of rounds run so far as a floor to expect, never a ceiling to plan toward.
   - Before writing the prompt, Claude re-reads the target doc's own mechanism sections and
     checks every rule or invariant stated for one specific case in its inventory against every
     sibling case the same inventory lists, fixing any gap directly rather than waiting for
     Codex to find it.
3. Codex runs **read-only** (`codex exec --sandbox read-only`). Codex reports findings and
   proposed edits; it does not touch files. Read-write access is not the default and is only
   used for a specific request in that request's scope, not carried over between rounds.

   **Always redirect stdin from `/dev/null`** on every `codex exec` invocation, even when the
   prompt is passed as an argument: the CLI's own `--help` text says stdin is appended as a
   `<stdin>` block whenever it's piped, and a Bash-tool-spawned process's stdin is never
   explicitly closed — so without `< /dev/null` the call can hang indefinitely printing "Reading
   additional input from stdin..." with the API request never starting.

   **Two different timeouts must both be set on every `codex exec` call, and neither substitutes
   for the other.** Wrap the call in a shell-level `timeout` (`timeout 540 codex exec ...`) so a
   genuine stall or a long high-effort round still returns control — this bounds the `codex exec`
   *process*. Separately, the Bash **tool call itself** takes its own `timeout` parameter (a field
   on the tool invocation, in milliseconds — not part of the shell command string) that bounds how
   long the harness lets the tool call run before it kills or auto-backgrounds it, independent of
   what the shell is doing. **This parameter defaults to 120000ms (2 minutes) when left unset, and
   a `high`-reasoning-effort round routinely runs longer than that** — so an invocation that only
   sets the shell-level `timeout 540` and omits the tool parameter gets silently killed/backgrounded
   at the 120s mark, long before the 540s shell timeout ever gets a chance to fire. Always pass the
   tool parameter explicitly, set above the shell-level value: **`timeout: 600000`** (its
   600000ms/10-minute hard cap) paired with `timeout 540` in the command string. Do not raise the
   shell-level wrapper to 600 or beyond — that removes the margin between the two and reintroduces
   the same race.

   **Reviews MUST be run in the foreground — never `run_in_background`, and never left running
   after the harness auto-backgrounds it.** A backgrounded `codex exec` call detaches the
   start/end-time and exit-code capture from the call itself, so there is no reliable way to tell
   a genuine stall from a long-but-healthy round, and no reliable way to know when it's safe to
   read the output file. Run it foreground, bounded by the shell-level `timeout`, and let it hit
   that timeout if it's going to — that's what the resume step below exists for. **If the harness
   auto-backgrounds the call anyway** (the tool-call notification reports the command moved to a
   background task instead of returning inline), that is itself proof the tool's own `timeout`
   parameter was missing or set too low — it is not a benign variant of "still running," and
   waiting for its completion notification is not a substitute for having run it foreground.
   Kill it immediately (`TaskStop`), fix the invocation to include the correct `timeout: 600000`,
   and reissue the call from scratch — do not wait on a call you know was misconfigured, and do
   not treat its eventual notification as a valid round result.

   **Redirect the raw output to the session's scratchpad directory, never straight into the
   tracked round file.** `codex exec`'s own stdout interleaves its final answer with every tool
   call it made along the way (shell commands, file reads, source dumps) — for a broad review
   this routinely runs to several thousand lines, almost all of which is scratch. Piping that
   with `>>` directly into `docs/designs/reviews/...` leaves the tracked file bloated with that
   transcript — wasted repo churn and wasted tokens reading through it. Instead:

   **This shell command is the Bash tool's `command` parameter only. The same tool call must also
   set `timeout: 600000` as a separate parameter** (not shown in the shell snippet below, since
   it isn't shell syntax) — this is the tool-call-level timeout described above.

   ```bash
   RAW="$SCRATCHPAD/YYYYMMDD-NNN-<slug>-roundN.raw.log" && \
   PROMPT_START=$(date -Iseconds) && echo "START: $PROMPT_START" && \
     timeout 540 codex exec --sandbox read-only "<the exact review prompt>" \
       < /dev/null >> "$RAW" 2>&1; \
     EXIT_CODE=$?; PROMPT_END=$(date -Iseconds); \
     echo "END: $PROMPT_END, EXIT_CODE: $EXIT_CODE"
   ```

   (`$SCRATCHPAD` is this session's scratchpad directory, given in the environment block at the
   start of the session — not a literal env var.) The captured start/end times and exit code feed
   the round's front matter and "Post-review verification" section; a non-zero `EXIT_CODE` means
   the round didn't finish and the output is not a real result to act on. Once the call succeeds,
   read `$RAW`, find the content under the *last* `## Review report (Codex final message)`-shaped
   answer (the same final message is often emitted twice — a `codex exec` streaming artifact, not
   two different answers — keep one copy), and write only that clean text into the tracked round
   file (front matter, then `## Review report (Codex final message)`, then that content) — the raw
   log itself is never committed and can be left in the scratchpad for the session to clean up.

   **If it times out (`EXIT_CODE 124`), don't restart from scratch — resume the session.** A
   `timeout`-killed `codex exec` has usually already done most or all of its research; the
   process is dead but the session (and its cached context) is not. Find the session id in the
   killed run's output (Codex prints it, or `--json` records it per-event), then:

   ```bash
   codex exec --sandbox read-only -m <same model> -c model_reasoning_effort="<same effort>" \
     resume <session-id> "<short finalize prompt: report findings from research already done, no further tool calls>" \
     < /dev/null >> "$RAW" 2>&1
   ```

   Still run this in the foreground, still under a shell-level `timeout`, still with the Bash
   tool's own `timeout: 600000` parameter set explicitly on this call too (a resume call gets no
   exemption from the rule above — omitting it here fails exactly the same way), still redirected
   from `/dev/null`, and still appended to the same scratchpad `$RAW` file, never the tracked
   round file. Two syntax traps:
   - `-m`/`-c`/`--sandbox` go **before** `resume`, not after —
     `codex exec resume <id> --sandbox ...` is a CLI syntax error (`EXIT_CODE 2`). The
     subcommand form is `codex exec [OPTIONS] resume <SESSION_ID> [PROMPT]`.
   - Don't drop `-m <model>` on the resume call: omitting it lets the resumed turn silently fall
     back to a different default model, so the round's model isn't what the front matter says.
   Record the session id (in `mode` or a `session` field) and every attempt (killed and
   resumed) in the round's "Post-review verification" section when a resume was needed.
4. Ask Codex for its findings under `## Review report (Codex final message)`, in one fixed shape
   used for every round regardless of round number: a `# | Severity | Section | Issue | Proposed
   action` table (severities `High`/`Medium`/`Low`, not `Critical`/minor), a separate `## Checked,
   no change` list for things already verified correct (never mixed into the findings table),
   `## Proposed edits`, `## Unresolved / disagreements`, and a one-line `## Verdict`
   (`CHANGES_PROPOSED` or `CONVERGED`).
5. Claude verifies each proposed finding against the actual repo/doc state (Codex's model
   knowledge can be stale relative to the doc's date), applies the ones that are genuine
   defects in place in the target doc, and declines the rest, recording why in the round file
   (step 6). Claude does **not** apply a finding just because Codex proposed it. Two rules
   govern what an applied edit may contain, and a third governs a finding that revisits settled
   ground:
   - **A fix is the smallest edit that corrects the error, and it carries no commentary on why
     the edit is correct** — no "since X only applies when...", "because Y is the only Z
     that...", "which matters because...", and no narration of the review itself ("this closes a
     gap round N left," "correcting round N's fix," "an earlier version of this section claimed
     X"). Such text carries no design claim, so the next fresh round spends effort scrutinising
     it for nothing; the reason an edit is correct belongs in the round file's Post-review
     verification section (step 6). An in-doc fix reads as if it had always been right, with no
     trace a review round or an earlier draft's wording ever existed: delete the wrong sentence
     and write the right one, don't keep both with a note explaining the difference.
   - **The doc's own design rationale is required content, and adding it is a fix like any
     other** — "why not X," why this approach over an alternative, what an invariant depends
     on. It is written in the doc's own voice, as if it had always been there, never as a note
     about what a reviewer said. It is added whenever the doc is found not to say it: because
     Codex flagged the omission, or because a finding was declined on grounds the doc should
     have stated itself (step 2's first kind of decline). The next fresh round reviews that
     rationale as design content — which is correct, since a wrong rationale is a real defect
     and should be found. Withholding rationale to keep the doc from growing is what forces
     every fresh round to re-open a question the doc could have answered on its own; the rule
     above bans text that carries no design claim, not text that carries the design's reasons.
   - **Reversal-check: a finding that contradicts rationale the doc already states is not new
     work until it engages that rationale.** The round's argument must say why the stated
     reason no longer holds, with fresh evidence from source — not just restate the
     alternative. If it does and
     the evidence checks out, apply it: the doc's rationale changes to the new fact, with no
     trace of the old one. If it doesn't, decline it and, if the existing rationale evidently
     wasn't clear enough to head the re-raise off, sharpen it rather than leaving it exactly
     as-is for the next fresh reviewer to trip over again. Silently re-flipping the design to
     match whichever round most recently argued for it is not convergence, it's oscillation.
6. Claude appends a `## Post-review verification (Claude)` section to the round's file recording:
   the invocation (mode, timing, exit code, confirmation the working tree was/wasn't touched by
   Codex), which findings were verified against source (with the evidence checked) and applied
   or declined, and why for any decline.
7. Claude appends a `## Status` section: how this round's finding rate compares to recent
   rounds (still finding new categories of issue vs. only edge cases), whether any
   finding was a consequence of the immediately preceding round's own fix, whether any finding
   reopened ground the doc's own rationale already covered (per step 5's reversal-check — and if
   so, whether this round's argument actually engaged that rationale or just re-raised the same
   alternative), and what happens next (another fresh round, at the same full scope, or
   stopping). A round with mostly reopened findings is a signal to go sharpen the doc's rationale
   before the next round, not a signal to narrow what the next round is asked to check. End the
   section with a one-line convergence tally, so the trend is legible without re-reading every
   round's prose:

   `Round N: F findings (HN-MN-LN), A applied, D declined`

   H/M/L are this round's own counts of `High`/`Medium`/`Low` findings straight from step 4's
   table (never `Critical`/minor — this project's severities are exactly the three step 4 uses),
   F = H+M+L. Follow it with the running series total in the same notation, computed by summing
   every prior round's own tally plus this round's — cheap addition over numbers each round file
   already states, never a re-read of their prose, e.g.:

   `Series total: 100 findings (42H-45M-13L) across 26 rounds`

   Then the convergence pattern itself, the most important line for a human reader: every round's
   count in order, oldest first, in the same notation, ending with this round. Build it by copying
   the previous round's `Findings:` line and appending ` -> ` and this round's `F (HN-MN-LN)`
   (round 1 starts the line), e.g.:

   `Findings: 12 (3H-6M-3L) -> 9 (2H-5M-2L) -> 4 (0H-3M-1L) -> 0 (0H-0M-0L)`

   Never abbreviate or elide earlier rounds; the full sequence is the point. This line is the
   primary content of any progress summary given directly to a human (a hand-off message, a chat
   reply, the doc's `status:` front matter under step 8), reproduced verbatim and leading the
   summary; per-round finding detail is secondary.

   A `CONVERGED` round's own tally is trivially `0 findings (0H-0M-0L)`. This tally is a
   convergence *signal*, not a stopping rule: a falling count is the trend to expect on the way
   there, and a `CONVERGED` verdict (step 8) is the only thing that actually stops the series.
8. Repeat with another round, always at the same full scope as every prior round, until a round
   returns `CONVERGED` or the user says to stop. A round whose findings were all declined is not
   a converged round — its verdict was `CHANGES_PROPOSED`, and only a fresh reviewer returning
   `CONVERGED` closes the series, never Claude declining everything; the doc gets whatever
   rationale the declines showed it was missing (step 5), and another full round runs. A
   `CONVERGED` verdict means this round, run full-scope and from scratch, found nothing wrong
   with what the doc currently claims — it is not architectural endorsement of the doc's
   underlying approach, and it is not a signal that future rounds (if the user asks for more, or
   a later revision reopens the doc) should be run any narrower than this one was. When a series
   converges, record that in the *target doc's own* `status:` front matter as prose (see
   `docs/designs/20260927-001-client-prediction-rollback.md` for a worked example) — not in a
   review file. The last round's `## Status` section is the only
   record of how the series ended; there is no separate summary section.

## File structure of a review round

Every round uses the same structure, regardless of round number:

```
---
title / target / date / reviewer / mode (+ session, only when a resume was needed)
---

# Codex review — <target> [— round N]

## Review report (Codex final message)

## Summary
## Findings
## Checked, no change
## Proposed edits
## Unresolved / disagreements
## Verdict

## Post-review verification (Claude)

## Status
```

A non-Codex review (e.g. a Claude-authored performance pass) uses `reviewer` + `focus` in the
front matter instead of `reviewer` + `mode`, and is resolved later with a `> **Resolution (round
N):**` blockquote note added at the top of that same file once the target doc addresses it,
rather than by appending further rounds of its own.
