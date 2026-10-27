#include "Oms.h"

#include <cmath>
#include <thread>
#include <chrono>

static void SleepSeconds(double s)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(s * 1000)));
}

double Oms::Clamp(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

bool Oms::ShouldStop()
{
    return abort || !hardware->GetStopButton();
}

void Oms::Stop()
{
    movement->Stop();
}

double Oms::StandHeight(int n)
{
    switch (n) {
        case 1:  return constant::STAND_1;
        case 2:  return constant::STAND_2;
        case 3:  return constant::STAND_3;
        case 4:  return constant::STAND_4;
        default: return -1.0;
    }
}

double Oms::StandPocketDepth(int n)
{
    return n == 1 ? constant::STAND_POCKET_1 : constant::STAND_POCKET;
}

// -------------------------------------------------------------------- lift
// Servo lift: command the height, then wait long enough for it to travel.
// ---------------------------------------------------- continuous lift
// Drive the lift up/down for up to `seconds`, stopping early at that
// direction's limit switch. `ran` = seconds actually run.
bool Oms::RunLiftUntil(bool up, double seconds, double &ran)
{
    ran = 0.0;
    bool hit = false;
    hardware->SetLiftRaw(up ? constant::LIFT_UP_CMD : constant::LIFT_DOWN_CMD);
    while (ran < seconds) {
        if (ShouldStop()) break;
        bool sw = up ? (constant::LIFT_HAS_TOP_SWITCH && hardware->LiftAtTop())
                     : (constant::LIFT_HAS_BOTTOM_SWITCH && hardware->LiftAtBottom());
        if (sw) { hit = true; break; }
        SleepSeconds(0.01);
        ran += 0.01;
    }
    hardware->SetLiftRaw(constant::LIFT_STOP_CMD);
    return hit;
}

bool Oms::LiftHomeSwitch()
{
    double ran;
    if (constant::LIFT_HAS_BOTTOM_SWITCH) {
        if (!RunLiftUntil(false, constant::LIFT_HOME_TIMEOUT, ran)) return false;   // never reached the switch
    } else {
        RunLiftUntil(false, constant::LIFT_DOWN_FULL_S, ran);   // no switch: run long enough to bottom out
        if (ShouldStop()) return false;
    }
    hardware->SetLiftEstimate(constant::LIFT_MIN);
    liftHomed = true;
    return true;
}

bool Oms::LiftRunTimed(bool up, double seconds)
{
    double ran;
    RunLiftUntil(up, seconds, ran);
    liftHomed = false;            // height is now unknown until measured / homed again
    return !ShouldStop();
}

bool Oms::LiftCalibrate()
{
    double up_t, down_t;
    if (!constant::LIFT_HAS_BOTTOM_SWITCH || !constant::LIFT_HAS_TOP_SWITCH) return false;   // needs both: use the timed (ruler) calibration
    if (!LiftHomeSwitch()) return false;
    SleepSeconds(0.3);
    if (!RunLiftUntil(true, constant::LIFT_HOME_TIMEOUT, up_t)) return false;
    SleepSeconds(0.3);
    if (!RunLiftUntil(false, constant::LIFT_HOME_TIMEOUT, down_t)) return false;
    if (up_t < 0.2 || down_t < 0.2) return false;
    constant::LIFT_UP_MM_S   = constant::LIFT_RAIL_MM / up_t;
    constant::LIFT_DOWN_MM_S = constant::LIFT_RAIL_MM / down_t;
    hardware->SetLiftEstimate(constant::LIFT_MIN);
    liftHomed = true;
    return true;
}

bool Oms::LiftTo(double mm)
{
    if (constant::LIFT_CONTINUOUS) {
        double target = Clamp(mm, constant::LIFT_MIN, constant::LIFT_MIN + constant::LIFT_RAIL_MM);
        if (!liftHomed && !LiftHomeSwitch()) return false;      // first move: find the bottom
        double h = hardware->GetLiftHeight();
        double ran;
        if (target <= constant::LIFT_MIN + 0.5 && constant::LIFT_HAS_BOTTOM_SWITCH) {   // to the bottom: use the switch
            bool hit = RunLiftUntil(false, constant::LIFT_HOME_TIMEOUT, ran);
            if (hit) hardware->SetLiftEstimate(constant::LIFT_MIN);
            return hit;
        }
        if (target >= constant::LIFT_MIN + constant::LIFT_RAIL_MM - 0.5 && constant::LIFT_HAS_TOP_SWITCH) {   // to the top
            bool hit = RunLiftUntil(true, constant::LIFT_HOME_TIMEOUT, ran);
            if (hit) hardware->SetLiftEstimate(constant::LIFT_MIN + constant::LIFT_RAIL_MM);
            return hit;
        }
        double d = target - h;
        if (std::fabs(d) < 1.0) return true;
        bool up = d > 0;
        double speed = std::fmax(up ? constant::LIFT_UP_MM_S : constant::LIFT_DOWN_MM_S, 1.0);
        bool hit = RunLiftUntil(up, std::fabs(d) / speed, ran);
        if (hit) hardware->SetLiftEstimate(up ? constant::LIFT_MIN + constant::LIFT_RAIL_MM : constant::LIFT_MIN);
        else     hardware->SetLiftEstimate(h + (up ? 1 : -1) * ran * speed);
        SleepSeconds(0.1);
        return !ShouldStop();
    }

    // Move the servo target in small steps at LIFT_SERVO_SPEED so a limit
    // switch can stop the lift part-way (Hardware::SetLiftHeight refuses to
    // go further into a pressed switch).
    double target = Clamp(mm, constant::LIFT_MIN, constant::LIFT_MAX);
    const double dt = 0.02;
    double step = std::fmax(constant::LIFT_SERVO_SPEED, 1.0) * dt;
    for (int guard = 0; guard < 5000; ++guard) {
        if (ShouldStop()) return false;
        double h = hardware->GetLiftHeight();
        double err = target - h;
        if (std::fabs(err) < 0.01) break;
        double next = h + (err > 0 ? std::fmin(step, err) : -std::fmin(step, -err));
        hardware->SetLiftHeight(next);
        if (std::fabs(hardware->GetLiftHeight() - h) < 1e-6) {
            // blocked by a limit switch: this is the end of travel - done
            return true;
        }
        SleepSeconds(dt);
    }
    SleepSeconds(0.15);   // let the servo finish settling
    return true;
}

bool Oms::LiftBy(double mm)
{
    return LiftTo(hardware->GetLiftHeight() + mm);
}

bool Oms::LiftStand(int n)
{
    double h = StandHeight(n);
    if (h < 0.0) return false;
    return LiftTo(h + constant::LIFT_PLACE_GAP);
}

// ------------------------------------------------------------------ pusher
// The servo is moved in small steps so the push is SLOW: the piece slides off
// the fork tip and drops flat instead of being flicked and flipped.
void Oms::SweepPusher(double target, double degPerSec)
{
    double rate = std::fmax(degPerSec, 1.0);
    double a = pusherAngle;
    const double dt = 0.02;
    while (std::fabs(target - a) > 0.5) {
        if (ShouldStop()) break;
        double step = rate * dt;
        a += (target > a) ? std::fmin(step, target - a) : -std::fmin(step, a - target);
        hardware->SetPusher(a);
        SleepSeconds(dt);
    }
    hardware->SetPusher(target);
    pusherAngle = target;
    SleepSeconds(constant::PUSH_SETTLE_S);
}

// Push: come down from park behind the piece (fast), sweep forward (SLOW),
// then go back to park so the arm never meets a storage base or stand wall.
void Oms::Push()
{
    if (constant::PUSHER_CONTINUOUS) {      // spin forward for a fixed time, then stop
        hardware->SetPusher(constant::PUSH_DEG);
        for (double t = 0; t < constant::PUSH_RUN_S && !ShouldStop(); t += 0.02) SleepSeconds(0.02);
        hardware->SetPusher(90.0);
        return;
    }
    SweepPusher(constant::RETRACT_DEG, constant::RETRACT_SPEED);
    SweepPusher(constant::PUSH_DEG,    constant::PUSH_SPEED);
    Retract();
}
void Oms::Retract()
{
    if (constant::PUSHER_CONTINUOUS) {      // spin back for the same time, then stop
        hardware->SetPusher(constant::PARK_DEG);
        for (double t = 0; t < constant::PUSH_RUN_S && !ShouldStop(); t += 0.02) SleepSeconds(0.02);
        hardware->SetPusher(90.0);
        return;
    }
    SweepPusher(constant::PARK_DEG, constant::RETRACT_SPEED);
}

// ---------------------------------------------------------------- compound
bool Oms::PickFromStorage()
{
    // Order matters - see the header comment. The piece rests on the fork;
    // the pusher only has to be out of the way.
    Retract();
    if (!LiftTo(constant::LIFT_UNDER))          return false;   // 1 just inside the slot
    if (!movement->Forward(constant::DRIVE_INSERT_STORAGE,
                           constant::DRIVE_PICKPLACE_SPEED))     return false;   // 2 fork under the piece
    if (!LiftTo(constant::LIFT_LOADED))         return false;   // 3 take the load
    if (!LiftTo(constant::LIFT_CLEAR_SLOT))     return false;   // 4 clear the slot
    if (!movement->Backward(constant::DRIVE_INSERT_STORAGE,
                            constant::DRIVE_PICKPLACE_SPEED))    return false;   // 5 withdraw
    return LiftTo(constant::LIFT_CARRY);                           // 6 carry height
}

bool Oms::PlaceOnStand(int n)
{
    double standH = StandHeight(n);
    if (standH < 0.0) return false;

    // 1. fork above the stand's top wall
    if (!LiftTo(standH + constant::LIFT_PLACE_GAP))              return false;
    // 2. drive forward until the fork tip is over the pocket
    if (!movement->Forward(constant::DRIVE_INSERT_STAND,
                           constant::DRIVE_PICKPLACE_SPEED))    return false;
    // 3. push the piece off the fork tip - it drops into the closed pocket
    Push();
    // 4. pull the pusher back
    Retract();
    // 5. withdraw
    if (!movement->Backward(constant::DRIVE_INSERT_STAND,
                            constant::DRIVE_PICKPLACE_SPEED))   return false;
    return LiftTo(constant::LIFT_CARRY);
}

// ------------------------------------------------------------------ teleop
void Oms::JogLift(double duty)
{
    if (std::fabs(duty) > 0.02) hardware->SetLift(duty);
}
