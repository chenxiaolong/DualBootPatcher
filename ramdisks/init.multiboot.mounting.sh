#!/sbin/busybox-static sh

# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ---

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

if [ -f /init.additional.sh ]; then
  /sbin/busybox-static sh /init.additional.sh
fi
