/*****************************************************************************
 * The hardware seam. Everything above this file talks only to Hardware, so
 * there is exactly one place that knows about Titan channels and wiring.
 *
 * WHEEL ORDER IS ALWAYS front-left, front-right, rear-left, rear-right.
 * Get it wrong and the robot drives sideways while believing it went
 * forward - both halves look correct in isolation, which is what makes it
 * such an expensive bug.
 *
 * A wheel running backwards is fixed by flipping its INVERT_* flag below,
 * never by negating in a behaviour. Negating in a behaviour gives you a
 * robot that drives correctly and does not know where it is.
 *****************************************************************************/

#pragma once

#include <string>

#include <frc/DigitalInput.h>
#include <frc/Ultrasonic.h>
#include <frc/AnalogInput.h>
#include "studica/Cobra.h"
#include <frc/DigitalOutput.h>
#include <frc/smartdashboard/SmartDashboard.h>

#include "studica/TitanQuad.h"
#include "studica/TitanQuadEncoder.h"
#include "studica/Servo.h"

#include "AHRS.h"
#include <map>
#include <memory>
#include "Constants.h"

class Hardware
{
public:
    Hardware();

    // ---- drive ----------------------------------------------------------
    void SetWheels(double fl, double fr, double rl, double rr);
    void StopWheels();

    double GetFL();   // mm travelled
    double GetFR();
    double GetRL();
    double GetRR();

    // ---- manipulator ----------------------------------------------------
    void   SetLiftHeight(double mm);  // servo lift: command a height
    double GetLiftHeight();           // mm, last commanded (no encoder)
    void   SetLift(double duty);      // kept for old callers: nudges the target
    void   ZeroLift(double height_mm);
    void   SetLiftRaw(double cmd);    // continuous lift: raw servo command (90 = stop)
    void   SetLiftEstimate(double mm) { liftTarget = mm; }
    bool   LiftAtTop();               // top limit switch pressed
    bool   LiftAtBottom();            // bottom limit switch pressed

    // ---- sensors --------------------------------------------------------
    double GetYaw();                  // deg, -180..180
    void   ResetYaw();
    void   ResetEncoders();

    // ---- clamp servo ----------------------------------------------------
    void SetPusher(double angle_deg);  // 0-180, PUSH_DEG / RETRACT_DEG

    // ---- control panel --------------------------------------------------
    bool GetStartButton();
    bool GetStopButton();
    bool RawStart();                  // the pin's actual level, for the Settings page
    bool GetResetButton();            // true = pressed (false if no reset button)
    int  RawReset();                  // pin level, -1 = no reset button
    bool RawStop();             // TRUE when NOT pressed - active low
    void SetRunningLED(bool on);
    void SetStoppedLED(bool on);
    // The ONLY place the LEDs are decided. Called every 20 ms from RobotPeriodic.
    void UpdateLEDs(bool estopLatched);

    // Everything stops. Call from the stop button and from any error path.
    void StopAll();

    void Publish();

    // Ask the drive Titan who it is over CAN. Empty firmware = not talking.
    std::string TitanInfoJson();

    // Port check: did the VMX accept each servo's PWM channel at start-up?
    bool LiftServoOk()   const;
    bool PusherServoOk() const;
    // Servo finder: move whatever servo is on PWM channel ch to deg (0-180).
    // Returns "" on success, or why it failed.
    std::string TestServo(int ch, double deg);

    // ---- sensors (each returns -1 when switched off / no reading) ----------
    double GetUltrasonicMM();
    double GetIRMM();
    double GetCobraV(int channel);        // 0-3, volts
    int    CobraLine();                   // bit mask of channels that see the line
    std::string SensorsJson();                   // push telemetry to Shuffleboard

private:
    studica::TitanQuad frontLeft {constant::TITAN_ID, constant::MOTOR_FREQ,
                                  constant::FRONT_LEFT};
    studica::TitanQuad frontRight{constant::TITAN_ID, constant::MOTOR_FREQ,
                                  constant::FRONT_RIGHT};
    studica::TitanQuad rearLeft  {constant::TITAN_ID, constant::MOTOR_FREQ,
                                  constant::REAR_LEFT};
    studica::TitanQuad rearRight {constant::TITAN_ID, constant::MOTOR_FREQ,
                                  constant::REAR_RIGHT};

    studica::TitanQuadEncoder encFL{frontLeft,  constant::FRONT_LEFT,
                                    constant::DIST_PER_TICK};
    studica::TitanQuadEncoder encFR{frontRight, constant::FRONT_RIGHT,
                                    constant::DIST_PER_TICK};
    studica::TitanQuadEncoder encRL{rearLeft,   constant::REAR_LEFT,
                                    constant::DIST_PER_TICK};
    studica::TitanQuadEncoder encRR{rearRight,  constant::REAR_RIGHT,
                                    constant::DIST_PER_TICK};


    studica::Servo pusherServo{constant::SERVO_CHANNEL};      // plain PWM, not Titan CAN
    studica::Servo liftServo  {constant::LIFT_SERVO_CHANNEL}; // lift is a servo too


    AHRS navX{frc::SPI::Port::kMXP};

    frc::DigitalInput  startButton{constant::START_BUTTON};
    frc::DigitalInput  stopButton {constant::STOP_BUTTON};
    frc::DigitalInput  liftTopSwitch   {constant::LIFT_TOP_SWITCH};
    frc::DigitalInput  liftBottomSwitch{constant::LIFT_BOTTOM_SWITCH};
    std::unique_ptr<frc::DigitalInput> resetButton;   // only if RESET_BUTTON >= 0
    frc::DigitalOutput runningLED {constant::RUNNING_LED};
    frc::DigitalOutput stoppedLED {constant::STOPPED_LED};


    double liftTarget = 40.0;
    std::map<int, std::unique_ptr<studica::Servo>> testServos;
    std::unique_ptr<frc::Ultrasonic>  ultrasonic;
    std::unique_ptr<frc::AnalogInput> irSensor;
    std::unique_ptr<studica::Cobra>   cobra;   // last commanded lift height, mm

    static double Clamp(double v, double lo, double hi);
};
