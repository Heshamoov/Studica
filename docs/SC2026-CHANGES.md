> **⚠ SUPERSEDED — kept as history, not current guidance.**
>
> This document's own recommendation ("port the action layer to C++")
> was tried and turned out to be wrong: the C++/WPILib build deploys
> cleanly but is linked for the real roboRIO's soft-float ABI, which
> cannot run on this hard-float VMX-pi (confirmed with `readelf`, not a
> guess). The actual robot program is C++ compiled on the robot, in
> `robot-onboard/` (ROS 2 is not installed on this robot image) — see
> `robot-onboard/README.md` and `START-HERE.md` for the current, correct
> picture. The mechanical analysis below (gripper redesign, lift
> shortening, etc.) is also superseded by the fork+clamp-servo design
> that actually got built — see `cad/endeffector.scad`. Left below for
> the historical record of how the project got from the old ESC2026
> brief to this year's SC2026 one.

# SC2026 vs ESC2026 — what changed, and what it costs us

The new pack (`SC2026_TP23`, `TD23`, `IL23`, dated 03.03.2026) describes a
**different and much simpler competition** than the `ESC2026_TP23_pre`
document everything so far was built against.

Read this before touching anything else. Several of my earlier decisions are
now wrong, and I would rather say so plainly than let you find out at the
arena.

---

## The headline: the language question is settled

`SC2026_IL23` — IT Requirements — says the host provides, per computer:

- **Studica FRC VS Code 2020**
- **Studica FRC Shuffleboard**
- **Studica FRC SmartDashboard**
- Windows

There is **no ROS 2 anywhere in the pack**. That is decisive: this is the
**WPILib / FRC** route, in **C++ or Java**.

Two consequences:

1. **The ROS 2 plan is dead.** `params.yaml`, `detector_node.py`,
   `ros_hardware.py`, rosbridge — none of it applies. My earlier
   recommendation was made before this document existed.
2. **"Shuffleboard" in the Test Project means FRC Shuffleboard**, not a
   display I write. "Display the object's color in the camera FOV in the
   shuffleboard" = `frc::SmartDashboard::PutString("color", ...)`. My
   `shuffleboard.py` big-text display was solving a problem that does not
   exist.

`reference/studica-foundations` is now not just a useful reference — it is
**the template**. Same toolchain, same libraries, same dashboard.

## Side by side

| | ESC2026 (old) | **SC2026 (new)** |
|---|---|---|
| Duration | multi-day, C1–C3 | **3 hours total** |
| Arena | 2×4 … 8×8 m | **2 × 2 m**, walls + floor tape |
| Object | 60×60×150 mm upright box | **75 × 75 × 10 mm flat piece** |
| Colours | red / green / blue | red / green / **yellow** / blue **+ a surprise colour** |
| Storage | 2-shelf rack, 120–130 cm long | **4 × storage base**, 100 mm cube, 80×80 cavity, **50 × 30 mm front slot** |
| Delivery | 4 floor areas | **4 stands: 25 / 50 / 75 / 100 mm high** |
| Modes | autonomous, teleop only for recovery | **teleop 30 pts AND autonomous 30 pts** |
| 3D print budget | 1.5 kg | **1.2 kg** |
| SD cards | (I said three clones) | **one micro SD per team**, sealed; sponsor has spares |
| Marking | 6 modules A–G | A1 Work Org 10 · B1 Design 10 · C1 Simple Tasks 20 · D1 Teleop 30 · E1 Autonomous 30 |

## Arena, dimensioned (TP23 appendix)

```
2000 × 2000 mm
  centre        600 × 600 mm square   <- start area, and the robot size limit
  top strip     4 storage bases, ~780 mm down from the top edge
  bottom strip  4 delivery stands, 600 mm up from the bottom edge
  floor tape    20 mm wide
```

## What this invalidates

**The gripper is designed for the wrong object.** It grips a 60 mm box across
55 mm-tall jaws. The new piece is a 75 × 75 × 10 mm plate that sits inside a
100 mm cube and is reached through a **50 mm wide × 30 mm tall slot**. So the
fingers must pass through a 50 × 30 mm opening — my base plate alone is 136 mm
wide. This is a redesign, not a tweak.

Two viable grasps, and the slot decides it:
- **Pinch the 10 mm edge** — needs jaws closing to ~6 mm; mine bottom out at 12 mm.
- **Grip across the 75 mm face** — needs >75 mm opening; mine maxes at 80 mm,
  leaving 5 mm of approach error. Too tight, and it will not fit the slot anyway.

**The lift is enormously over-engineered.** I built for shelves at 250 and
550 mm and proved 715 mm of reach. The new delivery stands are **25, 50, 75
and 100 mm**. The whole useful range is about 150 mm. A 595 mm mast is dead
weight against a 1.2 kg print budget.

**Teleop is now a scored mode worth 30 points**, equal to autonomous. I built
it as a penalty-reduction fallback. It needs to be a proper gamepad control
scheme, practised.

**Vision needs a fifth, unknown colour.** "The camera should recognise a
surprise color." Fixed red/green/blue bands cannot do that. It needs a
teach-a-colour step at the morning briefing — hold the object up, sample it,
store it.

## What survives

- **All four HTML tools** — language-agnostic. The port planner already emits
  C++ and Java, with the `TitanQuad` three-argument form corrected.
- **The field map** — needs a 2×2 m preset and the fixed arena layout.
- **The run console** — the order still comes from a morning briefing.
- **Vision algorithms** — colour segmentation, range-from-known-size and the
  camera geometry are all portable; the maths does not care about the language.
- **The action vocabulary as a design** — `forward`, `turn_by`, `lift_to`,
  `gripper_open` map directly onto section C1 "Simple Tasks", which is worth
  20 marks on its own. The *structure* ports to C++; the Python does not run
  on the competition laptop.
- **`presets.py` as the list of numbers** — the values change, the discipline
  of keeping them in one place does not.
- **The labs and the first-hour checklist**, with ROS steps swapped for
  VS Code deploy steps.

## What I would do next, in order

1. **Rebuild the gripper around the 50 × 30 mm slot and the 75 × 75 × 10 mm
   plate.** This is the long pole — it is mechanical, it needs printing and
   testing, and nothing else can be validated without it.
2. **Cut the lift down** to roughly 0–150 mm of travel and reclaim the mass.
3. **Port the action layer to C++** on the `studica-foundations` structure.
   `Constants.h` ← `presets.py`, `Hardware.h` ← `hardware.py`, and the verbs
   into a `Movement`/`Oms` pair.
4. **Add the 2 × 2 m arena** to the field map as a fixed preset.
5. **Add yellow + a teach-a-colour mode** to the detector.
6. **Build a teleop scheme** and practise it — 30 marks.

## The one thing that did not change

You still have not run `tools/vmx_probe.sh`. It matters less now that ROS 2 is
out of the picture, but knowing what is on the SD card still decides your
first hour.
