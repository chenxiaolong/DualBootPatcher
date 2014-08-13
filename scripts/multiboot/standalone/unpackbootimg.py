#!/usr/bin/env python3

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

# Python 2 compatibility
from __future__ import print_function

import binascii
import os
import struct
import sys

BOOT_MAGIC = "ANDROID!"
BOOT_MAGIC_SIZE = 8
BOOT_NAME_SIZE = 16
BOOT_ARGS_SIZE = 512


class UnpackbootimgError(Exception):
    pass


def read_padding(f, itemsize, pagesize):
    pagemask = pagesize - 1

    if (itemsize & pagemask) == 0:
        return 0

    count = pagesize - (itemsize & pagemask)

    f.read(count)

    return count


def bytes_to_str(data):
    temp = binascii.hexlify(data).decode("utf-8")
    # return "\\x" + "\\x".join(a + b for a, b in zip(temp[::2], temp[1::2]))
    return "".join(a + b for a, b in zip(temp[::2], temp[1::2]))


show_output = True
use_stdout = False


def print_i(line):
    if show_output:
        if use_stdout:
            print(line)
        else:
            print(line, file=sys.stderr)


def extract(filename, directory):
    basename = os.path.split(filename)[1]
    f = open(filename, 'rb')

    i = 0
    while i <= 512:
        f.seek(i, os.SEEK_SET)
        temp = f.read(BOOT_MAGIC_SIZE)
        if temp == BOOT_MAGIC.encode("ASCII"):
            print_i("Found Android header")
            break
        i += 1

    if i > 512:
        raise UnpackbootimgError("Android header not found")

    # Read Android header
    f.seek(0, os.SEEK_SET)

    sformat = '<'
    # magic
    sformat += str(BOOT_MAGIC_SIZE) + 's'
    # kernel_size, kernel_addr, ramdisk_size, ramdisk_addr,
    # second_size, second_addr, tags_addr, page_size, dt_size, unused
    sformat += '10I'
    # name
    sformat += str(BOOT_NAME_SIZE) + 's'
    # cmdline
    sformat += str(BOOT_ARGS_SIZE) + 's'
    # id (unsigned[8])
    sformat += str(8 * 4) + 's'

    header_size = struct.calcsize(sformat)
    header = f.read(header_size)

    try:
        magic, kernel_size, kernel_addr, \
            ramdisk_size, ramdisk_addr, \
            second_size, second_addr, \
            tags_addr, page_size, \
            dt_size, unused, \
            board, cmdline, ident = struct.unpack(sformat, header)
    except struct.error as e:
        raise UnpackbootimgError(str(e))

    print_i("- magic:        " + magic.decode('ASCII'))
    print_i("- kernel_size:  " + str(kernel_size))
    print_i("- kernel_addr:  " + hex(kernel_addr))
    print_i("- ramdisk_size: " + str(ramdisk_size))
    print_i("- ramdisk_addr: " + hex(ramdisk_addr))
    print_i("- second_size:  " + str(second_size))
    print_i("- second_addr:  " + hex(second_addr))
    print_i("- tags_addr:    " + hex(tags_addr))
    print_i("- page_size:    " + str(page_size))
    print_i("- dt_size:      " + str(dt_size))
    print_i("- unused:       " + hex(unused))
    print_i("- name:         " + board.decode('ASCII'))
    print_i("- cmdline:      " + cmdline.decode('ASCII'))
    print_i("- id:           " + bytes_to_str(ident)[:40])  # SHA1 is 40 chars

    print_i("")

    # cmdline
    out = open(os.path.join(directory, basename + "-cmdline"), 'wb')
    out.write((cmdline.decode('ASCII').rstrip('\0') + '\n').encode('ASCII'))
    out.close()

    # base
    out = open(os.path.join(directory, basename + "-base"), 'wb')
    out.write(('%08x\n' % (kernel_addr - 0x00008000)).encode('ASCII'))
    out.close()

    # ramdisk_offset
    out = open(os.path.join(directory, basename + "-ramdisk_offset"), 'wb')
    out.write(('%08x\n' % (ramdisk_addr - kernel_addr + 0x00008000)).encode('ASCII'))
    out.close()

    # second_offset
    out = open(os.path.join(directory, basename + "-second_offset"), 'wb')
    out.write(('%08x\n' % (second_addr - kernel_addr + 0x00008000)).encode('ASCII'))
    out.close()

    # tags_offset
    out = open(os.path.join(directory, basename + "-tags_offset"), 'wb')
    out.write(('%08x\n' % (tags_addr - kernel_addr + 0x00008000)).encode('ASCII'))
    out.close()

    # page_size
    out = open(os.path.join(directory, basename + "-pagesize"), 'wb')
    out.write(('%d\n' % page_size).encode('ASCII'))
    out.close()

    read_padding(f, header_size, page_size)

    # zImage
    out = open(os.path.join(directory, basename + "-zImage"), 'wb')
    out.write(f.read(kernel_size))
    out.close()

    read_padding(f, kernel_size, page_size)

    # ramdisk
    ramdisk = f.read(ramdisk_size)
    if ramdisk[0] == 0x02 and ramdisk[1] == 0x21:
        out = open(os.path.join(directory, basename + "-ramdisk.lz4"), 'wb')
    else:
        out = open(os.path.join(directory, basename + "-ramdisk.gz"), 'wb')
    out.write(ramdisk)
    out.close()

    read_padding(f, ramdisk_size, page_size)

    # second
    out = open(os.path.join(directory, basename + "-second"), 'wb')
    out.write(f.read(second_size))
    out.close()

    read_padding(f, second_size, page_size)

    # dt
    out = open(os.path.join(directory, basename + "-dt"), 'wb')
    out.write(f.read(dt_size))
    out.close()

    f.close()


def main():
    if len(sys.argv) < 3:
        print_i("Usage: %s -i boot.img [-o output_directory]" % sys.argv[0])
        sys.exit(1)

    filename = ""
    directory = ""

    counter = 1
    while counter < len(sys.argv):
        if sys.argv[counter] == "-i":
            filename = sys.argv[counter + 1]
            counter += 2
        elif sys.argv[counter] == "-o":
            directory = sys.argv[counter + 1]
            counter += 2
        else:
            print_i("Unrecognized argument " + sys.argv[counter])
            sys.exit(1)

    if filename == "":
        print_i("No boot image specified")
        sys.exit(1)

    if not os.path.exists(filename):
        print_i(filename + " does not exist")
        sys.exit(1)

    if directory == "":
        directory = os.getcwd()
    elif not os.path.exists(directory):
        os.makedirs(directory)

    global use_stdout

    try:
        use_stdout = True
        extract(filename, directory)
    except (OSError, UnpackbootimgError) as e:
        use_stdout = False
        print_i("Failed: " + str(e))


if __name__ == "__main__":
    main()