#!/usr/bin/env python3

# Python 2 compatibility
from __future__ import print_function

import binascii
import gzip
import os
import sys
import zlib

BOOT_MAGIC = "ANDROID!"
BOOT_MAGIC_SIZE = 8
BOOT_NAME_SIZE = 16
BOOT_ARGS_SIZE = 512

class boot_img_hdr:
  def __init__(self):
    self.magic        = None

    self.kernel_size  = None  # size in bytes
    self.kernel_addr  = None  # physical load addr

    self.ramdisk_size = None  # size in bytes
    self.ramdisk_addr = None  # physical load addr

    self.second_size  = None  # size in bytes
    self.second_addr  = None  # physical load addr

    self.tags_addr    = None  # physical addr for kernel tags
    self.page_size    = None  # flash page size we assume
    self.unused       = None  # future expansion: should be 0

    self.name         = None  # asciiz product name

    self.cmdline      = None

    self.id           = None  # timestamp / checksum / sha1 / etc

class loki_hdr:
  def __init__(self):
    self.magic = None         # 0x494b4f4c
    self.recovery = 0         # 0 = boot.img, 1 = recovery.img
    self.build = None         # Build number

    self.orig_kernel_size = 0
    self.orig_ramdisk_size = 0
    self.ramdisk_addr = 0

def to_int(data):
  # int.from_bytes() is available in Python >= 3.2
  if sys.hexversion < 0x03020000 and sys.hexversion >= 0x03000000:
    pass
  elif sys.hexversion < 0x03000000:
    import struct
    return struct.unpack("<L", data)[0]
  else:
    return int.from_bytes(data, byteorder = 'little', signed = False)

def bytes_to_str(data):
  temp = binascii.hexlify(data).decode("utf-8")
  return "\\x" + "\\x".join(a + b for a, b in zip(temp[::2], temp[1::2]))

show_output = True
use_stdout = False
def print_i(line):
  if show_output:
    if use_stdout:
      print(line)
    else:
      print(line, file = sys.stderr)

def extract(filename, directory):
  basename = os.path.split(filename)[1]
  f = open(filename, 'rb')

  i = 0
  while i <= 32:
    f.seek(i, os.SEEK_SET)
    temp = f.read(BOOT_MAGIC_SIZE)
    if temp == BOOT_MAGIC.encode("ASCII"):
      print_i("Found Android header")
      break;
    i += 1

  if i > 512:
    raise Exception("Android header not found")

  # Read Android header
  f.seek(0, os.SEEK_SET)
  a_hdr = boot_img_hdr()

  a_hdr.magic        = f.read(BOOT_MAGIC_SIZE)
  a_hdr.kernel_size  = to_int(f.read(4))
  a_hdr.kernel_addr  = to_int(f.read(4))
  a_hdr.ramdisk_size = to_int(f.read(4))
  a_hdr.ramdisk_addr = to_int(f.read(4))
  a_hdr.second_size  = to_int(f.read(4))
  a_hdr.second_addr  = to_int(f.read(4))
  a_hdr.tags_addr    = to_int(f.read(4))
  a_hdr.page_size    = to_int(f.read(4))
  a_hdr.unused       = f.read(8)
  a_hdr.name         = f.read(BOOT_NAME_SIZE)
  a_hdr.cmdline      = f.read(BOOT_ARGS_SIZE)
  a_hdr.id           = f.read(8)
  print_i("- magic:        " + a_hdr.magic.decode('ascii'))
  print_i("- kernel_size:  " + str(a_hdr.kernel_size))
  print_i("- kernel_addr:  " + hex(a_hdr.kernel_addr))
  print_i("- ramdisk_size: " + str(a_hdr.ramdisk_size))
  print_i("- ramdisk_addr: " + hex(a_hdr.ramdisk_addr))
  print_i("- second_size:  " + str(a_hdr.second_size))
  print_i("- second_addr:  " + hex(a_hdr.second_addr))
  print_i("- tags_addr:    " + hex(a_hdr.tags_addr))
  print_i("- page_size:    " + str(a_hdr.page_size))
  print_i("- unused:       " + bytes_to_str(a_hdr.unused))
  print_i("- name:         " + a_hdr.name.decode('ascii'))
  print_i("- cmdline:      " + a_hdr.cmdline.decode('ascii'))
  print_i("- id:           " + bytes_to_str(a_hdr.id))

  print_i("")

  # Look for loki header
  f.seek(0x400, os.SEEK_SET)
  temp = f.read(4)
  if temp == "LOKI".encode("ASCII"):
    print_i("Found Loki header")
  else:
    raise Exception("Could not find Loki header")

  # Read Loki header
  f.seek(0x400, os.SEEK_SET)
  l_hdr = loki_hdr()

  l_hdr.magic             = f.read(4)
  l_hdr.recovery          = to_int(f.read(4))
  l_hdr.build             = f.read(128)
  l_hdr.orig_kernel_size  = to_int(f.read(4))
  l_hdr.orig_ramdisk_size = to_int(f.read(4))
  l_hdr.ramdisk_addr      = to_int(f.read(4))
  print_i("- magic:             " + l_hdr.magic.decode('ascii'))
  print_i("- recovery:          " + str(l_hdr.recovery))
  print_i("- build:             " + l_hdr.build.decode('ascii'))
  print_i("- orig_kernel_size:  " + str(l_hdr.orig_kernel_size))
  print_i("- orig_ramdisk_size: " + str(l_hdr.orig_ramdisk_size))
  print_i("- ramdisk_addr:      " + hex(l_hdr.ramdisk_addr))

  print_i("")

  # Get total size
  f.seek(0, os.SEEK_END)
  total_size = f.tell()

  # Find ramdisk's gzip header
  gzip_offset = 0x400 + 148 # size of Loki header
  while gzip_offset < total_size:
    f.seek(gzip_offset, os.SEEK_SET)
    temp = f.read(4)
    if temp == b"\x1F\x8B\x08\x00":
      print_i("Found gzip header at: " + hex(gzip_offset))
      break
    gzip_offset += 4

  # The ramdisk is supposed to be from the gzip header to EOF, but loki needs
  # to store a copy of aboot, so it is stored in 0x200 bytes are the end, which
  # Dan Rosenberg calls the "fake" area.
  gzip_size = total_size - gzip_offset - 0x200
  # The line above is for Samsung kernels only. Uncomment this one for use on
  # LG kernels:
  #gzip_size = total_size - gzip_offset - a_hdr.page_size
  print_i("Ramdisk size: %i (may include some padding)" % gzip_size)

  # Get kernel size
  # Unfortunately, the original size is not stored in the Loki header on
  # kernels for Samsung devices and we can't reverse Dan Rosenberg's expression
  # because data is loss with the bitwise and:
  # hdr->kernel_size = ((hdr->kernel_size + page_mask) & ~page_mask) + hdr->ramdisk_size;
  # Luckily, the zimage headers stores the size at 0x2c, so we can get it from
  # there. (http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309)
  f.seek(a_hdr.page_size + 0x2c, os.SEEK_SET)
  kernel_size = to_int(f.read(4))
  print_i("Kernel size: " + str(kernel_size))

  print_i("")

  print_i("Writing kernel command line to %s-cmdline ..." % basename)
  out = open(os.path.join(directory, basename + "-cmdline"), 'wb')
  out.write((a_hdr.cmdline.decode("ascii").rstrip('\0') + "\n").encode("ascii"))
  out.close()

  print_i("Writing base address to %s-base ..." % basename)
  out = open(os.path.join(directory, basename + "-base"), 'wb')
  # 0x00008000 is the same constant used in unpackbootimg.c
  out.write((hex(a_hdr.kernel_addr - 0x00008000).lstrip("0x") + "\n").encode("ascii"))
  out.close()

  print_i("Writing page size to %s-pagesize ..." % basename)
  out = open(os.path.join(directory, basename + "-pagesize"), 'wb')
  out.write((str(a_hdr.page_size) + "\n").encode("ascii"))
  out.close()

  print_i("Writing kernel to %s-zImage ..." % basename)
  f.seek(a_hdr.page_size, os.SEEK_SET)
  kernel_data = f.read(kernel_size)
  out = open(os.path.join(directory, basename + "-zImage"), 'wb')
  out.write(kernel_data)
  out.close()

  # Truncate padding by recompression. Due to a bug in Python's gzip module, we
  # can't use it (see this question: http://stackoverflow.com/questions/4928560/how-can-i-work-with-gzip-files-which-contain-extra-data)
  print_i("Writing ramdisk to %s-ramdisk.gz ..." % basename)
  f.seek(gzip_offset, os.SEEK_SET)
  ramdisk_data = f.read(gzip_size)
  out = gzip.open(os.path.join(directory, basename + "-ramdisk.gz"), 'wb')
  out.write(zlib.decompress(ramdisk_data, 16 + zlib.MAX_WBITS))
  out.close()

  f.close()

  print_i("")
  print_i("Successfully unloki'd " + filename)

if __name__ == "__main__":
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
    print_i("No loki image specified")
    sys.exit(1)

  if not os.path.exists(filename):
    print_i(filename + " does not exist")
    sys.exit(1)

  if directory == "":
    directory = os.getcwd()
  elif not os.path.exists(directory):
    os.makedirs(directory)

  try:
    use_stdout = True
    extract(filename, directory)
  except Exception as e:
    use_stdout = False
    print_i("Failed: " + str(e))
