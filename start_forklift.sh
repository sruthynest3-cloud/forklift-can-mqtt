#!/usr/bin/env bash
set -e
cd ~/forklift_c

sudo modprobe vcan
sudo ip link add dev vcan0 type vcan 2>/dev/null || true
sudo ip link set up vcan0

# Ensure private network IP is active on ens4 if needed
sudo ip addr add 192.168.50.1/24 dev ens4 2>/dev/null || true
sudo ip link set up ens4 2>/dev/null || true

./forklift_sim vcan0 &
SIM_PID=$!
trap "kill $SIM_PID 2>/dev/null || true" EXIT

sleep 1
./can_mqtt_gateway vcan0 tcp://192.168.50.2:1883
