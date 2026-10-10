# Changelog: 20261011-003 preflight snapshot userData

## Revision 1 (after round 1)

- §3: removed a duplicated "Three candidates" sentence; re-wrapped the intro line.
- §4 U2: free-slot liveness is `body.id == i`, `shape.id == i`, `joint.jointId == i`. The earlier text pointed at the contact test (`c->contactId == i`), which does not apply to bodies or joints, and body/joint writers have no liveness test today.
- §4 U2 test 5: checks the image slot and the restored record directly; creating a body overwrites `userData`, so the old create-then-read check could not detect a stale pointer.
- §4 U2 test 3: `b3Body_IsValid` on the id is false; the getter asserts a valid id.

## Revision 2 (after round 2)

- §3: the "one-line change" claim about a future default flip is removed; zero flags means off permanently. §6 item 2: an on-by-default option needs an opt-out flag.
- §3: unknown flag bits return `NULL` with no assert; the default assert handler breakpoints, so the old promise could not be met in debug.
