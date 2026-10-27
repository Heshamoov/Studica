# Readiness — SC2026 Skill 23

Honest status for this year's competition (2×2 m arena, flat pieces, fork +
clamp servo, no extension axis, Studica VMX-pi). Three categories:
**verified** = actually run and checked, **untested** = written but never
run on this hardware, **blocked** = needs something before it can go further.

---

## Verified

| What | How it was checked | Result |
|---|---|---|
| Program compiles and links **on the robot** | `make -B -j4` on the real VMX-pi (g++ 10.2, WPILib 2020.3.2 Pi builds bundled) | builds `./robot` with no errors |
| Program starts on the robot | `./run.sh` | "Robot program starting", no missing libraries |
| navX IMU | startup log | "navX-Sensor Connected", startup calibration complete |
| HTTP server | `curl localhost:5800/state` on the robot | valid JSON telemetry |
| Settings page and API | tested against a fake robot server that copies the routes | edit, validation, rollback, restart banner, calibration helpers all correct |
| C++ syntax of every source file | cross-compiler on the PC (syntax only) | clean |
| Storage base slot geometry | official `SC2026_TP23` appendix | slot 50×30 mm, bottom edge 30 mm off the floor (30–60 mm band) |
| Delivery stand pocket depth | same appendix | 20 mm (25 mm stand), 40 mm (50/75/100 mm stands) |
| Fork/bracket/arm CAD | measured against real Studica STEP files (`75002` servo, `76148` horn, `76014` U-channel) | zero interference; hole pattern matches the channel's Ø3.5 grid |

## Known problems on the real robot

- **Lift height reads nonsense** (`"lift":315795.2` in `/state`). The lift
  encoder is not zeroed or is on the wrong port. Look at it in
  `tools/settings.html` while moving the lift by hand.
- **No USB camera found**: the log repeats "Connecting to USB camera on
  /dev/video0". Plug the camera in and check `ls /dev/video*`.
- **`RobotPeriodic()` takes ~43 ms** against a 20 ms loop, so the watchdog
  prints overrun messages. Noisy, not fatal.
- **Robot clock is 2022** after every power-up (no battery clock). Set it
  with `sudo date -s` before compiling.
- **Browser proxy**: if the guide says "not reachable" while SSH works, the
  browser (not Windows) is using a proxy for `10.x.x.x`. Try a private window.

## Untested — written, never run on the real robot

- **Every motor command**: wheels, lift, direction flags. Use the spin buttons
  on the Settings page first (wheels on blocks).
- **Encoders** and the conversion to millimetres (`WHEEL_RADIUS`, `PULSE_PER_REV`).
- **Clamp servo** and its angles (`CLAMP_OPEN_DEG` 90 / `CLAMP_HOLD_DEG` 10 are guesses).
- **Start/stop buttons and LEDs** on the pins in Settings (8, 9, 13, 12 from the Studica course project; the stop button's polarity is unconfirmed).
- **`pick` and `place_stand`**: `DRIVE_INSERT_STORAGE` 90 mm and `DRIVE_INSERT_STAND` 70 mm are guesses.
- **Colour classification** under real lighting; the ranges are untested defaults.
- **Restart from the browser** (exit code 42 + `run.sh` loop) on the real robot.
- **The servo bracket print**: checked in CAD, never printed and test-fitted.

## Blocked — needs something from you

1. **Robot on blocks + battery** for the first motor tests.
2. **USB camera** plugged in (and a colour piece of each colour).
3. **Measurements**: real wheel radius and top speed, storage-slot and stand distances.
4. **Which U-channel face the fork/bracket bolt to**: the CAD assumes a channel-cell alignment; confirm before printing more.

---

## Before you travel

No internet on site. Everything must be on the SD card and the sealed USB
stick per `SC2026_IL23`:

- [ ] This whole folder, on the cards **and** the USB stick
- [ ] `robot-onboard` compiled on the robot at least once, on this exact hardware, and `./run.sh` shows "panel server: LISTENING"
- [ ] `settings.cfg` backed up (Settings page → "Copy my settings")
- [ ] Robot control panel: start button, stop button, running/stopped LEDs wired and working
- [ ] 3D printed parts weighed, under the print budget in `SC2026_IL23`
- [ ] Robot measured inside the 600×600×600 mm start box
- [ ] Laptop can open `BEGINNER-GUIDE.html`, `tools/settings.html` and `tools/controlpanel.html` offline

## Order of work

1. Guide Steps 1–9: build it, connect, copy, compile, run, open Settings.
2. Settings page: test each wheel, fix ports and directions.
3. Calibrate wheel size, lift scale, clamp angles.
4. Full `pick` / `place_stand` cycle; expect the insert distances to need adjusting.
5. Tune colour ranges under the venue lighting, and again after the briefing reveals the hazard colour.

## Known limitations

- **No extension axis** by design: the chassis drives to insert and withdraw the fork.
- **The camera does not stream to the browser.** Colour is classified on the robot and reported; there is no live video.
