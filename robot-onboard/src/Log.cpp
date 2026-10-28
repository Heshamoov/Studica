#include "Log.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <deque>
#include <fstream>
#include <mutex>
#include <sys/stat.h>

namespace {
std::mutex mtx;
std::deque<std::string> lines;
std::ofstream file;

std::string Stamp()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    int ms = static_cast<int>(duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000);
    char buf[32];
    std::strftime(buf, sizeof buf, "%H:%M:%S", std::localtime(&t));
    char out[40];
    std::snprintf(out, sizeof out, "%s.%03d", buf, ms);
    return out;
}

std::string Esc(const std::string &s)
{
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (static_cast<unsigned char>(c) >= 32) o += c;
    }
    return o;
}
} // namespace

namespace robotlog
{

void Log(const std::string &text)
{
    std::string line = Stamp() + "  " + text;
    std::lock_guard<std::mutex> lock(mtx);
    lines.push_back(line);
    while (lines.size() > 400) lines.pop_front();
    if (!file.is_open()) {
        ::mkdir("logs", 0777);
        file.open("logs/events.log", std::ios::app);
        if (file.is_open()) file << "\n===== robot program started =====\n";
    }
    if (file.is_open()) { file << line << "\n"; file.flush(); }
    std::printf("[log] %s\n", line.c_str());
    std::fflush(stdout);
}

std::string TailJson(int maxLines)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::string o = "{\"lines\":[";
    int start = static_cast<int>(lines.size()) - maxLines;
    if (start < 0) start = 0;
    bool first = true;
    for (size_t i = start; i < lines.size(); ++i) {
        if (!first) o += ",";
        first = false;
        o += "\"" + Esc(lines[i]) + "\"";
    }
    o += "]}";
    return o;
}

} // namespace robotlog
