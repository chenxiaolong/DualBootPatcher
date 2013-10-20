# unload exfat drivers if configured to off in ausdim config file

if [ "`/sbin/busybox grep exfat=off $AUSDIM_CONFIG`" ]; then
  /sbin/busybox rmmod /lib/modules/exfat_fs.ko
  /sbin/busybox rmmod /lib/modules/exfat_core.ko
fi
