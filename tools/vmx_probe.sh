#!/usr/bin/env bash
# Run this ON THE VMX (ssh pi@10.12.34.2), not on the laptop.
#   bash vmx_probe.sh 2>&1 | tee ~/vmx_probe_report.txt
# Then copy vmx_probe_report.txt back to the laptop so we know what we're working with.

line() { echo; echo "=============== $1 ==============="; }

line "OS / HOST"
hostname; cat /etc/os-release 2>/dev/null | head -3; uname -a

line "PYTHON"
for p in python3 python; do command -v $p >/dev/null && echo "$p -> $($p -V 2>&1)"; done
python3 -c "import cv2; print('opencv', cv2.__version__)" 2>&1 | tail -1
python3 -c "import numpy; print('numpy', numpy.__version__)" 2>&1 | tail -1

line "VMX HAL PYTHON"
ls -d /usr/local/src/hal_python_examples 2>/dev/null && ls /usr/local/src/hal_python_examples
python3 -c "import vmxpi_hal_python as v; print('vmxpi_hal_python OK')" 2>&1 | tail -1

line "ROS 2"
ls /opt/ros 2>/dev/null || echo "no /opt/ros -- ROS 2 NOT installed"
if [ -d /opt/ros ]; then
  for d in /opt/ros/*; do
    echo "--- sourcing $d"
    . "$d/setup.bash" 2>/dev/null && { echo "ROS_DISTRO=$ROS_DISTRO"; ros2 pkg list 2>/dev/null | grep -i studica || echo "  (no studica packages)"; }
  done
fi

line "STUDICA ROS2 WORKSPACE"
ls -d ~/studica_ws 2>/dev/null && ls ~/studica_ws || echo "no ~/studica_ws"
find / -name 'titan_example.py' -o -name 'params.yaml' -path '*studica*' 2>/dev/null | head -20

line "CAN / TITAN"
ip -br link show 2>/dev/null | grep -i can || echo "no CAN net iface (Titan may be on SPI/USB - that's normal)"
lsmod 2>/dev/null | grep -i -E 'can|mcp' || true

line "CAMERAS"
ls /dev/video* 2>/dev/null || echo "no /dev/video*"
command -v v4l2-ctl >/dev/null && v4l2-ctl --list-devices 2>/dev/null

line "I2C / SPI / SERIAL"
ls /dev/i2c-* /dev/spidev* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
command -v i2cdetect >/dev/null && sudo i2cdetect -y 1 2>/dev/null

line "DISK / NET"
df -h / | tail -1
ip -br addr

line "DONE"
echo "Copy ~/vmx_probe_report.txt back to the laptop."
