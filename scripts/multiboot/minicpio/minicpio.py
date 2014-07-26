# Copyright (c) 2014 Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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


# This is a miniature implementation of cpio supporting only enough of the
# file format for the purpose of the dual boot patcher. Only the "new format"
# for the cpio entries are supported. Because of the use case, everything is
# loaded into memory (needs 2x the file size). Don't use this with large files!

# Supported files
# - Regular files
# - Directories
# - Symlinks
# - Hardlinks


import multiboot.operatingsystem as OS

import os
import stat
import struct

###
debug = False
###

MAGIC_NEW = "070701"                 # new format
MAGIC_NEW_CRC = "070702"             # new format w/crc
MAGIC_OLD_ASCII = "070707"           # old ascii format
MAGIC_OLD_BINARY = int("070707", 8)  # old binary format

# Constants from cpio.h

# A header with a filename "TRAILER!!!" indicates the end of the archive.
CPIO_TRAILER = "TRAILER!!!"

IO_BLOCK_SIZE = 512


class CpioEntryNew(object):
    def __init__(self):
        super(CpioEntryNew, self).__init__()

        self.sformat = '<'
        self.sformat += '6s'  # c_magic     - "070701" for "new" portable format
                              #               "070702" for CRC format
        self.sformat += '8s'  # c_ino
        self.sformat += '8s'  # c_mode
        self.sformat += '8s'  # c_uid
        self.sformat += '8s'  # c_gid
        self.sformat += '8s'  # c_nlink
        self.sformat += '8s'  # c_mtime
        self.sformat += '8s'  # c_filesize  - must be 0 for FIFOs and directories
        self.sformat += '8s'  # c_dev_maj
        self.sformat += '8s'  # c_dev_min
        self.sformat += '8s'  # c_rdev_maj  - only valid for chr and blk special files
        self.sformat += '8s'  # c_rdev_min  - only valid for chr and blk special files
        self.sformat += '8s'  # c_namesize  - count includes terminating NUL in pathname
        self.sformat += '8s'  # c_chksum    - 0 for "new" portable format; for CRC format
                              #               the sum of all the bytes in the file

        self.headersize = 0
        self.alignmentBoundary = 0

        self.magic = None
        self.ino = None
        self.mode = None
        self.uid = None
        self.gid = None
        self.nlink = None
        self.mtime = None
        self.filesize = None
        self.dev_maj = None
        self.dev_min = None
        self.rdev_maj = None
        self.rdev_min = None
        self.namesize = None
        self.chksum = None

        self.name = None
        self._content = None

        self.headersize = 110
        self.alignmentBoundary = 4

    def unpack(self, data, offset=0):
        unpacked = struct.unpack_from(self.sformat, data, offset)

        self.magic = unpacked[0].decode('ascii')
        self.ino = int(unpacked[1], 16)
        self.mode = int(unpacked[2], 16)
        self.uid = int(unpacked[3], 16)
        self.gid = int(unpacked[4], 16)
        self.nlink = int(unpacked[5], 16)
        self.mtime = int(unpacked[6], 16)
        self.filesize = int(unpacked[7], 16)
        self.dev_maj = int(unpacked[8], 16)
        self.dev_min = int(unpacked[9], 16)
        self.rdev_maj = int(unpacked[10], 16)
        self.rdev_min = int(unpacked[11], 16)
        self.namesize = int(unpacked[12], 16)
        self.chksum = int(unpacked[13], 16)

        namestart = offset + struct.calcsize(self.sformat)
        nameend = namestart + self.namesize

        self.name = data[namestart:nameend - 1].decode('ascii')

        padding = (4 - (nameend % 4)) % 4

        datastart = nameend + padding
        dataend = datastart + self.filesize

        self._content = data[datastart:dataend]

        padding = (4 - (dataend % 4)) % 4

        return dataend + padding

    def pack(self, data):
        header = struct.pack(
            self.sformat,
            self.magic.encode('ascii'),
            ('%08x' % self.ino).encode('ascii'),
            ('%08x' % self.mode).encode('ascii'),
            ('%08x' % self.uid).encode('ascii'),
            ('%08x' % self.gid).encode('ascii'),
            ('%08x' % self.nlink).encode('ascii'),
            ('%08x' % self.mtime).encode('ascii'),
            ('%08x' % self.filesize).encode('ascii'),
            ('%08x' % self.dev_maj).encode('ascii'),
            ('%08x' % self.dev_min).encode('ascii'),
            ('%08x' % self.rdev_maj).encode('ascii'),
            ('%08x' % self.rdev_min).encode('ascii'),
            ('%08x' % self.namesize).encode('ascii'),
            ('%08x' % self.chksum).encode('ascii'))

        offset = len(data)

        data += header

        # name
        data += self.name.encode('ascii') + b'\x00'

        namestart = offset + struct.calcsize(self.sformat)
        nameend = namestart + self.namesize
        padding = (4 - (nameend % 4)) % 4

        data += b'\x00' * padding

        # content
        data += self._content

        datastart = nameend + padding
        dataend = datastart + self.filesize
        padding = (4 - (dataend % 4)) % 4

        data += b'\x00' * padding

        return data

    @property
    def content(self):
        return self._content

    @content.setter
    def content(self, value):
        self._content = value
        self.filesize = len(value)

    def dump(self):
        # C_ISCTG  = int("0110000", 8)
        filetype = stat.S_IFMT(self.mode)

        if stat.S_ISDIR(self.mode):
            ftypestr = "directory"
        elif stat.S_ISLNK(self.mode):
            ftypestr = "symbolic link"
        elif stat.S_ISREG(self.mode):
            ftypestr = "regular file"
        elif stat.S_ISFIFO(self.mode):
            ftypestr = "pipe"
        elif stat.S_ISCHR(self.mode):
            ftypestr = "character device"
        elif stat.S_ISBLK(self.mode):
            ftypestr = "block device"
        elif stat.S_ISSOCK(self.mode):
            ftypestr = "socket"
        # elif filetype == C_ISCTG:
        #     ftypestr = "what is this"
        else:
            ftypestr = 'unknown (%o)' % filetype

        print('Filename:        %s' % self.name)
        print('Filetype:        %s' % ftypestr)
        print('Magic:           %s' % self.magic)
        print('Inode:           %i' % self.ino)
        print('Mode:            %o' % self.mode)
        print('Permissions:     %o' % (self.mode - filetype))
        print('UID:             %i' % self.uid)
        print('GID:             %i' % self.gid)
        print('Links:           %i' % self.nlink)
        print('Modified:        %i' % self.mtime)
        print('File size:       %i' % self.filesize)
        print('dev_maj:         %x' % self.dev_maj)
        print('dev_min:         %x' % self.dev_min)
        print('rdev_maj:        %x' % self.rdev_maj)
        print('rdev_min:        %x' % self.rdev_min)
        print('Filename length: %i' % self.namesize)
        print('Checksum:        %i' % self.chksum)
        print('')


class CpioFile:
    def __init__(self):
        self.members = []
        self.inodemap = dict()

    def load_file(self, filename):
        with open(filename, 'rb') as f:
            self.load(f.read())

    def load(self, data):
        pos = 0

        while True:
            member = CpioEntryNew()
            pos = member.unpack(data, pos)

            if debug:
                member.dump()

            if member.name == CPIO_TRAILER:
                break

            # Make sure the existing inodes won't conflict with ones from newly
            # added files, so make them negative for now

            member.ino *= -1

            if member.ino in self.inodemap:
                self.inodemap[member.ino].append(member)
            else:
                self.inodemap[member.ino] = [member]

            self.members.append(member)

    def create(self):
        buf = bytes()

        # Uniquify inodes
        cur_inode = 300000

        # The kernel wants directories to come first
        temp = []

        # Assign inodes
        for inode in self.inodemap:
            for i in self.inodemap[inode]:
                i.ino = cur_inode
                temp.append(i)

            cur_inode += 1

        temp = sorted(temp, key=lambda x: x.name)

        for i in temp:
            buf = i.pack(buf)

        member = CpioEntryNew()
        member.magic = MAGIC_NEW
        member.ino = cur_inode
        member.mode = 0
        member.uid = 0
        member.gid = 0
        member.nlink = 1  # Must be 1 for crc format
        member.mtime = 0
        member.filesize = 0
        member.dev_maj = 0
        member.dev_min = 0
        member.rdev_maj = 0
        member.rdev_min = 0
        member.namesize = len(CPIO_TRAILER) + 1
        member.chksum = 0
        member.name = CPIO_TRAILER
        member.content = b''
        buf = member.pack(buf)

        # Pad until end of block
        buf += b'\x00' * (IO_BLOCK_SIZE - (len(buf) % IO_BLOCK_SIZE))

        return buf

    def remove(self, name):
        for i in self.members:
            if i.name == name:
                # Remove from inode map
                if len(self.inodemap[i.ino]) > 1:
                    # Not going to deal with relocating data to another
                    # hard link
                    raise Exception('Trying to replace hard link with data')

                self.members.remove(i)
                self.inodemap[i.ino].remove(i)
                if len(self.inodemap[i.ino]) == 0:
                    del self.inodemap[i.ino]
                break

    def add_symlink(self, source, target):
        if not source or not target:
            raise Exception('The symlink source and target must be valid')

        self.remove(target)

        member = CpioEntryNew()
        member.magic = MAGIC_NEW

        # Give the member an inode number
        member.ino = 0
        while True:
            member.ino -= 1
            if member.ino not in self.inodemap:
                break

        self.inodemap[member.ino] = [member]

        member.mode = stat.S_IFLNK
        member.mode |= 0o777

        member.uid = 0
        member.gid = 0
        member.nlink = 1
        member.mtime = 0
        member.dev_maj = 0
        member.dev_min = 0
        member.rdev_maj = 0
        member.rdev_min = 0

        member.name = target
        member.namesize = len(target) + 1

        member.content = source.encode('UTF-8')

        member.chksum = 0

        self.members.append(member)

    def add_file(self, filename, name=None, perms=None):
        if name is None:
            name = filename

        # If file exists, delete it
        self.remove(name)

        member = CpioEntryNew()
        member.magic = MAGIC_NEW

        # Make sure we get some inode number if it's zero
        member.ino = os.stat(filename).st_ino
        if member.ino == 0:
            while True:
                member.ino -= 1
                if member.ino not in self.inodemap:
                    break

        if member.ino not in self.inodemap:
            # Make sure we only get the file contents once per inode
            self.inodemap[member.ino] = [member]

            with open(filename, 'rb') as f:
                member.content = f.read()
                member.filesize = len(member.content)

        else:
            self.inodemap[member.ino].append(member)

            member.content = b''
            member.filesize = 0

        fstat = os.stat(filename)

        if perms is not None:
            member.mode = stat.S_IFMT(fstat.st_mode)
            member.mode |= perms
        else:
            member.mode = os.stat(filename).st_mode

        member.uid = fstat.st_uid
        member.gid = fstat.st_gid

        if member.nlink == 0:
            if os.path.isdir(filename):
                member.nlink = 2
            else:
                # Assume hard links don't exist if Python can't get nlink value
                member.nlink = 1
        else:
            member.nlink = fstat.st_nlink

        member.mtime = os.path.getmtime(filename)
        if OS.is_windows():
            member.dev_maj = 0
            member.dev_min = 0
            member.rdev_maj = 0
            member.rdev_min = 0
        else:
            member.dev_maj = os.major(fstat.st_dev)
            member.dev_min = os.minor(fstat.st_dev)
            member.rdev_maj = os.major(fstat.st_rdev)
            member.rdev_min = os.minor(fstat.st_rdev)

        member.name = name
        member.namesize = len(name) + 1

        member.chksum = 0

        self.members.append(member)

    def get_file(self, name):
        for i in self.members:
            if i.name == name:
                return i

        return None
