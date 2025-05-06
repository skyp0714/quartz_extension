#! /bin/bash

sudo scripts/setupdev.sh load
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
echo 2 | sudo tee /sys/devices/cpu/rdpmc
# turn off turbo_boost mode
sudo sh -c 'echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo'

