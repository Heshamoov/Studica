# Lab exercises — SC2026 Skill 23

Progressive exercises, each with a **pass criterion** you can actually
measure. They double as the team's regression suite: when something
breaks the week before competition, re-run them in order and the first
failure tells you where the problem is.

Rule for every lab: **write the number down.** "It works" is not a
result. "Drove 1000mm, measured 985mm, −1.5%" is.

This replaces an earlier version written for a different competition
(ESC2026 — gripper, shelves, fruit vision). This year's robot has a fork
+ clamp servo, no extension axis, and colour-sorts flat pieces instead of
identifying fruit.

---

## Stage 0 — Before you touch code

### Lab 0.1 · Map the hardware
Open `tools/portplanner.html`. Add every device physically on the robot
and give each one the name you'll see on the Settings page (`tools/settings.html`).

**Pass:** no pin clashes flagged, and every channel in the tool matches
what's actually wired.

### Lab 0.2 · Connect and look around
Get onto `WorldSkills-1234`, reach the robot at `10.12.34.2`, run `./run.sh`
on the robot and open `tools/settings.html` (`docs/FIRST-HOUR.md` §0-1).

**Pass:** everyone on the team can get the Settings page to say
"connected" without help.

---

## Stage 1 — Make it move

### Lab 1.1 · One motor, one direction
Spin one wheel at 30% for two seconds. Nothing else.

**Pass:** the correct wheel turns the direction you expected.

**If it turns backwards:** tick that wheel's **reversed** box on the Settings page
(`INVERT_*`), **not** by negating in a behaviour. Otherwise
drive and odometry disagree, and you get a robot that moves correctly
but doesn't know where it is. That bug takes a day to find.

### Lab 1.2 · Read an encoder
Turn a wheel by hand exactly one revolution and note how far the robot
thinks it went (position in the Settings page's live readout).

**Pass:** one revolution reads 2π × wheel radius (about 320 mm for a
51 mm radius), within 2%.

### Lab 1.3 · Verify the mecanum signs
Put the robot **on blocks** and command forward, strafe left and turn
from the panel.

**Pass:** forward — all four wheels the same direction; strafe in an X
(FL/RR one way, FR/RL the other); turn with the two sides opposed.

**Why on blocks:** a sign error at floor level launches the robot
sideways into a table leg.

### Lab 1.4 · Teleop
Gamepad → drive. Left stick moves, right stick rotates.

**Pass:** all eight directions behave as expected and it stops when
released. A button aborts any running sequence.

---

## Stage 2 — Make it accurate

### Lab 2.1 · Drive straight, 1 m
Command `forward:1000`. Measure with a tape.

**Pass:** within ±20mm, **three runs in a row**. One good run is luck.

**Record:** commanded, measured, error %, battery voltage. Then repeat
on a low battery — voltage changes everything, which is exactly why
closed-loop control exists.

### Lab 2.2 · Turn 90°
`turn_by:90`, encoder-only first, then again with the navX IMU fused in
(already the default in `Movement.cpp`'s odometry).

**Pass:** within ±2° using the IMU, and clearly better than
encoder-only would be.

### Lab 2.3 · The square
Drive a 1m square (`forward:1000, turn_by:90` × 4) and return to start.
Mark the floor with tape first.

**Pass:** finishes within 50mm of the mark, heading within 5°.

**Why this one is the real test:** it compounds every error you have.
Whatever distance calibration and heading drift you're carrying shows up
here as a gap you can literally measure with a ruler.

---

## Stage 3 — Make it pick and place

### Lab 3.1 · Clamp calibration
Command `clamp_open` and `clamp_hold` from the panel, watching the real
arm.

**Pass:** open clears the storage base rim cleanly; hold presses the
piece flat without the servo buzzing (overtravel).

### Lab 3.2 · Pick from a real storage base
Command `pick` positioned in front of a real base.

**Pass:** the sequence completes (lift into the slot → drive forward →
lift, taking the load → clamp hold → lift clear → drive backward → carry
height) and the piece is actually on the fork afterward, not dropped or
dragged.

**If it drags the piece against the rim:** the lift step before a drive
step didn't clear far enough — check the lift heights (Settings → Lift, Pick and place)
against your real slot dimensions.

### Lab 3.3 · Place on each of the four stands
`place_stand:1` through `place_stand:4`.

**Pass:** all four succeed — piece released cleanly inside each pocket,
fork backs out without snagging the wall.

**If it snags on the way out:** the "rise back above the wall before
backing out" step (see `Oms.cpp`'s header comment) isn't clearing the real
wall height — raise **Hover gap** (Settings → Pick and place) against the real stand.

### Lab 3.4 · Colour classification
Hold each of the five colours (red/green/yellow/blue + your own stand-in
for the hazard colour, since the real one isn't known until the
briefing) in front of the camera.

**Pass:** the camera's classification is correct for all five, under
your actual workshop lighting.

**Re-run this under different light before the competition** — the
venue won't be lit like your workshop, and HSV thresholds drift with
white balance.

---

## Stage 4 — Put it together

### Lab 4.1 · Full pick-and-place cycle
`pick` from a storage base, drive to a stand, `place_stand`. Repeat with
`sort_piece` instead of a fixed stand number, so colour routing picks
the destination automatically.

**Pass:** three consecutive successes without human help.

### Lab 4.2 · Full run rehearsal
Load a complete routine on the panel (or trigger the physical start
button, which runs `LoadFallbackRoutine()`), run it end to end.

**Pass:** a complete run — pick, identify colour, deliver to the correct
stand, return — with no manual intervention.

---

## Assessment grid

| Stage | Skill demonstrated | Scored section |
|---|---|---|
| 0 | Hardware modelling, systematic setup | A1 |
| 1 | Actuators, sensors, sign conventions | B1, D1 |
| 2 | Closed-loop control, error budgets | D1, E1 |
| 3 | Manipulation, colour classification | C1, E1 |
| 4 | Integration, full autonomous run | D1, E1 |

## For instructors

The habit worth teaching hardest is **measure, record, compare**. Most
teams at this level change three things at once and then can't say which
one helped. Labs 2.1–2.3 exist specifically to build the discipline of
changing one variable and writing down the number.

The second habit is **fix it at the right layer**. Lab 1.1's inversion
note is the template: a wrong-direction wheel has a correct fix
(the Settings page) and a tempting wrong one (negate in code). The tempting
one works today and breaks odometry tomorrow.
