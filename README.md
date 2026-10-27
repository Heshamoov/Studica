# SC2026 — Skill 23, Autonomous Mobile Robotics

## How students get the dashboard and updates

- **Install (once):** download **Studica-START-HERE.zip** from the `dist` branch
  (https://github.com/Heshamoov/Studica/raw/dist/Studica-START-HERE.zip), extract, open **START HERE.html** in Chrome or Edge.
- **Update:** on a Wi-Fi with internet, the dashboard shows **🔄 Update available** → one click. The new version is kept in the browser; no files to copy.
- **Robot:** when the laptop is on the robot's Wi-Fi, **🔄 Update robot** sends the new program to the robot (it compiles and restarts).
- **Teacher:** edit files in this repo (dashboard = `BEGINNER-GUIDE.html`, robot program = `robot-onboard/`), commit, push to `main`.
  GitHub Actions builds the `dist` branch automatically (≈1 min). `config.json` holds the problem-report form URL.
- **New robot (once):** `SETUP-ROBOT (teacher, once per robot).bat` from a laptop on the robot's Wi-Fi.
- **Problem reports → GitHub Issues:** students press *Send to teacher*; the report (team name from the robot's Wi-Fi,
  robot state, settings, logs) is queued and sent when the laptop has internet. It needs a tiny relay (free Cloudflare Worker):
  1. GitHub → Settings → Developer settings → Fine-grained tokens → repo `Studica`, permission **Issues: read & write**. Copy the token.
  2. workers.cloudflare.com → Create Worker → paste `build/report-worker.js` → Deploy → Settings → Variables:
     `REPO = Heshamoov/Studica`, secret `GITHUB_TOKEN = <token>`. Copy the worker URL.
  3. Put that URL in `config.json` → `report_url`, push. Each report becomes an Issue labelled with the team name.


Team workspace. Hardware: Studica kit, VMX-pi + Titan Quad, mecanum drive,
front-mounted elevator with a fork + servo clamp (no extension axis, no
gripper — this year's object is a flat piece, not a box).

**New? Open [BEGINNER-GUIDE.html](BEGINNER-GUIDE.html).** One linear
walkthrough with copy buttons and test buttons, from printing the parts to a
full autonomous run.

**Everything else is linked from [index.html](index.html)** (the Workbench).

| Where | What |
|---|---|
| [PROJECT-CONTEXT.md](PROJECT-CONTEXT.md) | One-page reference: competition facts, robot, decisions, current status |
| [BEGINNER-GUIDE.html](BEGINNER-GUIDE.html) | Step-by-step for a beginner. Start here. |
| [tools/settings.html](tools/settings.html) | Change every robot number in the browser, test wheels, calibrate |
| [tools/controlpanel.html](tools/controlpanel.html) | Buttons for every robot action, plus the routine builder |
| [tools/fieldmap.html](tools/fieldmap.html) | Court map: waypoints, walls, areas |
| [tools/portplanner.html](tools/portplanner.html) | Plan wiring, catch pin conflicts (planning aid only) |
| [robot-onboard/](robot-onboard) | The robot program (C++). See its README |
| [START-HERE.md](START-HERE.md) | Connection details, what each scored section needs |
| [docs/FIRST-HOUR.md](docs/FIRST-HOUR.md) | Printable bring-up checklist at the robot |
| [docs/LABS.md](docs/LABS.md) | Lab exercises with measurable pass criteria |
| [READINESS.md](READINESS.md) | What is verified, untested, blocked; pre-travel checklist |
| [cad/](cad) | Fork, clamp arm and servo bracket (STL/STEP) |
| [old/](old) | Earlier attempts (Python/ROS 2, roboRIO C++, ESC2026 prototype). Reference only |

## How it fits together

```
Laptop (browser)  ──Wi-Fi──►  Robot (VMX-pi)
 tools/*.html                  robot-onboard  →  ./run.sh
 (Settings, Control panel)     HTTP :5800  →  Sequence → Movement / Oms → Hardware → Titan, servo, navX
```

The pages talk to the robot over plain HTTP on port 5800. One dispatch
table (`Sequence.cpp`) serves the buttons, saved routines and the physical
start button, so they cannot disagree.

**Numbers live on the robot**, in `settings.cfg`, edited from
`tools/settings.html`. Copying new program code never overwrites them, so
five teams can share the same code and each keep their own calibration.

## Decisions already made, and why

**C++ compiled on the robot.** ROS 2 is not installed on this image, and
the roboRIO libraries that ship on the board are soft-float builds the Pi
cannot load. So the program bundles the Raspberry Pi builds of WPILib and
compiles with the robot's own `g++`. Full trace in
[robot-onboard/README.md](robot-onboard/README.md).

**No extension axis — the chassis drives instead.** Lift + fork + one clamp
servo. Reach comes from driving the chassis forward/backward to insert and
withdraw the fork. The order of lift-vs-drive steps matters: get it wrong and
the fork drags the piece against a wall or rim (see `src/Oms.cpp`).

**Wheel direction is a setting, never a code negation.** Tick the
"reversed" box in the Settings page. Negating inside a behaviour gives a
robot that drives correctly but does not know where it is.

**IMU heading over encoder heading.** Mecanum wheels slip in rotation and
heading error corrupts x and y. Movement fuses navX yaw.

**Every blocking move has a timeout.** A robot waiting forever on a jammed
lift is a robot scoring zero.

**The HTTP API is the one stable interface.** It survived two language
changes unchanged; keep it that way.

## Rules of the workspace

- Tools are single self-contained HTML files: no CDN, no framework, no
  internet. Do not "improve" them with a library. There is no internet on site.
- 3D parts are printed by the mentor; see [cad/README.md](cad/README.md).
