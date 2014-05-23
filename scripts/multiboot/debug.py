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

import multiboot.operatingsystem as OS

import os
import sys


def is_debug():
    if OS.is_android() \
            or 'PATCHER_DEBUG' in os.environ \
            and os.environ['PATCHER_DEBUG'] == 'true':
        return True
    else:
        return False


def debug(msg):
    # Send to stderr on Android, don't show on PC
    if is_debug():
        print(msg, file=sys.stderr)
