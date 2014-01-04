#!/usr/bin/env python3

# Python 2 compatibility
from __future__ import print_function

import binascii
import gzip
import hashlib
import os
import struct
import sys
import zlib

BOOT_MAGIC = "ANDROID!"
BOOT_MAGIC_SIZE = 8
BOOT_NAME_SIZE = 16
BOOT_ARGS_SIZE = 512


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
    if board is None:          board          = ""
    if cmdline is None:        cmdline        = ""
    if base is None:           base           = 0x10000000
    if kernel_offset is None:  kernel_offset  = 0x00008000
    if ramdisk_offset is None: ramdisk_offset = 0x01000000
    if second_offset is None:  second_offset  = 0x00f00000
    if tags_offset is None:    tags_offset    = 0x00000100

    kernel_addr  = base + kernel_offset
    ramdisk_addr = base + ramdisk_offset
    second_addr  = base + second_offset
    tags_addr    = base + tags_offset

    if len(board) >= BOOT_NAME_SIZE:
        raise Exception("Board name too large")

    if len(cmdline) > BOOT_ARGS_SIZE - 1:
        raise Exception("Kernel command line too large")

    kernel_data = None
    kernel_size = 0
    f = open(kernel, 'rb')
    kernel_data = f.read()
    f.close()
    kernel_size = len(kernel_data)

    ramdisk_data = None
    ramdisk_size = 0
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

    f = open(filename, 'wb')

    sformat = '<'
    sformat += str(BOOT_MAGIC_SIZE) + 's'  # magic
    sformat += '10I'                       # kernel_size, kernel_addr,
                                           # ramdisk_size, ramdisk_addr,
                                           # second_size, second_addr,
                                           # tags_addr, page_size,
                                           # dt_size, unused
    sformat += str(BOOT_NAME_SIZE) + 's'   # name
    sformat += str(BOOT_ARGS_SIZE) + 's'   # cmdline
    sformat += str(8 * 4) + 's'            # id (unsigned[8])

    f.write(struct.pack(
        sformat,
        BOOT_MAGIC.encode('ascii'), # magic
        kernel_size,                # kernel_size
        kernel_addr,                # kernel_addr
        ramdisk_size,               # ramdisk_size
        ramdisk_addr,               # ramdisk_addr
        second_size,                # second_size
        second_addr,                # second_addr
        tags_addr,                  # tags_addr
        page_size,                  # page_size
        dt_size,                    # dt_size
        0,                          # unused
        board.encode('ascii'),      # name
        cmdline.encode('ascii'),    # cmdline
        sha.digest()                # id
    ))

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


def auto_from_dir(directory, prefix):
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

    ramdisk = None
    if os.path.exists(prefix + '-ramdisk.gz'):
        ramdisk = prefix + '-ramdisk.gz'
    elif os.path.exists(prefix + '-ramdisk.lz4'):
        ramdisk = prefix + '-ramdisk.lz4'

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


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print_i("Usage: %s" % sys.argv[0])
        print_i("       -o|--output <filename>")
        print_i("")
        print_i("Manual mode:")
        print_i("       [ --board <name> ]")
        print_i("       [ --base <address> ]")
        print_i("       [ --cmdline <kernel-cmdline> ]")
        print_i("       [ --pagesize <size> ]")
        print_i("       [ --kernel_offset <address> ]")
        print_i("       [ --ramdisk_offset <address> ]")
        print_i("       [ --second_offset <address> ]")
        print_i("       [ --tags_offset <address> ]")
        print_i("       --kernel <filename>")
        print_i("       --ramdisk <filename>")
        print_i("       [ --second <filename> ]")
        print_i("       [ --dt <filename> ]")
        print_i("")
        print_i("Automatic mode:")
        print_i("       --auto_dir <directory>")
        print_i("       --auto_prefix <prefix>")
        sys.exit(1)

    board = None
    base = None
    cmdline = None
    page_size = None
    kernel_offset = None
    ramdisk_offset = None
    second_offset = None
    tags_offset = None
    kernel = None
    ramdisk = None
    second = None
    dt = None
    output = None

    auto_dir = None
    auto_base = None

    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '--board':
            board = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--base':
            base = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--cmdline':
            cmdline = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--pagesize':
            page_size = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--kernel_offset':
            kernel_offset = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--ramdisk_offset':
            ramdisk_offset = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--second_offset':
            second_offset = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--tags_offset':
            tags_offset = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--kernel':
            kernel = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--ramdisk':
            ramdisk = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--second':
            second = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--dt':
            dt = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '-o' or sys.argv[i] == '--output':
            output = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--auto_dir':
            auto_dir = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--auto_prefix':
            auto_base = sys.argv[i + 1]
            i += 2
        else:
            print_i("Unrecognized argument " + sys.argv[i])
            sys.exit(1)

    if not output:
        print_i("No output filename specified")
        sys.exit(1)

    if (auto_dir and not auto_base) or (auto_base and not auto_dir):
        print_i("Automatic mode used, but second argument is missing")
        sys.exit(1)

    try:
        use_stdout = True

        if auto_dir and auto_base:
            auto_from_dir(auto_dir, auto_base)

        else:
            if not kernel:
                print_i("No kernel specified")
                sys.exit(1)

            if not ramdisk:
                print_i("No ramdisk specified")
                sys.exit(1)

            if base:           base           = int(base, 16)
            if page_size:      page_size      = int(page_size)
            if kernel_offset:  kernel_offset  = int(kernel_offset, 16)
            if ramdisk_offset: ramdisk_offset = int(ramdisk_offset, 16)
            if second_offset:  second_offset  = int(second_offset, 16)
            if tags_offset:    tags_offset    = int(tags_offset, 16)

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
    except Exception as e:
        use_stdout = False
        print_i("Failed: " + str(e))
