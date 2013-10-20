# LED fading

if /sbin/busybox [ "`/sbin/busybox grep led_fading=fade $AUSDIM_CONFIG`" ]; then

  echo "1" > /sys/class/sec/led/led_fade
  echo "40" > /sys/class/sec/led/led_intensity

  echo $(date) "LED fading normal enabled" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_fading=fade-bright $AUSDIM_CONFIG`" ]; then

  echo "1" > /sys/class/sec/led/led_fade
  echo "192" > /sys/class/sec/led/led_intensity

  echo $(date) "LED fading bright enabled" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_fading=blink-bright $AUSDIM_CONFIG`" ]; then

  echo "0" > /sys/class/sec/led/led_fade
  echo "192" > /sys/class/sec/led/led_intensity

  echo $(date) "LED blinking bright enabled" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_fading=fade-dark $AUSDIM_CONFIG`" ]; then

  echo "1" > /sys/class/sec/led/led_fade
  echo "15" > /sys/class/sec/led/led_intensity

  echo $(date) "LED fading dark enabled" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_fading=blink-dark $AUSDIM_CONFIG`" ]; then

  echo "0" > /sys/class/sec/led/led_fade
  echo "15" > /sys/class/sec/led/led_intensity

  echo $(date) "LED blinking dark enabled" >> $AUSDIM_LOGFILE

fi


# LED speed

if /sbin/busybox [ "`/sbin/busybox grep led_speed=2x $AUSDIM_CONFIG`" ]; then

  echo "2" > /sys/class/sec/led/led_speed

  echo $(date) "LED speed set to 2x" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_speed=3x $AUSDIM_CONFIG`" ]; then

  echo "3" > /sys/class/sec/led/led_speed

  echo $(date) "LED speed set to 3x" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_speed=4x $AUSDIM_CONFIG`" ]; then

  echo "4" > /sys/class/sec/led/led_speed

  echo $(date) "LED speed set to 4x" >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep led_speed=5x $AUSDIM_CONFIG`" ]; then

  echo "5" > /sys/class/sec/led/led_speed

  echo $(date) "LED speed set to 5x" >> $AUSDIM_LOGFILE

fi
