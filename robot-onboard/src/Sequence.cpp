#include "Sequence.h"
#include "Settings.h"
#include "Camera.h"

#include <frc/smartdashboard/SmartDashboard.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>

std::vector<std::string> Sequence::KnownCommands()
{
    return {
        // drive
        "forward", "backward", "strafe_left", "strafe_right",
        "turn_by", "turn_to", "stop",
        // lift (no extension axis — the chassis drives forward/backward
        // to insert and withdraw the fork; see "pick" / "place_stand")
        "lift_to", "lift_by", "lift_home", "lift_carry", "lift_stand",
        "lift_find_bottom", "lift_calibrate", "lift_run_up", "lift_run_down",
        // compound
        "pick", "place_stand",
        // housekeeping
        "wait", "set_position_zero", "zero_lift",
        "led_running", "led_stopped",
        // pusher (clamp_open/clamp_hold kept as aliases for old routines)
        "push", "retract", "clamp_open", "clamp_hold",
        // bring-up tests: spin ONE wheel (0=FL 1=FR 2=RL 3=RR) or the lift
        "spin_wheel", "spin_lift",
        // sensors
        "drive_to_us", "drive_to_ir", "drive_to_line", "follow_line",
        // colour / camera
        "read_colour", "sort_piece",
    };
}

bool Sequence::Execute(const std::string &cmd, double arg)
{
    // ---- drive ----------------------------------------------------------
    if (cmd == "forward")      return movement->Forward(arg);
    if (cmd == "backward")     return movement->Backward(arg);
    if ((cmd == "strafe_left" || cmd == "strafe_right") && constant::DRIVE_TANK) {
        lastError = "this robot's wheels point straight (Drive layout = tank): it cannot drive sideways";
        return false;
    }
    if (cmd == "strafe_left")  return movement->StrafeLeft(arg);
    if (cmd == "strafe_right") return movement->StrafeRight(arg);
    if (cmd == "turn_by")      return movement->TurnBy(arg);
    if (cmd == "turn_to")      return movement->TurnTo(arg);
    if (cmd == "stop")       { movement->Stop(); manipulator->Stop(); return true; }

    // ---- lift -----------------------------------------------------------
    if (cmd == "lift_to")        return manipulator->LiftTo(arg);
    if (cmd == "lift_run_up")   return manipulator->LiftRunTimed(true,  arg);
    if (cmd == "lift_run_down") return manipulator->LiftRunTimed(false, arg);
    if (cmd == "lift_find_bottom") {
        if (manipulator->LiftHomeSwitch()) return true;
        lastError = "bottom limit switch not reached - check its pin, its polarity, and LIFT_DOWN_CMD";
        return false;
    }
    if (cmd == "lift_calibrate") {
        if (!manipulator->LiftCalibrate()) {
            lastError = constant::LIFT_HAS_BOTTOM_SWITCH && constant::LIFT_HAS_TOP_SWITCH ? "calibration failed - both limit switches must work (check the live readout)" : "automatic calibration needs BOTH switches - use the ruler calibration on the Settings page";
            return false;
        }
        std::string msg;
        settings::Set("LIFT_UP_MM_S", std::to_string(constant::LIFT_UP_MM_S), msg);
        settings::Set("LIFT_DOWN_MM_S", std::to_string(constant::LIFT_DOWN_MM_S), msg);
        return true;
    }
    if (cmd == "lift_by")        return manipulator->LiftBy(arg);
    if (cmd == "lift_home")      return manipulator->LiftHome();
    if (cmd == "lift_carry")     return manipulator->LiftCarry();
    if (cmd == "lift_stand")     return manipulator->LiftStand(static_cast<int>(arg));

    // ---- compound -------------------------------------------------------
    if (cmd == "pick")        return manipulator->PickFromStorage();
    if (cmd == "place_stand") return manipulator->PlaceOnStand(static_cast<int>(arg));

    // ---- housekeeping ---------------------------------------------------
    if (cmd == "wait") {
        // arg in seconds, but honour the stop button while waiting
        double t = 0.0;
        while (t < arg) {
            if (!hardware->GetStopButton()) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            t += 0.02;
        }
        return true;
    }
    if (cmd == "set_position_zero") { movement->SetPosition(0, 0, 0); return true; }
    if (cmd == "zero_lift")   { hardware->ZeroLift(arg);   return true; }
    // LEDs follow one rule (Hardware::UpdateLEDs); these stay for old routines.
    if (cmd == "led_running" || cmd == "led_stopped") return true;

    // ---- sensors ---------------------------------------------------------
    // drive_to_us:80   drive until the ultrasonic reads 80 mm
    // drive_to_ir:50   same with the Sharp IR sensor
    // drive_to_line:500  forward (max 500 mm) until the Cobra sees a line
    // follow_line:400  follow the line for 400 mm
    if (cmd == "drive_to_us") {
        if (!constant::US_ENABLED) { lastError = "ultrasonic is switched off in Settings"; return false; }
        return movement->DriveToDistance(arg, false);
    }
    if (cmd == "drive_to_ir") {
        if (!constant::IR_ENABLED) { lastError = "IR sensor is switched off in Settings"; return false; }
        return movement->DriveToDistance(arg, true);
    }
    if (cmd == "drive_to_line") {
        if (!constant::COBRA_ENABLED) { lastError = "Cobra is switched off in Settings"; return false; }
        if (movement->DriveToLine(arg > 0 ? arg : 1000)) return true;
        lastError = "no line found within " + std::to_string((int)(arg > 0 ? arg : 1000)) + " mm";
        return false;
    }
    if (cmd == "follow_line") {
        if (!constant::COBRA_ENABLED) { lastError = "Cobra is switched off in Settings"; return false; }
        if (movement->FollowLine(arg)) return true;
        lastError = "lost the line";
        return false;
    }

    // ---- bring-up tests -------------------------------------------------
    // spin_wheel:N  N = 0 front-left, 1 front-right, 2 rear-left, 3 rear-right.
    //   Spins that ONE wheel forward at 30% for 1 s, so you can see which
    //   wheel it is and which way it turns. Wheels off the ground first!
    // spin_lift:+1 / spin_lift:-1  moves the servo lift up / down 10 mm.
    if (cmd == "spin_wheel") {
        int n = static_cast<int>(arg);
        if (n < 0 || n > 3) { lastError = "spin_wheel needs 0-3"; return false; }
        double d[4] = {0, 0, 0, 0};
        d[n] = 0.30;
        // Re-send every 20 ms like Studica's own code: the Titan stops a
        // motor that stops receiving commands (CAN safety timeout).
        for (int t = 0; t < 50; ++t) {
            hardware->SetWheels(d[0], d[1], d[2], d[3]);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        hardware->StopWheels();
        return true;
    }
    if (cmd == "spin_lift") {
        bool up = arg >= 0;
        if (!hardware->GetStopButton()) {
            lastError = "the STOP button reads PRESSED (nothing wired to its pin reads as pressed) - "
                        "wire it, or untick 'Use the stop button' in Settings for bench testing";
            return false;
        }
        if (constant::LIFT_CONTINUOUS) {
            // Bring-up test: just spin the servo 0.5 s that way. No homing
            // first (homing drives DOWN to the bottom switch, which is the
            // opposite of 'Lift up' and needs switches that may not be wired).
            if ((up   && constant::LIFT_HAS_TOP_SWITCH    && hardware->LiftAtTop()) ||
                (!up  && constant::LIFT_HAS_BOTTOM_SWITCH && hardware->LiftAtBottom())) {
                lastError = std::string("the lift ") + (up ? "TOP" : "BOTTOM") +
                            " switch reads PRESSED - if it is not wired, untick 'Lift has a " +
                            (up ? "TOP" : "BOTTOM") + " switch' in Settings";
                return false;
            }
            return manipulator->LiftRunTimed(up, 0.5);
        }
        return manipulator->LiftBy(up ? 10.0 : -10.0);
    }

    // ---- clamp ----------------------------------------------------------
    if (cmd == "push"    || cmd == "clamp_open") { manipulator->Push();    return true; }
    if (cmd == "retract" || cmd == "clamp_hold") { manipulator->Retract(); return true; }

    // ---- camera commands ------------------------------------------------
    // read_colour: reads the current colour from the camera sensor and
    //   publishes it to Shuffleboard. Returns true always.
    if (cmd == "read_colour") {
        if (!camera) lastColour = "no camera";
        if (camera) {
            std::string col = camera->GetColour();
            lastColour = col;
            frc::SmartDashboard::PutString("colour", col);
        }
        return true;
    }

    // sort_piece: reads the piece colour and places it on the matching stand.
    //   Colour → stand mapping (matches the E1 scoring order):
    //     red=1 (25mm), green=2 (50mm), yellow=3 (75mm), blue=4 (100mm)
    //   hazard/none → uses the arg as fallback stand number (default 1).
    if (cmd == "sort_piece") {
        int stand = static_cast<int>(arg > 0 ? arg : 1.0);
        if (camera) {
            std::string col = camera->GetColour();
            lastColour = col;
            if      (col == "red")    stand = 1;
            else if (col == "green")  stand = 2;
            else if (col == "yellow") stand = 3;
            else if (col == "blue")   stand = 4;
            frc::SmartDashboard::PutString("sort_colour", col);
            frc::SmartDashboard::PutNumber("sort_stand",  stand);
        }
        return manipulator->PlaceOnStand(stand);
    }

    lastError = "unknown command: " + cmd;
    return false;
}

// ------------------------------------------------------------------ routine
void Sequence::Clear()
{
    steps.clear();
    current = -1;
    lastError.clear();
}

void Sequence::Add(const std::string &cmd, double arg)
{
    steps.push_back({cmd, arg});
}

bool Sequence::RunAll()
{
    running = true;
    lastError.clear();
    movement->abort = false;
    manipulator->abort = false;


    for (size_t i = 0; i < steps.size(); ++i) {
        current = static_cast<int>(i);
        frc::SmartDashboard::PutString("step", steps[i].cmd);
        frc::SmartDashboard::PutNumber("step_index", static_cast<double>(i));

        if (!Execute(steps[i].cmd, steps[i].arg)) {
            // Fail fast. Blundering on after a failed pick means carrying a
            // piece you never actually collected.
            if (lastError.empty())
                lastError = "step " + std::to_string(i) + " (" + steps[i].cmd +
                            ") failed or timed out";
            running = false;
            movement->Stop();
            manipulator->Stop();
            return false;
        }
    }

    running = false;
    current = -1;
    movement->Stop();
    manipulator->Stop();

    return true;
}

void Sequence::Abort()
{
    movement->abort = true;
    manipulator->abort = true;
    running = false;
}

// -------------------------------------------------------------------- parse
bool Sequence::Parse(const std::string &text)
{
    Clear();
    std::stringstream ss(text);
    std::string item;

    while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        auto colon = item.find(':');
        std::string cmd = colon == std::string::npos ? item : item.substr(0, colon);
        double arg = 0.0;
        if (colon != std::string::npos) {
            try { arg = std::stod(item.substr(colon + 1)); }
            catch (...) { lastError = "bad number in: " + item; return false; }
        }
        cmd.erase(std::remove_if(cmd.begin(), cmd.end(),
                                 [](unsigned char c){ return std::isspace(c); }),
                  cmd.end());

        auto known = KnownCommands();
        if (std::find(known.begin(), known.end(), cmd) == known.end()) {
            // Reject the whole routine rather than silently skipping a step -
            // a routine that quietly drops its pick is worse than one that
            // refuses to load.
            lastError = "unknown command: " + cmd;
            Clear();
            return false;
        }
        Add(cmd, arg);
    }
    return true;
}

std::string Sequence::Describe() const
{
    std::string out;
    for (size_t i = 0; i < steps.size(); ++i) {
        if (i) out += ",";
        out += steps[i].cmd + ":" + std::to_string(steps[i].arg);
    }
    return out;
}
