#!/usr/bin/env python3

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
