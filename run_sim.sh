#!/usr/bin/env bash
set -e

# 1. Install vcan kernel module if missing
if ! modprobe -n vcan >/dev/null 2>&1; then
    echo "[SETUP] Installing vcan kernel module..."
    sudo apt-get update
    sudo apt-get install -y "linux-modules-extra-$(uname -r)"
fi

# 2. Load vcan driver and bring up vcan0 interface
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan 2>/dev/null || true
sudo ip link set up vcan0

# 3. Pull latest dev code and build
git pull origin dev
make all

# 4. Stop background simulator automatically when you press Ctrl+C
trap 'echo ""; echo "[STOP] Shutting down simulator..."; kill $SIM_PID 2>/dev/null || true' EXIT

# 5. Start simulator in background and stream candump in foreground
./forklift_sim vcan0 &
SIM_PID=$!
sleep 0.5
candump vcan0