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

# Make sure the modem firmware partitions are mounted
mount -t vfat -o ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337 /dev/block/platform/msm_sdcc.1/by-name/apnhlos /firmware
mount -t vfat -o ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337 /dev/block/platform/msm_sdcc.1/by-name/modem /firmware-modem
