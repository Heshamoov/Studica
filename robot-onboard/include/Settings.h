/*****************************************************************************
 * Runtime settings: every number a team tunes, changeable from the browser.
 *
 *   /settings                 -> JSON list of every setting (value, default,
 *                                limits, unit, help, "live" or "restart")
 *   /set?key=K&value=V        -> change one, validate, save to settings.cfg
 *   /reset?key=K  (or all)    -> back to the built-in default
 *   /restart                  -> exit code 42; run.sh restarts the program
 *
 * "live"    settings take effect immediately.
 * "restart" settings (wiring, ports) are read once when the hardware is
 *           created, so they need a restart to apply.
 *
 * The file is plain KEY=value lines, so a mentor can also read or edit it.
 *****************************************************************************/

#pragma once

#include <string>

namespace settings
{
    // Register everything, load settings.cfg if present, recompute derived
    // values. Call once, first thing in RobotInit.
    void Init();

    std::string Json();

    // Returns false and fills msg on a bad key/value.
    bool Set(const std::string &key, const std::string &value, std::string &msg);
    bool Reset(const std::string &key, std::string &msg);   // key "all" = everything

    bool RestartPending();
}
