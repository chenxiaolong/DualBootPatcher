#!/sbin/busybox sh

#Mounting system
mount -o remount,rw /system/sbin/busybox mount -t rootfs -o remount,rw rootfs

#Set booted flag to set Super Max
echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_booted;

#Set ondemand sampling rate
echo 60000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate;

#Set some build props
/system/bin/setprop pm.sleep_mode 1
/system/bin/setprop ro.ril.disable.power.collapse 0
/system/bin/setprop ro.telephony.call_ring.delay 1000

#Enable and set zram and swappines
#insmod /system/lib/modules/zram.ko num_devices=4;
#echo 1 > /sys/block/zram0/reset
#echo 524288000 > /sys/block/zram0/disksize
#mkswap /dev/block/zram0
#swapon /dev/block/zram0
#echo 70 > /proc/sys/vm/swappiness

#Disable knox
pm disable com.sec.knox.seandroid

#Unmounting system
/sbin/busybox mount -t rootfs -o remount,ro rootfs mount -o remount,ro /system
