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

import argparse
import hashlib
import os
import struct
import sys
import textwrap

BOOT_MAGIC = "ANDROID!"
BOOT_MAGIC_SIZE = 8
BOOT_NAME_SIZE = 16
BOOT_ARGS_SIZE = 512


class MkbootimgError(Exception):
    pass


def write_padding(f, pagesize, itemsize):
    pagemask = pagesize - 1

    if (itemsize & pagemask) == 0:
        return 0

    count = pagesize - (itemsize & pagemask)

    if sys.hexversion < 0x03000000:
        f.write(struct.pack('<' + str(count) + 's', '\0' * 131072))
    else:
        f.write(struct.pack('<' + str(count) + 's', bytes(0x00) * 131072))


show_output = True
use_stdout = False


def print_i(line):
    if show_output:
        if use_stdout:
            print(line)
        else:
            print(line, file=sys.stderr)


def build(filename, board=None, base=None, cmdline=None, page_size=None,
          kernel_offset=None, ramdisk_offset=None, second_offset=None,
          tags_offset=None, kernel=None, ramdisk=None, second=None, dt=None):
    # Defaults
    if board is None:
        board = ""
    if cmdline is None:
        cmdline = ""
    if base is None:
        base = 0x10000000
    if kernel_offset is None:
        kernel_offset = 0x00008000
    if ramdisk_offset is None:
        ramdisk_offset = 0x01000000
    if second_offset is None:
        second_offset = 0x00f00000
    if tags_offset is None:
        tags_offset = 0x00000100

    kernel_addr = base + kernel_offset
    ramdisk_addr = base + ramdisk_offset
    second_addr = base + second_offset
    tags_addr = base + tags_offset

    if len(board) >= BOOT_NAME_SIZE:
        raise ValueError("Board name too large")

    if len(cmdline) > BOOT_ARGS_SIZE - 1:
        raise ValueError("Kernel command line too large")

    f = open(kernel, 'rb')
    kernel_data = f.read()
    f.close()
    kernel_size = len(kernel_data)

    f = open(ramdisk, 'rb')
    ramdisk_data = f.read()
    f.close()
    ramdisk_size = len(ramdisk_data)

    second_data = None
    second_size = 0
    if second and os.path.exists(second):
        f = open(second, 'rb')
        second_data = f.read()
        f.close()
        second_size = len(second_data)

    dt_data = None
    dt_size = 0
    if dt and os.path.exists(dt):
        f = open(dt, 'rb')
        dt_data = f.read()
        f.close()
        dt_size = len(dt_data)

    try:
        sha = hashlib.sha1()
        sha.update(kernel_data)
        sha.update(struct.pack('<I', kernel_size))
        sha.update(ramdisk_data)
        sha.update(struct.pack('<I', ramdisk_size))
        if second_data:
            sha.update(second_data)
            sha.update(struct.pack('<I', second_size))
        if dt_data:
            sha.update(dt_data)
            sha.update(struct.pack('<I', dt_size))
    except struct.error as e:
        raise MkbootimgError(str(e))

    f = open(filename, 'wb')

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

    try:
        f.write(struct.pack(
            sformat,
            BOOT_MAGIC.encode('ascii'),  # magic
            kernel_size,                 # kernel_size
            kernel_addr,                 # kernel_addr
            ramdisk_size,                # ramdisk_size
            ramdisk_addr,                # ramdisk_addr
            second_size,                 # second_size
            second_addr,                 # second_addr
            tags_addr,                   # tags_addr
            page_size,                   # page_size
            dt_size,                     # dt_size
            0,                           # unused
            board.encode('ascii'),       # name
            cmdline.encode('ascii'),     # cmdline
            sha.digest()                 # id
        ))

    except struct.error as e:
        raise MkbootimgError('Failed to pack arguments: ' + str(e))

    write_padding(f, page_size, struct.calcsize(sformat))

    f.write(kernel_data)
    write_padding(f, page_size, len(kernel_data))

    f.write(ramdisk_data)
    write_padding(f, page_size, len(ramdisk_data))

    if second_data:
        f.write(second_data)
        write_padding(f, page_size, len(second_data))

    if dt_data:
        f.write(dt_data)
        write_padding(f, page_size, len(dt_data))

    f.close()


def auto_from_dir(output, directory, prefix):
    prefix = os.path.join(directory, prefix)

    base = None
    if os.path.exists(prefix + '-base'):
        f = open(prefix + '-base', 'rb')
        base = int(f.readline().decode('ASCII').rstrip('\n'), 16)
        f.close()

    cmdline = None
    if os.path.exists(prefix + '-cmdline'):
        f = open(prefix + '-cmdline', 'rb')
        cmdline = f.readline().decode('ASCII').rstrip('\n')
        f.close()

    page_size = None
    if os.path.exists(prefix + '-pagesize'):
        f = open(prefix + '-pagesize', 'rb')
        page_size = int(f.readline().decode('ASCII').rstrip('\n'))
        f.close()

    ramdisk_offset = None
    if os.path.exists(prefix + '-ramdisk_offset'):
        f = open(prefix + '-ramdisk_offset', 'rb')
        ramdisk_offset = int(f.readline().decode('ASCII').rstrip('\n'), 16)
        f.close()

    second_offset = None
    if os.path.exists(prefix + '-second_offset'):
        f = open(prefix + '-second_offset', 'rb')
        second_offset = int(f.readline().decode('ASCII').rstrip('\n'), 16)
        f.close()

    tags_offset = None
    if os.path.exists(prefix + '-tags_offset'):
        f = open(prefix + '-tags_offset', 'rb')
        tags_offset = int(f.readline().decode('ASCII').rstrip('\n'), 16)
        f.close()

    kernel = None
    if os.path.exists(prefix + '-zImage'):
        kernel = prefix + '-zImage'
    else:
        raise FileNotFoundError(prefix + '-zImage')

    ramdisk = None
    if os.path.exists(prefix + '-ramdisk.gz'):
        ramdisk = prefix + '-ramdisk.gz'
    elif os.path.exists(prefix + '-ramdisk.lz4'):
        ramdisk = prefix + '-ramdisk.lz4'
    else:
        raise FileNotFoundError(prefix + '-ramdisk.{gz,lz4}')

    second = None
    if os.path.exists(prefix + '-second'):
        second = prefix + '-second'

    dt = None
    if os.path.exists(prefix + '-dt'):
        dt = prefix + '-dt'

    build(output,
          board=None,
          base=base,
          cmdline=cmdline,
          page_size=page_size,
          kernel_offset=None,
          ramdisk_offset=ramdisk_offset,
          second_offset=second_offset,
          tags_offset=tags_offset,
          kernel=kernel,
          ramdisk=ramdisk,
          second=second,
          dt=dt)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.formatter_class = argparse.RawDescriptionHelpFormatter
    parser.description = textwrap.dedent('''
    mkbootimg - Create Android boot images
    --------------------------------------
    ''')

    parser.add_argument('-o', '--output',
                        help='Path to resulting boot image',
                        required=True)

    manualmode = parser.add_argument_group('Manual mode')
    manualmode.add_argument('--board',
                            help='ASCII product board name')
    manualmode.add_argument('--base',
                            help='Base address that the offsets are added to')
    manualmode.add_argument('--cmdline',
                            help='Kernel command line')
    manualmode.add_argument('--pagesize',
                            help='Page size of the flash memory')
    manualmode.add_argument('--kernel_offset',
                            help='Physical address for the kernel')
    manualmode.add_argument('--ramdisk_offset',
                            help='Physical address for the ramdisk')
    manualmode.add_argument('--second_offset',
                            help='Physical address for second bootloader')
    manualmode.add_argument('--tags_offset',
                            help='Physical address for kernel tags')
    manualmode.add_argument('--kernel',
                            help='Path to kernel image',
                            required=True)
    manualmode.add_argument('--ramdisk',
                            help='Path to ramdisk image',
                            required=True)
    manualmode.add_argument('--second',
                            help='Path to second bootloader image')
    manualmode.add_argument('--dt',
                            help='Path to device tree image')

    automode = parser.add_argument_group('Automatic mode')
    automode.add_argument('--auto_dir',
                          help='Directory containing extracted boot image')
    automode.add_argument('--auto_prefix',
                          help='Filename prefix of the extracted files')

    args = parser.parse_args()

    board = args.board
    base = args.base
    cmdline = args.cmdline
    page_size = args.pagesize
    kernel_offset = args.kernel_offset
    ramdisk_offset = args.ramdisk_offset
    second_offset = args.second_offset
    tags_offset = args.tags_offset
    kernel = args.kernel
    ramdisk = args.ramdisk
    second = args.second
    dt = args.dt
    output = args.output

    auto_dir = args.auto_dir
    auto_base = args.auto_prefix

    if (auto_dir and not auto_base) or (auto_base and not auto_dir):
        print_i('Automatic mode used, but second argument is missing')
        sys.exit(1)

    global use_stdout

    try:
        use_stdout = True

        if auto_dir and auto_base:
            auto_from_dir(output, auto_dir, auto_base)

        else:
            if base:
                base = int(base, 16)
            if page_size:
                page_size = int(page_size)
            if kernel_offset:
                kernel_offset = int(kernel_offset, 16)
            if ramdisk_offset:
                ramdisk_offset = int(ramdisk_offset, 16)
            if second_offset:
                second_offset = int(second_offset, 16)
            if tags_offset:
                tags_offset = int(tags_offset, 16)

            build(output,
                  board=board,
                  base=base,
                  cmdline=cmdline,
                  page_size=page_size,
                  kernel_offset=kernel_offset,
                  ramdisk_offset=ramdisk_offset,
                  second_offset=second_offset,
                  tags_offset=tags_offset,
                  kernel=kernel,
                  ramdisk=ramdisk,
                  second=second,
                  dt=dt)

    except OSError as e:
        use_stdout = False
        print_i('Failed: ' + str(e))

    except MkbootimgError as e:
        use_stdout = False
        print_i(str(e) +
                ' (Most likely a parameter or file is missing or incorrect)')


if __name__ == "__main__":
    parse_args()
