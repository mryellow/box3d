---

title: Codex review — bit-exact history ring, round 27
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-29
reviewer: Codex CLI 0.157.0 (gpt-6-sol)
mode: codex exec --sandbox read-only, model gpt-6-sol, reasoning effort medium. Single attempt,
  no resume needed (exit code 0 within the shell-level timeout).

---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 27

## Review report (Codex final message)

## Summary

The whole-world image and journal approach is sound in outline, and the state inventory matches the main source paths I checked. Four points need tightening before the bit-exact guarantee and phase 2 cost target are reviewable.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §7.4 | The proposed CCD “min of results” does not say how it handles a nearer solid hit rejected by `preSolve`. In `src/solver.c`, only an accepted hit reduces the fraction. Taking the raw minimum would change simulation results. | Define a deterministic candidate order and take the minimum **accepted** hit. Add a test with a rejected nearer hit. |
| 2 | Medium | §§6, 9 | §6 correctly requires undoing an unclosed journal segment before a rewind, but §9’s numbered restore procedure omits it. A same-tick rewind after a cold setter would fail if implemented from §9 alone. | Add pending-segment undo and disposal as the first restore step, including the `T == P` case. |
| 3 | Medium | §§12–13 | The phase 2 cost gate uses `large_world`, whose million proxies are static (`shared/benchmarks.c`). It cannot demonstrate that removing raw kinematic and dynamic tree images fixes the stated sleeping-proxy cost. | Add large sleeping dynamic and kinematic proxy scenes to the capture-memory and time acceptance tests. |
| 4 | Low | §§1, 9 | §9 says awake arrays are resized “not reallocated,” while §1 correctly allows a restore-time realloc when the target image exceeds current capacity. | State that arrays are resized in place when capacity permits and reallocated otherwise. |

## Checked, no change

- `ScrubBackward` and `b3HashWorldState` support the document’s limited claim about transforms and velocities; the document correctly calls for a broader hash.
- The six world id pools, proxy free lists, moved tree bits, sensor overlap swapping, hull refcounts, and shape material ownership have corresponding source paths in the inventory.
- The documented CCD sensor cap and explosion traversal-order risks are present in `src/solver.c` and `src/physics_world.c`.
- `docs/faq.md`, `docs/simulation.md`, `test/test_determinism.c`, and `CMakeLists.txt` support the stated determinism baseline and its build constraints.
- The document distinguishes phase 1’s proxy-sized tree images from phase 2’s awake-proportional target, and treats §11.1’s figures as supplied baseline context.

## Proposed edits

Resolve finding 1 in the CCD algorithm and its test; make §9’s restore sequence complete; expand §12’s phase 2 cost scenes; reconcile the array-resize wording.

## Unresolved / disagreements

No disagreement with the whole-world recommendation. The documented solver-owner decision on the order changes remains open. I did not re-measure the supplied performance figures or run tests.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only -m gpt-6-sol -c model_reasoning_effort="medium"`,
stdin from `/dev/null`, foreground, 540s shell timeout and 600000ms Bash-tool timeout. Started
2026-09-29T14:18:50+10:00, ended 14:24:49+10:00, exit code 0 — no resume needed. Codex ran
read-only; `git status --short` before and after was identical, so the working tree was untouched
by Codex. Same fresh full-scope prompt as rounds 18 to 26, no declined-findings list. The final
message above is unchanged.

**Findings verified against source.**

1. **Applied.** In `b3ContinuousQueryCallback` (`solver.c`), a solid hit lowers the running fraction
   only after `preSolveFcn` accepts it, so a rejected nearer hit does not clip the sweep. §7.4's
   "min of the results" over every candidate would take the rejected hit's fraction. It now reads
   the min of the accepted results, and the shape-id-order alternative keeps the first accepted.
   §12 test 7's competing-solid-hits case now includes a nearer hit that `preSolve` rejects.
2. **Applied.** §6 requires walking and discarding the live entries since `historyTick` before a
   rewind's own undo, including a rewind with no step since the last capture, but §9's numbered
   procedure started at the closed segments and did not mention them. Step 2 now begins with
   undoing and discarding the live entries, including when T == P.
3. **Applied.** `CreateLargeWorld` (`shared/benchmarks.c`) creates a million static bodies and
   `StepLargeWorld` drops a few dynamic spheres, so its kinematic and dynamic trees stay small and
   its cost cannot show whether phase 2's removal of the raw tree images works. §12 test 5 now also
   takes a scene of a million sleeping dynamic and kinematic bodies with few awake, and §13's phase 2
   gate names it instead of `large_world`.
4. **Applied.** §9 step 3 said "resize (not reallocate)" while §1 allows a restore-time realloc of a
   resized awake array. Step 3 now sets the counts and grows capacity only when an image count
   exceeds it.

4 applied, 0 declined.

## Status

Finding count rose to 4 (1H-2M-1L) from 3, with a High again after one round without. The High is in
the §7.4 CCD change's interaction with `preSolve`, text that predates round 26; the Mediums and Low
are a restore-procedure omission, a cost gate that cannot exercise what it gates, and a wording
conflict. None is a consequence of round 26's fixes, and none reopened ground the doc's rationale
covered. The counts since round 17 (3, 4, 2, 4, 3, 2, 3, 2, 3, 4) remain flat at 2 to 4, so the
series is not converged.

Next: another fresh, full-scope round. The verdict was `CHANGES_PROPOSED`; not converged.

`Round 27: 4 findings (1H-2M-1L), 4 applied, 0 declined`

`Series total: 132 findings (46H-72M-14L) across 27 rounds`

`Findings: 12 (6H-6M-0L) -> 11 (8H-3M-0L) -> 8 (4H-4M-0L) -> 9 (5H-4M-0L) -> 6 (3H-2M-1L) -> 4 (2H-2M-0L) -> 5 (2H-2M-1L) -> 5 (1H-4M-0L) -> 4 (1H-2M-1L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 4 (1H-3M-0L) -> 7 (2H-4M-1L) -> 5 (2H-3M-0L) -> 4 (0H-3M-1L) -> 6 (1H-4M-1L) -> 4 (0H-2M-2L) -> 3 (0H-3M-0L) -> 4 (1H-2M-1L) -> 2 (0H-2M-0L) -> 4 (2H-1M-1L) -> 3 (0H-2M-1L) -> 2 (1H-1M-0L) -> 3 (1H-1M-1L) -> 2 (0H-2M-0L) -> 3 (0H-2M-1L) -> 4 (1H-2M-1L)`
