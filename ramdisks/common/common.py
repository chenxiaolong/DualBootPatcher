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

import multiboot.config as config
import multiboot.fileio as fileio

FSTAB_REGEX = r'^(#.+)?(/dev/\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)'
EXEC_MOUNT = 'exec /sbin/busybox-static sh /init.multiboot.mounting.sh\n'


def modify_default_prop(cpiofile, partition_config):
    cpioentry = cpiofile.get_file('default.prop')
    if not cpioentry:
        return

    buf = cpioentry.content
    buf += fileio.encode('ro.patcher.patched=%s\n' % partition_config.id)
    buf += fileio.encode('ro.patcher.version=%s\n' % config.get_version())

    cpioentry.content = buf


def add_syncdaemon(cpiofile):
    cpioentry = cpiofile.get_file('init.rc')

    buf = cpioentry.content
    buf += fileio.encode('\nservice syncdaemon /sbin/syncdaemon\n')
    buf += fileio.encode('    class main\n')
    buf += fileio.encode('    user root\n')
    buf += fileio.encode('    oneshot\n')

    cpioentry.content = buf


def remove_and_relink_busybox(cpiofile):
    # Remove bundled busybox
    cpioentry = cpiofile.get_file('sbin/busybox')
    if not cpioentry:
        return

    # The symlink will overwrite the original file
    cpiofile.add_symlink('busybox-static', 'sbin/busybox')


def init(cpiofile, partition_config):
    modify_default_prop(cpiofile, partition_config)
    add_syncdaemon(cpiofile)
    remove_and_relink_busybox(cpiofile)
