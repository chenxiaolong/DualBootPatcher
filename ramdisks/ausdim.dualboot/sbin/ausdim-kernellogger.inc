if /sbin/busybox [ "`/sbin/busybox grep kernellogger=on $AUSDIM_CONFIG`" ]; then

  echo 1 > /sys/kernel/printk_mode/printk_mode

  echo $(date) Kernel logger enabled >> $AUSDIM_LOGFILE

else

  echo 0 > /sys/kernel/printk_mode/printk_mode

  echo $(date) Kernel logger disabled >> $AUSDIM_LOGFILE

fi
