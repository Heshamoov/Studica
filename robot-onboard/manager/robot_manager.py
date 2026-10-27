#!/usr/bin/env python3
"""
Robot manager - runs on the VMX, always on (systemd service "robot-manager").

It lets the student web page (BEGINNER-GUIDE.html) do everything that used to
need a terminal: set the clock, compile, start, stop and restart the robot
program, show the logs, and receive a new copy of the program code.

HTTP on port 5801. Every reply allows any origin, because the page is opened
straight from the laptop's disk (file://).

    GET  /status                 what is going on (JSON)
    GET  /time?epoch=<ms>        set the robot clock from the laptop
    GET  /start  /stop  /restart the robot program
    GET  /build?full=1&start=1   compile (full = rebuild everything), then start
    GET  /log?name=build|run&lines=200   last lines of a log (text)
    GET  /upload_begin           start sending new code
    POST /upload?path=src/X.cpp  one file (raw body)
    GET  /upload_commit          put the new code in place, compile, start

Only the Python standard library is used - nothing to install.
"""

import json
import os
import shutil
import signal
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

PORT = 5801
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))   # robot-onboard/
LOGS = os.path.join(ROOT, "logs")
BUILD_LOG = os.path.join(LOGS, "build.log")
RUN_LOG = os.path.join(LOGS, "terminal.log")
STAGE = os.path.join(ROOT, ".upload")
ALLOWED = ("src/", "include/", "manager/", "Makefile", "run.sh", "VERSION")

lock = threading.RLock()
state = {
    "proc": None,          # Popen of run.sh
    "building": False,
    "build_ok": None,      # None = never built by the manager
    "build_started": 0.0,
    "build_secs": 0.0,
    "message": "",
    "started_at": 0.0,
    "upload_files": 0,
}


def log_line(msg):
    os.makedirs(LOGS, exist_ok=True)
    with open(os.path.join(LOGS, "manager.log"), "a") as f:
        f.write(time.strftime("%Y-%m-%d %H:%M:%S ") + msg + "\n")


def has_binary():
    return os.path.isfile(os.path.join(ROOT, "robot"))


def robot_running():
    """True if a robot program is running (started by us or by hand)."""
    return subprocess.call(["pgrep", "-x", "robot"], stdout=subprocess.DEVNULL) == 0


# ------------------------------------------------------------- start / stop
def start_program():
    with lock:
        if state["building"]:
            return False, "still compiling - it starts by itself when done"
        if not has_binary():
            return False, "the program is not compiled yet - press Build"
        stop_program(quiet=True)
        os.makedirs(LOGS, exist_ok=True)
        run = os.path.join(ROOT, "run.sh")
        os.chmod(run, 0o755)
        state["proc"] = subprocess.Popen(
            ["/bin/bash", run], cwd=ROOT,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            start_new_session=True)
        state["started_at"] = time.time()
        log_line("program started")
        return True, "starting"


def stop_program(quiet=False):
    with lock:
        p = state["proc"]
        if p and p.poll() is None:
            try:
                os.killpg(p.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        subprocess.call(["pkill", "-x", "robot"])
        for _ in range(30):
            if not robot_running():
                break
            time.sleep(0.1)
        subprocess.call(["pkill", "-9", "-x", "robot"])
        if p:
            try:
                os.killpg(p.pid, signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
        state["proc"] = None
        if not quiet:
            log_line("program stopped")
        return True, "stopped"


# -------------------------------------------------------------------- build
def build(full=False, then_start=True):
    with lock:
        if state["building"]:
            return False, "already compiling"
        state["building"] = True
        state["build_ok"] = None
        state["build_started"] = time.time()
        state["message"] = "compiling"
    threading.Thread(target=_build_thread, args=(full, then_start), daemon=True).start()
    return True, "compiling started"


def _build_thread(full, then_start):
    stop_program(quiet=True)
    os.makedirs(LOGS, exist_ok=True)
    cmd = ["make", "-j4"] + (["-B"] if full else [])
    with open(BUILD_LOG, "w") as f:
        f.write("$ " + " ".join(cmd) + "   (" + time.strftime("%H:%M:%S") + ")\n")
        f.flush()
        rc = subprocess.call(cmd, cwd=ROOT, stdout=f, stderr=subprocess.STDOUT)
        f.write("\n== finished with code %d ==\n" % rc)
    with lock:
        state["building"] = False
        state["build_ok"] = rc == 0
        state["build_secs"] = time.time() - state["build_started"]
        state["message"] = "compiled OK" if rc == 0 else "compile FAILED - see the build log"
    log_line("build %s (%.0f s)" % ("ok" if rc == 0 else "FAILED", state["build_secs"]))
    if rc == 0 and then_start:
        start_program()


# ------------------------------------------------------------------- upload
def upload_begin():
    shutil.rmtree(STAGE, ignore_errors=True)
    os.makedirs(STAGE)
    state["upload_files"] = 0
    return True, "ready"


def safe_path(rel):
    rel = rel.replace("\\", "/").lstrip("/")
    if ".." in rel.split("/") or not rel.startswith(ALLOWED):
        return None
    return rel


def upload_file(rel, data):
    rel = safe_path(rel)
    if rel is None:
        return False, "path not allowed"
    dst = os.path.join(STAGE, rel)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "wb") as f:
        f.write(data)
    state["upload_files"] += 1
    return True, "ok"


def upload_commit():
    if not os.path.isdir(STAGE) or state["upload_files"] == 0:
        return False, "nothing was sent"
    n = 0
    for dirpath, _, files in os.walk(STAGE):
        for name in files:
            src = os.path.join(dirpath, name)
            rel = os.path.relpath(src, STAGE)
            dst = os.path.join(ROOT, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(src, dst)
            os.utime(dst, None)          # "now", so make sees it as changed
            n += 1
    shutil.rmtree(STAGE, ignore_errors=True)
    log_line("new code received: %d files" % n)
    ok, msg = build(full=False, then_start=True)
    return ok, "%d files copied, %s" % (n, msg)


# --------------------------------------------------------------------- misc
def set_clock(epoch_ms):
    want = epoch_ms / 1000.0
    if abs(time.time() - want) < 30:
        return True, "clock already right"
    rc = subprocess.call(["date", "-s", "@%d" % int(want)], stdout=subprocess.DEVNULL)
    log_line("clock set from laptop (rc %d)" % rc)
    return rc == 0, "clock set" if rc == 0 else "could not set the clock"


def tail(path, lines):
    try:
        with open(path, "rb") as f:
            f.seek(0, 2)
            size = f.tell()
            f.seek(max(0, size - 200 * 1024))
            data = f.read().decode("utf-8", "replace")
        return "\n".join(data.splitlines()[-lines:])
    except FileNotFoundError:
        return "(no log yet)"


def version():
    try:
        with open(os.path.join(ROOT, "VERSION")) as f:
            return f.read().strip()
    except OSError:
        return ""


def robot_name():
    """Team name = the robot's Wi-Fi name (hostapd ssid), else the hostname."""
    for path in ("/etc/hostapd/hostapd.conf", "/etc/hostapd.conf"):
        try:
            with open(path) as f:
                for line in f:
                    if line.strip().startswith("ssid="):
                        return line.strip()[5:].strip()
        except OSError:
            pass
    try:
        import socket
        return socket.gethostname()
    except Exception:
        return ""


def status():
    with lock:
        building = state["building"]
        b_elapsed = time.time() - state["build_started"] if building else state["build_secs"]
        return {
            "manager": True,
            "version": version(),
            "name": robot_name(),
            "running": robot_running(),
            "building": building,
            "build_ok": state["build_ok"],
            "build_seconds": round(b_elapsed),
            "has_binary": has_binary(),
            "message": state["message"],
            "robot_time": time.strftime("%Y-%m-%d %H:%M:%S"),
            "clock_ok": time.localtime().tm_year >= 2025,
            "run_seconds": round(time.time() - state["started_at"]) if state["started_at"] else 0,
        }


# --------------------------------------------------------------------- http
class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _send(self, code, body, ctype="application/json"):
        if isinstance(body, (dict, list)):
            body = json.dumps(body)
        data = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype + "; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Private-Network", "true")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _reply(self, ok_msg):
        ok, msg = ok_msg
        self._send(200, {"ok": ok, "result": msg})

    def do_OPTIONS(self):
        self._send(204, "")

    def do_GET(self):
        u = urlparse(self.path)
        q = {k: v[0] for k, v in parse_qs(u.query).items()}
        r = u.path
        try:
            if r == "/status":
                return self._send(200, status())
            if r == "/time":
                return self._reply(set_clock(float(q.get("epoch", "0"))))
            if r == "/start":
                return self._reply(start_program())
            if r == "/stop":
                return self._reply(stop_program())
            if r == "/restart":
                stop_program(quiet=True)
                return self._reply(start_program())
            if r == "/build":
                return self._reply(build(q.get("full") == "1", q.get("start", "1") == "1"))
            if r == "/log":
                path = BUILD_LOG if q.get("name") == "build" else RUN_LOG
                return self._send(200, tail(path, int(q.get("lines", "200"))), "text/plain")
            if r == "/upload_begin":
                return self._reply(upload_begin())
            if r == "/upload_commit":
                return self._reply(upload_commit())
            self._send(404, {"ok": False, "result": "unknown route"})
        except Exception as e:      # never let the manager die on a bad request
            self._send(500, {"ok": False, "result": str(e)})

    def do_POST(self):
        u = urlparse(self.path)
        q = {k: v[0] for k, v in parse_qs(u.query).items()}
        try:
            n = int(self.headers.get("Content-Length", "0"))
            data = self.rfile.read(n) if n else b""
            if u.path == "/upload":
                return self._reply(upload_file(q.get("path", ""), data))
            self._send(404, {"ok": False, "result": "unknown route"})
        except Exception as e:
            self._send(500, {"ok": False, "result": str(e)})


def main():
    os.makedirs(LOGS, exist_ok=True)
    log_line("manager starting in " + ROOT)
    # At power-up: start the program straight away (compile first if needed),
    # so the robot is ready without anyone typing anything.
    if not robot_running():
        if has_binary():
            start_program()
        else:
            build(full=False, then_start=True)
    ThreadingHTTPServer.allow_reuse_address = True
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()


if __name__ == "__main__":
    main()
