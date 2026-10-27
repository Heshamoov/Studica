# SC2026 — Skill 23 Autonomous Mobile Robotics — Getting Started

Hardware: Studica kit — VMX-pi controller + Titan Quad motor controller.
Language: **C++, compiled on the robot** (see section 0).

Beginner? Skip this file and open [BEGINNER-GUIDE.html](BEGINNER-GUIDE.html).
This file is the reference for connection details and the scoring rules.

---

## 0. The stack, in one paragraph

The robot program is in [robot-onboard/](robot-onboard). You copy it to the
robot, compile it there with the robot's own `g++`, and run it. The browser
pages in `tools/` talk to it over Wi-Fi on port 5800. There is no ROS 2 on
this robot image, and the WPILib libraries that ship on the board are
roboRIO (soft-float) builds the Pi cannot load — so the program carries its
own Raspberry Pi (hard-float) copies. Details:
[robot-onboard/README.md](robot-onboard/README.md). Earlier attempts
(Python/ROS 2, roboRIO C++) are in [old/](old) for reference only.

---

## 1. Connect to the VMX-pi

Factory default is Wi-Fi Access Point mode.

- **SSID:** `WorldSkills-1234`  ·  **Password:** `password`
- **Robot IP over Wi-Fi:** `10.12.34.2`
- **Robot IP over Ethernet:** `172.22.11.2`
- **Login:** user `pi` / password `raspberry`
- Your **laptop** gets `10.12.34.x` (for example `.7`). Do not put that address in the tools — the robot is `.2`.

```bash
ssh pi@10.12.34.2
```

If SSH does not work, plug a keyboard and micro-HDMI monitor into the VMX.

**Five robots, one room?** They all start with the same Wi-Fi name. Rename
each one once so teams can tell them apart — Step 4 of the beginner guide.

> The robot's clock is not battery-backed and has no internet, so `date`
> shows 2022. Set it with `sudo date -s "YYYY-MM-DD HH:MM:SS"` after every
> power-up, or `make` gets confused about which files are newer.

**Browser cannot reach the robot but SSH works?** Your browser is probably
using a proxy. Test in a private window or another browser, and check the
laptop is on the robot's Wi-Fi (its address must start `10.12.34.`).

## 2. Get the program running

1. `scp -r robot-onboard pi@10.12.34.2:~/` (laptop)
2. On the robot: `cd ~/robot-onboard && make -j4 && ./run.sh`
3. Open [tools/settings.html](tools/settings.html) — it should say "connected".

Then follow [docs/FIRST-HOUR.md](docs/FIRST-HOUR.md), in order.

## 3. Tuning is done in the browser

Wiring (which motor is where), wheel direction, wheel size, clamp angles,
insert distances and colour ranges are all on
[tools/settings.html](tools/settings.html). Saved on the robot in
`settings.cfg`; never overwritten when you copy new code.

---

## 4. What the Test Project actually asks for

| Section | Marks | What it needs |
|---|---|---|
| **A1** Work Organization & Behaviour | 10 | Tidy bench, schedule, robot LED shows moving vs. stopped |
| **B1** Robot Design | 10 | Expert inspection of the build |
| **C1** Simple Tasks | 20 | Individual drive/lift/clamp commands, and grouped sequences, via the control panel |
| **D1** Test Project Run — Teleoperated | 30 | Full gamepad-driven pick/place run |
| **E1** Test Project Run — Autonomous | 30 | Camera-driven colour sort, fully autonomous |

Objects: red / green / yellow / blue flat pieces (75×75×10 mm), plus one
surprise hazard colour revealed at the morning briefing. Four storage
bases (100 mm cube, front slot 30–60 mm off the floor). Four delivery
stands (25/50/75/100 mm, walled pockets).

Full detail: `SC2026_TP23 1.pdf`, `SC2026_IL23 1.pdf`, `SC2026_TD23 1.pdf`
in the project root — the actual rules; everything else here is derived from them.

### Hard constraints worth pinning on the wall
- Robot must fit the start box in `SC2026_TP23`'s appendix.
- No internet on site — everything must already be on the SD card / USB stick.
- 3D-printed and sheet-stock limits are in `SC2026_IL23`.

---

## Reference links

- Studica WorldSkills docs — https://docs.wsr.studica.com/en/latest/
- VMX networking — https://docs.wsr.studica.com/en/latest/docs/VMX/networking.html
- Titan Quad — https://docs.wsr.studica.com/en/latest/docs/Titan/titan.html
- navX IMU — https://docs.wsr.studica.com/en/latest/docs/VMX/navx.html
- Studica Learn — https://learn.studica.com/
