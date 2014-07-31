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
import os
import re
import shutil
import subprocess
import sys
import textwrap

sys.dont_write_bytecode = True

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

from multiboot.patcher import SyncdaemonPatcher
import multiboot.fileinfo as fileinfo


ui = OS.ui
filename = None


def parse_args():
    global filename

    parser = argparse.ArgumentParser()
    parser.formatter_class = argparse.RawDescriptionHelpFormatter
    parser.description = textwrap.dedent('''
    syncdaemon updater
    ------------------
    Takes any boot image and installs or updates syncdaemon in it
    ''')

    parser.add_argument('file',
                        help='The file to patch',
                        action='store')

    args = parser.parse_args()

    filename = args.file


def start_patching():
    file_info = fileinfo.FileInfo()
    file_info.filename = filename
    # file_info.device = 'jflte'

    patcher = SyncdaemonPatcher(file_info)
    newfile = patcher.start_patching()

    if not newfile:
        sys.exit(1)

    fileext = os.path.splitext(filename)
    base = fileext[0] + '_syncdaemon'
    targetfile = base + fileext[1]

    shutil.copyfile(newfile, targetfile)
    os.remove(newfile)

    if not OS.is_android():
        ui.info("Path: " + targetfile)


try:
    parse_args()
    start_patching()
except Exception as e:
    ui.failed(str(e))
    sys.exit(1)
