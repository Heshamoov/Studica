/*****************************************************************************
 * SC2026 Skill 23 - every tunable number, in ONE file.
 *
 * WHY ONE FILE
 * You will be re-tuning under time pressure with 3 hours on the clock. If a
 * number lives inside a behaviour you have to read code to change it. Here,
 * anyone on the team can change it and the diff is one line.
 *
 * RULE: no behaviour file may contain a bare number with physical meaning.
 * If you find one, it belongs here.
 *
 * UNITS: millimetres and degrees. The Studica course uses cm; we use mm so
 * every competition dimension is written exactly as the official SC2026_TP23
 * appendix states it, with no mental conversion. Do not mix.
 *
 * RUNTIME SETTINGS: every value below can be changed from the web panel
 * (tools/settings.html) without recompiling. Changes are saved to
 * settings.cfg next to the program and reloaded at start-up. The values
 * written here are only the DEFAULTS. See Settings.cpp for which ones apply
 * instantly and which need a restart.
 *
 * MECHANISM: no extension axis. The elevator (front-mounted, rail starts at
 * 30 mm off the floor) only moves vertically; the chassis itself drives
 * forward/backward to insert and withdraw the fork. See Oms.cpp.
 *****************************************************************************/

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

namespace constant
{
    // ---------------------------------------------------------------- CAN
    inline int TITAN_ID   = 42;
    inline int MOTOR_FREQ = 15600;   // 3rd TitanQuad arg - see notes

    // Drive motor channels.  <-- SET THESE FROM YOUR OWN BRING-UP TABLE
    inline int FRONT_LEFT   = 0;
    inline int FRONT_RIGHT  = 1;
    inline int REAR_LEFT    = 2;
    inline int REAR_RIGHT   = 3;

    // Manipulator. The lift is a Titan channel too — no extension motor,
    // the chassis drives forward/backward to insert and withdraw the fork.
    inline int LIFT_MOTOR   = 0;     // on TITAN_ID_2
    inline int TITAN_ID_2   = 43;

    // ------------------------------------------------------------ encoders
    inline double WHEEL_RADIUS  = 51.0;    // mm  <-- MEASURE
    inline double PULSE_PER_REV = 1464.0;  // from the Studica course
    inline double GEAR_RATIO    = 1.0;     // NOTE: 1.0 not 1/1.
                                                     // integer division would
                                                     // silently give 0 or 1
    inline double DIST_PER_TICK =
        (2.0 * M_PI * WHEEL_RADIUS) / (PULSE_PER_REV * GEAR_RATIO);

    inline double LIFT_MM_PER_TICK   = 0.10;  // <-- CALIBRATE

    // ---------------------------------------------------------- drivetrain
    // Mecanum, wheels front-left / front-right / rear-left / rear-right.
    inline double HALF_LENGTH = 150.0;   // mm, centre to axle
    inline double HALF_WIDTH  = 150.0;   // mm, centre to wheel
    inline double K_LEVER     = HALF_LENGTH + HALF_WIDTH;

    // Rim speed at duty 1.0.  <-- MEASURE: drive one wheel at 1.0 for a
    // fixed time and divide. A wrong value here scales every move.
    inline double MAX_SPEED = 700.0;     // mm/s

    inline double SPEED_CREEP  = 100.0;  // final approach
    inline double SPEED_SLOW   = 220.0;  // near obstacles
    inline double SPEED_CRUISE = 450.0;  // open court

    inline double TURN_SLOW = 60.0;      // deg/s
    inline double TURN_FAST = 140.0;

    inline double POS_TOLERANCE = 15.0;  // mm
    inline double ANG_TOLERANCE = 3.0;   // deg
    inline double KP_LINEAR  = 1.6;
    inline double KP_ANGULAR = 2.2;
    inline double SLOW_RADIUS = 250.0;   // mm, start easing off

    // ------------------------------------------------------- the objects
    // SC2026_TP23 appendix: flat piece 75 x 75 x 10 mm, R3 corners, lying FLAT.
    inline double PIECE_W = 75.0;
    inline double PIECE_T = 10.0;

    // Storage base: 100 mm cube, 80 x 80 internal cavity.
    // Front face: an 80 mm-wide top-loading mouth tapers 12.5 mm down (R5)
    // into a straight 50 mm-wide x 30 mm-tall SIDE SLOT. That slot's bottom
    // edge sits 30 mm off the floor (top edge at 60 mm) — confirmed from the
    // official appendix, and it's exactly why the elevator rail starts at
    // 30 mm: the fork enters/exits through this slot, not from underneath.
    inline double BASE_OUTER   = 100.0;
    inline double BASE_CAVITY  = 80.0;
    inline double SLOT_W       = 50.0;
    inline double SLOT_H       = 30.0;
    inline double SLOT_BOTTOM  = 30.0;   // mm off floor
    inline double SLOT_TOP     = SLOT_BOTTOM + SLOT_H;   // 60

    // Delivery stands: 120 x 120 outer, 95 x 95 inner cavity, 5 mm wall.
    // The fork (46 mm across) has ~24 mm clearance each side inside the
    // 95 mm pocket, so only the NEAR wall matters for entry/exit height.
    inline double STAND_1 = 25.0;
    inline double STAND_2 = 50.0;
    inline double STAND_3 = 75.0;
    inline double STAND_4 = 100.0;

    // Pocket depth (rim to floor) — confirmed from the appendix. Stand 1 is
    // too short for the 40 mm depth the taller three share.
    inline double STAND_POCKET_1 = 20.0;   // stand 1 (25 mm) only
    inline double STAND_POCKET   = 40.0;   // stands 2, 3, 4

    // ------------------------------------------------------------- lift
    // All heights are the FORK TOP SURFACE above the arena floor.
    // LIFT_MIN = 30: the elevator is front-mounted and physically cannot
    // reach the floor — its rail starts exactly where the storage slot does.
    inline double LIFT_MIN        = 30.0;
    inline double LIFT_MAX        = 200.0;
    inline double LIFT_HOME       = 40.0;
    inline double LIFT_CARRY      = 40.0;

    // Pick, inside the storage slot (30-60 mm band):
    //   enter low in the slot, under wherever the piece is loaded, then
    //   rise a few mm to take the load before backing out.
    inline double LIFT_UNDER      = SLOT_BOTTOM + 2.0;   // 32, just inside the slot
    inline double LIFT_LOAD_RISE  = 8.0;
    inline double LIFT_LOADED     = LIFT_UNDER + LIFT_LOAD_RISE;  // 40
    inline double LIFT_CLEAR_SLOT = SLOT_TOP + 5.0;      // 65, clear of the slot on withdrawal

    // Place, inside a delivery pocket:
    //   approach above the near wall, lower onto the pocket floor, release,
    //   then rise back above the wall before backing out.
    inline double LIFT_PLACE_GAP  = 6.0;   // hover clearance above a wall/rim

    inline double LIFT_TOLERANCE = 2.0;    // mm
    inline double LIFT_KP        = 0.020;  // duty per mm
    inline double LIFT_MAX_DUTY  = 0.75;
    inline double LIFT_MIN_DUTY  = 0.12;   // below this it will not move

    // ------------------------------------------------------- chassis drive
    // Replaces the old extension axis: the robot itself drives forward to
    // insert the fork and backward to withdraw it.  <-- CALIBRATE all of these
    // against the real storage base / delivery stand once you can measure.
    inline double DRIVE_INSERT_STORAGE = 90.0;   // mm, into the storage slot
    inline double DRIVE_INSERT_STAND   = 70.0;   // mm, over a delivery pocket
    inline double DRIVE_PICKPLACE_SPEED = SPEED_CREEP;  // slow — controlled insertion

    // -------------------------------------------------------------- fork
    inline double FORK_W = 44.0;   // must clear the 50 mm opening
    inline double FORK_T = 5.0;

    // ------------------------------------------------------------ arena
    inline double ARENA_W    = 2000.0;
    inline double ARENA_H    = 2000.0;
    inline double START_BOX  = 600.0;   // centre square

    // ------------------------------------------------- control panel I/O
    // Confirmed by the Studica course project. NOTE the stop button reads
    // TRUE when NOT pressed (active low) - test the polarity yourself.
    // Buttons on FlexDIO 8-10: the HiCurrDIO bank (12-21) is OUTPUT-only as
    // delivered (direction jumper fitted), so a button there always reads 0.
    inline int START_BUTTON = 8;
    inline int STOP_BUTTON  = 9;
    // RESET button (optional). -1 = not fitted. When pressed: stop everything,
    // clear the e-stop (red LED off), zero position/heading - ready for START.
    inline int RESET_BUTTON = 10;
    inline bool RESET_ACTIVE_LOW = true;
    inline int RUNNING_LED  = 14;   // green, HiCurrDIO output
    inline int STOPPED_LED  = 15;   // red, HiCurrDIO output
    // LED rule (Hardware::UpdateLEDs): RED on only while the stop button is
    // pressed or after an emergency stop; GREEN on otherwise.
    // Tick LED_ACTIVE_LOW if an LED is wired from +5V to the pin (lights when
    // the pin is LOW) - then both LEDs look "on" when the code says off.
    inline bool LED_ACTIVE_LOW = false;
    // Studica's buttons read 0 when PRESSED (active low), like the course code.
    inline bool START_ACTIVE_LOW = true;
    // Stop button: true = reads 0 when PRESSED (Studica wiring). A pin with
    // nothing connected also reads 0, i.e. "pressed" - that is the safe side.
    inline bool STOP_ACTIVE_LOW = true;
    // Untick to ignore the stop button completely (bench testing only!).
    inline bool STOP_ENABLED = true;
    // Untick to ignore the start button (bench testing without it wired:
    // an unconnected pin picks up noise and would start the routine).
    inline bool START_ENABLED = true;
    // Servo MODE. Studica servos can be set (with the servo programmer) to
    // ANGLE mode (0-180 = a position) or CONTINUOUS mode (spins: 90 = stop).
    // Angle mode is the right one for both the pusher and the lift. If a
    // servo is still in continuous mode, tick its box so it is stopped (90)
    // on STOP / E-stop, and the pusher runs for PUSH_RUN_S instead of
    // moving to an angle.
    inline bool PUSHER_CONTINUOUS = false;
    inline bool LIFT_CONTINUOUS   = true;
    inline double PUSH_RUN_S      = 0.8;     // continuous mode only

    // CONTINUOUS LIFT (servo spins like a motor, no encoder). Height comes
    // from the limit switches + time:
    //   home   = drive down until the BOTTOM switch -> height = LIFT_MIN
    //   travel = run up/down for distance / speed seconds
    //   calibrate (lift_calibrate) = time bottom->top and top->bottom over
    //            LIFT_RAIL_MM to measure the up/down speeds
    inline double LIFT_UP_CMD    = 180.0;   // servo "angle" that spins it UP   (90 = stop)  <-- CHECK
    inline double LIFT_DOWN_CMD  = 0.0;     // servo "angle" that spins it DOWN
    inline double LIFT_STOP_CMD  = 90.0;    // stop (trim if it creeps)
    inline double LIFT_RAIL_MM   = 150.0;   // fork travel from bottom switch to top switch  <-- MEASURE
    inline double LIFT_UP_MM_S   = 20.0;    // measured by lift_calibrate
    inline double LIFT_DOWN_MM_S = 25.0;    // measured by lift_calibrate
    inline double LIFT_HOME_TIMEOUT = 15.0; // s, give up if a switch is never reached
    // Not every team has both switches. Without the BOTTOM switch, "find
    // bottom" runs down for LIFT_DOWN_FULL_S (long enough to reach the bottom
    // from anywhere) and assumes it is there. Without switches the speeds are
    // calibrated by hand: run for a time, measure with a ruler.
    inline bool   LIFT_HAS_BOTTOM_SWITCH = true;
    inline bool   LIFT_HAS_TOP_SWITCH    = true;
    inline double LIFT_DOWN_FULL_S       = 6.0;

    // Elevator limit switches (the small yellow micro-switches). FlexDIO pins.
    // The lift will not move further up while the TOP switch is pressed, nor
    // further down while the BOTTOM switch is pressed.
    inline int  LIFT_TOP_SWITCH    = 6;
    inline int  LIFT_BOTTOM_SWITCH = 7;
    inline bool LIFT_SWITCH_ACTIVE_LOW = true;   // switch to ground: pressed reads 0

    // ----------------------------------------------------------- timing
    inline double CONTROL_DT   = 0.02;   // s, 50 Hz
    inline double MOVE_TIMEOUT = 12.0;   // s - a stuck move must end
    inline double LIFT_TIMEOUT = 6.0;

    // ----------------------------------------------------------- clamp servo
    // studica::Servo is a plain PWM device (extends frc::PWM) — it plugs
    // into a PWM header on the controller directly, NOT a Titan CAN channel.
    // <-- SET CHANNEL AFTER WIRING
    // PUSHER, not a clamp: the delivery pockets are closed, so the piece is
    // held above the pocket and pushed off the fork tip to drop in.
    // Servo pins are typed as the number PRINTED on the VMX (0-21).
    // The PWM channel is worked out from it (settings Recompute):
    // printed 0-11 (FlexDIO) -> PWM 10-21, printed 12-21 (HiCurrDIO) -> PWM 0-9.
    inline int    SERVO_PIN       = 13;    // pusher servo, printed pin (HiCurrDIO -> PWM 1)
    inline int    LIFT_SERVO_PIN  = 12;    // lift servo, printed pin (HiCurrDIO -> PWM 0)
    inline int    SERVO_CHANNEL   = 1;      // derived from SERVO_PIN
    // Angles for the pendulum pusher (cad/make_pusher.py). Fit the horn so the
    // arm hangs STRAIGHT DOWN at servo 150 deg; then park (150 back) clears
    // the mast, the support beam and the storage base (checked in CAD).
    inline double PARK_DEG        = 0.0;   // arm up and back, 30 deg past vertical            <-- CALIBRATE
    inline double RETRACT_DEG     = 128.0; // push START: paddle just behind the piece (150-22) <-- CALIBRATE
    inline double PUSH_DEG        = 172.0; // push END: piece off the fork tip (150+22)        <-- CALIBRATE
    inline double PUSH_SETTLE_S   = 0.3;   // pause after the pusher arrives
    inline double PUSH_SPEED      = 40.0;  // deg/s while pushing - slow, so the piece slides, not flips
    inline double RETRACT_SPEED   = 180.0; // deg/s coming back

    // ------------------------------------------------------------ vision
    inline int CAM_WIDTH  = 640;
    inline int CAM_HEIGHT = 480;

    // --------------------------------------------------------- comms
    // FRC reserves 5800-5810 for team use, so the control panel is safe here.
    inline int COMMAND_PORT = 5800;


    // ------------------------------------------------------ lift SERVO
    // The lift is a positional servo (no encoder). Height is set by angle,
    // mapped linearly between two calibrated points. Open loop: the code
    // waits LIFT_SERVO_SPEED to let it travel.
    inline int    LIFT_SERVO_CHANNEL = 0;      // derived from LIFT_SERVO_PIN
    inline double LIFT_ANGLE_AT_MIN  = 0.0;    // servo angle when fork is at LIFT_MIN
    inline double LIFT_ANGLE_AT_MAX  = 180.0;  // servo angle when fork is at LIFT_MAX
    inline double LIFT_SERVO_SPEED   = 60.0;   // mm/s, used only to wait for travel

    // ------------------------------------------------- motor directions
    // Flip a flag if that wheel/lift turns the wrong way. Never fix it by
    // negating in a behaviour - the robot would drive right but lose track
    // of where it is.
    inline bool INVERT_FL   = false;
    inline bool INVERT_FR   = true;
    inline bool INVERT_RL   = false;
    inline bool INVERT_RR   = true;
    inline bool INVERT_LIFT = false;
    // Wheel LAYOUT. false = mecanum, or omni wheels at 45 deg in the corners
    // ("X-drive"): same maths, can drive sideways. true = omni/normal wheels
    // pointing straight forward ("tank"): forward/back and turning only.
    inline bool DRIVE_TANK = false;

    // ------------------------------------------------------------ sensors
    // Each one can be switched off (no errors on a robot that lacks it).
    // Pin numbers = the numbers printed on the VMX. Set them from the port planner.
    inline bool   US_ENABLED   = false;   // ultrasonic (front)
    inline int    US_PING      = 0;       // DIO pin of the ping / trig wire
    inline int    US_ECHO      = 1;       // DIO pin of the echo wire
    inline bool   IR_ENABLED   = false;   // Sharp IR range sensor (front)
    inline int    IR_CHANNEL   = 0;       // printed Analog 0-3
    inline bool   COBRA_ENABLED = false;  // Cobra line follower (I2C port)
    inline double COBRA_VREF   = 5.0;     // Cobra reference voltage
    inline double COBRA_LINE_V = 2.5;     // a channel above this voltage = it sees the line   <-- CALIBRATE
    inline double SENSOR_SPEED = 120.0;   // mm/s while driving to a distance / line
    inline double SENSOR_TOL   = 10.0;    // mm, close enough for drive_to_*
    // Encoder direction, separate from motor direction: tick if a wheel spun
    // FORWARD makes its encoder reading go DOWN (Settings page live readout).
    inline bool ENC_INVERT_FL = false;
    inline bool ENC_INVERT_FR = false;
    inline bool ENC_INVERT_RL = false;
    inline bool ENC_INVERT_RR = false;

    // ------------------------------------------- colour thresholds (HSV)
    // OpenCV HSV: H 0-180, S 0-255. Tune under the lighting you compete in.
    inline double RED_H_LO   =   0.0;  inline double RED_H_HI   =  10.0;
    inline double RED_H_LO2  = 170.0;  inline double RED_H_HI2  = 180.0;
    inline double RED_S_MIN  =  90.0;
    inline double GREEN_H_LO =  40.0;  inline double GREEN_H_HI =  90.0;
    inline double GREEN_S_MIN=  80.0;
    inline double YELLOW_H_LO=  22.0;  inline double YELLOW_H_HI=  38.0;
    inline double YELLOW_S_MIN=100.0;
    inline double BLUE_H_LO  = 100.0;  inline double BLUE_H_HI  = 135.0;
    inline double BLUE_S_MIN =  80.0;
    inline double SAT_MIN_COLOUR = 50.0;
}
