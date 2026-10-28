#include "Hardware.h"
#include <cmath>
#include <cstdio>

Hardware::Hardware()
{
    if (constant::RESET_BUTTON >= 0) resetButton = std::make_unique<frc::DigitalInput>(constant::RESET_BUTTON);
    // Optional sensors: only created when switched on in Settings.
    if (constant::US_ENABLED) {
        ultrasonic = std::make_unique<frc::Ultrasonic>(constant::US_PING, constant::US_ECHO);
        frc::Ultrasonic::SetAutomaticMode(true);
    }
    if (constant::IR_ENABLED)    irSensor = std::make_unique<frc::AnalogInput>(constant::IR_CHANNEL);
    if (constant::COBRA_ENABLED) cobra    = std::make_unique<studica::Cobra>(static_cast<float>(constant::COBRA_VREF));
    ResetEncoders();
    ResetYaw();
    StopAll();
    UpdateLEDs(false);
}

double Hardware::Clamp(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

// ------------------------------------------------------------------- drive
void Hardware::SetWheels(double fl, double fr, double rl, double rr)
{
    frontLeft .Set(Clamp(constant::INVERT_FL ? -fl : fl, -1.0, 1.0));
    frontRight.Set(Clamp(constant::INVERT_FR ? -fr : fr, -1.0, 1.0));
    rearLeft  .Set(Clamp(constant::INVERT_RL ? -rl : rl, -1.0, 1.0));
    rearRight .Set(Clamp(constant::INVERT_RR ? -rr : rr, -1.0, 1.0));
}

void Hardware::StopWheels()
{
    SetWheels(0.0, 0.0, 0.0, 0.0);
}

// GetRaw() returns TICKS. The Studica course project passes DIST_PER_TICK
// to the encoder constructor but still converts by hand, so we do the same -
// it works whether or not the library also scales internally, and GetRaw()
// is the one method the course code proves exists.
double Hardware::GetFL() { return ((constant::INVERT_FL != constant::ENC_INVERT_FL) ? -1.0 : 1.0) * encFL.GetRaw() * constant::DIST_PER_TICK; }
double Hardware::GetFR() { return ((constant::INVERT_FR != constant::ENC_INVERT_FR) ? -1.0 : 1.0) * encFR.GetRaw() * constant::DIST_PER_TICK; }
double Hardware::GetRL() { return ((constant::INVERT_RL != constant::ENC_INVERT_RL) ? -1.0 : 1.0) * encRL.GetRaw() * constant::DIST_PER_TICK; }
double Hardware::GetRR() { return ((constant::INVERT_RR != constant::ENC_INVERT_RR) ? -1.0 : 1.0) * encRR.GetRaw() * constant::DIST_PER_TICK; }

// ------------------------------------------------------------- manipulator
// The lift is a positional servo: height -> angle, linear between the two
// calibrated points. There is no feedback, so the height reported is the
// one last commanded.
void Hardware::SetLiftRaw(double cmd) { liftServo.SetAngle(Clamp(cmd, 0.0, 180.0)); }

bool Hardware::LiftAtTop()    { return liftTopSwitch.Get()    != constant::LIFT_SWITCH_ACTIVE_LOW; }
bool Hardware::LiftAtBottom() { return liftBottomSwitch.Get() != constant::LIFT_SWITCH_ACTIVE_LOW; }

void Hardware::SetLiftHeight(double mm)
{
    using namespace constant;
    // A continuous servo cannot hold a height: never send it 'angles'.
    if (LIFT_CONTINUOUS) { liftServo.SetAngle(LIFT_STOP_CMD); return; }
    double h = Clamp(mm, LIFT_MIN, LIFT_MAX);
    // Limit switches win over any command: never go further into a pressed end.
    if (h > liftTarget && LiftAtTop())    h = liftTarget;
    if (h < liftTarget && LiftAtBottom()) h = liftTarget;
    double span = LIFT_MAX - LIFT_MIN;
    double t = span > 0 ? (h - LIFT_MIN) / span : 0.0;
    double ang = LIFT_ANGLE_AT_MIN + t * (LIFT_ANGLE_AT_MAX - LIFT_ANGLE_AT_MIN);
    if (INVERT_LIFT) ang = 180.0 - ang;
    liftServo.SetAngle(Clamp(ang, 0.0, 180.0));
    liftTarget = h;
}

double Hardware::GetLiftHeight() { return liftTarget; }

void Hardware::SetLift(double duty)
{
    // Teleop/old callers pass a duty; treat it as "move the target a bit".
    if (duty != 0.0) SetLiftHeight(liftTarget + duty * 4.0);
}

void Hardware::ZeroLift(double height_mm) { SetLiftHeight(height_mm); }

// ------------------------------------------------------------------ pusher
void Hardware::SetPusher(double angle_deg)
{
    pusherServo.SetAngle(Clamp(angle_deg, 0.0, 180.0));
}

// ----------------------------------------------------------------- sensors
double Hardware::GetYaw()   { return navX.GetYaw(); }
void   Hardware::ResetYaw() { navX.ZeroYaw(); }

void Hardware::ResetEncoders()
{
    encFL.Reset();
    encFR.Reset();
    encRL.Reset();
    encRR.Reset();
}

// ----------------------------------------------------------- control panel
bool Hardware::GetStartButton() { return constant::START_ENABLED && startButton.Get() != constant::START_ACTIVE_LOW; }   // true = pressed
// TRUE = NOT pressed (ok to move). Polarity and on/off come from Settings.
bool Hardware::GetStopButton()
{
    if (!constant::STOP_ENABLED) return true;
    bool pressed = stopButton.Get() != constant::STOP_ACTIVE_LOW;
    return !pressed;
}
bool Hardware::GetResetButton() { return resetButton && resetButton->Get() != constant::RESET_ACTIVE_LOW; }
int  Hardware::RawReset() { return resetButton ? (resetButton->Get() ? 1 : 0) : -1; }
bool Hardware::RawStart() { return startButton.Get(); }
bool Hardware::RawStop()  { return stopButton.Get(); }

void Hardware::SetRunningLED(bool on) { runningLED.Set(on != constant::LED_ACTIVE_LOW); }
void Hardware::SetStoppedLED(bool on) { stoppedLED.Set(on != constant::LED_ACTIVE_LOW); }

void Hardware::UpdateLEDs(bool estopLatched)
{
    bool stopPressed = !GetStopButton();          // active low
    bool red = stopPressed || estopLatched;
    SetStoppedLED(red);
    SetRunningLED(!red);
}

void Hardware::StopAll()
{
    StopWheels();
    // A CONTINUOUS servo keeps spinning until told 90 (= stop). An ANGLE
    // servo just holds its position, so it is left alone.
    if (constant::PUSHER_CONTINUOUS) pusherServo.SetAngle(90.0);
    if (constant::LIFT_CONTINUOUS)   liftServo.SetAngle(constant::LIFT_STOP_CMD);
}

// --------------------------------------------------------------- telemetry
void Hardware::Publish()
{
    frc::SmartDashboard::PutNumber("lift_mm",   GetLiftHeight());
    frc::SmartDashboard::PutNumber("yaw_deg",   GetYaw());
    frc::SmartDashboard::PutNumber("enc_fl",    GetFL());
    frc::SmartDashboard::PutNumber("enc_fr",    GetFR());
    frc::SmartDashboard::PutNumber("enc_rl",    GetRL());
    frc::SmartDashboard::PutNumber("enc_rr",    GetRR());
    frc::SmartDashboard::PutBoolean("start_btn", GetStartButton());
    frc::SmartDashboard::PutBoolean("stop_btn",  GetStopButton());
}

std::string Hardware::TitanInfoJson()
{
    auto esc = [](std::string v) {
        std::string o;
        for (char c : v) { if (c == '"' || c == '\\') o += '\\'; if (c >= 32) o += c; }
        return o;
    };
    std::string fw = esc(frontLeft.GetFirmwareVersion());
    std::string hw = esc(frontLeft.GetHardwareVersion());
    std::string sn = esc(frontLeft.GetSerialNumber());
    char buf[512];
    std::snprintf(buf, sizeof buf,
        "{\"can_id_expected\":%d,\"id\":%d,\"firmware\":\"%s\",\"hardware\":\"%s\","
        "\"serial\":\"%s\",\"temp_c\":%.1f,\"enc_raw\":[%.0f,%.0f,%.0f,%.0f],"
        "\"talking\":%s}",
        constant::TITAN_ID, frontLeft.GetID(), fw.c_str(), hw.c_str(), sn.c_str(),
        frontLeft.GetControllerTemp(), encFL.GetRaw(), encFR.GetRaw(), encRL.GetRaw(), encRR.GetRaw(),
        (sn.empty() || sn == "00000000-00000000-00000000") ? "false" : "true");
    return buf;
}

bool Hardware::LiftServoOk()   const { return liftServo.GetError().GetCode() == 0; }
bool Hardware::PusherServoOk() const { return pusherServo.GetError().GetCode() == 0; }

std::string Hardware::TestServo(int ch, double deg)
{
    deg = Clamp(deg, 0.0, 180.0);
    if (ch == constant::LIFT_SERVO_CHANNEL) {
        if (!LiftServoOk()) return "PWM " + std::to_string(ch) + " was rejected by the VMX at start-up (see log)";
        liftServo.SetAngle(deg); return "";
    }
    if (ch == constant::SERVO_CHANNEL) {
        if (!PusherServoOk()) return "PWM " + std::to_string(ch) + " was rejected by the VMX at start-up (see log)";
        pusherServo.SetAngle(deg); return "";
    }
    auto it = testServos.find(ch);
    if (it == testServos.end()) {
        auto sv = std::make_unique<studica::Servo>(ch);
        if (sv->GetError().GetCode() != 0)
            return "PWM " + std::to_string(ch) + " is not usable: " + sv->GetError().GetMessage() +
                   " (PWM 0-9 need the HiCurrDIO jumper; FlexDIO pin n = PWM 10+n)";
        it = testServos.emplace(ch, std::move(sv)).first;
    }
    it->second->SetAngle(deg);
    return "";
}

// ----------------------------------------------------------------- sensors
double Hardware::GetUltrasonicMM()
{
    if (!ultrasonic || !ultrasonic->IsRangeValid()) return -1.0;
    return ultrasonic->GetRangeMM();
}

// Sharp IR: same formula as Studica's mobile-robotics-foundations course code
// (distance in cm = 27.726 * V^-1.2045), returned in mm.
double Hardware::GetIRMM()
{
    if (!irSensor) return -1.0;
    double v = irSensor->GetAverageVoltage();
    if (v < 0.1) return -1.0;
    return 10.0 * 27.726 * std::pow(v, -1.2045);
}

double Hardware::GetCobraV(int ch)
{
    if (!cobra || ch < 0 || ch > 3) return -1.0;
    return cobra->GetVoltage(ch);
}

int Hardware::CobraLine()
{
    int m = 0;
    for (int i = 0; i < 4; ++i) if (GetCobraV(i) > constant::COBRA_LINE_V) m |= 1 << i;
    return m;
}

std::string Hardware::SensorsJson()
{
    char b[256];
    std::snprintf(b, sizeof b, "\"us_mm\":%.0f,\"ir_mm\":%.0f,\"cobra\":[%.2f,%.2f,%.2f,%.2f],\"line\":%d",
                  GetUltrasonicMM(), GetIRMM(), GetCobraV(0), GetCobraV(1), GetCobraV(2), GetCobraV(3), CobraLine());
    return b;
}
