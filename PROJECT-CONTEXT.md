# PROJECT CONTEXT — SC2026 Skill 23 (read this first)

One-file summary of the robot, the competition, the decisions made and the
current state. Written so a fresh session (or a new mentor) can pick the
project up without the chat history. If something here disagrees with a
file it points at, the file wins; then fix this page.

Last updated: 2026-09-25.

---

## 1. The competition

**SC2026 Skill 23 — Autonomous Mobile Robotics** (Emirates Skills / WorldSkills format).
Official documents in the project root: `SC2026_TP23 1.pdf` (test project + drawings),
`SC2026_IL23 1.pdf` (infrastructure list), `SC2026_TD23 1.pdf` (technical description),
`SC2026_Training_Program 1.pdf`. `ESC2026_TP23_pre - v2.pdf` is the OLD brief — ignore it.

| Item | Value |
|---|---|
| Arena | 2 x 2 m; start box 600 mm centre square; robot must fit 600 x 600 x 600 mm |
| Time / team | 3 hours, team of 2 |
| Flat pieces | 75 x 75 x 10 mm, R3 corners, lying flat. Red, green, yellow, blue + one **surprise hazard colour** told at the morning briefing |
| Storage bases | 100 mm cube, 80 x 80 mm cavity. Front is a stepped slot: 80 mm mouth tapers (R5) into a **50 x 30 mm side slot, bottom edge 30 mm off the floor (band 30-60 mm)** |
| Delivery stands | 4 stands, 25 / 50 / 75 / 100 mm high, 120 x 120 outer, 95 x 95 inner pocket, 5 mm wall. Pocket depth 20 mm (25 mm stand), 40 mm (the other three) |
| Scoring (100) | **A1** 10 work organisation + LED shows moving vs stopped · **B1** 10 robot design · **C1** 20 simple tasks (single and grouped commands) · **D1** 30 teleoperated run · **E1** 30 autonomous run |
| Rules | No internet on site. One micro SD + one USB stick per team, sealed with the Workshop Manager. 3D-print / sheet limits in IL23 |
| Colour to stand | red 1 (25 mm), green 2 (50), yellow 3 (75), blue 4 (100); hazard/none -> fallback stand (default 1) |

## 2. The robot

- **Controller:** Studica **VMX-pi** (Raspberry Pi 4-class, Debian 11 "Raspbian", 32-bit armv7l, **hard-float**), kernel 5.10. **Titan Quad** motor controller over CAN for the 4 wheels (ID **42**). The lift is a servo, not a Titan motor. **navX** IMU on the VMX (SPI). Two servos on PWM headers: lift servo and pusher servo (not yet wired to headers as of 2026-09-25). USB camera (none detected yet). Xbox gamepad for teleop.
- **Drive:** mecanum, 4 wheels, wheel order always FL, FR, RL, RR. Wheel radius default 51 mm (unmeasured), max speed 700 mm/s (unmeasured), encoder 1464 counts/rev.
- **Mechanism (no extension axis):** front-mounted elevator driven by a **positional servo** (no encoder; height = angle, calibrated from two points on the Settings page) + one-piece fork + a **servo PUSHER** (not a clamp). The **chassis drives forward/backward** to insert and withdraw the fork.
- **Why a pusher:** delivery pockets are closed, so the piece is held just above the stand and pushed off the fork tip to drop in. The push is a SLOW servo sweep (Push speed ~40 deg/s) so it slides and lands flat instead of flipping.
- **Pusher part:** `cad/pusher_arm.stl/.step` (`cad/make_pusher.py`), a pendulum paddle on the 76148 horn: shaft ~116 mm above the fork top (arm R = 115 mm, shortened 15 mm on 2026-10-01 to clear the fork mount), ~34 mm behind the tip, swing +-22 deg about straight down gives ~86 mm travel. Horn fitted so the arm hangs straight down at servo 150 deg: Park 0, Push start 128, Push end 172 (park clears mast/beam/base in CAD). Servo carried on a horizontal U-channel support beam from the lift carriage. Renders: `cad/pusher_pick.png`, `cad/pusher_place.png`; Inventor: `cad/pusher_assembly_pick.step`, `_place.step` (`cad/make_pusher_assembly.py`).
- **Pick sequence:** pusher retract -> lift 32 mm -> drive forward (insert) -> lift 40 (piece lifts off) -> lift 65 (clear the slot) -> drive backward -> lift carry 40.
- **Place sequence:** lift so the fork is just above the stand's top wall -> drive forward (fork tip over the pocket) -> slow PUSH -> retract -> drive backward -> carry.
- **Parts (3D printed by the mentor, not by the assistant):** `cad/fork_combined`, `clamp_arm`, `servo_bracket` (STL and STEP). Sized to real Studica parts (75002 servo, 76148 horn, 76014 U-channel).

### Network / login
| | |
|---|---|
| Wi-Fi (factory AP) | SSID `WorldSkills-1234`, password `password` (rename per robot: guide Step 4) |
| Robot IP | Wi-Fi `10.12.34.2` · Ethernet `172.22.11.2` |
| Login | user `pi`, password `raspberry` |
| Laptop IP | `10.12.34.x` (was `.7`) — never put the laptop's address in the tools |
| Web API | HTTP port **5800** on the robot |

## 3. Software stack — DECIDED

**C++ compiled ON the robot.** Program: `robot-onboard/`. Flow: `scp -r robot-onboard pi@10.12.34.2:~/` -> on the robot `make -j4` -> `./run.sh`. Browser tools talk to it on :5800.

Why (each was tested, not assumed):
1. **No ROS 2** on this image (`/opt/ros` missing, no `ros2`). The Python/ROS 2 plan is dead.
2. The roboRIO-target GradleRIO build deploys but its binary is **soft-float**; this board is **hard-float** (`readelf -h` flags 0x200 vs 0x400).
3. WPILib libs shipped in `/usr/local/frc/third-party/lib` are **also soft-float** and the Pi loader silently skips them. So `robot-onboard/wpilib/` bundles the `linuxraspbian` hard-float WPILib 2020.3.2 + OpenCV 3.4.7, and `robot-onboard/lib/` the Studica `linuxraspbian` static libs.
4. The PC has no Pi cross compiler, but the robot has `g++ 10.2`. navX is compiled from its sources zip.
5. Studica's Titan driver on the board is JNI-only (Java); C++ uses `libStudica.a` + `libStudicaDriver.a` from Studica's maven (`linuxraspbian`).

### Tuning without code
All tunables are **runtime settings** stored on the robot in `settings.cfg` (never overwritten by `scp`). Edit them in **`tools/settings.html`** (wiring, motor directions, wheel size, lift, clamp angles, insert distances, colour ranges). Wiring needs a restart (button on the page; `run.sh` restarts on exit code 42). Defaults live in `robot-onboard/include/Constants.h`; the list of settings is `robot-onboard/src/Settings.cpp`.

### HTTP API (:5800)
`/state` (now also `enabled`, `colour`) `/do?cmd=&arg=` `/commands` `/load?seq=` `/run` `/abort` `/estop` `/settings` `/set?key=&value=` `/reset?key=` `/restart` `/drive?vx=&vy=&w=` (browser teleop, -1..1, 0.4 s deadman) `/enable?on=0|1`. The robot enables itself at start with Studica `MockDS` (no FRC Driver Station; a disabled robot blocks all outputs). Routine text: `forward:500,turn_by:90,pick`. Bring-up commands: `spin_wheel:0..3` (FL FR RL RR, 1 s at 30%), `spin_lift:1|-1` (+-10 mm). Pusher: `push`, `retract` (`clamp_open`/`clamp_hold` kept as aliases).

## 4. Where everything is

| Path | What |
|---|---|
| `BEGINNER-GUIDE.html` | 18-step interactive guide (Copy buttons, test buttons, saved checkmarks). Start here. `BEGINNER-GUIDE.md` = command cheat sheet |
| `index.html` | Workbench hub (8 phases, links every file) |
| `tools/settings.html` | Robot settings, wheel tests, calibration helpers |
| `tools/controlpanel.html` | Buttons for every action, routine builder, export |
| `tools/fieldmap.html`, `portplanner.html` | Field map; pin planner (planning aid only) |
| `tools/runconsole.html`, `camview.html` | LEGACY from the old brief |
| `robot-onboard/` | The robot program. `README.md` there = technical reference |
| `docs/FIRST-HOUR.md`, `docs/LABS.md` | Printable bring-up checklist; lab exercises |
| `READINESS.md` | Verified / untested / blocked, pre-travel checklist |
| `cad/` | Printable parts + CAD sources (CadQuery / OpenSCAD) |
| `old/` | Archived: `robot_py` (Python/ROS 2), `robot-cpp` (roboRIO C++), `robot` (ESC2026 prototype) |

## 5. Current status (2026-09-25)

**Verified on the real robot:** compiles on the robot, starts, navX connects, `curl localhost:5800/state` returns JSON, robot is reachable over SSH.
**Known problems:**
- **FIX APPLIED 2026-09-28 (untested on robot):** `robot-onboard/wpilib/libwpiHal.so` replaced with Studica's VMX-pi HAL (`https://dev.studica.com/maven/release/com/kauailabs/vmx/first/hal/hal-cpp/2020.3.2-vmxpi-20/`, links `libvmxpi_hal_cpp.so`), plus its headers in `include/hal`. `run.sh` now re-runs itself with sudo (hardware needs root). Before this the Enable button did nothing.
- **(was) BLOCKER:** the bundled WPILib HAL runs in SIMULATION (log shows SimulationPeriodic, navX SimDevice), so no motor/servo command reaches hardware. Need the robot-side check of how Studica's launcher provides the real VMX HAL (robotCommand / HAL extensions), or talk CAN/PWM through libvmxpi_hal_cpp directly.
- Lift and pusher servos not yet plugged into PWM headers.
- Settings page "wheel spin" buttons do not move wheels yet (under investigation).
- `/state` lift read `315795` (encoder unzeroed or wrong port).
- No USB camera detected (`/dev/video0` missing).
- `RobotPeriodic()` ~43 ms vs 20 ms loop: watchdog overrun spam (harmless).
- Robot clock is 2022 after every boot: `make` thinks nothing changed. Use `sudo date -s` and `make -B`.
- Browser showed `ERR_PROXY_CONNECTION_FAILED` to 10.12.34.2 while SSH worked, Windows proxy was OFF: a browser-level proxy/extension. Try a private window or another browser.
**LEDs (2026-09-26):** one rule in `Hardware::UpdateLEDs` every 20 ms: RED only while the stop button is pressed or after an e-stop (`/abort`, panel STOP, Esc); GREEN otherwise. E-stop clears on the next command/run/enable or the start button. Setting `LED_ACTIVE_LOW` for LEDs wired from +5V. User reported both LEDs lit at once before this change.
**VMX channel rules (from Studica's HAL source, 2026-09-28):** DIO 0-11 = FlexDIO (in/out). DIO 12-21 = HiCurrDIO: outputs only when its jumper is set to OUTPUT (jumper currently REMOVED = inputs). PWM 0-9 = HiCurrDIO (needs the jumper); PWM 10-21 = FlexDIO pins 0-11. LEDs are wired on FlexDIO, so their pin settings must be 0-11 (defaults 12/13 were rejected). Servo PWM 2/3 were rejected for the same reason.
**Lift limit switches (yellow micro-switches):** settings LIFT_TOP_SWITCH (6), LIFT_BOTTOM_SWITCH (7), active-low by default; the lift ramps at Lift speed and never moves further into a pressed switch. Shown live on the Settings page and in /state (lift_top, lift_bottom).
**Loop time:** RobotPeriodic measured 0.6-0.95 s on the real HAL; the program now prints a timing breakdown every 5 s to find the slow part. MockDS enables in AUTONOMOUS mode, so the control loop now runs in both auto and teleop, and enabling no longer auto-runs the routine.
**Lift (2026-10-08):** the lift servo's 0-180 deg range only moves the fork ~2 cm, so the lift must be a CONTINUOUS servo (set with the Studica servo programmer; setting LIFT_CONTINUOUS). Height = limit switches + time: lift_find_bottom homes to the bottom switch (= LIFT_MIN), lift_calibrate times bottom->top->bottom over LIFT_RAIL_MM and saves LIFT_UP_MM_S / LIFT_DOWN_MM_S; lift_to runs for distance/speed. Pusher should be ANGLE mode (PUSHER_CONTINUOUS fallback exists). STOP/E-stop now set continuous servos to 90. Settings page warns about pin clashes (an LED on DIO 0 + servo on PWM 10 = same FlexDIO pin 0).
**Untested on hardware:** every motor command, encoders, pusher angles and lift-servo calibration (guesses), start/stop buttons and LEDs (pins 8, 9, 13, 12; stop is active-low), pick/place distances (90 / 70 mm guesses), colour ranges, restart-from-browser.

## 6. Working agreements with the user

- The user is a **beginner** with **5 teams**; wants everything interactive in the browser and step-by-step.
- **3D parts:** the user prints them; do no more CAD/print work unless asked.
- Tools are single self-contained HTML files, **no CDN**, no internet needed on site.
- Do not put shell commands in a browser button (impossible); use Copy buttons for terminal commands and real buttons for robot actions.
- Fix wheel direction with the `INVERT_*` setting, never by negating in code.
- Keep the HTTP API stable; one dispatch table (`Sequence.cpp`) serves buttons, routines and the start button.
- Confirm hardware facts on the robot; do not guess (it has been wrong before: ROS 2, soft-float libs).

## 7. Handy commands

```bash
# laptop
scp -r robot-onboard pi@10.12.34.2:~/
ssh pi@10.12.34.2
# robot
sudo date -s "YYYY-MM-DD HH:MM:SS"
cd ~/robot-onboard && make -j4 && ./run.sh        # make -B -j4 to force
pkill robot                                       # stop a stray copy
curl -s localhost:5800/state                      # is the server alive?
curl -s localhost:5800/commands                   # which binary is running?
```
