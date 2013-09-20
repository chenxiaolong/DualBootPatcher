SYSTEM_DEVICE="/dev/block/mmcblk0p16"

# Remount system device to rw
/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system

# Delete affected current settings
/sbin/busybox rm /system/etc/init.d/*dkgov*
/sbin/busybox rm /system/etc/init.d/*dksched*int*
/sbin/busybox rm /system/etc/init.d/*dksched*ext*
/sbin/busybox rm /system/etc/init.d/*dkcpumaxfreq*
/sbin/busybox rm /system/etc/init.d/*dkcpuminfreq*
/sbin/busybox rm /system/etc/init.d/*dkscreenoffreq*
/sbin/busybox rm /system/etc/init.d/*dkcpuuv*
/sbin/busybox rm /system/etc/init.d/*dkgpufreq*
/sbin/busybox rm /system/etc/init.d/*dkgpuuv*

# Copy default scripts
/sbin/busybox cp /res/scripts/$GOVERNOR_DEFAULT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$SCHEDULERINT_DEFAULT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$SCHEDULEREXT_DEFAULT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$CPUFREQ_DEFAULT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$CPUUV_DEFAULT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$GPUFREQ_DEFAULT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$GPUUV_DEFAULT /system/etc/init.d/

# set permissions of scripts
/sbin/busybox chmod 755 /system/etc/init.d/*

# execute all ausdim kernel init.d files to apply settings
for file in /system/etc/init.d/*_???dk* ; 
do
	/sbin/busybox sh "$file"
done

# Delete default settings
/sbin/busybox rm /system/etc/init.d/*dkgov*
/sbin/busybox rm /system/etc/init.d/*dksched*int*
/sbin/busybox rm /system/etc/init.d/*dksched*ext*
/sbin/busybox rm /system/etc/init.d/*dkcpuminfreq*
/sbin/busybox rm /system/etc/init.d/*dkcpumaxfreq*
/sbin/busybox rm /system/etc/init.d/*dkscreenoffreq*
/sbin/busybox rm /system/etc/init.d/*dkcpuuv*
/sbin/busybox rm /system/etc/init.d/*dkgpufreq*
/sbin/busybox rm /system/etc/init.d/*dkgpuuv*

# Copy profile scripts
/sbin/busybox cp /res/scripts/$GOVERNOR /system/etc/init.d/
/sbin/busybox cp /res/scripts/$SCHEDULERINT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$SCHEDULEREXT /system/etc/init.d/
/sbin/busybox cp /res/scripts/$CPUFREQ /system/etc/init.d/
/sbin/busybox cp /res/scripts/$CPUUV /system/etc/init.d/
/sbin/busybox cp /res/scripts/$GPUFREQ /system/etc/init.d/
/sbin/busybox cp /res/scripts/$GPUUV /system/etc/init.d/

# set permissions of scripts
/sbin/busybox chmod 755 /system/etc/init.d/*

# execute all ausdim kernel init.d files to apply settings
for file in /system/etc/init.d/*_???dk* ; 
do
	/sbin/busybox sh "$file"
done

# Remount system device back to rw
/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system
