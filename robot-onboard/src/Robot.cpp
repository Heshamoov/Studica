/*****************************************************************************
 * SC2026 Skill 23 - main robot program.
 *
 * THREE WAYS TO DRIVE THIS ROBOT, all going through the same dispatch table
 * in Sequence.cpp so they can never disagree:
 *
 *   1  the web control panel   tools/controlpanel.html -> HTTP :5800
 *   2  the physical start button  runs the stored routine
 *   3  a hard-coded fallback routine, in case the panel is unreachable
 *
 * (3) exists because a competition is not the place to discover that a
 * network is down. If nothing has been loaded, the start button still runs
 * something sensible.
 *****************************************************************************/

#include <frc/TimedRobot.h>
#include <frc/DriverStation.h>
#include <frc/smartdashboard/SmartDashboard.h>
#include <frc/livewindow/LiveWindow.h>
#include <frc/XboxController.h>

#include <memory>
#include <thread>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <frc/Timer.h>

#include "Constants.h"
#include "Hardware.h"
#include "Movement.h"
#include "Oms.h"
#include "Sequence.h"
#include "CommandServer.h"
#include "Settings.h"
#include "Log.h"
#include "studica/MockDS.h"
#include "Camera.h"

class Robot : public frc::TimedRobot
{
public:
    void RobotInit() override
    {
        settings::Init();   // load settings.cfg BEFORE any hardware is created
        hardware    = std::make_unique<Hardware>();
        movement    = std::make_unique<Movement>(hardware.get());
        manipulator = std::make_unique<Oms>(hardware.get(), movement.get());
        camera      = std::make_unique<Camera>();
        sequence    = std::make_unique<Sequence>(movement.get(),
                                                 manipulator.get(),
                                                 hardware.get(),
                                                 camera.get());
        server      = std::make_unique<CommandServer>(sequence.get(),
                                                      movement.get(),
                                                      manipulator.get(),
                                                      hardware.get());

        hardware->ZeroLift(constant::LIFT_HOME);   // (ignored for a continuous lift servo)
        hardware->StopAll();                         // continuous servos start STOPPED (90)
        movement->SetPosition(0.0, 0.0, 0.0);

        LoadFallbackRoutine();
        server->Start(constant::COMMAND_PORT);
        // No FRC Driver Station on this robot. Without this the robot stays
        // DISABLED and WPILib blocks every motor and servo output.
        ds.Enable();
        std::printf("titan check: %s\n", hardware->TitanInfoJson().c_str());
        robotlog::Log("program started. titan: " + hardware->TitanInfoJson());
        robotlog::Log("pins: start " + std::to_string(constant::START_BUTTON) + ", stop " + std::to_string(constant::STOP_BUTTON) + ", reset " + std::to_string(constant::RESET_BUTTON) +
                      ", LEDs green " + std::to_string(constant::RUNNING_LED) + " red " + std::to_string(constant::STOPPED_LED) +
                      ", lift switches top " + std::to_string(constant::LIFT_TOP_SWITCH) + " bottom " + std::to_string(constant::LIFT_BOTTOM_SWITCH) +
                      ", servos lift pin " + std::to_string(constant::LIFT_SERVO_PIN) + " pusher pin " + std::to_string(constant::SERVO_PIN));
        {
            // Same physical pin used twice? (servo PWM 10-21 = pins 0-11,
            // PWM 0-9 = pins 12-21; DIO channel = pin number)
            struct U { const char *n; int pin; } u[] = {
                {"start button", constant::START_BUTTON}, {"stop button", constant::STOP_BUTTON}, {"reset button", constant::RESET_BUTTON},
                {"green LED", constant::RUNNING_LED}, {"red LED", constant::STOPPED_LED},
                {"lift top switch", constant::LIFT_TOP_SWITCH}, {"lift bottom switch", constant::LIFT_BOTTOM_SWITCH},
                {"pusher servo", constant::SERVO_PIN}, {"lift servo", constant::LIFT_SERVO_PIN}};
            for (size_t i = 0; i < sizeof u / sizeof u[0]; ++i)
                for (size_t j = i + 1; j < sizeof u / sizeof u[0]; ++j)
                    if (u[i].pin >= 0 && u[i].pin == u[j].pin) {
                        pinClash = true;
                        robotlog::Log(std::string("PIN CLASH: ") + u[i].n + " and " + u[j].n + " are both on board pin " +
                                      std::to_string(u[i].pin) + " - the start button is DISABLED until you fix it in Settings > Wiring");
                    }
        }
        robotlog::Log(std::string("PORT CHECK: lift servo pin ") + std::to_string(constant::LIFT_SERVO_PIN) +
                      (hardware->LiftServoOk() ? " OK" : " REJECTED by the VMX") + ", pusher servo pin " +
                      std::to_string(constant::SERVO_PIN) + (hardware->PusherServoOk() ? " OK" : " REJECTED by the VMX") +
                      ". Pins 12-21 drive servos only with the HiCurrDIO jumper fitted.");
        robotlog::Log(std::string("buttons now: start ") + (hardware->GetStartButton() ? "PRESSED" : "released") +
                      ", stop " + (hardware->GetStopButton() ? "released" : "PRESSED (motors will be blocked!)"));
        std::fflush(stdout);
        camera->Start();

        frc::SmartDashboard::PutString("panel",
            "http://10.12.34.2:" + std::to_string(constant::COMMAND_PORT));
        frc::LiveWindow::GetInstance()->DisableAllTelemetry();
    }

    void RobotPeriodic() override
    {
        // Loop-time report every 5 s: shows which part is slow (target 20 ms).
        double t0 = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        int req = server->enableRequest.exchange(-1);
        if (req == 1) ds.Enable();
        if (req == 0) { sequence->Abort(); hardware->StopAll(); ds.Disable(); }
        auto T = [] { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); };
        double t1 = T(); hardware->UpdateLEDs(server->estopLatched.load());
        // Continuous lift: whenever no command is running, keep sending STOP,
        // so nothing (a stale command, a restart, a dropped page) can leave it
        // spinning.
        if (constant::LIFT_CONTINUOUS && !server->Busy() && !sequence->IsRunning())
            hardware->SetLiftRaw(constant::LIFT_STOP_CMD);
        double t2 = T(); movement->UpdateOdometry();
        double t3 = T(); hardware->Publish();
        double t4 = T(); movement->Publish(); camera->Publish();
        frc::SmartDashboard::PutBoolean("busy", server->Busy());
        frc::SmartDashboard::PutNumber("seq_steps",
                                       static_cast<double>(sequence->Size()));
        double t5 = T();
        tLed += t2 - t1; tOdo += t3 - t2; tPub += t4 - t3; tRest += (t5 - t4) + (t1 - t0);
        ++nLoops;
        if (t5 - tReport > 5.0) {
            std::printf("loop timing (avg ms over %d loops): LEDs %.1f  odometry/encoders %.1f  "
                        "telemetry %.1f  other %.1f\n", nLoops, 1000 * tLed / nLoops,
                        1000 * tOdo / nLoops, 1000 * tPub / nLoops, 1000 * tRest / nLoops);
            std::fflush(stdout);
            tLed = tOdo = tPub = tRest = 0; nLoops = 0; tReport = t5;
        }
    }

    // Studica's MockDS enables the robot in AUTONOMOUS mode (seen in the log).
    // We do NOT auto-run the routine on enable - that would drive the robot the
    // moment the program starts. The routine runs from the start button or the
    // panel. Both modes run the same hand-control loop below.
    void AutonomousInit() override
    {
        sequence->Abort();
        hardware->StopAll();
    }

    void AutonomousPeriodic() override { ControlStep(); }

    // -----------------------------------------------------------------
    // TELEOP  (D1 — 30 marks)
    //
    // Left stick  : drive (vx = forward/back, vy = strafe left/right)
    // Right stick : rotate (omega)
    // Left trigger (LT)  : jog lift DOWN
    // Right trigger (RT) : jog lift UP
    // Left bumper  (LB)  : jog extension RETRACT
    // Right bumper (RB)  : jog extension EXTEND
    // A button     : abort any running sequence (emergency stop)
    // Start button : run stored autonomous routine
    //
    // The slew-rate limiters prevent the motors from being commanded to full
    // speed instantly, which on mecanum causes the robot to lurch. 3 m/s² is
    // a comfortable arena value — raise it if the driver wants sharper response.
    // -----------------------------------------------------------------
    void TeleopInit() override
    {
        // Abort any running sequence when we enter teleop so the driver gets
        // instant control. This also fires when you press the E-stop.
        sequence->Abort();
        hardware->StopAll();
    }

    void TeleopPeriodic() override { ControlStep(); }

    void ControlStep()
    {
        // ---- physical panel buttons ----
        bool startPressed = hardware->GetStartButton();
        bool stopNow = !hardware->GetStopButton();
        if (stopNow != stopWasPressed) {
            robotlog::Log(stopNow ? "STOP button PRESSED - all motors blocked until released"
                                  : "stop button released - motors allowed");
            stopWasPressed = stopNow;
        }
        if (startPressed != startLogged) {
            robotlog::Log(startPressed ? "start button pressed" : "start button released");
            startLogged = startPressed;
        }
        // Start = held steadily for 10 loops (~0.2 s), and only after it was
        // seen released. A servo pulse or noise on the pin can no longer
        // start the routine.
        startHeld = startPressed ? startHeld + 1 : 0;
        if (startHeld == 10 && startReleasedOnce && !server->Busy() && !pinClash) {
            server->estopLatched = false;
            robotlog::Log("start button: running the stored routine");
            RunStoredRoutine();
        }
        if (!startPressed) startReleasedOnce = true;
        startWasPressed = startPressed;

        // RESET button: held ~0.2 s -> stop everything, clear e-stop, zero pose.
        resetHeld = hardware->GetResetButton() ? resetHeld + 1 : 0;
        if (resetHeld == 10) {
            robotlog::Log("RESET button: stopped, e-stop cleared, position zeroed - ready for START");
            server->ResetFromButton();
            hardware->ResetEncoders();
            hardware->ResetYaw();
            movement->SetPosition(0.0, 0.0, 0.0);
            startReleasedOnce = !startPressed;
        }

        if (!hardware->GetStopButton()) {           // active-low
            sequence->Abort();
            hardware->StopAll();
            return;
        }

        // Only read the gamepad if one is really plugged in. With none, the
        // reads can return junk (phantom A = stop everything, phantom bumper =
        // pusher sweep), which blocked the loop and cancelled the wheel tests.
        bool padConnected = frc::DriverStation::GetInstance().GetStickButtonCount(0) > 0;

        // ---- gamepad: abort button ----
        if (padConnected && gamepad.GetAButton()) {
            sequence->Abort();
            hardware->StopAll();
            return;
        }

        // If a sequence is running (started by Start button or HTTP), let it run.
        if (server->Busy()) return;

        // ---- browser teleop (control panel drive pad) wins while active ----
        double wx, wy, ww;
        if (server->WebDrive(wx, wy, ww)) {
            if (!webDriving) robotlog::Log("hand driving from the browser started");
            movement->Drive(wx * constant::MAX_SPEED, wy * constant::MAX_SPEED,
                            ww * constant::TURN_FAST);
            webDriving = true;
            return;
        }
        if (webDriving) { movement->Stop(); webDriving = false; robotlog::Log("hand driving stopped"); }
        if (!padConnected) return;

        // ---- gamepad: drive ----
        // Dead-band of 0.08 removes stick drift.
        auto db = [](double v) { return std::abs(v) < 0.08 ? 0.0 : v; };

        double vx    = -db(gamepad.GetY(frc::GenericHID::kLeftHand))  * constant::MAX_SPEED;
        double vy    = -db(gamepad.GetX(frc::GenericHID::kLeftHand))  * constant::MAX_SPEED;
        double omega = -db(gamepad.GetX(frc::GenericHID::kRightHand)) * constant::TURN_FAST;

        // Slew-rate limit: smooth out sudden stick movements.
        auto slew = [](double target, double prev, double maxStep) {
            double d = target - prev;
            if (d >  maxStep) d =  maxStep;
            if (d < -maxStep) d = -maxStep;
            return prev + d;
        };
        vxPrev    = slew(vx,    vxPrev,    DriveSlew());
        vyPrev    = slew(vy,    vyPrev,    DriveSlew());
        omegaPrev = slew(omega, omegaPrev, TurnSlew());
        double vxL = vxPrev, vyL = vyPrev, omegaL = omegaPrev;

        movement->Drive(vxL, vyL, omegaL);

        // ---- gamepad: lift jog ----
        // RT raises, LT lowers. GetTriggerAxis(hand) returns 0..1.
        double liftDuty = gamepad.GetTriggerAxis(frc::GenericHID::kRightHand)
                         - gamepad.GetTriggerAxis(frc::GenericHID::kLeftHand);
        manipulator->JogLift(liftDuty * 0.60);

        // ---- gamepad: clamp ----
        // RB = retract pusher, LB = push
        // servo directly (open / hold), edge-triggered so holding the
        // button doesn't keep re-issuing the same servo command.
        bool rbPressed = gamepad.GetBumper(frc::GenericHID::kRightHand);
        bool lbPressed = gamepad.GetBumper(frc::GenericHID::kLeftHand);
        if (rbPressed && !rbWasPressed) manipulator->Retract();
        if (lbPressed && !lbWasPressed) manipulator->Push();
        rbWasPressed = rbPressed;
        lbWasPressed = lbPressed;
    }

    void DisabledInit() override
    {
        sequence->Abort();
        hardware->StopAll();
    }

private:
    std::unique_ptr<Hardware>      hardware;
    std::unique_ptr<Movement>      movement;
    std::unique_ptr<Oms>           manipulator;
    std::unique_ptr<Sequence>      sequence;
    std::unique_ptr<CommandServer> server;
    std::unique_ptr<Camera>        camera;

    frc::XboxController gamepad{0};         // USB port 0 on the Driver Station

    // Max change per 20 ms tick (~50 Hz). ~3 m/s² drive, ~4×TURN_FAST turn.
    double DriveSlew() const { return constant::MAX_SPEED * 3.0 / 50.0; }
    double TurnSlew()  const { return constant::TURN_FAST * 4.0 / 50.0; }
    double vxPrev = 0.0, vyPrev = 0.0, omegaPrev = 0.0;

    bool startWasPressed = true;   // a button already "pressed" at start-up (wrong pin, floating input) must never start a routine
    bool webDriving = false;
    int  startHeld = 0;
    bool startReleasedOnce = false;
    bool pinClash = false;
    bool stopWasPressed = false, startLogged = false;
    int  resetHeld = 0;
    double tLed = 0, tOdo = 0, tPub = 0, tRest = 0, tReport = 0;
    int nLoops = 0;
    studica::MockDS ds;
    bool rbWasPressed = false;
    bool lbWasPressed = false;

    void RunStoredRoutine()
    {
        // On its own thread: the steps block, and blocking TimedRobot's
        // periodic loop would stall telemetry and the abort path with it.
        std::thread([this] { sequence->RunAll(); }).detach();
    }

    void LoadFallbackRoutine()
    {
        // A complete pick-and-place, so the start button always does
        // something useful even with no panel attached. Replace it from the
        // panel with /load once you have a real routine.
        sequence->Parse(
            "led_running:1,"
            "lift_home:0,"
            "forward:400,"
            "read_colour:0,"    // log what colour we see before picking
            "pick:0,"
            "backward:300,"
            "turn_by:180,"
            "forward:400,"
            "sort_piece:1,"     // place on the stand that matches the colour
            "backward:200,"
            "lift_home:0,"
            "led_stopped:1");
    }
};

#ifndef RUNNING_FRC_TESTS
int main() { return frc::StartRobot<Robot>(); }
#endif
