#!/sbin/busybox-static sh

# Standard Linux commands are not available until /system is mounted, so we'll
# just use busybox for everything

# PATCHER REPLACE ME - DO NOT REMOVE

/sbin/busybox-static mkdir -p $TARGET_SYSTEM $TARGET_CACHE $TARGET_DATA

/sbin/busybox-static mount -o bind $TARGET_SYSTEM /system
/sbin/busybox-static mount -o bind $TARGET_CACHE /cache
/sbin/busybox-static mount -o bind $TARGET_DATA /data

# Eventually, maybe share /data/app
#/sbin/busybox-static mkdir -p /raw-data/app /data/app
#/sbin/busybox-static chmod 0771 /data/app
#/sbin/busybox-static chmod system:system /data/app
#/sbin/busybox-static mount -o bind /raw-data/app /data/app
