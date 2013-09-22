#!/sbin/sh

set_secondary_kernel() {
  dd if=/dev/block/platform/msm_sdcc.1/by-name/boot of=/raw-system/dual-kernels/secondary.img
}

mount_system() {
  mkdir -p /raw-system /system
  chmod 0755 /raw-system
  chown 0:0 /raw-system
  mount -t $1 $2 /raw-system
  mkdir -p /raw-system/dual
  mount -o bind /raw-system/dual /system
}

unmount_system() {
  umount /system
  umount /raw-system
}

mount_data() {
  mkdir -p /raw-data /data
  chmod 0755 /raw-data
  chown 0:0 /raw-data
  mount -t $1 $2 /raw-data
  mkdir -p /raw-data/dual /raw-data/media
  mount -o bind /raw-data/dual /data
  mount -o bind /raw-data/media /data/media
}

unmount_data() {
  umount /data/media
  umount /data
  umount /raw-data
}

mount_cache() {
  mkdir -p /raw-cache /cache
  chmod 0755 /raw-cache
  chown 0:0 /raw-cache
  mount -t $1 $2 /raw-cache
  mkdir -p /raw-cache/dual
  mount -o bind /raw-cache/dual /cache
}

unmount_cache() {
  umount /cache
  umount /raw-cache
}

case "$1" in
  set-secondary-kernel)
    set_secondary_kernel
    ;;
  mount-system)
    mount_system $2 $3
    ;;
  unmount-system)
    unmount_system
    ;;
  mount-data)
    mount_data $2 $3
    ;;
  unmount-data)
    unmount_data
    ;;
  mount-cache)
    mount_cache $2 $3
    ;;
  unmount-cache)
    unmount_cache
    ;;
esac
