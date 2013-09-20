if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=128000000 $AUSDIM_CONFIG`" ]; then

	echo 128000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequency 128000000 applied >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=200000000 $AUSDIM_CONFIG`" ]; then

	echo 200000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequency 200000000 applied >> $AUSDIM_LOGFILE

fi


if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=320000000 $AUSDIM_CONFIG`" ]; then

	echo 320000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequencies 320000000 applied >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=450000000 $AUSDIM_CONFIG`" ]; then

	echo 450000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequencies 450000000 applied >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=504000000 $AUSDIM_CONFIG`" ]; then

	echo 504000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequencies 504000000 applied >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=545000000 $AUSDIM_CONFIG`" ]; then

	echo 545000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequencies 545000000 applied >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=600000000 $AUSDIM_CONFIG`" ]; then

	echo 600000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequencies 600000000 applied >> $AUSDIM_LOGFILE

fi

if /sbin/busybox [ "`/sbin/busybox grep max_gpuclk=627000000 $AUSDIM_CONFIG`" ]; then

	echo 627000000 > /sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/max_gpuclk

	echo $(date) GPU frequencies 627000000 applied >> $AUSDIM_LOGFILE

fi
