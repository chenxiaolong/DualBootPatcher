# change governor to interactive, conservative, userspace, powersave, ondemand, performance with its respective defaults
GOVERNOR=""

if /sbin/busybox [ "`/sbin/busybox grep governor=badass $AUSDIM_CONFIG`" ]; then
  GOVERNOR="badass"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=adaptive $AUSDIM_CONFIG`" ]; then
  GOVERNOR="adaptive"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=wheatley $AUSDIM_CONFIG`" ]; then
  GOVERNOR="wheatley"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=userspace $AUSDIM_CONFIG`" ]; then
  GOVERNOR="userspace"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=powersave $AUSDIM_CONFIG`" ]; then
  GOVERNOR="powersave"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=dancedance $AUSDIM_CONFIG`" ]; then
  GOVERNOR="dancedance"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=smartassH3 $AUSDIM_CONFIG`" ]; then
  GOVERNOR="smartassH3"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=interactive $AUSDIM_CONFIG`" ]; then
  GOVERNOR="interactive"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=performance $AUSDIM_CONFIG`" ]; then
  GOVERNOR="performance"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=abyssplugv2 $AUSDIM_CONFIG`" ]; then
  GOVERNOR="abyssplugv2"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=ktoonservativeq $AUSDIM_CONFIG`" ]; then
  GOVERNOR="ktoonservativeq"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=conservative $AUSDIM_CONFIG`" ]; then
  GOVERNOR="conservative"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=asswax $AUSDIM_CONFIG`" ]; then
  GOVERNOR="asswax"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=intellidemand $AUSDIM_CONFIG`" ]; then
  GOVERNOR="intellidemand"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=ondemand $AUSDIM_CONFIG`" ]; then
  GOVERNOR="ondemand"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=conservative $AUSDIM_CONFIG`" ]; then
  GOVERNOR="conservative"
fi

if /sbin/busybox [ "`/sbin/busybox grep governor=badass $AUSDIM_CONFIG`" ]; then
  GOVERNOR="badass"
fi

if [ ! -z $GOVERNOR ]; then
sleep 3
chmod 666 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 
echo "$GOVERNOR" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
chmod 666 /sys/devices/system/cpu/cpu1/online
echo "1" > /sys/devices/system/cpu/cpu1/online
chmod 444 /sys/devices/system/cpu/cpu1/online
chmod 666 /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor 
echo "$GOVERNOR" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
chmod 444 /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
chmod 666 /sys/devices/system/cpu/cpu2/online
echo "1" > /sys/devices/system/cpu/cpu2/online
chmod 444 /sys/devices/system/cpu/cpu2/online
chmod 666 /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor 
echo "$GOVERNOR" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
chmod 444 /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
chmod 666 /sys/devices/system/cpu/cpu3/online
echo "1" > /sys/devices/system/cpu/cpu3/online
chmod 444 /sys/devices/system/cpu/cpu3/online
chmod 666 /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor 
echo "$GOVERNOR" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
chmod 444 /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor

	echo $(date) Cpu governor set to "$GOVERNOR" >> $AUSDIM_LOGFILE

fi
