# Review Workflow

**Status:** Draft

A citable checklist for the design-doc review loop that precedes implementation. Cite rules by ID
(`U-C2`, `AC-3`) in round records instead of restating them. This is not ordinary code review
(`/code-review`, PRs).

- **Parameters** and **Running a round** are this repo's own values and mechanics. Where Part A
  says "the project's X", it means the Parameters row for X.
- **Part A (`U-*`)** is project-neutral and identical in every repo that uses this loop. Do not edit
  it per project; a change to it is made in every copy.
- **Part B (`AC-*`)** is customised per project. It turns the project's goal into prohibitions on
  architecture. Part A refers to it by role (`Part B`), never by content.

---

## Parameters (this repo)

| Parameter | Value |
|---|---|
| Reviewer command | `codex exec --sandbox read-only` |
| Model / effort | `gpt-6-sol` / `high`, always passed explicitly |
| Doc location and IDs | `docs/designs/YYYYMMDD-NNN-<slug>.md` |
| Round record | one file per round in `docs/designs/reviews/`, `YYYYMMDD-NNN-<slug>[-roundN].md` (`NNN` is the target's number that day; all rounds against a target reuse it); front matter `title`, `target`, `date`, `reviewer`, `previous`, `mode` (+ `session` only if resumed); sections `## Exact request sent to Codex`, `## Review report (Codex final message)` (Summary / Findings / Checked, no change / Proposed edits / Unresolved / Verdict), `## Post-review verification (Claude)`, `## Status` (tripwire + tallies). A non-Codex review uses `reviewer` + `focus` and is resolved by a `> **Resolution (round N):**` note at its top |
| Changelog | `docs/designs/YYYYMMDD-NNN-<slug>-changelog.md`, one per doc |
| Owner | the user |
| Severity scale | `H`/`M`/`L`; tallies `Round N: F findings (HN-MN-LN), A applied, D declined`, series total, and the full `Findings: ... -> ...` line |
| Citations | source by path only, never line (`solver.c`); docs by `§N` |
| Status words | Draft -> Canonical -> Closed/Implemented, bare, in front matter `status:`; no `review:` line; the convergence record stays in the last round file's `## Status` |
| Split-check length | 400 lines |
| Criteria review threshold | 10 rounds without converging |
| Memory doc | none yet; deferred gaps go in the design doc's successor or a `docs/designs/` note until one exists |
| Context docs | the source tree the design describes (`src/`, `include/`, `test/`) |
| Commits | plain descriptive message, no type prefix; rounds may accumulate uncommitted while a series runs |
| Scratch | session scratchpad (`$SCRATCHPAD`), raw logs never committed |
| Out of scope | `README.md` is product-facing and never references `docs/designs/` |

---

## Running a round (this repo)

```bash
RAW="$SCRATCHPAD/YYYYMMDD-NNN-<slug>-roundN.raw.log"
PROMPT_START=$(date -Iseconds); echo "START: $PROMPT_START"
timeout 540 codex exec -m gpt-6-sol -c model_reasoning_effort="high" --sandbox read-only \
  "<prompt>" < /dev/null > "$RAW" 2>&1
EXIT_CODE=$?; echo "END: $(date -Iseconds), EXIT_CODE: $EXIT_CODE"
```

- Foreground only. Never `run_in_background`, never `&` + polling. If the agent shell tool
  auto-backgrounds the call, kill it and reissue with the tool timeout set.
- `< /dev/null` always: the CLI reads stdin even with a prompt argument and otherwise waits for EOF.
- Two timeouts: shell wrapper `540`, agent shell tool `timeout: 600000` (its cap). The wrapper
  must stay below the tool's; never raise it to 600.
- Model and effort explicit on every call, including sanity checks and `resume`; confirm the
  captured header shows them.
- Non-zero exit = no result (including the usage-limit exit 1). Do not act on it (U-R8).
- Exit 124 with a long trace of real reads = work timeout: resume the session to finalize; if the
  resume cannot finish either, bound the work (one doc or one Part per round). Exit 124 with
  little output = stall: retry plain, then `RUST_LOG=info`, then `RUST_LOG=debug`, then run the saved
  prompt through the desktop Codex app and fold the answer in as a normal response.
- Resume: `codex exec -m gpt-6-sol -c model_reasoning_effort="high" --sandbox read-only resume <session-id>
  "<finalize from research already done, no further tool calls>" < /dev/null >> "$RAW" 2>&1`.
  Options go before `resume`; dropping `-m` silently reverts the model. Record the provenance.
- Then write the round record from `$RAW` (U-R4): the exact request, then the reviewer's closing
  answer (the last copy, if the stream emitted it twice).

---

## Part A: Universal rules

### U-G — Gates (apply before round 1 and again before Canonical)

- [ ] **U-G1** Every `AC-*` (Part B) checked against the doc by the author, not left to the reviewer.
- [ ] **U-G2** A failed AC means rewrite, not patch: name the AC, the structure, and what it shadows.
- [ ] **U-G3** Do not price a cheaper version of a structure that should not exist; delete it and
      everything that existed only to support it.
- [ ] **U-G4** A clean round is not architectural endorsement. Zero findings shows local
      consistency only; re-run U-G1 before marking Canonical.
- [ ] **U-G5** A falling finding count is not evidence the shape is right; a rising one after a
      rewrite is normal.
- [ ] **U-G6** Every rule stated for one case is checked against every sibling case in the same
      inventory before sending.
- [ ] **U-G7** Every claim the design depends on about a system the project does not own is
      confirmed against that system, or a recorded observation of it, and the doc cites the
      observation. An unconfirmed claim is a question for the owner, not a design input (U-S7).

### U-C — Document content (design docs)

- [ ] **U-C1** The doc holds decisions, required behaviours, and the reasons for them (U-C4);
      nothing else.
- [ ] **U-C2** No test mechanism (spies, call counts, fixtures), coverage tables or inventories.
- [ ] **U-C3** No history, review narration, round counts, finding numbers, reviewer names, or
      citations into round records or the changelog. Front matter says only what the doc is and
      what it supersedes. The only mention of review is the bare status word, and the Provenance
      section (U-C4).
- [ ] **U-C4** The reasoning for a declined finding is restated plainly in the revision's
      Provenance section: the decision's reason and any rejected alternative, with no round or
      finding numbers. Where it is a design reason it also sits beside the decision, in the doc's
      own voice. Write it whenever a declined finding shows the doc did not say why.
- [ ] **U-C5** No restated content of another doc; cross-reference it (Restatement Test: a
      paragraph that could be pasted into the doc it cites belongs there).
- [ ] **U-C6** No redefinition, narrowing or repurposing of an established term (Terminology Rule).
- [ ] **U-C7** Name only what the design uses; naming a wider structure invites code that
      traverses it.
- [ ] **U-C8** No enumerations of cases the design does not distinguish.
- [ ] **U-C9** Self-references use the doc's own `§N`, never line numbers. External code is cited
      by path, with line numbers only as the project's Parameters allow.
- [ ] **U-C10** Test mechanism, fixtures and test-to-requirement mapping belong to the
      Implementation Prompt, which is exempt from U-C1/U-C2 and from the U-T3 fix-size rule.

### U-P — Review prompt

- [ ] **U-P1** Scope the prompt to the document and what it cites.
- [ ] **U-P2** Never instruct the reviewer to read a previous version. A reviewer that looks at one
      on its own initiative is not at fault.
- [ ] **U-P3** A re-raised finding means the doc's reasoning or the prompt's scope wording was
      unclear: check the scope first. Scope is stated in terms of what the document claims to
      decide; completeness means completeness of what the document states, because the design as
      a whole is complete only when the Implementation Prompt (U-C10) is reviewed and canonical.
      A decision the document assigns to the Implementation Prompt is outside the pass. Scope
      wording is not a declined finding.
- [ ] **U-P4** Every round is the same full pass at the same scope. Never narrow after clean rounds.
      The only fixed-scope exception is a target defined by a diff, stated in every prompt.
- [ ] **U-P5** Review the doc's own content only, never its history and never citations into other
      review artifacts. The prompt states as out of scope: the changelog, any "what changed"
      narrative, and any citation into a round record, finding list or finding-count log. If one
      of these slips into the doc and the reviewer flags it, that is noise, not a finding; fix it
      without re-running the round. A finding about another doc goes under Unresolved.
- [ ] **U-P6** Pressure-test the design. Never ask whether invariants have tests or whether a
      coverage table is complete.
- [ ] **U-P7** Don't ask the reviewer to check index or bookkeeping.
- [ ] **U-P8** Citations into round records or the changelog are never claims and never findings,
      and are never checked.
- [ ] **U-P9** After a clean round, run a confirmation round: the same prompt at the same scope
      (U-P4). The prompt adds no skeptical framing; running the round again is the check.
      Implementation review (U-I3) is exempt; see U-I9.
- [ ] **U-P10** Request one fixed output shape every round: Findings table (`# | Severity |
      Section | Issue | Proposed action`, real defects only, "None." if empty), separate
      **Checked, no change**, Proposed edits, Unresolved, one-line Verdict
      (`CONVERGED` / `CHANGES_PROPOSED`).
- [ ] **U-P11** Existing settled primitives are tools, not review targets. Judge what the design
      adds and which question a primitive answers. A believed-defective primitive is its own doc,
      raised with the owner.
- [ ] **U-P12** State in the prompt that citations follow the project's citation format and that a
      missing line number is not a finding.
- [ ] **U-P13** A decision recorded in the document's Provenance (U-C4) is settled and binding on
      the reviewer. It is not re-opened unless a finding shows fresh evidence that its reason no
      longer holds (U-V4). The prompt may state it as scope.

### U-V — Verifying findings

- [ ] **U-V1** The raw response is saved before it is read or judged.
- [ ] **U-V2** Every finding is checked against live code/docs. A review is input, not a mandate.
- [ ] **U-V3** Precedent claims are grepped/read before being repeated.
- [ ] **U-V4** Reversal check: a finding that contradicts an earlier reasoned decision is declined
      unless it engages why the reason no longer holds, with fresh source evidence. A decline
      restates the original reasoning plainly in the revision's Provenance section (U-C4).
- [ ] **U-V5** A finding whose only fix adds machinery (state, guard, retry, list entry,
      reconciliation) is checked against Part B first; if it fits, rewrite the surrounding
      structure instead.
- [ ] **U-V6** A "missing test" or "incomplete inventory" finding is declined as Implementation
      Prompt scope unless the doc itself decides a test and that decision is wrong.
- [ ] **U-V7** A repeated declined finding is re-verified, not waved through.
- [ ] **U-V8** Declining with a technical rebuttal is a normal, valued outcome.
- [ ] **U-V9** A round whose findings were all declined is not converged.
- [ ] **U-V10** Every finding declined this round has its reasoning in the revision's
      Provenance section (U-C4), before the next round is sent. A decline recorded only in the round record is unfinished.

### U-F — Folding fixes

- [ ] **U-F1** A fix states the corrected fact and nothing else: no justification, not even one
      clause.
- [ ] **U-F2** No trace of the review: never "closes a gap round N left", never "corrects round N".
- [ ] **U-F3** Delete the wrong sentence and write the right one; never keep both with a note.
- [ ] **U-F4** Deleting is a fix. Remove a section/state/fallback that only guards a case the
      design created, with whatever existed only to support it.
- [ ] **U-F5** Delete a replaced test in the same edit that writes its replacement.
- [ ] **U-F6** Fix the property, not the letter; mutation-verify the requirement as written.
- [ ] **U-F7** After a fix, grep the repo for the name of every mechanism or text the fix removed or
      changed, and for the claim itself. Every copy changes in the same edit.
- [ ] **U-F8** After a bulk/mechanical edit, grep/script-check for partial or orphaned matches.
- [ ] **U-F9** The reason an *adopted* edit is correct goes in the round record or changelog, never
      in the doc. (Reasoning for a *declined* finding is different: U-C4.)

### U-R — Round records

- [ ] **U-R1** Each round records, in order: the exact request, the reviewer's response verbatim,
      and the author's resolution (per-finding adopted / declined + why / carried forward,
      convergence line, tripwire). Files per the project's Round record row. A record is never
      revised after its round closes.
- [ ] **U-R2** A doc is one file, edited in place across rounds; a revision is never a new file.
      History lives in the changelog (U-R6) and the round records, not in copies of the doc.
- [ ] **U-R3** The resolution is finished before the next revision is drafted, as two separate
      actions. "Record this and start the next round" in one breath is the failure mode.
- [ ] **U-R4** Raw reviewer output goes to a scratch file. The record holds the exact request and
      the reviewer's closing answer verbatim, trimmed of tool trace. A copy of the target doc
      echoed inside the transcript is noise; each revision is always freshly authored.
- [ ] **U-R5** The convergence line is carried forward in full, never elided, in the project's
      severity scale: `N rounds: a/b/c -> a/b/c -> ... -> SETTLED 0/0/0`. Only the Findings table
      counts. It leads any progress summary given to a human.
- [ ] **U-R6** One running changelog file per doc (per the project's Changelog row), edited in
      place, appended before each new revision. It is where history and verbosity go. Never sent to
      the reviewer, never cited from the design doc; citations inside it are never checked.
- [ ] **U-R7** Re-check the next free identifier immediately before creating a file (concurrent
      sessions collide); renumber the later file, never the earlier.
- [ ] **U-R8** Non-zero reviewer exit means no result; do not act on it. Record the provenance of a
      killed-then-resumed round.
- [ ] **U-R9** Before a round, `git diff` the target; read the whole diff, fix any mangling first.
- [ ] **U-R10** When the doc is marked Canonical, the changelog's closing entry names any rule whose
      cost in this series exceeded what it caught: rule ID and the case, nothing else. No entry if
      none. These entries are input to reconsidering Part B (B8) and Part A.

### U-T — Tripwire (every resolution, between rounds)

- [ ] **U-T1** Record whether the round's fixes added or removed a named mechanism.
- [ ] **U-T2** Record whether findings clustered in a section that produced findings last round, and
      whether any finding was a consequence of the previous round's own fix. Record whether the raw
      log shows the reviewer reading round records or the changelog; such a read is evidence of a
      gap in Provenance (U-C4, U-T5), not a reviewer fault (U-P2).
- [ ] **U-T3** Record doc length in lines, previous round -> this round, and the lines each fix
      added. Growth is expected as required behaviours are stated; length is not itself a defect.
      The defect is a fix larger than the finding: a minor finding answered by paragraphs rather
      than a clause, or any narrative of how the decision was reached (U-C1, U-F1, U-C4). Added
      prose is what the next round's findings land on. Cut such a fix back to the corrected fact
      or the one-clause reason before the next round.
- [ ] **U-T4** Findings clustering in one section, answered by fixes that keep adding machinery:
      re-apply Part B to that section as if it were round 1. Scope of the next round is unchanged.
- [ ] **U-T5** A round of mostly re-raised findings means the doc's rationale is unclear (U-C4), not
      that the next round should be narrower.
- [ ] **U-T6** Above the project's split-check length, record whether the doc decides more than one
      independent thing. If it does, split before the next round: the doc keeps one decision, each
      other moves to a new doc with its own number (U-R7) and changelog (U-R6). Length alone is
      still not a defect (U-T3).

### U-S — Stop and ask

- [ ] **U-S1** Three consecutive rounds each finding a substantial structural problem: pause and
      report.
- [ ] **U-S2** Scope growing past the original ask: pause and report.
- [ ] **U-S3** A finding forks the design (two viable directions, different user-visible
      consequences): the owner decides.
- [ ] **U-S4** Findings in the same section three rounds running, or fixes that repeatedly add
      paragraphs where a clause was due: cut per U-C1 and U-F1 first; if it recurs after a cut,
      pause and report.
- [ ] **U-S5** Owner reversal wins over the process; record it and excise the withdrawn material.
- [ ] **U-S6** Do not defend or price a design that fails Part B; rewrite it.
- [ ] **U-S7** Unanticipated case in a design: fail closed and hand to a human, not branch to
      discover which situation applies.
- [ ] **U-S8** Convergence needs a fresh round at full scope returning zero findings. There is no
      round cap and no round number that justifies narrowing.

### U-E — Evidence (author and reviewer)

- [ ] **U-E1** State the exact proposition first, then name the evidence that could falsify *it*.
- [ ] **U-E2** Produced, but who reads it? Ask separately of the code and of the test.
- [ ] **U-E3** Absence claims name the type/module that would contain the thing, and read it. An
      absent parameter is evidence about an interface, not a mechanism.
- [ ] **U-E4** Never assert absence from a capped search (`head`, `-m`); rerun uncapped or count.
- [ ] **U-E5** Proxy is not target: enumerating readers of a field is not enumerating the
      decisions that depend on its fact.
- [ ] **U-E6** Unreachability claims name the component that rejects the state and confirm its own
      doc says so.
- [ ] **U-E7** Causal claims carry scope: "X contributes" is not "X is the only cause".
- [ ] **U-E8** Count after classifying, not after arguing membership toward a threshold.
- [ ] **U-E9** Agreement is not evidence; only the source is. Re-derive an inherited premise
      empirically before a cycle builds on it.
- [ ] **U-E10** A fix that adds explanatory text is grepped for what it falsifies.

### U-I — Implementation and status

- [ ] **U-I1** Reconcile a spec that predates code changes against current code before
      implementation starts.
- [ ] **U-I2** The first implementation increment lands with tests that fail without the change;
      never zero new tests.
- [ ] **U-I3** Then self-review (Stage 2); independent implementation review (Stage 3) under U-I8
      and U-I9, with U-P and U-V otherwise applying; a closure doc (suite counts, pre-existing failures
      named, lint/type status, disclosed deviations).
- [ ] **U-I4** Disclose deviations and out-of-scope observations plainly; track deferred gaps as
      numbered items in the project's memory doc and check them before touching the same files.
- [ ] **U-I5** Status is a bare word (per the project's Status words row). Canonical is the literal
      next edit after the confirming clean round, never written before it.
- [ ] **U-I6** A doc is never deleted. One replaced by a different doc (new subject, new number)
      is set to Closed and points forward in its status line only.
- [ ] **U-I7** A doc edited only as a side effect of another's review does not earn Canonical.
- [ ] **U-I8** Implementation review checks the code against the Implementation Prompt and the design
      it cites, and nothing else. A finding names the clause the code fails and an input the shipped
      code can produce today; anything else is an observation (U-I4: disclosed, no fix, no round).
      A fix restores what the design says. If it needs behaviour the design does not state, or lands
      in an area an earlier fix of this review touched, it is not made in the review: it gets its own
      new design document, run through the design process, and is implemented as its own increment.
- [ ] **U-I9** Implementation review ends at the first round with no findings; there is no
      confirmation round.

---

## Part B: Acceptance criteria (customised per project)

**Purpose.** Part A governs how review runs. Part B governs what the architecture is allowed to be.
It restates the project's own goal as prohibitions, so the architectural mistakes most likely to
undermine that goal are rejected before any review round, not discovered in round fifteen.

The goal differs per project, so the criteria do too:

| Project goal | What the criteria therefore forbid |
|---|---|
| Real-time performance (a game) | Caches or indexes that hide computation; loops that answer a question every tick; per-item bounds that multiply over a population |
| Integration with a system the project does not own (a remote service, a browser session) | Shadow copies of its state; enumerating its states; automated recovery from the unexpected (fail closed to a human) |
| Deterministic simulation / rollback | Frozen copies of live structures; "these callers must do X" lists; checks that change control flow |
| Durable orchestration / persistence | Derived or cached state; side tables beside the canonical store; reconciling two independently changing records |

### Deriving a project's list

- [ ] **B1** State the project's goal or hard constraint in one sentence (performance, determinism,
      safety, auditability...).
- [ ] **B2** List the architectural mistakes that most threaten it, taken from real past failures:
      a series that did not converge, a design converged on and then discarded, a defect class
      that recurred.
- [ ] **B3** Write each as a prohibition on a *shape*, naming this project's own structures
      (template below). A criterion with no project-specific noun belongs in Part A.
- [ ] **B4** Check each criterion against a known bad design from B2: it must fail it by naming a
      structure, not by taste.
- [ ] **B5** Keep the list short (under ten). A criterion that cannot be failed concretely is
      guidance; drop it.
- [ ] **B6** Prohibitions, not heuristics: failing one means rewrite (U-G2), not a finding to patch.
- [ ] **B7** Scope: criteria govern what a design *adds*, never a re-litigation of a settled
      primitive it calls (U-P11). In scope at the boundary is which question a primitive answers.
- [ ] **B8** Reconsider the acceptance criteria when a series passes the project's criteria review
      threshold without converging, converges and is then discarded, or when U-R10 entries name
      the same criterion twice.

**Template**

> **AC-n: Short name.** The forbidden shape, in one sentence. What it shadows, scans or enumerates,
> named for this project. The substitute, in one sentence.

### Recurring shapes (a catalogue to instantiate, not a list to adopt)

These recurred independently across projects with different goals. Adopt a row only where its
population exists in this project, naming that population.

| Shape | Forbidden | Substitute |
|---|---|---|
| Shadow structure | A cache, mirror, side table or state machine duplicating a population the design does not own | Read the live source at point of use |
| Index | An auxiliary structure built only to find or count faster | Fix the addressing |
| Enumerated completeness | "These callers/states must..." lists | One choke point; one generic outcome for "not what I expected" |
| Reconciliation | Logic keeping two independently changing structures consistent | Delete the second; derive on demand |
| Check as control flow | A validator, staleness guard, retry-and-classify loop or confirm gate | Fix the construction; assert, never branch |
| Bounded search | A loop or retry to *discover* a situation, however small the bound | Executing an identified step is fine; searching is not |
| Unreachable source | A consumer rebuilding what its canonical source should answer | Make the source reachable and answering |
| Unneeded primitive | A new type, state field or config field expressible over existing ones | Metadata or a derived value |

---

## Acceptance criteria: box3d

Goal: deterministic, bit-exact rollback over a live solver whose populations (solver sets, the
constraint graph, islands, ID pools) are created, destroyed, merged, split, slept and reclassified
by engine code the design does not control. Origin: the history-ring series
(`docs/designs/20260927-002`, 43 rounds; `docs/designs/20260929-002`, 21 rounds).

- [ ] **AC-1: No shadow structure over an engine population.** No subset, scope set, shadow graph or
      side table duplicating entities tracked by a solver set, the constraint graph, an island or
      an ID pool. Read the live structure at the point of use.
- [ ] **AC-2: No enumerated completeness.** No "the following functions must call X" (journal
      write, invalidation, link update). Route the mutation through one function that is the only
      legal way to perform it, so a bypass fails to compile or asserts.
- [ ] **AC-3: No reconciliation** between a scope over the world and the world, a shadow graph and
      the live graph, or staged values and live records. Delete the second; derive on demand.
- [ ] **AC-4: By construction, not by check.** No validator, generation check, staleness guard,
      "confirm still valid" gate or reconciliation pass. An assertion of a construction-guaranteed
      invariant is fine; it is never control flow.
