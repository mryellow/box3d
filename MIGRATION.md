# Migrating from Box3D to Box3D-Evo

Behaviour changes a program written against Box3D meets when it moves to Box3D-Evo. Each entry
says what changed, why, and what to do about it. API additions that break nothing are not listed.

## `b3Body_SetTransform` wakes the body

**Box3D.** Setting the transform of a sleeping dynamic or kinematic body left it asleep at the new
transform. Its broad-phase proxy was marked and the next step searched the sleeping proxies for the
new overlaps, creating the contacts with other sleeping or static bodies in the disabled set.

**Box3D-Evo.** `b3Body_SetTransform` wakes a sleeping dynamic or kinematic body, with the island it
is touching, as the velocity, force and impulse setters already do. The body then settles at the
new transform under the simulation. Static bodies are unchanged: moving one wakes nothing, as
before.

**Why.** The broad-phase keeps the proxies of sleeping dynamic bodies in a tree of their own, so
the step's pair search follows the awake bodies and not the sleeping population. Waking the moved
body means that tree never holds a moved proxy of its own and is never searched for one: the step
runs no self pass over the sleeping tree, and the moved body's pairs are found from the awake tree
with everything else that moved.

**What to do.** If a body should stay asleep where it is put, call `b3Body_SetAwake( bodyId, false )`
right after `b3Body_SetTransform`. The body is asleep again at once, and the next step still finds
its new overlaps: contacts with sleeping and static bodies go to the disabled set as before, and a
contact with an awake body wakes it when they touch. Code that moved sleeping bodies as a cheap
operation and relied on them staying asleep now pays for a wake unless it does this: a woken island
is simulated until it comes to rest again, half a second of rest by default.
