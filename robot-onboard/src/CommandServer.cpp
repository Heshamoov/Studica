#include "CommandServer.h"
#include <cstdio>
#include "Settings.h"
#include "Log.h"
#include <cstdlib>
#include <frc/DriverStation.h>
#include <frc/Timer.h>
#include <thread>
#include <chrono>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <sstream>
#include <iomanip>

CommandServer::~CommandServer()
{
    Stop();
}

void CommandServer::Start(int port)
{
    quit = false;
    serverThread = std::thread(&CommandServer::ServerLoop, this, port);
    workerThread = std::thread(&CommandServer::WorkerLoop, this);
}

void CommandServer::Stop()
{
    quit = true;
    jobCv.notify_all();
    if (listenFd >= 0) { ::shutdown(listenFd, SHUT_RDWR); ::close(listenFd); listenFd = -1; }
    if (serverThread.joinable()) serverThread.join();
    if (workerThread.joinable()) workerThread.join();
}

// ------------------------------------------------------------------ worker
void CommandServer::WorkerLoop()
{
    while (!quit) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(jobMutex);
            jobCv.wait(lock, [&]{ return quit || !jobs.empty(); });
            if (quit) return;
            job = jobs.front();
            jobs.pop();
        }

        busy = true;
        // A new command is a fresh start: clear any old abort (from start-up,
        // an earlier E-stop, or a stopped routine), otherwise every move
        // would refuse to start. RunAll clears it itself.
        if (!job.runAll) { movement->abort = false; manipulator->abort = false; }
        bool ok = job.runAll ? sequence->RunAll()
                             : sequence->Execute(job.cmd, job.arg);
        std::string why = sequence->LastError();
        if (!ok && why.empty()) {
            if (!hardware->GetStopButton())               why = "STOP button is pressed (check the stop-button pin setting)";
            else if (movement->abort || manipulator->abort) why = "aborted (E-stop or STOP)";
            else                                           why = "did not finish in time (timeout) - is the robot moving? check encoders";
        }
        lastResult = ok ? "ok" : ("FAILED: " + why);
        robotlog::Log(std::string(job.runAll ? "routine" : job.cmd) + " finished: " + lastResult);
        busy = false;
    }
}

// ------------------------------------------------------------------ server
void CommandServer::ServerLoop(int port)
{
    listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) { std::perror("panel server: socket() failed"); return; }

    int one = 1;
    ::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::perror("panel server: cannot bind port (is another copy of the robot program still running?)"); return; }
    if (::listen(listenFd, 4) < 0) { std::perror("panel server: listen() failed"); return; }
    std::printf("panel server: LISTENING on port %d - the browser can connect now\n", port);
    std::fflush(stdout);

    while (!quit) {
        int fd = ::accept(listenFd, nullptr, nullptr);
        if (fd < 0) { if (quit) break; continue; }

        char buf[2048];
        ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            std::string req(buf);

            // "GET /path?query HTTP/1.1"
            std::string path;
            auto sp1 = req.find(' ');
            auto sp2 = req.find(' ', sp1 + 1);
            if (sp1 != std::string::npos && sp2 != std::string::npos)
                path = req.substr(sp1 + 1, sp2 - sp1 - 1);

            std::string body = Handle(path);

            std::ostringstream resp;
            resp << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: application/json\r\n"
                 // the panel is opened from file://, whose origin is "null",
                 // so it needs a permissive CORS header to read the reply
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Cache-Control: no-store\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
            std::string out = resp.str();
            ::send(fd, out.data(), out.size(), 0);
        }
        ::close(fd);
    }
}

// ----------------------------------------------------------------- routing
std::string CommandServer::Handle(const std::string &path)
{
    std::string route = path, query;
    auto q = path.find('?');
    if (q != std::string::npos) { route = path.substr(0, q); query = path.substr(q + 1); }

    // --- answered immediately, even mid-move -----------------------------
    if (route == "/state")  return StateJson();

    if (route == "/abort" || route == "/estop") {
        estopLatched = true;
        robotlog::Log("E-STOP from the browser: everything stopped");
        sequence->Abort();
        hardware->StopAll();
        {   // drop anything queued, or it runs the moment the abort lands
            std::lock_guard<std::mutex> lock(jobMutex);
            std::queue<Job> empty;
            jobs.swap(empty);
        }
        lastResult = "aborted";
        return "{\"ok\":true,\"result\":\"aborted\"}";
    }

    if (route == "/commands") {
        std::ostringstream o;
        o << "{\"commands\":[";
        auto list = Sequence::KnownCommands();
        for (size_t i = 0; i < list.size(); ++i) {
            if (i) o << ",";
            o << "\"" << list[i] << "\"";
        }
        o << "]}";
        return o.str();
    }

    // --- settings: answered immediately ----------------------------------
    if (route == "/settings") return settings::Json();
    if (route == "/titan")    return hardware->TitanInfoJson();
    if (route == "/servotest") {
        // pin = number printed on the VMX (0-21); ch = raw PWM (old pages)
        int pin = std::atoi(QueryValue(query, "pin", "-1").c_str());
        int ch = pin >= 0 ? (pin < 12 ? pin + 10 : pin - 12) : std::atoi(QueryValue(query, "ch", "-1").c_str());
        double deg = std::atof(QueryValue(query, "deg", "90").c_str());
        std::string err = hardware->TestServo(ch, deg);
        robotlog::Log("servo finder: pin " + std::to_string(pin) + " (PWM " + std::to_string(ch) + ")" + " -> " + std::to_string((int)deg) + " deg " + (err.empty() ? "sent" : "FAILED: " + err));
        return std::string("{\"ok\":") + (err.empty() ? "true" : "false") + ",\"result\":\"" + (err.empty() ? "sent" : err) + "\"}";
    }
    if (route == "/log")      return robotlog::TailJson(std::atoi(QueryValue(query, "n", "200").c_str()));

    // --- browser teleop: answered immediately ---------------------------
    if (route == "/drive") {
        if (busy.load()) return "{\"ok\":false,\"result\":\"busy - a command is running\"}";
        auto num = [&](const char *k) {
            try { double v = std::stod(QueryValue(query, k, "0"));
                  return v < -1 ? -1.0 : (v > 1 ? 1.0 : v); } catch (...) { return 0.0; } };
        std::lock_guard<std::mutex> lock(driveMutex);
        dVx = num("vx"); dVy = num("vy"); dW = num("w");
        dTime = frc::Timer::GetFPGATimestamp();
        return "{\"ok\":true}";
    }

    if (route == "/enable") {
        enableRequest = QueryValue(query, "on", "1") == "0" ? 0 : 1;
        robotlog::Log(enableRequest == 1 ? "enable requested" : "disable requested");
        if (enableRequest == 1) estopLatched = false;
        return std::string("{\"ok\":true,\"result\":\"") +
               (enableRequest == 1 ? "enabling" : "disabling") + "\"}";
    }

    if (route == "/set") {
        std::string msg;
        bool ok = settings::Set(UrlDecode(QueryValue(query, "key")),
                                UrlDecode(QueryValue(query, "value")), msg);
        robotlog::Log("setting " + UrlDecode(QueryValue(query, "key")) + " = " + UrlDecode(QueryValue(query, "value")) + " -> " + msg);
        return std::string("{\"ok\":") + (ok ? "true" : "false") +
               ",\"result\":\"" + msg + "\",\"restart_pending\":" +
               (settings::RestartPending() ? "true" : "false") + "}";
    }

    if (route == "/reset") {
        std::string msg;
        bool ok = settings::Reset(UrlDecode(QueryValue(query, "key", "all")), msg);
        return std::string("{\"ok\":") + (ok ? "true" : "false") +
               ",\"result\":\"" + msg + "\",\"restart_pending\":" +
               (settings::RestartPending() ? "true" : "false") + "}";
    }

    if (route == "/restart") {
        robotlog::Log("restart requested from the browser");
        // Exit code 42 tells run.sh to start the program again. Stop the
        // motors first, and leave a moment for this reply to reach the browser.
        sequence->Abort();
        hardware->StopAll();
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::fflush(stdout);
            _exit(42);
        }).detach();
        return "{\"ok\":true,\"result\":\"restarting\"}";
    }

    // --- everything below queues work ------------------------------------
    if (busy.load())
        return "{\"ok\":false,\"result\":\"busy\"}";

    if (route == "/do") {
        estopLatched = false;
        std::string cmd = QueryValue(query, "cmd");
        double arg = 0.0;
        try { arg = std::stod(QueryValue(query, "arg", "0")); } catch (...) {}
        if (cmd.empty()) return "{\"ok\":false,\"result\":\"no cmd\"}";
        robotlog::Log("command: " + cmd + " " + QueryValue(query, "arg", "0"));
        {
            std::lock_guard<std::mutex> lock(jobMutex);
            jobs.push({cmd, arg, false});
        }
        jobCv.notify_one();
        return "{\"ok\":true,\"result\":\"queued\"}";
    }

    if (route == "/load") {
        std::string seq = UrlDecode(QueryValue(query, "seq"));
        if (!sequence->Parse(seq)) {
            return "{\"ok\":false,\"result\":\"" + sequence->LastError() + "\"}";
        }
        return "{\"ok\":true,\"result\":\"loaded " +
               std::to_string(sequence->Size()) + " steps\"}";
    }

    if (route == "/run") {
        robotlog::Log("run routine (" + std::to_string(sequence->Size()) + " steps)");
        estopLatched = false;
        if (sequence->Size() == 0)
            return "{\"ok\":false,\"result\":\"no sequence loaded\"}";
        {
            std::lock_guard<std::mutex> lock(jobMutex);
            jobs.push({"", 0.0, true});
        }
        jobCv.notify_one();
        return "{\"ok\":true,\"result\":\"running\"}";
    }

    return "{\"ok\":false,\"result\":\"unknown route\"}";
}

std::string CommandServer::StateJson()
{
    std::ostringstream o;
    o << std::fixed << std::setprecision(1);
    o << "{"
      << "\"busy\":"    << (busy.load() ? "true" : "false")
      << ",\"running\":" << (sequence->IsRunning() ? "true" : "false")
      << ",\"step\":"    << sequence->CurrentStep()
      << ",\"steps\":"   << sequence->Size()
      << ",\"x\":"       << movement->GetX()
      << ",\"y\":"       << movement->GetY()
      << ",\"theta\":"   << movement->GetTheta()
      << ",\"lift\":"    << hardware->GetLiftHeight()
      << ",\"yaw\":"     << hardware->GetYaw()
      << ",\"start_btn\":" << (hardware->GetStartButton() ? "true" : "false")
      << ",\"stop_btn\":"  << (hardware->GetStopButton()  ? "true" : "false")
      << ",\"enabled\":" << (frc::DriverStation::GetInstance().IsEnabled() ? "true" : "false")
      << ",\"enc\":[" << hardware->GetFL() << "," << hardware->GetFR() << "," << hardware->GetRL() << "," << hardware->GetRR() << "]"
      << ",\"servo_ok\":[" << (hardware->LiftServoOk() ? "true" : "false") << "," << (hardware->PusherServoOk() ? "true" : "false") << "]"
      << ",\"raw_start\":" << (hardware->RawStart() ? 1 : 0) << ",\"raw_stop\":" << (hardware->RawStop() ? 1 : 0) << ",\"raw_reset\":" << hardware->RawReset() << ",\"reset_btn\":" << (hardware->GetResetButton() ? "true" : "false")
      << "," << hardware->SensorsJson()
      << ",\"lift_top\":" << (hardware->LiftAtTop() ? "true" : "false")
      << ",\"lift_bottom\":" << (hardware->LiftAtBottom() ? "true" : "false")
      << ",\"estop\":" << (estopLatched.load() ? "true" : "false")
      << ",\"colour\":\"" << sequence->LastColour() << "\""
      << ",\"result\":\"" << lastResult << "\""
      << "}";
    return o.str();
}

// ------------------------------------------------------------------ helpers
std::string CommandServer::UrlDecode(const std::string &s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int v = 0;
            std::istringstream(s.substr(i + 1, 2)) >> std::hex >> v;
            out += static_cast<char>(v);
            i += 2;
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

std::string CommandServer::QueryValue(const std::string &query,
                                      const std::string &key,
                                      const std::string &fallback)
{
    std::stringstream ss(query);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        if (pair.substr(0, eq) == key) return pair.substr(eq + 1);
    }
    return fallback;
}

bool CommandServer::WebDrive(double &vx, double &vy, double &w)
{
    std::lock_guard<std::mutex> lock(driveMutex);
    if (dTime < 0 || frc::Timer::GetFPGATimestamp() - dTime > 0.4) return false;
    vx = dVx; vy = dVy; w = dW;
    return true;
}

void CommandServer::ResetFromButton()
{
    sequence->Abort();
    hardware->StopAll();
    {
        std::lock_guard<std::mutex> lock(jobMutex);
        std::queue<Job> empty;
        jobs.swap(empty);
    }
    estopLatched = false;
    lastResult = "reset";
}
