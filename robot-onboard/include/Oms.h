/*****************************************************************************
 * OMS - Object Manipulation System: LIFT (servo) + FORK + PUSHER (servo).
 * No extension axis: the chassis drives forward/backward for reach.
 *
 * The delivery pockets are CLOSED, so the piece cannot be lowered in and
 * the fork pulled out. Instead it is held above the pocket and the pusher
 * slides it off the fork tip so it drops in.
 *
 * THE PICK (storage base, front slot 30-60 mm):
 *   0  pusher PARKED (arm up and back, above the 100 mm base)
 *   1  lift to  32    just inside the bottom of the slot
 *   2  drive forward  fork slides under the piece
 *   3  lift to  40    piece lifts off
 *   4  lift to  65    clear of the slot - only now is it safe to withdraw
 *   5  drive backward
 *   6  lift to carry height (40 mm)
 *
 * THE PLACE (delivery stand, closed pocket):
 *   1  lift so the fork is above the stand's top wall (height + gap)
 *   2  drive forward until the fork tip is over the pocket
 *   3  push: arm comes down behind the piece, sweeps SLOWLY forward,
 *      the piece slides off the tip and drops in
 *   4  arm back to park
 *   5  drive backward
 *   6  carry height
 *
 * The lift servo has no encoder: height is open loop (angle calibrated from
 * two points on the Settings page).
 *****************************************************************************/

#pragma once

#include "Hardware.h"
#include "Movement.h"
#include "Constants.h"

class Oms
{
public:
    Oms(Hardware *hw, Movement *mv) : hardware(hw), movement(mv) {}

    // ---- axis verbs (block until done or timed out) ----------------------
    bool LiftTo(double mm);
    // continuous-lift helpers (limit switches + timing)
    bool LiftHomeSwitch();          // down to the bottom switch, height = LIFT_MIN
    bool LiftCalibrate();           // time both directions, save the speeds (needs both switches)
    bool LiftRunTimed(bool up, double seconds);   // for the ruler calibration
    bool liftHomed = false;
    bool LiftBy(double mm);
    void Stop();

    // ---- named positions --------------------------------------------------
    bool LiftHome()      { return LiftTo(constant::LIFT_HOME); }
    bool LiftCarry()     { return LiftTo(constant::LIFT_CARRY); }
    bool LiftStand(int n);                       // 1..4 -> 25/50/75/100 mm

    // ---- pusher ---------------------------------------------------------
    void Push();         // down behind the piece, slow sweep off the tip, back to park
    void Retract();      // park: arm up and back, clear of storage base and stands

    // ---- compound: the two that actually score ---------------------------
    bool PickFromStorage();      // the 7-step sequence above
    bool PlaceOnStand(int n);    // the 6-step sequence above

    // ---- open loop, for teleop -------------------------------------------
    void JogLift(double duty);

    bool abort = false;

private:
    Hardware *hardware;
    Movement *movement;
    bool ShouldStop();
    bool RunLiftUntil(bool up, double seconds, double &ran);   // returns true if a switch was hit
    void SweepPusher(double target, double degPerSec);
    double pusherAngle = constant::PARK_DEG;   // last commanded pusher angle
    static double Clamp(double v, double lo, double hi);
    static double StandHeight(int n);
    static double StandPocketDepth(int n);
};
