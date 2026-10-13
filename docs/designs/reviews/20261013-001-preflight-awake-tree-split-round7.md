---
title: Review round 7 - preflight awake tree split
target: 20261013-001-preflight-awake-tree-split.md
date: 2026-10-13
reviewer: codex exec (gpt-6-sol, effort high, read-only)
previous: 20261013-001-preflight-awake-tree-split-round6.md
mode: full pass
---

# Round 7

## Exact request sent to Codex

Identical to round 1 (same prompt text, same scope).

Run: foreground, `timeout 540`, exit 0, 2026-10-13T18:52:48+10:00 to 18:59:05+10:00. Header
confirmed `model: gpt-6-sol`, `reasoning effort: high`.

## Review report (Codex final message)

## 1. Summary

The pair passes, migration hooks, and stated rebuild race largely match the live source. Two defects remain: sparse awake trees can retain the old pair-gather cost, and the proposed operation journal cannot restore a reused proxy slot.

## 2. Findings

| # | Severity | Section | Issue | Proposed action |
|---|---|---|---|---|
| 1 | H | 1, 4, 5.1 | The promised awake-population pair-gather cost does not always hold. `b3GatherMovedSiblings` scans to `nodeEnd`; removing sleeping proxies leaves that high-water mark intact. An empty tree’s rebuild returns without compacting it, and an ordered tree with an unmoved root does not rebuild. Thus a later static or kinematic move can repeatedly trigger a scan of slots formerly occupied by thousands of awake proxies. | Specify compaction or reset of the awake tree after substantial sleep migration, including the empty-tree case; otherwise narrow the cost claim. |
| 2 | M | 5.2 | The cheaper operation journal’s remove entry omits the proxy’s category bits and user data. If a removed slot is reused, allocation clears it; reversing the later create does not recover the earlier values. Re-inserting the old proxy can then restore the wrong filtering bits and shape identity. | Record those values in the remove entry, or retain a proxy-destroy identity entry for this variant, and revise its byte estimate. |

## 3. Checked, no change

- The two-bit proxy key has a spare code, and the identified fixed-index tree readers and walks need the fourth tree.
- The proposed self and cross passes cover the pair classes the current broad phase searches; moved bits propagate to the root.
- Sleep and wake currently leave proxies untouched. Finalization, transform writes, proxy creation, and reset explain why migration must preserve the moved bit.
- Contact processing can wake a sleeping set while the queued rebuild is pending. CCD and explosion also need the stated fourth-tree handling.
- The churn table’s means are consistent with its shape counts and step counts. I found no source-backed reason its stated boundary-scan method would miss a sleep/wake migration in those scenes.
- The awake image covers the tree’s live allocation order; the byte-journal approach covers physical non-awake tree writes when tree order matters.

## 4. Proposed edits

Add an awake-tree compaction rule to B2/B4 and qualify the opening cost claim until that rule is specified. Add category bits and user data to the operation-journal remove payload and update its cost estimate.

## 5. Unresolved

The owner choices in section 6 remain open.

## 6. Verdict

CHANGES_PROPOSED

## Post-review verification (Claude)

1. **Adopted (H).** `b3GatherMovedSiblings` scans every pair below `nodeEnd`; `b3RemoveLeaf` keeps
   DFS order and marks nothing, so after a sleep migration the awake tree is ordered with an
   unmoved root and `b3DynamicTree_Rebuild` returns without compacting; an empty tree returns at
   once (`dynamic_tree.c`, `broad_phase.c`). Fix: B2 marks the awake tree's root moved after a
   sleep migration so the step's rebuild compacts `nodeEnd`, and resets an emptied tree; 5.1's
   bound follows (nodes compact at the next step, the `proxies` array keeps its high-water mark).
2. **Adopted (M).** "The proxy slot" in the operation-log remove payload meant the slot's 24 bytes
   (user data, category bits, node, next) but did not say so. Fix: spelled out; the estimate stands.

## Owner decisions received with this round (U-S5)

- Decision 1: the split applies to the kinematic tree as well. Five trees; the proxy key's type
  field widens to three bits. B0, B1, B4, 5.1, 5.2 and §6 revised accordingly in the next revision.
- Decision 4: the implementation branch's base, including whether the tree-order PR is applied
  first, is set by the Implementation Prompt. Decision 4 removed from §6; §1 says so.

## Status

- Verdict: CHANGES_PROPOSED. Not converged (U-S8).
- Round 7: 2 findings (H1-M1-L0), 2 applied, 0 declined.
- Findings: 4/3/0 -> 1/4/2 -> 1/2/2 -> 0/1/0 -> 2/0/0 -> 2/3/0 -> 1/1/0 -> pending
- Tripwire: fixes add or remove a named mechanism: one rule added (mark the awake root moved after
  a sleep migration). Clustering: the high-water bound drew findings in rounds 1, 3 and 7, each a
  different consequence of removes not compacting (U-T2). Reviewer read of round records or
  changelog: none in the raw log. Doc length: 328 -> 336 lines; round 6's fixes added 8 lines. The
  next revision also carries the owner's kinematic decision, a structural change outside the
  findings.
