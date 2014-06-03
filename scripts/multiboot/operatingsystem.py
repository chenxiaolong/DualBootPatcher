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

import multiboot.ui.androidui as androidui
import multiboot.ui.commandlineui as commandlineui
import multiboot.ui.dummyui as dummyui

import os
import platform
import sys

ui           = dummyui

android      = False

rootdir      = ""
ramdiskdir   = ""
patchdir     = ""
binariesdir  = ""
patchinfodir = ""

patch        = ""

runonce      = False

if not runonce:
    runonce = True

    rootdir      = os.path.dirname(os.path.realpath(__file__))
    rootdir      = os.path.join(rootdir, "..", "..")
    ramdiskdir   = os.path.join(rootdir, "ramdisks")
    patchdir     = os.path.join(rootdir, "patches")
    binariesdir  = os.path.join(rootdir, "binaries")
    patchinfodir = os.path.join(rootdir, "patchinfo")

    sys.path.append(rootdir)

    if os.name == "posix":
        if platform.system() == "Linux":
            # Android
            if 'BOOTCLASSPATH' in os.environ:
                ui          = androidui
                android     = True
                binariesdir = os.path.join(binariesdir, "android")
                patch       = os.path.join(binariesdir, "patch")
                patch       = os.path.realpath(patch)

            # Desktop Linux
            else:
                ui          = commandlineui
                patch       = "patch"

        elif platform.system() == "Darwin":
            ui              = commandlineui
            patch           = "patch"

        else:
            raise Exception("Unsupported posix system")

    elif os.name == "nt":
        ui                  = commandlineui
        binariesdir         = os.path.join(binariesdir, "windows")
        # Windows wants anything named patch.exe to run as Administrator
        patch               = os.path.join(binariesdir, "hctap.exe")

    else:
        raise Exception("Unsupported operating system")

    if 'ANDROID_DEBUG' in os.environ:
        ui = androidui
        android = True


def is_windows():
    if os.name == "nt":
        return True
    else:
        return False


def is_linux():
    if os.name == "posix" and platform.system() == "Linux":
        return True
    else:
        return False


def is_darwin():
    if os.name == "posix" and platform.system() == "Linux":
        return True
    else:
        return False


def is_android():
    return android
