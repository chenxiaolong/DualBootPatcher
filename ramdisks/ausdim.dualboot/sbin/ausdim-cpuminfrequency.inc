# change min cpu frequency

# Initialize variable and read frequency setting
FREQ=""
FREQ=$(/sbin/busybox sed -n -e 's/.*cpu_minfreq=\(.*\)(.*/\1/p' $AUSDIM_CONFIG)

# Only apply value if valid and not default
if [ ! "$FREQ" = "default" ] && [ "${FREQ:-0}" -ge 81 ] && [ "${FREQ:-0}" -le 2160 ] ; then
        echo $(( $FREQ * 1000 )) > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        echo $(date) Min CPU set to $FREQ MHz >> $AUSDIM_LOGFILE
fi
