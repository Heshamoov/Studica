> **Note on current relevance:** the API facts below (three-argument
> `TitanQuad`, `studica::Servo::SetAngle`, `PULSE_PER_REV = 1464`, active-low
> stop button) are still true and already carried over into
> `robot-onboard/include/Constants.h` and `robot-onboard/src/Hardware.cpp`. The **architecture
> mapping table** below ("Reference (C++) | Ours (Python)") points at
> `robot/actions/` — an older Python prototype for the previous ESC2026
> brief (now in `old/robot/`), not `robot-onboard/`, which is this year's
> actual robot. For the current architecture, see `robot-onboard/README.md`.

# Notes from `mobile-robotics-foundations`

Extracted to [reference/studica-foundations/](../reference/studica-foundations).
Five progressive C++/WPILib lessons on a Studica kit, by Felipe Ferreira.
It is a **different robot** from yours, but the API usage is real and working,
which makes it more trustworthy than the documentation on several points.

---

## It caught two real bugs in our generated code

| | Docs said | Working code does |
|---|---|---|
| `TitanQuad` | `{CAN_ID, motor}` — two args | **`{CAN_ID, 15600, motor}`** — three, with PWM frequency in the middle |
| Servo | `frc::Servo`, `Set(0..1)` | **`studica::Servo`**, `SetAngle(degrees)` |

Two-argument `TitanQuad` would not have compiled. Both are now fixed in
`tools/portplanner.html`, which emits `MOTOR_FREQ_HZ = 15600` as a named
constant.

## Verified API surface

```cpp
studica::TitanQuad        motor{TITAN_ID, 15600, MOTOR_CH};
studica::TitanQuadEncoder enc{motor, MOTOR_CH, DIST_PER_TICK};
studica::Servo            servo{8};          servo.SetAngle(deg);
studica::Cobra            cobra{};           cobra.GetVoltage(ch);
AHRS                      navX{frc::SPI::Port::kMXP};   navX.GetYaw();
frc::Ultrasonic           us{ping, echo};    us.Ping(); us.GetRangeMM();
frc::AnalogInput          sharp{2};
frc::DigitalInput/Output  button{8} / led{13};
```

## Numbers worth stealing

```cpp
PULSE_PER_REV = 1464                  // encoder resolution
WHEEL_RADIUS  = 5.1 cm
DIST_PER_TICK = (2*PI*R) / PULSE_PER_REV
```

**`PULSE_PER_REV = 1464` is the single most useful number here.** It is the
`dist_per_tick` calibration I had told you to measure from scratch. Verify it
on your own wheels — radius differs — but the encoder count is a property of
the Titan, so it should carry over.

**Sharp IR voltage → centimetres:**
```cpp
distance_cm = pow(voltage, -1.2045) * 27.726;
```
An empirical curve fit. Sharp sensors are badly non-linear; this saves you
building the curve yourself.

**Speed → PWM**, calibrated per wheel: `cm/s ÷ 70` (back), `÷ 55` (side).
Those divisors *are* the max wheel speed in cm/s. This is exactly the
`max_speed` measurement Lab 2.1 asks for, and it is per-wheel, not global.

## The control panel is confirmed

```cpp
START_BUTTON = DIO 8      RUNNING_LED = DIO 13
STOP_BUTTON  = DIO 9      STOPPED_LED = DIO 12
limit_high   = DIO 10     limit_low   = DIO 11    // elevator end stops
```

This settles the point I flagged from the CID: a real competition robot has a
physical start/stop panel and running/stopped LEDs. Build yours.

Note the **stop button is active-low** — the code stops the motors when
`GetStopButton()` reads *false*. Wire and test the polarity deliberately.

The elevator uses **limit switches to re-zero its height estimate**
(`if (!GetLimitHigh()) height = 40;`). That is a good trick: dead-reckoned
height drifts, and the end stops give you a free absolute reference.

## The architecture matches ours

| Reference (C++) | Ours (Python) |
|---|---|
| `Constants.h` | `robot/actions/presets.py` |
| `Hardware.h/.cpp` | `robot/actions/hardware.py` |
| `Movement.cpp` | `robot/nav/mecanum.py` + `actions.py` |
| `Oms.cpp` (Object Manipulation System) | gripper + lift actions |
| `Camera.cpp` | `robot/vision/detector.py` |

Arriving at the same split independently is reassuring. Their **OMS** naming
is the Studica term for the gripper/lift assembly — worth adopting so your
documentation matches the Experts' vocabulary.

## Where it differs from your robot

- **Three-wheel omni**, not mecanum. Wheels at 90° / 210° / 330°, so the
  kinematics do not transfer. Ours are correct for your four mecanum wheels.
- **The elevator is a Titan motor channel** (`ELEVATOR_MOTOR = 2`) with limit
  switches — matches your rack-and-pinion lift well.
- **Centimetres and degrees** throughout. Ours is metres and radians
  (ROS REP-103). Do not mix the two; pick one and convert at the edges.
- **Blocking `do/while` with `delay()`.** Simpler to read than our state
  machines, but it cannot be cancelled mid-move — which Module G's
  reposition-and-restart rule actually requires.

## One bug in the reference, for the students

```cpp
static constexpr double GEAR_RATIO = 1/1;   // integer division!
```
This is `1` only by luck. Change it to `1/2` and you get **0**, then
`DIST_PER_TICK` becomes infinity. Write `1.0/2.0`. Good Lab discussion.
