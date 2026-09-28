---
title: Codex review — bit-exact world history ring
target: docs/designs/20260927-002-bit-exact-history-ring.md
date: 2026-09-28
reviewer: Codex CLI 0.157.0, gpt-6-sol
mode: broad (round 8), fresh session, no resume needed
---

# Codex review — docs/designs/20260927-002-bit-exact-history-ring.md — round 8

## Review report (Codex final message)

## Summary

The hot image and cold journal approach is broadly consistent with the source tree. Two restore paths need more detail before the design can guarantee a valid, replayable world.

## Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | High | §6, §9 | Capture includes an awake mesh contact's `triangleCache`, but restore describes only manifold allocation and copying the rest of `b3Contact`. The cache is a heap-backed array that the narrow phase resizes and reads. Copying its array header from the image could install a stale pointer; leaving the live header in place would retain the wrong cache. (`contact.h`, `mesh_contact.c`) | Restore the captured cache count and contents into live storage, and exclude its data pointer from the contact record copy. |
| 2 | Medium | §5.3, §7.1, §9 | Sensor destruction frees `hits`, `overlaps1`, and `overlaps2`, while the ownership rule names only `overlaps2`. Although the first two arrays are scratch, undoing destruction must give their restored array headers valid storage or reset them. The next sensor step swaps `overlaps1` and `overlaps2` and can append through the former scratch buffer. (`sensor.c`, `sensor.h`, `shape.c`) | Specify how undo and redo handle all three sensor array headers and their storage. Reset scratch arrays to valid empty arrays if their contents are not retained. |

## Checked, no change

- Contact pair keys are sorted before contact creation, supporting §7.4's tree traversal independence for pair creation. (`broad_phase.c`)
- The sensor task compares the new overlap set with the previous one and sets `eventBits` when it changes, supporting §5.2's change signal. (`sensor.c`)
- The existing recording test restores a keyframe, steps forward, and compares the hashes covered by the current hash function, as qualified in §1. (`test/test_recording.c`, `recording_replay.c`)
- The source defines 24 graph colours, matching the inventory in §5.1. (`constants.h`, `constraint_graph.h`)

## Proposed edits

1. In §9, add a mesh-contact restore step alongside manifold restore: resize or allocate the live `triangleCache`, copy the captured entries, and preserve its live pointer when copying `b3Contact`.
2. In §7.1 and §9, define sensor array ownership across destruction, undo, and redo, including valid initialization of the scratch arrays. Add both cases to §12's restore and replay tests.

## Unresolved / disagreements

None.

## Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

**Invocation.** `codex exec --sandbox read-only` (CLI default model/effort), foreground, stdin
from `/dev/null`, shell-level `timeout 540`, Bash tool `timeout: 600000`. Start
2026-09-28T20:09:03+10:00, end 20:11:33+10:00, `EXIT_CODE 0` — no resume needed, returned inline
in under 3 minutes. Raw log had the final message duplicated (a `codex exec` streaming artifact);
one copy kept above. Codex ran read-only; working tree was untouched by it going in. The curated
declined-findings list from rounds 1–7 (nine items) was included in the prompt; Codex did not
re-raise or dispute any of them.

Each finding independently re-verified against source (via a forked sub-agent):

- **#1 (mesh-contact `triangleCache` pointer not excluded from the `b3Contact` record copy at
  restore, mirroring round 5's `manifolds`/island-array bug), CONFIRMED, applied.** Confirmed
  `triangleCache` (`b3Array(b3TriangleCache)`, `contact.h`) lives in `b3MeshContact`, a member of
  the union embedded directly in `b3Contact` (alongside `b3ConvexContact`) — not inside the
  separately-restored `manifolds` allocation. §9 step 3's existing carve-out protected only
  `manifolds` from the "copy the rest of the record" clause, leaving `meshContact.triangleCache`
  exposed to exactly the same stale-pointer hazard round 5 fixed for `manifolds`. Confirmed this
  is a live, previously-solved-once hazard, not speculative: the Phase-0 serializer
  (`world_snapshot.c`, `b3SerContacts`/`b3DesContacts`) already null out
  `meshContact.triangleCache.{data,count,capacity}` around the raw struct copy specifically
  because "`b3Contact` is not fully POD," naming this field alongside `manifolds`. Fixed by
  extending §9 step 3 with the same resize-and-copy-content treatment already given to
  `manifolds`, and adding `meshContact.triangleCache` to the fields excluded from the record
  copy.

- **#2 (sensor `hits`/`overlaps1` arrays freed by destroy are unprotected by the ownership-transfer
  mechanism that only names `overlaps2`), CONFIRMED, applied — for a sharper reason than the
  summary states.** Confirmed `b3DestroySensor` (`sensor.c`) unconditionally frees all three
  arrays via `b3Array_Destroy` (`container.h`), which zeroes the header — safe going forward, a
  zeroed header is a valid empty array. The actual hazard is on *undo* of the destroy: §7.1's
  ownership-transfer paragraph names only `overlaps2`, so `hits`/`overlaps1` fall through to the
  default structural-write journal entry, a plain record-write of the `b3Sensor` struct's old
  bytes — captured *before* `b3DestroySensor` ran, so those old bytes still hold the live
  pre-destroy pointers. Once `b3DestroySensor`'s frees run, undo copying those old bytes back
  verbatim reinstalls dangling pointers (not the harmless zeroed state destroy itself would leave)
  into the live `b3Sensor`, which the next `b3SensorTask` step's swap-then-append then operates
  on. (`overlaps1`'s *content*, once its pointer is non-dangling, is separately self-healing each
  step via the swap with `overlaps2`, whose ownership-transfer already restores it correctly — the
  gap is specifically about pointer lifetime for `hits`/`overlaps1`, not their content or order.)
  Fixed by extending §7.1's ownership-transfer list from "a destroyed sensor's `overlaps2` array"
  to all three arrays.

## Status

Round 8 of an ongoing series (rounds 1–7 committed). Both findings were genuine and applied, both
in the same bug class as round 5's #1: a live-state-corrupting stale/dangling-pointer restore bug
that a "copy the rest of the record" or "copy the old struct bytes back" clause introduces for a
heap-owned pointer field the clause doesn't know to exclude. This is the third round to find this
exact class of defect (round 5 for `manifolds`/island arrays, round 8 for `triangleCache` and
sensor arrays) — each time in a *different* structure, suggesting the underlying risk (any
pointer-bearing struct copied wholesale by "copy the rest of the record" or a structural-write
undo) is systemic rather than fully enumerated yet. Finding rate (8, 8, 8, 7, 8, 3, 1, 2 applied)
ticked back up from round 7's low. Another full-scope round is warranted, with attention to
whether any other pointer-bearing struct in the inventory still has an unguarded wholesale-copy
path.
