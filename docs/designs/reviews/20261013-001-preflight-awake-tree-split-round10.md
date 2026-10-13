---
title: Review round 10 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round9.md
mode: full pass
---

# Round 10

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T19:32:57+10:00 to 19:40:46+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The split’s pair passes, proxy-key encoding, sleep and wake migration, and rewind treatment are broadly consistent with the cited source. I found one new traversal hazard in B3: locking public queries does not stop every API that can wake a sleeping body and migrate its proxies.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 2, B3 | A public query callback can call `b3Body_SetLinearVelocity` with a nonzero velocity, or `b3Body_ApplyForce` with `wake=true`, on a sleeping body. These paths use `b3GetWorld` and `b3WakeBodyWithLock`, whose lock check is only an assertion (`src/body.c`). In a build without assertions, the wake migrates proxies during the tree traversal and can invalidate the node array held by `b3DynamicTree_Query` (`src/dynamic_tree.c`). Before B2, that wake made no tree write. | Make the chosen traversal policy cover every wake-capable API at runtime, or defer all proxy migrations until traversal ends. State the rule in B3. |

## 3. Checked, no change

- The four tree codes fit the existing two-bit proxy key; the body-type count remains three (`src/broad_phase.h`, `src/broad_phase.c`).
- B1’s stated self and cross passes cover the current moved-proxy pair cases, including sleeping–sleeping contacts and static compounds (`src/broad_phase.c`, `src/contact.c`).
- The rebuild is enqueued before contact-state processing, where a begin-touch can wake a sleeping set (`src/broad_phase.c`, `src/physics_world.c`, `src/island.c`).
- The listed fixed-index readers, CCD bullet query, explosion traversal, and sensor queries require the fourth-tree handling described (`src/physics_world.c`, `src/solver.c`, `src/sensor.c`).
- The section 4 means are consistent with its shape counts and step counts; the cited `sleep` scene has ten 210-shape pyramids (`shared/benchmarks.c`).
- The awake-tree image sizes and the distinction between physical awake restore and journaled non-awake proxy identity are consistent with `b3DynamicTree` and the cited rewind design (§§5.4, 7.4).

## 4. Proposed edits

Add the wake-capable paths in `src/body.c` to B3’s callback analysis. Specify the runtime guard or deferral point that prevents migration throughout a public traversal.

## 5. Unresolved

None within the requested scope.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3Body_SetLinearVelocity` and `b3Body_ApplyForce` get the world with
   `b3GetWorld` and wake through `b3WakeBodyWithLock`, which asserts the lock is clear, sets it,
   wakes, and clears it (`body.c`); in a build without assertions a query callback's wake proceeds
   while `b3DynamicTree_Query` holds the node array. A lock that only some entry points check at
   runtime cannot be the rule. Fix: B3 is rewritten around one rule at the migration choke point:
   when `world->locked` is set the migration is queued, and the queue drains when the lock is
   released and, in the step, right after the tree task is joined (`solver.c`), which precedes
   the finalize stage that runs CCD. Every site that sets or clears the lock does so through one
   lock and unlock pair, so draining is a property of unlock, not a list of callers. The same rule
   resolves the rebuild race (a begin-touch wake in `b3Collide` queues, the drain follows the
   join) and the explosion traversal (Explode holds the lock already). Decisions 2 and 3 in §6 are
   therefore settled by construction and removed; the reason is in Provenance.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 10: 1 finding (H1-M0-L0), 1 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> 0/1/1 -> 0/1/1 -> 1/0/0 -> pending
- Tripwire: fixes add or remove a named mechanism: one added (a migration queue drained on
  unlock), two removed (the join-before-contact-state rule and the per-traversal lock rule, both
  subsumed). Clustering: B3's traversal rule drew findings in rounds 5, 6 and 10, each a wake
  path the previous fix did not cover (U-T4): this fix replaces the enumeration of readers and
  guards with one choke point. Reviewer read of round records or changelog: none in the raw log.
  Doc length: 357 -> 362 lines. Past the criteria review threshold (10 rounds); noted for B8.
