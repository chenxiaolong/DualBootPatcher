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
esac
