#!/sbin/busybox-static sh

# Make sure the modem firmware partitions are mounted
mount -t vfat -o ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337 /dev/block/platform/msm_sdcc.1/by-name/apnhlos /firmware
mount -t vfat -o ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337 /dev/block/platform/msm_sdcc.1/by-name/mdm /firmware-mdm
