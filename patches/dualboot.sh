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
  mkdir -p /raw-data/dual
  mount -o bind /raw-data/dual /data
}

unmount_data() {
  umount /data
  umount /raw-data
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
esac
