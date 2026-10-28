/*****************************************************************************
 * One command name -> one robot action, and a list of them -> a routine.
 *
 * This is the layer the control panel talks to. Every button on the web page
 * sends a command NAME and a NUMBER; every autonomous routine is just a list
 * of the same pairs. There is exactly one dispatch table, so a button and a
 * saved sequence can never disagree about what "forward 500" means.
 *
 * That also means you can build and re-order a routine from the panel without
 * recompiling, which matters when you have 3 hours and one deploy cycle costs
 * two minutes.
 *****************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "Movement.h"
#include "Oms.h"
#include "Camera.h"

struct Step
{
    std::string cmd;
    double      arg = 0.0;
};

class Sequence
{
public:
    Sequence(Movement *mv, Oms *oms, Hardware *hw, Camera *cam = nullptr)
        : movement(mv), manipulator(oms), hardware(hw), camera(cam) {}

    // Run one command. Returns false if it failed, timed out, or was aborted.
    bool Execute(const std::string &cmd, double arg);

    // ---- the stored routine ----------------------------------------------
    void Clear();
    void Add(const std::string &cmd, double arg);
    bool RunAll();                       // stops at the first failure
    size_t Size() const { return steps.size(); }

    // Parse "forward:500,turn_by:90,pick" into steps. Deliberately a flat
    // text format: it survives a URL, a text file and a copy-paste, and needs
    // no JSON parser on the robot.
    bool Parse(const std::string &text);
    std::string Describe() const;

    // ---- state, for the panel --------------------------------------------
    int  CurrentStep() const { return current; }
    bool IsRunning()   const { return running; }
    std::string LastError() const { return lastError; }
    std::string LastColour() const { return lastColour; }

    void Abort();

    // Every command the panel may send. Kept next to the dispatch table so
    // the two cannot drift apart.
    static std::vector<std::string> KnownCommands();

private:
    Movement *movement;
    Oms      *manipulator;
    Hardware *hardware;
    Camera   *camera;      // may be nullptr if camera not fitted

    std::vector<Step> steps;
    int  current = -1;
    bool running = false;
    std::string lastError;
    std::string lastColour = "-";
};
