#!/bin/bash
# One-time install of the robot manager on a VMX (the teacher runs
# SETUP-ROBOT.bat on the laptop, which runs this). Safe to run again.
#
# After this, the robot program starts by itself at every power-up, and the
# student page can set the clock, compile, start, stop and restart it.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [ "$(id -u)" -ne 0 ]; then exec sudo bash "$0" "$@"; fi

if ! command -v python3 >/dev/null; then
    echo "ERROR: python3 is not on this robot. Tell your teacher."; exit 1
fi

# Laptop time passed in by SETUP-ROBOT.bat (the robot has no battery clock).
if [ -n "$1" ]; then date -s "$1" >/dev/null && echo "clock set to $(date)"; fi

chmod +x "$ROOT/run.sh" "$ROOT/manager/robot_manager.py"

# Throw away old compiled files: the manager compiles fresh, so nothing old
# (built with a wrong clock) can survive.
rm -rf "$ROOT/build" "$ROOT/robot"

# Stop anything started by hand, so only the manager owns the program.
pkill -x robot 2>/dev/null || true

cat > /etc/systemd/system/robot-manager.service <<EOF
[Unit]
Description=SC2026 robot manager (port 5801) - starts the robot program
After=network.target

[Service]
ExecStart=/usr/bin/python3 $ROOT/manager/robot_manager.py
WorkingDirectory=$ROOT
Restart=always
RestartSec=2
User=root

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable robot-manager >/dev/null 2>&1
systemctl restart robot-manager
sleep 2
if systemctl is-active --quiet robot-manager; then
    echo
    echo "======================================================"
    echo "  DONE. The robot helper is installed and running."
    echo "  The robot program now compiles (first time: a few"
    echo "  minutes) and then starts by itself."
    echo "  Students only need BEGINNER-GUIDE.html from now on."
    echo "======================================================"
else
    echo "ERROR: the helper did not start:"
    systemctl status robot-manager --no-pager | tail -20
    exit 1
fi
