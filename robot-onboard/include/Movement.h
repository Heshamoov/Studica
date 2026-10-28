/*****************************************************************************
 * Mecanum kinematics and the drive verbs.
 *
 * FRAME: +x forward, +y LEFT, +theta counter-clockwise. Millimetres, degrees.
 *
 * Every move has a TIMEOUT. A robot waiting forever on a wheel that has
 * jammed is a robot scoring zero, and the Studica course project this is
 * modelled on has no timeouts at all - that is the one thing worth NOT
 * copying from it.
 *
 * Every move also honours the stop button, checked inside the loop.
 *****************************************************************************/

#pragma once

#include "Hardware.h"
#include "Constants.h"

class Movement
{
public:
    explicit Movement(Hardware *hw) : hardware(hw) {}

    // ---- the verbs (each blocks until done or timed out) -----------------
    bool Forward(double mm,  double speed = constant::SPEED_CRUISE);
    bool Backward(double mm, double speed = constant::SPEED_CRUISE);
    bool StrafeLeft(double mm,  double speed = constant::SPEED_CRUISE);
    bool StrafeRight(double mm, double speed = constant::SPEED_CRUISE);
    bool TurnBy(double deg, double rate = constant::TURN_FAST);
    bool TurnTo(double deg, double rate = constant::TURN_FAST);
    bool DriveTo(double x, double y, double theta,
                 double speed = constant::SPEED_CRUISE);
    void Stop();

    // ---- sensor moves (block until done, timed out, or aborted) ----------
    bool DriveToDistance(double mm, bool useIR);   // front sensor reads mm
    bool DriveToLine(double maxMM);                // forward until the Cobra sees a line
    bool FollowLine(double mm);                    // follow a line for mm

    // ---- open loop, for teleop -------------------------------------------
    void Drive(double vx, double vy, double omega);   // mm/s, mm/s, deg/s

    // ---- odometry ---------------------------------------------------------
    void   UpdateOdometry();
    void   SetPosition(double x, double y, double theta);
    double GetX()     { return x_mm; }
    double GetY()     { return y_mm; }
    double GetTheta() { return theta_deg; }

    void Publish();

    // Set by the command server so a run can be aborted from the panel.
    bool abort = false;

private:
    Hardware *hardware;

    double x_mm = 0.0, y_mm = 0.0, theta_deg = 0.0;
    double lastFL = 0.0, lastFR = 0.0, lastRL = 0.0, lastRR = 0.0;
    bool   haveLast = false;
    double yawOffset = 0.0;

    void InverseKinematics(double vx, double vy, double omega,
                           double &fl, double &fr, double &rl, double &rr);
    void ForwardKinematics(double dfl, double dfr, double drl, double drr,
                           double &dx, double &dy, double &dth);

    bool ShouldStop();                 // stop button, or an abort from the UI
    static double WrapDeg(double a);
    static double Clamp(double v, double lo, double hi);
    void Normalise(double &fl, double &fr, double &rl, double &rr);
};
