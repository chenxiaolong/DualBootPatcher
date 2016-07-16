#!/system/bin/sh

# Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
#
# This file is part of MultiBootPatcher
#
# MultiBootPatcher is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# MultiBootPatcher is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.

set -x

cleanup() {
    set +x
    echo "Killing background processes"
    local jobs
    jobs=$(jobs -p)
    if [ -n "${jobs}" ]; then
        kill -KILL ${jobs}
        wait
    fi
}

trap "cleanup" EXIT INT TERM

export PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin
export LD_LIBRARY_PATH=/vendor/lib:/system/lib

# Enable use of the SS daemon
setprop ro.securestorage.support false

# The daemons use these symlinks
ln -s /system/etc /etc
ln -s /system/vendor /vendor

# Create mountpoints
mkdir -p /efs
mkdir -p /firmware
mkdir -p /firmware-mdm

# Create temporary paths needed by the daemons
#rm -r /dev/.secure_storage
#mkdir -p /dev/.secure_storage
mkdir -p /data
mkdir -p /data/app
mkdir -p /data/app/mcRegistry

# Wait for block devices
for path in /dev/block/platform/msm_sdcc.1/by-name/apnhlos \
            /dev/block/platform/msm_sdcc.1/by-name/mdm \
            /dev/block/platform/msm_sdcc.1/by-name/efs; do
    until [ -e "${path}" ]; do
        sleep 1
    done
done

# Mount partitions
mount -t vfat -o ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337 \
    /dev/block/platform/msm_sdcc.1/by-name/apnhlos \
    /firmware
mount -t vfat -o ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337 \
    /dev/block/platform/msm_sdcc.1/by-name/mdm \
    /firmware-mdm
mount -t ext4 -o nosuid,nodev,noatime,noauto_da_alloc,discard,journal_async_commit,errors=panic \
    /dev/block/platform/msm_sdcc.1/by-name/efs \
    /efs

# Required by vold
# Originally from: /system/bin/prepare_param.sh
ln -s /dev/block/platform/msm_sdcc.1/by-name/param /dev/block/param

# Mobicore initialization
# Originally from: /system/bin/mobicore-{presetup,startup}.sh

/system/bin/mcStarter

# Load mobicore trustlets
for i in ffffffff000000000000000000000005.tlbin \
         ffffffff000000000000000000000006.tlbin \
         ffffffffd00000000000000000000006.tlbin \
         07010000000000000000000000000000.tlbin \
         ffffffff000000000000000000000009.tlbin \
         ffffffff000000000000000000000008.tlbin \
         ffffffff000000000000000000000004.tlbin \
         ffffffffd00000000000000000000004.tlbin; do
    ln -s "/system/app/mcRegistry/${i}" "/data/app/mcRegistry/${i}"
done

# Run daemon in background
/system/bin/mcDriverDaemon -r /system/app/mcRegistry/FFFFFFFF000000000000000000000001.drbin &

# Required by vold
#/system/bin/secure_storage_daemon &

# Wait for services to start (ugh...)
#until getprop sys.mobicoredaemon.enable | grep -q true \
#        && [ -e /dev/.secure_storage/ssd_socket ]; do
until getprop sys.mobicoredaemon.enable | grep -q true; do
    sleep 0.2
done

# Sending SIGSTOP to self alerts mbtool to stop waiting for us.
# Once it receives this signal, it will send SIGCONT.
kill -STOP $$

# Wait until background processes exit or we're killed
wait
