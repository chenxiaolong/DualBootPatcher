#!/sbin/sh

set -x

set_secondary_kernel() {
  mkdir -p /raw-system/dual-kernels
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

format_system() {
  if mount | grep -q raw-system; then
    rm -rf /raw-system/dual
  elif mount | grep -q system; then
    rm -rf /system/dual
  else
    mount /system
    rm -rf /system/dual
    umount /system
  fi
}

mount_data() {
  mkdir -p /raw-data /data
  chmod 0755 /raw-data
  chown 0:0 /raw-data
  mount -t $1 $2 /raw-data
  mkdir -p /raw-data/dual /raw-data/media /raw-data/dual/media
  mount -o bind /raw-data/dual /data
  mount -o bind /raw-data/media /data/media
}

unmount_data() {
  umount /data/media
  umount /data
  umount /raw-data
}

format_data() {
  if mount | grep -q raw-data; then
    rm -rf /raw-data/dual
  elif mount | grep -q data; then
    rm -rf /data/dual
  else
    mount /data
    rm -rf /data/dual
    umount /data
  fi
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

format_cache() {
  if mount | grep -q raw-cache; then
    rm -rf /raw-cache/dual
  elif mount | grep -q cache; then
    rm -rf /cache/dual
  else
    mount /cache
    rm -rf /cache/dual
    umount /cache
  fi
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
  format-system)
    format_system
    ;;
  mount-data)
    mount_data $2 $3
    ;;
  unmount-data)
    unmount_data
    ;;
  format-data)
    format_data
    ;;
  mount-cache)
    mount_cache $2 $3
    ;;
  unmount-cache)
    unmount_cache
    ;;
  format-cache)
    format_cache
    ;;
esac
