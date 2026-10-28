/*****************************************************************************
 * A short, readable event log (not the WPILib watchdog noise).
 *
 * Every line is kept in memory (last 400) for the web page (/log) and
 * appended to logs/events.log next to the program, so a mentor can read it
 * after the fact:   cat ~/robot-onboard/logs/events.log
 *****************************************************************************/

#pragma once

#include <string>

namespace robotlog
{
    void Log(const std::string &line);     // timestamped, thread-safe
    std::string TailJson(int maxLines);    // {"lines":[...]} for /log
}
