#!/usr/bin/env python3

import multiboot.ui.androidui as androidui
import multiboot.ui.commandlineui as commandlineui
import multiboot.ui.dummyui as dummyui

import os
import platform

ui             = dummyui

android        = False

rootdir        = ""
ramdiskdir     = ""
patchdir       = ""
binariesdir    = ""
patchinfodir   = ""

mkbootimg      = ""
unpackbootimg  = ""
patch          = ""
cpio           = ""
bash           = ""

runonce        = False

if not runonce:
    runonce = True

    rootdir      = os.path.dirname(os.path.realpath(__file__))
    rootdir      = os.path.join(rootdir, "..", "..")
    ramdiskdir   = os.path.join(rootdir, "ramdisks")
    patchdir     = os.path.join(rootdir, "patches")
    binariesdir  = os.path.join(rootdir, "binaries")
    patchinfodir = os.path.join(rootdir, "patchinfo")

    if os.name == "posix":
        if platform.system() == "Linux":
            # Android
            if 'BOOTCLASSPATH' in os.environ:
                ui            = androidui
                android       = True
                binariesdir   = os.path.join(binariesdir, "android")
                mkbootimg     = os.path.join(binariesdir, "mkbootimg")
                unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
                patch         = os.path.join(binariesdir, "patch")
                cpio          = os.path.join(binariesdir, "cpio")
                patch         = os.path.realpath(patch)

            # Desktop Linux
            else:
                ui            = commandlineui
                binariesdir   = os.path.join(binariesdir, "linux")
                mkbootimg     = os.path.join(binariesdir, "mkbootimg")
                unpackbootimg = os.path.join(binariesdir, "unpackbootimg")
                patch         = "patch"
                cpio          = "cpio"

        elif platform.system() == "Darwin":
            ui                = commandlineui
            binariesdir       = os.path.join(binariesdir, "osx")
            mkbootimg         = os.path.join(binariesdir, "mkbootimg")
            unpackbootimg     = os.path.join(binariesdir, "unpackbootimg")
            patch             = "patch"
            cpio              = "cpio"

        else:
            raise Exception("Unsupported posix system")

    elif os.name == "nt":
        ui                    = commandlineui
        binariesdir           = os.path.join(binariesdir, "windows")
        mkbootimg             = os.path.join(binariesdir, "mkbootimg.exe")
        unpackbootimg         = os.path.join(binariesdir, "unpackbootimg.exe")
        # Windows wants anything named patch.exe to run as Administrator
        patch                 = os.path.join(binariesdir, "hctap.exe")
        cpio                  = os.path.join(binariesdir, "cpio.exe")
        # Windows really sucks
        bash                  = os.path.join(binariesdir, "bash.exe")

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


def is_android():
    return android
