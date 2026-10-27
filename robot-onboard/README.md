# robot-onboard — the robot program (C++, built on the robot)

This is the program that runs on the Studica VMX-pi. It is compiled **on the
robot itself**, so the result is the right kind of binary for that board.

**Beginner? Follow [../BEGINNER-GUIDE.html](../BEGINNER-GUIDE.html).** This
file is the technical reference.

## Why it is built on the robot

- Studica's WPILib libraries are the Raspberry Pi (hard-float) builds. The
  ones that ship in `/usr/local/frc/third-party/lib` on the board are
  roboRIO (soft-float) builds that the Pi silently refuses to load. That is
  why `wpilib/` here holds the real Pi builds (WPILib 2020.3.2, hard-float,
  no NI dependencies).
- The PC has no Raspberry Pi cross compiler, but the robot has `g++`.
- ROS 2 is **not** installed on this robot image, so there is no Python/ROS
  route. (The old attempt is in `../old/`.)

## What is in here

```
Makefile      builds ./robot on the robot (make -j4; make -B -j4 to force)
run.sh        starts it with the right library path; restarts on exit code 42
src/          the program (Robot, Hardware, Movement, Oms, Sequence,
              CommandServer, Camera, Settings) + navX sources (src/navx)
include/      headers: WPILib 2020.3.2, Studica, navX, and our own
              (Constants.h = defaults, Settings.h = runtime settings)
lib/          Studica libraries for Raspberry Pi (libStudica.a, libStudicaDriver.a)
wpilib/       WPILib + OpenCV for Raspberry Pi (hard-float shared libraries)
settings.cfg  created on the robot by the Settings page (not in this folder)
```

## Build and run

From the laptop (once, or when the program changes):

```bash
scp -r robot-onboard pi@10.12.34.2:~/
```

On the robot over SSH:

```bash
sudo date -s "YYYY-MM-DD HH:MM:SS"     # robot has no battery clock; use the real time
cd ~/robot-onboard
make -j4                                # first build takes a few minutes
chmod +x run.sh
./run.sh
```

Wait for `panel server: LISTENING on port 5800`.

If `make` says "up to date" after you copied new files, the clock is
confused: run `make -B -j4`.

## Tuning: no code editing

Every number a team tunes is a runtime setting. Open
`tools/settings.html` in a browser (laptop on the robot's Wi-Fi). Changes
are checked, applied, and saved on the robot in `settings.cfg`, which
`scp` never overwrites. Wiring settings need a restart; the page has a
button and `run.sh` brings the program back automatically (exit code 42).

`include/Constants.h` holds only the **defaults**. Edit it only to change
what a brand-new robot starts with; then rebuild.

## HTTP API (port 5800)

| Route | Purpose |
|---|---|
| `/state` | telemetry JSON (position, lift, yaw, buttons, result) |
| `/do?cmd=&arg=` | run one command (see `/commands`) |
| `/commands` | list every command |
| `/load?seq=` , `/run` | load and run a routine, e.g. `forward:500,turn_by:90,pick` |
| `/abort` , `/estop` | stop everything |
| `/settings` | JSON list of every setting (value, default, limits, help) |
| `/set?key=&value=` | change one setting, validated and saved |
| `/reset?key=` | back to default (`key=all` for everything) |
| `/restart` | exit with code 42 so `run.sh` restarts the program |
| `/drive?vx=&vy=&w=` | browser teleop, each -1..1; stops by itself 0.4 s after the last call |
| `/enable?on=1` / `?on=0` | enable / disable outputs (the program enables itself at start via Studica `MockDS`) |

Bring-up commands: `spin_wheel:0..3` (FL, FR, RL, RR, 1 s at 30%) and
`spin_lift:1|-1` (0.4 s nudge).

## Adding a setting

1. Add an `inline` variable in `include/Constants.h` (the default).
2. Add one line in `src/Settings.cpp` `Register()` (`D` for a decimal,
   `I` for a whole number that needs a restart, `B` for a yes/no).
3. If other values are derived from it, add them to `Recompute()`.

The web page builds itself from `/settings`, so nothing else changes.

## Known issues

- With no USB camera the log repeats "Connecting to USB camera on
  /dev/video0". Harmless.
- `RobotPeriodic()` takes about 43 ms against a 20 ms loop, so the watchdog
  prints overrun messages. Harmless, but noisy.
- `/state` showed a nonsense lift height (315795 mm) on first run: the lift
  encoder is not zeroed or is on the wrong port. Check in the Settings page.
- Every calibration value is an unmeasured default until you calibrate it.
