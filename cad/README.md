> **Update 2026-09-25: the clamp is now a PUSHER.** Delivery pockets are closed,
> so the piece is held just above the stand and pushed off the fork tip.
> Print `pusher_arm.stl` (x1, PETG, flat, no supports) instead of `clamp_arm.stl`.
> Built by `make_pusher.py`; geometry and mounting in that file's header;
> side view in `pusher_side_view.png`. The lift is driven by a servo too.

# End Effector CAD — SC2026 Skill 23

Fork + servo clamp, no gripper. Source: [`endeffector.scad`](endeffector.scad).

This replaces an earlier gripper design (`gripper.scad`, described in the
old version of this file) built for a *different* competition's object (a
60mm box). This year's object is a 75×75×10mm **flat piece**, reached
through a 50×30mm slot — a gripper's jaws can't fit through that opening
at all. `gripper.scad` and `lift.scad` are kept in this folder only as
old files; don't print from them.

## Why this shape

The piece is flat and rests inside a 100mm storage cube, reachable only
through a 50×30mm side slot 30–60mm off the floor (confirmed from the
official `SC2026_TP23` appendix). A gripper's jaws can't pass through
that slot. The fork can: it's a flat U-shaped scoop that slides in under
the piece, lifts a few millimetres to take the load, and carries it — no
grip force to tune, nothing to drop by opening at the wrong moment.

The one thing a bare fork can't do is stop the piece sliding off during a
mecanum strafe. That's what the **clamp servo** is for: a single arm that
swings down and presses the piece flat against the fork once it's
loaded. One motor, one rigid link, pivoting like a gate on a horizontal
shaft — not a second gripper, just a hold-down.

## The three printed parts

| Part | File | Qty | Notes |
|---|---|---|---|
| Fork | [`fork_combined.stl`](fork_combined.stl) | 1 | One piece — both tines + crossbar. Prints lying flat, **no supports needed**. |
| Clamp arm | [`clamp_arm.stl`](clamp_arm.stl) | 1 | Bolts to the servo horn. Hub matches the Studica 76148 horn exactly (Ø29mm disc, Ø10mm boss, 4 holes on a 16×16mm square). |
| Servo bracket | [`servo_bracket.stl`](servo_bracket.stl) | 1 | Sized to the **real** Studica 75002 servo (measured from its own STEP file, not a generic guess). Bolts to a Studica U-channel. |

All PETG, no exotic settings needed. Print one of each first and confirm
they mate correctly before committing to a second set as spares.

## How it works — read this before assembling

The clamp servo's shaft is **horizontal**, not vertical. That's
deliberate and easy to get backwards: a vertical shaft can only swivel
the arm sideways in a flat plane, which can never reach down onto a
piece. A horizontal shaft lets the arm swing through a vertical arc —
open (tipped back, clear of everything) to hold (swung down, pressing
flat). If you ever redesign the bracket, keep the shaft horizontal.

Full pick/place sequence, in order, is documented in
[`../robot-onboard/src/Oms.cpp`](../robot-onboard/src/Oms.cpp)'s header comment — read that before
wiring anything, since doing the steps in the wrong order drags the
piece against a wall or rim.

## Numbers

| | |
|---|---|
| Fork width | 46mm (fits the 50mm slot with 2mm clearance each side) |
| Fork thickness | 5mm |
| Storage slot | 50×30mm, bottom edge 30mm off the floor |
| Delivery pocket clearance | 95mm inner vs. 46mm fork — ~24mm clearance each side, only the near wall matters |
| Clamp arm hub | Ø29mm disc, Ø10mm centre boss, 4× Ø3.5 holes on a 16×16mm square (Studica 76148 horn pattern) |
| Servo bracket mounting | Studica U-channel pattern (76014): Ø3.5 holes at ±8mm around each 48mm-pitch cell |

## Kit interface — confirmed against real Studica STEP files, not guessed

Every dimension above was measured directly from `75002 - Multi-Mode
Smart Servo.stp`, `76148 - Servo Horn 25T.stp`, and `76014-240mm-u-channel.stp`
— not estimated from a generic servo, and not carried over from the old
gripper's numbers. If you have a different servo or a different U-channel
length, re-measure from its own STEP file rather than reusing these.

Studica's hole pattern is **not** a simple uniform grid — it's Ø3.5 holes
at ±8mm around each 48mm-pitch channel cell centre. `fork_combined`'s
mounting holes and `servo_bracket`'s channel-facing holes both follow
this exact pattern, verified by direct comparison against the channel's
own STEP geometry.

## Build

```bash
cd cad
py make_assembly.py           # fork + clamp arm -> STEP
py make_servo_bracket.py      # servo bracket -> STEP, checked for zero
                               # interference against the real servo model
```

Both use [CadQuery](https://cadquery.readthedocs.io/) (`pip install
cadquery`) to produce real solid STEP geometry — not a triangulated
mesh — so they open cleanly in Inventor as proper bodies you can
constrain with joints. `endeffector.scad` (OpenSCAD) is the same design,
kept in sync by hand, and is what actually produces the `.stl` files for
printing.

If you change a dimension, change it in **both** `endeffector.scad` and
the matching `make_*.py` script, or the STL (for printing) and the STEP
(for Inventor/assembly planning) will silently drift apart.

## Still to verify before printing a full spares set

1. **Servo PWM channel, clamp open angle, clamp hold angle** on the
   Settings page (`tools/settings.html`) — set from real bench testing,
   not from CAD.
2. **Which face of the U-channel** the servo bracket actually bolts to
   on your real carriage — the CAD assumes a specific cell alignment.
3. Print one servo bracket and confirm the real 75002 servo actually
   drops into the pocket and its ears land on the screw holes before
   printing a second one.

## Mass budget

3D printed parts have a weight cap per `SC2026_IL23` — weigh the three
parts above as you print them and check against it, including spares.
