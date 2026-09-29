---

title: Codex review — bit-exact history ring, round 33
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 33

## Review report (Codex final message)

## Summary

The whole-world rewind recommendation is reasonable, and the document identifies several real costs and order dependencies. I found two gaps in the proposed compile-time journal guarantee, plus an incomplete account of the solver work proposed for future incremental replay. I reviewed the document and permitted source read-only; I did not run tests. I did not read the other design files or anything under `docs/designs/reviews/`.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §5.2, §7.3, §13 | The proposed hot accessor exposes awake set arrays and sensors, but excludes sparse records. The step currently writes awake sparse records directly, including contact flags in [physics_world.c](src/physics_world.c) and shape AABBs in [solver.c](src/solver.c). Making those records read-only outside their write accessors, as phase 1 requires, leaves these hot paths with no permitted write route. | Specify phase-checked hot write access for awake sparse records, including their owned blocks, and explain how it coexists with structural journaling. |
| 2 | High | §2 requirement 7, §5.2 | Const access to a record does not make a pointer *inside* it const. For example, a const `b3Contact` can still expose its mutable `manifolds` pointee ([contact.h](src/contact.h)). The proposed writable sensor-element accessor also exposes fields beyond overlap scratch ([sensor.c](src/sensor.c)). Thus the stated “cannot reach the underlying field any other way” compile-time guarantee is not established for these sibling cases. | Make journal-owned pointees and sensor structural fields inaccessible through read or hot views; provide narrow operations for each permitted hot mutation. |
| 3 | Medium | §11.2(4), §15 | The inventory names six barriers to exact incremental replay, but its proposed solver changes address only four. Wake recolouring in stored order and merge-survivor choice remain without a remedy, while §15 says the listed changes would make incremental replay possible. | Add concrete ordering rules for wake recolouring and merge choice, or describe the four changes as partial prerequisites. |
| 4 | Low | §3 | “Hash tables are never iterated, only queried” is too broad: the hull database is iterated for statistics in [physics_world.c](src/physics_world.c). This does not appear to affect simulation determinism. | Qualify the statement to simulation decisions. |

## Checked, no change

- [docs/faq.md](docs/faq.md) says rollback determinism is unavailable; the design correctly distinguishes that public capability from the narrower recording test.
- `ScrubBackward` compares replay hashes, and [recording.c](src/recording.c) hashes body transforms and velocities only. The document states that limit rather than treating the test as proof of full-state exactness.
- The proxy free list is separate from the world id pools ([dynamic_tree.c](src/dynamic_tree.c)); accounting for proxy identity is necessary.
- The sensor pass sorts and deduplicates overlaps ([sensor.c](src/sensor.c)), and the documented CCD sensor cap and running-fraction dependence are present in [solver.c](src/solver.c).
- The document consistently treats events at restored tick T as unavailable and events after T as replay outputs. Its budget discussion also explicitly permits overruns.

## Proposed edits

Resolve findings 1 and 2 together with an access table for each record and owned block: who may read it, who may write it during a step, and which operation journals structural writes. Then complete the six-case incremental replay inventory and narrow the hash-table wording.

## Unresolved / disagreements

The tree-order changes remain a deliberate solver-behaviour decision for the owner, as §16 says. The reviewed source and existing hashes do not establish full-state bit exactness; the proposed full-state hash and replay tests are appropriate gates.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T15:54:07+10:00, ended 15:59:39+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` (excluding review files) before and after was identical, so the
working tree was untouched by Codex. Same fresh full-scope prompt as rounds 18 to 32, no
declined-findings list. The final message above is unchanged apart from absolute path prefixes and
line-number suffixes on its links.

**Findings verified against the doc and source.**

1. **Applied.** The step writes awake sparse records in place (`physics_world.c` sets contact
   flags such as `b3_simDisjoint`, `solver.c` writes a shape's `aabb`), and §5.2 made those record
   arrays const outside the write accessor while round 32's sharpening limited the hot accessor to
   set arrays and sensors, leaving the hot path no write route. §5.2 now has the hot accessor also
   return element pointers to the record of an awake owner, asserted awake and in a step, and
   states that structural writes go through the write accessor even inside a step.
2. **Applied.** A `const b3Contact*` still yields a writable `manifolds` pointee (`contact.h`), so
   const-only record access did not cover owned blocks, and the sensor element handed to the sensor
   pass exposed `shapeId` and other fields beyond the scratch arrays (`sensor.c` swaps `overlaps1`
   and `overlaps2` and clears the latter). §5.2 now declares block-holding fields pointer-to-const,
   with the block's write function in its defining file the only cast, and gives the sensor pass a
   view of `overlaps1`, `overlaps2` and `hits` rather than the element.
3. **Applied.** §11.2 (4) lists six order dependences and four remedies; wake recolouring in stored
   order and merge survivor choice had none. Two remedies are added (wake recolouring in
   shape-pair-key order; merge survivor and link order by lowest body id among the merged islands).
   Body ids are caller-visible and identical in a replay for an island neither timeline touches, unlike
   island ids from the global pool. Both remain future work, each with its own review, as the section says.
4. **Applied.** `physics_world.c` iterates the hull database to total statistics bytes, so §3's
   "never iterated" was too broad. It now says never iterated by anything that affects simulation
   state; §12 already treats the map's bucket order as not state.

4 applied, 0 declined. The status line's finding sequence and round count in the target doc were
updated to include this round.

## Status

Finding count is 4 (2H-1M-1L), the same as round 32. Finding 1 is a consequence of round 32's own
fix (its sharpening of the hot accessor removed the write route the step needs), and finding 2
is the same §5.2 boundary one layer down (pointees and sensor fields), not previously
covered. Findings 3 and 4 are older text (§11.2 and §3). None reopened settled rationale. §5.2's
compile-time boundary has now produced Highs in rounds 32 and 33, each found by tracing a sibling
case (pointee blocks, sparse records, sensors) of the mechanism, which is a signal to re-check that
paragraph against every record type before the next round. The counts since round 17 (3, 4, 2, 4,
3, 2, 3, 2, 3, 4, 3, 3, 3, 3, 4, 4) remain 2 to 4; not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 33: 4 findings (2H-1M-1L), 4 applied, 0 declined`

`Series total: 152 findings (50H-83M-19L) across 33 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L) -> 3 (0H-3M-0L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 3 (0H-2M-1L) -> 4 (2H-1M-1L) -> 4 (2H-1M-1L)`
