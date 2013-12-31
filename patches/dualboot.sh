#!/sbin/sh

set -x
#set -u

FIELD_FILE=/tmp/fields.sh

get_field() {
  touch $FIELD_FILE
  unset $1
  . $FIELD_FILE
  set +u
  eval "echo \"\$$1\""
  set -u
}

set_field() {
  echo "$1=\"$2\"" >> $FIELD_FILE
}

field_equal() {
  if [ "x$(get_field $1)" = "x$2" ]; then
    return 0
  else
    return 1
  fi
}

################################################################

## The variables below are the defaults. The patcher WILL override them.

# Location, filesystem, and mount point of the raw partitions
DEV_SYSTEM="/dev/block/platform/msm_sdcc.1/by-name/system::ext4::/raw-system"
DEV_CACHE="/dev/block/platform/msm_sdcc.1/by-name/cache::ext4::/raw-cache"
DEV_DATA="/dev/block/platform/msm_sdcc.1/by-name/userdata::ext4::/raw-data"

# Where the non-primary ROM is going to be installed
TARGET_SYSTEM="/raw-system/dual"
TARGET_CACHE="/raw-cache/dual"
TARGET_DATA="/raw-data/dual"

# Which partitions the directories above reside
TARGET_SYSTEM_PARTITION=$DEV_SYSTEM
TARGET_CACHE_PARTITION=$DEV_CACHE
TARGET_DATA_PARTITION=$DEV_DATA

# Name of kernel image in /data/media/0/MultiKernels/
KERNEL_NAME="secondary"

################################################################

# PATCHER REPLACE ME - DO NOT REMOVE

################################################################

# When the target 'partition' needs to be mounted, mount the underlying
# partition first before doing the bind mount.

mount_raw_partition() {
  local DEV=$(echo $1 | awk -F:: '{print $1}')
  local FS=$(echo $1 | awk -F:: '{print $2}')
  local MNT=$(echo $1 | awk -F:: '{print $3}')

  # Unique alphanumeric variable name
  local HASH=ABCD$(echo $MNT | md5sum | awk '{print $1}')

  local TIMES_MOUNTED=$(get_field $HASH)
  if [ -z $TIMES_MOUNTED ]; then
    TIMES_MOUNTED=0
  fi

  if [ $TIMES_MOUNTED -eq 0 ]; then
    mkdir -p $MNT
    chmod 0755 $MNT
    chown 0:0 $MNT
    mount -t $FS $DEV $MNT
  fi

  let "TIMES_MOUNTED += 1"
  set_field $HASH $TIMES_MOUNTED
}

mount_system() {
  if ! field_equal SYSTEM_MOUNTED true; then
    mount_raw_partition $TARGET_SYSTEM_PARTITION

    mkdir -p /system $TARGET_SYSTEM
    mount -o bind $TARGET_SYSTEM /system

    set_field SYSTEM_MOUNTED true
  fi
}

mount_cache() {
  if ! field_equal CACHE_MOUNTED true; then
    mount_raw_partition $TARGET_CACHE_PARTITION

    mkdir -p /cache $TARGET_CACHE
    mount -o bind $TARGET_CACHE /cache

    set_field CACHE_MOUNTED true
  fi
}

mount_data() {
  if ! field_equal DATA_MOUNTED true; then
    mount_raw_partition $TARGET_DATA_PARTITION
    # TODO: Fix case where the target's /data is not on the /data partition
    #mount_raw_partition $DEV_DATA

    local MNT=$(echo $DEV_DATA | awk -F:: '{print $3}')

    mkdir -p /data $TARGET_DATA $TARGET_DATA/media $MNT/media
    mount -o bind $TARGET_DATA /data
    mount -o bind $MNT/media /data/media

    set_field DATA_MOUNTED true
  fi
}

unmount_raw_partition() {
  local DEV=$(echo $1 | awk -F:: '{print $1}')
  local FS=$(echo $1 | awk -F:: '{print $2}')
  local MNT=$(echo $1 | awk -F:: '{print $3}')

  # Unique alphanumeric variable name
  local HASH=ABCD$(echo $MNT | md5sum | awk '{print $1}')

  local TIMES_MOUNTED=$(get_field $HASH)
  if [ -z $TIMES_MOUNTED ]; then
    TIMES_MOUNTED=0
  fi

  if [ $TIMES_MOUNTED -eq 1 ] || [ "x$2" = "xforce" ]; then
    umount $MNT
  fi

  if [ $TIMES_MOUNTED -gt 0 ]; then
    let "TIMES_MOUNTED -= 1"
    set_field $HASH $TIMES_MOUNTED
  fi
}

unmount_system() {
  umount /system
  unmount_raw_partition $TARGET_SYSTEM_PARTITION
  set_field SYSTEM_MOUNTED false
}

unmount_cache() {
  umount /cache
  unmount_raw_partition $TARGET_CACHE_PARTITION
  set_field CACHE_MOUNTED false
}

unmount_data() {
  umount /data/media
  umount /data
  unmount_raw_partition $TARGET_DATA_PARTITION
  set_field DATA_MOUNTED false
}

unmount_everything() {
  unmount_system
  unmount_cache
  unmount_data
  unmount_raw_partition $TARGET_SYSTEM_PARTITION force
  unmount_raw_partition $TARGET_CACHE_PARTITION force
  unmount_raw_partition $TARGET_DATA_PARTITION force
}

################################################################

format_system() {
  local OLDPWD=$(pwd)
  if [ -d $TARGET_SYSTEM ]; then
    cd $TARGET_SYSTEM
    rm -rf *
    cd $OLDPWD
  else
    mount_system
    if [ -d $TARGET_SYSTEM ]; then
      cd $TARGET_SYSTEM
      rm -rf *
      cd $OLDPWD
    fi
    unmount_system
  fi
}

format_cache() {
  local OLDPWD=$(pwd)
  if [ -d $TARGET_CACHE ]; then
    cd $TARGET_CACHE
    rm -rf *
    cd $OLDPWD
  else
    mount_cache
    if [ -d $TARGET_CACHE ]; then
      cd $TARGET_CACHE
      rm -rf *
      cd $OLDPWD
    fi
    unmount_cache
  fi
}

format_data() {
  local OLDPWD=$(pwd)
  if [ -d $TARGET_DATA ]; then
    cd $TARGET_DATA
    rm -rf $(find -maxdepth 1 -mindepth 1 ! -name media)
    cd $OLDPWD
  else
    mount_data
    if [ -d $TARGET_DATA ]; then
      cd $TARGET_DATA
      rm -rf $(find -maxdepth 1 -mindepth 1 ! -name media)
      cd $OLDPWD
    fi
    unmount_data
  fi
}

################################################################

set_multi_kernel() {
  local MNT=$(echo $DEV_DATA | awk -F:: '{print $3}')

  local MOUNT=false
  if ! mount | grep -q $MNT; then
    MOUNT=true
  fi

  if [ "x${MOUNT}" = "xtrue" ]; then
    mount_raw_partition $DEV_DATA
  fi

  local KERNELS_DIR=$MNT/media/0/MultiKernels/
  local DEV_BOOT="/dev/block/platform/msm_sdcc.1/by-name/boot"
  mkdir -p $KERNELS_DIR
  dd if=$DEV_BOOT of=$KERNELS_DIR/$KERNEL_NAME.img
  chmod 775 $KERNELS_DIR/$KERNEL_NAME.img

  if [ "x${MOUNT}" = "xtrue" ]; then
    unmount_raw_partition $DEV_DATA
  fi
}

################################################################

case "$1" in
  mount-system)         mount_system         ;;
  mount-cache)          mount_cache          ;;
  mount-data)           mount_data           ;;
  unmount-system)       unmount_system       ;;
  unmount-cache)        unmount_cache        ;;
  unmount-data)         unmount_data         ;;
  unmount-everything)   unmount_everything   ;;
  format-system)        format_system        ;;
  format-cache)         format_cache         ;;
  format-data)          format_data          ;;
  set-multi-kernel)     set_multi_kernel     ;;
esac
