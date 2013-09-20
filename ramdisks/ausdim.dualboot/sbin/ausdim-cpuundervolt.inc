if /sbin/busybox [ "`/sbin/busybox grep cpu_undervolting=25 $AUSDIM_CONFIG`" ]; then
  echo "1312 1287 1262 1237 1212 1187 1137 1087 1050 1025 987 962 950 925 900 887 875 875 875 875 825 825 825 " > /sys/devices/system/cpu/cpu0/cpufreq/UV_mV_table
  echo $(date) CPU undervolting by -25mv applied >> $AUSDIM_LOGFILE
fi

if /sbin/busybox [ "`/sbin/busybox grep cpu_undervolting=50 $AUSDIM_CONFIG`" ]; then
  echo "1287 1262 1237 1212 1187 1162 1112 1062 1025 1000 962 937 925 900 875 862 850 850 850 850 800 800 800 " > /sys/devices/system/cpu/cpu0/cpufreq/UV_mV_table
  echo $(date) CPU undervolting by -50mv applied >> $AUSDIM_LOGFILE
fi

if /sbin/busybox [ "`/sbin/busybox grep cpu_undervolting=75 $AUSDIM_CONFIG`" ]; then
  echo "1262 1237 1212 1187 1162 1137 1087 1037 1000 975 937 912 900 875 850 837 825 825 825 825 775 775 775  " > /sys/devices/system/cpu/cpu0/cpufreq/UV_mV_table
  echo $(date) CPU undervolting by -75mv applied >> $AUSDIM_LOGFILE
fi

if /sbin/busybox [ "`/sbin/busybox grep cpu_undervolting=100 $AUSDIM_CONFIG`" ]; then
  echo "1237 1212 1187 1162 1137 1112 1062 1012 975 950 912 887 875 850 825 812 800 800 800 800 750 750 750 " > /sys/devices/system/cpu/cpu0/cpufreq/UV_mV_table
  echo $(date) CPU undervolting by -100mv applied >> $AUSDIM_LOGFILE
fi
