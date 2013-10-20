if /sbin/busybox [ "`/sbin/busybox grep autoroot=on $AUSDIM_CONFIG`" ] || [ -f /data/media/0/autoroot ]; then

	echo $(date) Auto root is enabled >> $AUSDIM_LOGFILE

	if [ ! -f /system/xbin/su ] && [ ! -f /system/bin/su ]; then
		/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system
		/sbin/busybox rm /system/bin/su
		/sbin/busybox rm /system/xbin/su
		/sbin/busybox cp /res/root/su /system/xbin/su
		/sbin/busybox chown 0.0 /system/xbin/su
		/sbin/busybox chmod 6755 /system/xbin/su
		/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system
		echo $(date) Auto root: su binaries copied >> $AUSDIM_LOGFILE
	fi

	if [ ! -f /system/app/Superuser.apk ] && [ ! -f /data/app/Superuser.apk ]; then
		/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system
		/sbin/busybox rm /system/app/Superuser.apk
		/sbin/busybox rm /data/app/Superuser.apk
		/sbin/busybox cp /res/root/Superuser.apk /system/app/Superuser.apk
		/sbin/busybox chown 0.0 /system/app/Superuser.apk
		/sbin/busybox chmod 644 /system/app/Superuser.apk
		/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system
		echo $(date) Auto root: Superuser app copied >> $AUSDIM_LOGFILE
	fi

	rm /data/media/0/autoroot

fi
