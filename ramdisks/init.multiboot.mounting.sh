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

bind_mount() {
  local source="$1"
  local target="$2"

  /sbin/busybox-static mkdir -p "$source" "$target"
  /sbin/busybox-static chmod 0771 "$target"
  /sbin/busybox-static chown system:system "$target"
  /sbin/busybox-static mount -o bind "$source" "$target"
}

share_app=false
share_app_asec=false

if [ -f '/data/patcher.share-app' ]; then
  share_app=true
fi

if [ -f '/data/patcher.share-app-asec' ]; then
  share_app_asec=true
fi

if [ "$share_app" = "true" ] || [ "$share_app_asec" = "true" ]; then
  bind_mount '/raw-data/app-lib' '/data/app-lib'
fi

if [ "$share_app" = "true" ]; then
  bind_mount '/raw-data/app' '/data/app'
fi

if [ "$share_app_asec" = "true" ]; then
  bind_mount '/raw-data/app-asec' '/data/app-asec'
fi

if [ -f /init.additional.sh ]; then
  /sbin/busybox-static sh /init.additional.sh
fi
