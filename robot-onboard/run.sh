#!/bin/bash
# Run the robot program with the correct libraries.
#
# The VMX-pi hardware layer (wpilib/libwpiHal.so, Studica's vmxpi build)
# talks to the real pins, CAN and PWM, which needs root - so this script
# re-starts itself with sudo (the same way Studica's own launcher does).
#
# If the program exits with code 42 (the "Restart robot program" button on
# the Settings page does this) it is started again automatically.
# Ctrl+C stops it for good.
cd "$(dirname "$0")"

if [ "$(id -u)" -ne 0 ]; then
    exec sudo "$0" "$@"
fi

# Only one program can own the VMX hardware (pigpio). Stop any copy that is
# still running from before, then clear a stale pigpio lock if one is left.
if pgrep -x robot >/dev/null; then
    echo "stopping the robot program that is already running..."
    pkill -x robot
    sleep 2
    pkill -9 -x robot 2>/dev/null
fi
if [ -f /var/run/pigpio.pid ] && ! pgrep -x robot >/dev/null && ! pgrep -x pigpiod >/dev/null; then
    rm -f /var/run/pigpio.pid
fi

export LD_LIBRARY_PATH="$PWD/wpilib:/usr/local/lib/vmxpi:$LD_LIBRARY_PATH"

mkdir -p logs
echo "===== $(date) run.sh start =====" >> logs/terminal.log

while true; do
    # Hide WPILib's timing noise (it misreads the VMX clock) and keep a copy
    # of everything else in logs/terminal.log.
    ./robot "$@" 2>&1 | grep --line-buffered -v -E "Watchdog not fed|Loop time of|UpdateValues\(\)|Periodic\(\):|Shuffleboard::Update|DisabledInit\(\):|^[[:space:]]*$" | tee -a logs/terminal.log
    code=${PIPESTATUS[0]}
    if [ "$code" -ne 42 ]; then
        exit "$code"
    fi
    echo "--- restarting robot program ---"
    sleep 1
done
