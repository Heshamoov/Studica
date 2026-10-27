/*****************************************************************************
 * SC2026 Skill 23 - colour detection for C1 (simple tasks) and E1 (autonomous).
 *
 * Reads a USB camera, classifies each frame's centre region into one of the
 * five piece colours (red, green, yellow, blue, and the surprise hazard) and
 * pushes the result to Shuffleboard.
 *
 * The sensor loop runs on its own thread so it never stalls the robot loop.
 * Call Start() once in RobotInit(), then Publish() every robot period.
 *****************************************************************************/

#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>

class Camera
{
public:
    Camera();
    ~Camera();

    void Start();
    void Publish();     // call from RobotPeriodic()

    // Last detected colour as a string: "red","green","yellow","blue",
    // "hazard", or "none". Thread-safe.
    std::string GetColour() const;

private:
    void Loop();

    std::thread       thread_;
    std::atomic<bool> running_{false};

    mutable std::mutex mutex_;
    std::string colour_{"none"};   // protected by mutex_
    double      confidence_{0.0};

    // Averaged HSV of the ROI in the last frame.
    double hue_{0.0}, sat_{0.0}, val_{0.0};
};
