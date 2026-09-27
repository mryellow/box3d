---

title: Codex review of docs/designs/20260927-001-client-prediction-rollback.md (round 6)
target: docs/designs/20260927-001-client-prediction-rollback.md
date: 2026-09-27
reviewer: OpenAI Codex CLI v0.157.0 (model gpt-6-sol / "Sol", reasoning effort high)
session: 01a0e07b-f41f-7152-893d-8d686fe5d061
mode: codex exec, sandbox read-only; Codex reports, Claude verifies and applies agreed edits

---

# Codex review — docs/designs/20260927-001-client-prediction-rollback.md (round 6)

## Review report (Codex final message)

## Summary

Round 6 **eliminates the old scope-set, second-graph, and shadow ownership conflicts**: a mixed
island can remain intact while only recorded bodies participate in a solve pass. It has **not
converged yet**, though. The source shows four High issues in the replacement mechanism, chiefly
joint effective mass, normal sleep transitions, compact-list lifetime, and the sleeping-neighbor
timestamp. These are narrower than rounds 3–5's ownership conflicts, but they need design rules
before a prototype can validate the proposal. This was a read-only review; no files were modified.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §9.2 | **A null joint solver index does not make an endpoint immovable.** Revolute, distance, and prismatic prepare copy both bodies' real inverse mass and inertia *before* assigning nullable indices; those values determine effective mass and remain in warm start and solve (`revolute_joint.c:279`, `distance_joint.c:268`, `prismatic_joint.c:320`). Widening the index check alone gives the frozen partner zero sampled velocity but still lets its mass weaken the scope body's response. Contact prepare does zero mass correctly, but it decodes a stored awake index and validates that encoding against the real body (`contact_solver.c:86`). | In every joint prepare, zero the effective mass and inertia of an inactive endpoint **before** computing constraint masses. In scalar and SIMD contact prepare, derive pass-local indices without changing the contact's stored awake encoding or its validation invariant. |
| 2 | High | §9.2 | **Normal sleeping can invalidate the fixed body list during replay.** Finalization marks only visited bodies' islands awake; the subsequent pass tries to sleep every unmarked awake island (`solver.c:816`, `solver.c:2221`). With scoped finalization, unrelated islands can sleep, and a scope island can sleep while its body remains in the bracket's fixed list. Sleeping removes and swap-compacts awake body states and transfers constraints (`solver_set.c:216`). Thus ordinary stepping can change indices and graph ownership during the bracket even though Begin/End perform no transfer. | Define replay sleep behavior explicitly: protect unvisited islands, and either defer sleep for scope islands until End or rebuild all affected body/constraint indices and lists after transitions. Include a mixed island and a scope body reaching sleep in validation. |
| 3 | High | §9.2 | **Append-only compact lists are not stable across the documented lifecycle.** A contact can change between the awake non-touching array and a graph colour, be destroyed, and have its ID reused; joints can be removed and reused too (`physics_world.c:968`, `contact.c:468`, `joint.c:802`). Appending at creation or a touching transition does not remove or reclassify old entries. A stale entry can skip a live boundary constraint, process one twice, or resolve to an unrelated reused ID. | Specify removal, reclassification, deduplication, and generation checks for bracket lists, or rebuild them from current scope-body edges once per replay step. The latter remains O(scope + boundary). |
| 4 | Medium | §9.2 | **Per-colour IDs alone do not feed the existing parallel solver.** Its stages size buffers and blocks from full colour arrays, pack convex contacts into SIMD groups, assign mesh manifold offsets, and use separate serial overflow storage (`solver.c:1471`, `solver.c:1528`, `contact_solver.c:2270`). A subset retains the graph's conflict-free colouring, but the current counts, spans, and overflow functions would still process full arrays. | Specify pass-local convex, mesh, and joint spans; packed constraint buffers and counts; and a scoped serial overflow path. Keep the graph's existing colours for scheduling. |
| 5 | High | §9.1–9.2 | **`validSinceTick ≤ T` can admit a body changed after T.** A mutation between completed ticks T and T+1 stamps the still-current `historyTick` T. Sleep entry also occurs inside step T+1 before the proposed end-of-step increment, so it stamps T (`solver.c:2192`). Either body then appears unchanged since T. The counter is also rewound during replay, so calling it monotonic across the whole session is inaccurate. | Define the stamp as the first history state for which the body's current state is valid, with explicit timing for between-step APIs and sleep entry. Specify how replay writes or restores that stamp, and test the immediate-after-T mutation case. |
| 6 | Medium | §9.2, §10 | **The slot arithmetic is fixed, but the Begin guard is only asserted in prose.** Incrementing then writing places the first replay step in T+1 as intended. `lastRestoredTick` alone cannot establish that no ordinary step occurred after restore; the current step path has no such rollback state (`physics_world.c:1038`). | Specify a pending-restore flag and the saved `historyTick`/step serial, invalidate the flag on ordinary Step, and require both at Begin. Save `now` at Begin for End's exact replay-count check. |

## Checked, no change

- **Old ownership conflict:** No scope transfer is required. The existing graph can retain
  mixed-island contacts and joints, and its colour conflict rule remains valid for a subset
  (`constraint_graph.c:216`). The document's "bit-for-bit identical before Begin and after End"
  wording applies only to Begin/End's own bookkeeping; restore and ordinary replay steps still
  alter live contacts and islands.
- **Contact restore:** Destroying and recreating contacts gives a genuinely cold contact. The
  document now discloses contact-ID churn and the artificial end/begin event pair;
  consumption-time shape-generation rechecking closes the stated reuse risk.
- **History and quality:** Increment-then-write removes round 5's slot offset, the API and Begin
  now agree that Begin has no T argument, and §13 gives a 2-degree angular bound.

## Round 5 findings: disposition

1. **Closed** — Resim no longer transfers part of a mixed island into another solver set.
2. **Closed** — There is no second graph or solver set for validators to count.
3. **Reopened differently** — Shadows are gone, but joint prepare still uses a frozen awake
   endpoint's real inverse mass (finding 1).
4. **Reopened differently** — Boundary joints retain ordinary ownership; their creation, removal,
   and reuse still require correct compact-list maintenance (finding 3).
5. **Still open** — The added stamp covers more events, but its value is wrong for changes
   immediately after T (finding 5).
6. **Reopened differently** — Whole-set wake need not define scope membership, but normal sleep
   during replay can invalidate fixed scope indices (finding 2).
7. **Closed** — Contact identity and event changes are explicitly accepted, and shape generations
   are rechecked when staged impulses are consumed.
8. **Closed** — §13 now states an angular metric and a numeric bound.
