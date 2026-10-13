# Changelog: 20261011-001 preflight tree-order-independent solver

## Revision 2 (after round 1)

- §1: dropped the cast clause (the step never casts); "physically equivalent" -> "physically
  comparable"; sensor events past the 8-cap can change (eight lowest ids kept, not nearest).
- A1: order-independence stated for argument-only `preSolve` and custom filters; stateful callbacks
  remain layout-dependent.
- A2: cost sentence corrected to match the `sensors.count == 0` skip; deferred `aabb` write must cover
  mesh, height and sensor shapes the first loop skips.
- §5 decision 1: noted that the sort variant also fixes `preSolve` call order. Decision taken by the
  author per the doc's own recommendation (initial fraction); guarantee narrowed rather than
  switching variant. Recorded in §6.
- §6 Provenance added.
- Not done: Tests / Done-when / §3 test items remain in the doc (U-C2, U-C10); they move when the
  Implementation Prompt exists, since they carry required pre-merge checks (A0 layout-change check).

## Revision 2 (cont.): stand-alone framing

- Owner: these are stand-alone solver improvements with no parent design. Removed the `parent:` front
  matter, the rewind motivation in §1 (replaced by layout depending on build/maintenance history), the
  "What is not needed" section, the rewind items in §4 (replaced by public queries and call-order-
  dependent callbacks), the no-mention-of-rewind checklist item, and "before a consumer exists" in §5.2.
