# Workflow: Codex review of design docs

This project uses OpenAI Codex CLI as a second reviewer for the design docs under
`docs/designs/` (e.g. `docs/designs/20260927-001-client-prediction-rollback.md`,
`docs/designs/20260927-002-bit-exact-history-ring.md`) before they're treated as settled.
This document describes how that review loop is run and where its output lives.

Only the process is described here — content decisions it produced live in the design docs
themselves (their `status:` front matter records the round history and rationale), not here.

## Where files are saved

- Every review round is written to `docs/designs/reviews/`, one file per round, never edited
  after the fact.
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
   rounds.
   - **Exception: declined findings travel with the prompt, as a curated list, never as a pointer
     to the reviews directory.** The prompt includes every finding declined so far across all
     rounds, each compressed to a terse one-line restatement of the finding plus the reason it was
     declined — compiled by Claude from the round files' own Post-review verification sections,
     not by directing Codex to go read those files itself. This is handed over as plain context,
     not framed as an invitation to argue or relitigate. If a fresh reviewer independently lands
     on the same ground again and disputes the stated reasoning by citing source directly, that
     citation gets checked against the actual source with the same rigor as any new finding — a
     repeat's content needs re-verification every time it recurs, not just a check that it was
     raised and declined before.
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
   with `>>` directly into `docs/designs/reviews/...` (as earlier rounds in this series did)
   leaves the tracked file bloated with that transcript until a later cleanup pass strips it back
   out — wasted repo churn and wasted tokens reading through it. Instead:

   **This shell command is the Bash tool's `command` parameter only. The same tool call must also
   set `timeout: 600000` as a separate parameter** (not shown in the shell snippet below, since
   it isn't shell syntax) — this is the tool-call-level timeout described above, and omitting it
   is the single most common way this step fails.

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
   knowledge can be stale relative to the doc's date) and applies the ones that are genuine
   defects, in place in the target doc, as the smallest fix that corrects the error. The fix
   states the corrected fact and nothing else — **no justification prose, ever, not even a
   clause**. A reason the fix is correct ("since X only applies when...", "because Y is the only
   Z that...", "which matters because...") is not part of the fix and does not belong in the doc
   at any length, not even trimmed to one clause: it is new, unreviewed surface that the next
   fresh-context round (step 2) will scrutinize as though it were original design content, and a
   loop that keeps growing the doc every round instead of shrinking to settled fact never reaches
   `CONVERGED`. If a reason genuinely needs recording, it goes in the round file's own
   Post-review verification section (step 6), never in the target doc. Claude does **not** apply
   a finding just because Codex proposed it, and records any finding it declines and why.
   - The target doc is a design doc, not a changelog of its own review: an in-doc fix reads as
     if it had always been right, with no trace that a review round, a finding, or an earlier
     draft's wording ever existed. Never write "this closes a gap round N left," "correcting
     round N's fix," "an earlier version of this section claimed X," or similar — that narration
     belongs in the round file's own Post-review verification section (step 6), not the doc.
     Delete a wrong sentence and write the right one; don't keep both with a note explaining the
     difference.
6. Claude appends a `## Post-review verification (Claude)` section to the round's file recording:
   the invocation (mode, timing, exit code, confirmation the working tree was/wasn't touched by
   Codex), which findings were verified against source (with the evidence checked) and applied
   or declined, and why for any decline.
7. Claude appends a `## Status` section: how this round's finding rate compares to recent
   rounds (still finding new categories of issue vs. narrowing to edge cases), whether any
   finding was a consequence of the immediately preceding round's own fix, and what happens
   next (another fresh round, at the same full scope, or stopping).
8. Repeat with another round, always at the same full scope as every prior round, while findings
   keep surfacing real, source-confirmed issues. A `CONVERGED` verdict means this round, run
   full-scope and from scratch, found nothing wrong with what the doc currently claims — it is
   not architectural endorsement of the doc's underlying approach, and it is not a signal that
   future rounds (if the user asks for more, or a later revision reopens the doc) should be run
   any narrower than this one was. Stop once a round's verdict is `CONVERGED` (empty findings) or
   the user says to stop. When a series converges, record that in the *target doc's own* `status:`
   front matter as prose (see `docs/designs/20260927-001-client-prediction-rollback.md` for a
   worked example) — not in a review file. The last round's `## Status` section is the only
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
