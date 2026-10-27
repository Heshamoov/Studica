#include "Movement.h"

#include <frc/smartdashboard/SmartDashboard.h>
#include <cmath>
#include <thread>
#include <chrono>

static void SleepSeconds(double s)
{
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(s * 1000)));
}

double Movement::Clamp(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

double Movement::WrapDeg(double a)
{
    while (a > 180.0)  a -= 360.0;
    while (a < -180.0) a += 360.0;
    return a;
}

bool Movement::ShouldStop()
{
    // The Studica course wires the stop button ACTIVE LOW: it reads true when
    // NOT pressed. Test your own polarity and flip this if needed.
    return abort || !hardware->GetStopButton();
}

// --------------------------------------------------------------- kinematics
void Movement::InverseKinematics(double vx, double vy, double omega,
                                 double &fl, double &fr, double &rl, double &rr)
{
    const double k = constant::K_LEVER;
    const double w = omega * M_PI / 180.0;      // deg/s -> rad/s
    if (constant::DRIVE_TANK) {                  // wheels straight: no sideways
        fl = rl = vx - k * w;
        fr = rr = vx + k * w;
        return;
    }
    fl = vx - vy - k * w;
    fr = vx + vy + k * w;
    rl = vx + vy - k * w;
    rr = vx - vy + k * w;
}

void Movement::ForwardKinematics(double dfl, double dfr, double drl, double drr,
                                 double &dx, double &dy, double &dth)
{
    dx  = (dfl + dfr + drl + drr) / 4.0;
    dy  = constant::DRIVE_TANK ? 0.0 : (-dfl + dfr + drl - drr) / 4.0;
    double dth_rad = (-dfl + dfr - drl + drr) / (4.0 * constant::K_LEVER);
    dth = dth_rad * 180.0 / M_PI;
}

void Movement::Normalise(double &fl, double &fr, double &rl, double &rr)
{
    // Scale, do not clip. Clipping each wheel independently bends the
    // direction of travel - worst exactly when you ask for full speed.
    double m = std::fmax(std::fmax(std::fabs(fl), std::fabs(fr)),
                         std::fmax(std::fabs(rl), std::fabs(rr)));
    if (m > 1.0) { fl /= m; fr /= m; rl /= m; rr /= m; }
}

void Movement::Drive(double vx, double vy, double omega)
{
    double fl, fr, rl, rr;
    InverseKinematics(vx, vy, omega, fl, fr, rl, rr);
    fl /= constant::MAX_SPEED;  fr /= constant::MAX_SPEED;
    rl /= constant::MAX_SPEED;  rr /= constant::MAX_SPEED;
    Normalise(fl, fr, rl, rr);
    hardware->SetWheels(fl, fr, rl, rr);
}

void Movement::Stop()
{
    hardware->StopWheels();
}

// ----------------------------------------------------------------- odometry
void Movement::UpdateOdometry()
{
    double fl = hardware->GetFL(), fr = hardware->GetFR();
    double rl = hardware->GetRL(), rr = hardware->GetRR();

    if (!haveLast) {
        lastFL = fl; lastFR = fr; lastRL = rl; lastRR = rr;
        haveLast = true;
        theta_deg = WrapDeg(-hardware->GetYaw() - yawOffset);
        return;
    }

    double dfl = fl - lastFL, dfr = fr - lastFR;
    double drl = rl - lastRL, drr = rr - lastRR;
    lastFL = fl; lastFR = fr; lastRL = rl; lastRR = rr;

    double dx, dy, dth;
    ForwardKinematics(dfl, dfr, drl, drr, dx, dy, dth);

    // Heading comes from the IMU, never from the wheels. Mecanum wheels slip
    // far more in rotation than translation, and because heading rotates the
    // translation into the field frame, a heading error corrupts x and y too.
    double newTheta = WrapDeg(-hardware->GetYaw() - yawOffset);

    // Integrate at the midpoint heading: markedly more accurate over a
    // curved step, for the same cost.
    double mid = theta_deg + WrapDeg(newTheta - theta_deg) / 2.0;
    double c = std::cos(mid * M_PI / 180.0);
    double s = std::sin(mid * M_PI / 180.0);

    x_mm += dx * c - dy * s;
    y_mm += dx * s + dy * c;
    theta_deg = newTheta;
}

void Movement::SetPosition(double x, double y, double theta)
{
    // Tell the robot where it is. Use after lifting it back onto the court -
    // pose-to-pose driving then converges from anywhere.
    x_mm = x; y_mm = y; theta_deg = theta;
    yawOffset = -hardware->GetYaw() - theta;
    haveLast = false;
}

// -------------------------------------------------------------------- verbs
bool Movement::DriveTo(double tx, double ty, double ttheta, double speed)
{
    const double dt = constant::CONTROL_DT;
    double elapsed = 0.0;
    int settled = 0;

    while (elapsed < constant::MOVE_TIMEOUT) {
        if (ShouldStop()) { Stop(); return false; }
        UpdateOdometry();

        double ex = tx - x_mm, ey = ty - y_mm;
        double dist = std::hypot(ex, ey);
        double eth = WrapDeg(ttheta - theta_deg);

        if (dist <= constant::POS_TOLERANCE &&
            std::fabs(eth) <= constant::ANG_TOLERANCE) {
            Stop();
            if (++settled >= 3) return true;
        } else {
            settled = 0;

            double v = constant::KP_LINEAR * dist;
            if (dist < constant::SLOW_RADIUS)
                v = std::fmin(v, speed * std::fmax(dist / constant::SLOW_RADIUS, 0.15));
            v = std::fmin(v, speed);

            double ux = dist > 1e-6 ? ex / dist : 0.0;
            double uy = dist > 1e-6 ? ey / dist : 0.0;

            // field-frame error -> body frame. This is the whole trick.
            double c = std::cos(-theta_deg * M_PI / 180.0);
            double s = std::sin(-theta_deg * M_PI / 180.0);
            double vx = v * (ux * c - uy * s);
            double vy = v * (ux * s + uy * c);
            double om = Clamp(constant::KP_ANGULAR * eth,
                              -constant::TURN_FAST, constant::TURN_FAST);

            Drive(vx, vy, om);
        }

        SleepSeconds(dt);
        elapsed += dt;
    }
    Stop();
    return false;                    // timed out - the caller decides what next
}

bool Movement::Forward(double mm, double speed)
{
    UpdateOdometry();
    double c = std::cos(theta_deg * M_PI / 180.0);
    double s = std::sin(theta_deg * M_PI / 180.0);
    return DriveTo(x_mm + mm * c, y_mm + mm * s, theta_deg, speed);
}

bool Movement::Backward(double mm, double speed)
{
    return Forward(-std::fabs(mm), speed);
}

bool Movement::StrafeLeft(double mm, double speed)
{
    UpdateOdometry();
    double c = std::cos(theta_deg * M_PI / 180.0);
    double s = std::sin(theta_deg * M_PI / 180.0);
    // +y in the body frame is left
    return DriveTo(x_mm - mm * s, y_mm + mm * c, theta_deg, speed);
}

bool Movement::StrafeRight(double mm, double speed)
{
    return StrafeLeft(-std::fabs(mm), speed);
}

bool Movement::TurnTo(double deg, double rate)
{
    const double dt = constant::CONTROL_DT;
    double elapsed = 0.0;
    int settled = 0;

    while (elapsed < constant::MOVE_TIMEOUT) {
        if (ShouldStop()) { Stop(); return false; }
        UpdateOdometry();

        double e = WrapDeg(deg - theta_deg);
        if (std::fabs(e) <= constant::ANG_TOLERANCE) {
            Stop();
            if (++settled >= 3) return true;
        } else {
            settled = 0;
            double om = Clamp(constant::KP_ANGULAR * e, -rate, rate);
            // break static friction near the target instead of creeping
            if (std::fabs(om) < 15.0) om = std::copysign(15.0, om);
            Drive(0.0, 0.0, om);
        }
        SleepSeconds(dt);
        elapsed += dt;
    }
    Stop();
    return false;
}

bool Movement::TurnBy(double deg, double rate)
{
    UpdateOdometry();
    return TurnTo(WrapDeg(theta_deg + deg), rate);
}

void Movement::Publish()
{
    frc::SmartDashboard::PutNumber("robot_x",  x_mm);
    frc::SmartDashboard::PutNumber("robot_y",  y_mm);
    frc::SmartDashboard::PutNumber("robot_th", theta_deg);
}

// --------------------------------------------------------------- sensor moves
// Forward/backward until the front sensor reads `mm` (+-SENSOR_TOL).
bool Movement::DriveToDistance(double mm, bool useIR)
{
    const double dt = constant::CONTROL_DT;
    for (double t = 0; t < constant::MOVE_TIMEOUT; t += dt) {
        if (ShouldStop()) { Stop(); return false; }
        double d = useIR ? hardware->GetIRMM() : hardware->GetUltrasonicMM();
        if (d < 0) { Stop(); return false; }               // sensor off / no reading
        double err = d - mm;                               // >0: too far, drive forward
        if (std::fabs(err) <= constant::SENSOR_TOL) { Stop(); return true; }
        double v = Clamp(err * 1.5, -constant::SENSOR_SPEED, constant::SENSOR_SPEED);
        if (std::fabs(v) < 40.0) v = std::copysign(40.0, v);
        Drive(v, 0.0, 0.0);
        UpdateOdometry();
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
    }
    Stop();
    return false;
}

bool Movement::DriveToLine(double maxMM)
{
    const double dt = constant::CONTROL_DT;
    double startX = x_mm, startY = y_mm;
    for (double t = 0; t < constant::MOVE_TIMEOUT; t += dt) {
        if (ShouldStop()) { Stop(); return false; }
        if (hardware->CobraLine() != 0) { Stop(); return true; }
        if (std::hypot(x_mm - startX, y_mm - startY) > maxMM) { Stop(); return false; }
        Drive(constant::SENSOR_SPEED, 0.0, 0.0);
        UpdateOdometry();
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
    }
    Stop();
    return false;
}

// Simple line follow: the four Cobra channels (0 = left ... 3 = right) give a
// position; steer toward it while driving forward `mm`.
bool Movement::FollowLine(double mm)
{
    const double dt = constant::CONTROL_DT;
    double startX = x_mm, startY = y_mm;
    const double w[4] = {-1.5, -0.5, 0.5, 1.5};
    for (double t = 0; t < constant::MOVE_TIMEOUT * 2; t += dt) {
        if (ShouldStop()) { Stop(); return false; }
        if (std::hypot(x_mm - startX, y_mm - startY) >= mm) { Stop(); return true; }
        int m = hardware->CobraLine();
        if (m == 0) { Stop(); return false; }               // lost the line
        double pos = 0, n = 0;
        for (int i = 0; i < 4; ++i) if (m & (1 << i)) { pos += w[i]; n += 1; }
        pos /= n;                                            // -1.5 (left) .. +1.5 (right)
        Drive(constant::SENSOR_SPEED, 0.0, -pos * 25.0);     // line to the right -> turn right
        UpdateOdometry();
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
    }
    Stop();
    return false;
}
