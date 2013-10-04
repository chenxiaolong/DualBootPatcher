#!/system/bin/sh

#Set Max Mhz for GPU
echo 450000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

#Set Max Mhz speed and booted flag to set Super Max
echo 1890000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq;
echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_booted;

echo "Running Post-Init Script"

#Check for forcing Samsung MTP
if [ -f /data/.ktoonsez/force_samsung_mtp ];
then
  # BackUp old post-init log
  echo "Forcing Samsung MTP"
  echo 1 > /sys/devices/system/cpu/cpufreq/ktoonsez/enable_google_mtp;
fi

echo $(date) END of post-init.sh
