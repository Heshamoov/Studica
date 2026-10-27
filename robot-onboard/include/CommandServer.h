/*****************************************************************************
 * A tiny HTTP server so the browser control panel can drive the robot.
 *
 * WHY NOT NETWORKTABLES
 * NetworkTables is the WPILib way and Shuffleboard speaks it, but a browser
 * cannot without a client library - and we are not allowed to pull one from
 * a CDN (no internet on site). Plain HTTP over a socket needs nothing on
 * either side: the page uses fetch(), the robot uses ~150 lines of sockets.
 *
 * Port 5800 is inside the FRC team-use range, so nothing else will claim it.
 *
 * THREADS
 *   server thread   accepts connections, parses, replies
 *   worker thread   runs ONE action at a time, because actions block
 * /abort and /state are answered on the server thread, so they still work
 * while a move is running. That is the whole reason for the split - an abort
 * you cannot send during a move is not an abort.
 *****************************************************************************/

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "Sequence.h"

class CommandServer
{
public:
    CommandServer(Sequence *seq, Movement *mv, Oms *oms, Hardware *hw)
        : sequence(seq), movement(mv), manipulator(oms), hardware(hw) {}
    ~CommandServer();

    void Start(int port);
    void Stop();

    bool Busy() const { return busy.load(); }

    // Browser teleop: /drive?vx=&vy=&w= (each -1..1). Returns false when no
    // fresh command arrived in the last 0.4 s - a dropped Wi-Fi link must stop
    // the robot, not leave it driving.
    bool WebDrive(double &vx, double &vy, double &w);

    // Set by /enable?on=0|1; Robot applies it to the MockDS.
    std::atomic<int> enableRequest{-1};

    // Set by /abort or /estop (red LED on); cleared by the next command, run,
    // enable, or the physical start button.
    std::atomic<bool> estopLatched{false};

    // Physical RESET button: abort, drop queued jobs, clear the e-stop.
    void ResetFromButton();   // -1 none, 0 disable, 1 enable

private:
    Sequence *sequence;
    Movement *movement;
    Oms      *manipulator;
    Hardware *hardware;

    std::thread serverThread;
    std::thread workerThread;
    std::atomic<bool> quit{false};
    std::atomic<bool> busy{false};
    int listenFd = -1;

    struct Job { std::string cmd; double arg; bool runAll; };
    std::queue<Job> jobs;
    std::mutex jobMutex;
    std::condition_variable jobCv;

    std::string lastResult = "idle";

    std::mutex driveMutex;
    double dVx = 0, dVy = 0, dW = 0;
    double dTime = -1.0;

    void ServerLoop(int port);
    void WorkerLoop();

    std::string Handle(const std::string &path);
    std::string StateJson();

    static std::string UrlDecode(const std::string &s);
    static std::string QueryValue(const std::string &query,
                                  const std::string &key,
                                  const std::string &fallback = "");
};
