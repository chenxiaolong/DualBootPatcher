#!/sbin/busybox-static sh

# Standard Linux commands are not available until /system is mounted, so we'll
# just use busybox for everything

/sbin/busybox-static mkdir -p /raw-system/dual /raw-cache/dual /raw-data/dual

/sbin/busybox-static mount -o bind /raw-system/dual /system
/sbin/busybox-static mount -o bind /raw-data/dual /data
/sbin/busybox-static mount -o bind /raw-cache/dual /cache

# Eventually, maybe share /data/app
#/sbin/busybox-static mkdir -p /raw-data/app
#/sbin/busybox-static chmod 0771 /data/app
#/sbin/busybox-static chmod system:system /data/app
#/sbin/busybox-static mount -o bind /raw-data/app /data/app
