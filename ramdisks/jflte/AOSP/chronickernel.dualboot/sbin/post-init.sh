#!/system/bin/sh

#set gpu max_clk rate
echo 504000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

#Set governor items
echo 378000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq;
echo 1890000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq;
echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_booted;
echo 1 > /sys/devices/system/cpu/cpufreq/ktoonsez/enable_oc;

echo $(date) END of post-init.sh
