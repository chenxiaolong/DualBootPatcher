# This file describes the ROM/Kernel/etc with the following information:
#   - Pattern in file name
#   - Patch to use
#   - Type of ramdisk (if any)
#   - Message to display to user
#
# Please copy this file to a new one before editing.

from fileinfo import FileInfo
import multiboot.autopatcher as autopatcher
import re

file_info = FileInfo()

# This is the regular expression for detecting a ROM using the filename. Here
# are some basic rules for how regex's work:
# Pattern        | Meaning
# -----------------------------------------------
# ^              | Beginning of filename
# $              | End of filename
# .              | Any character
# \.             | Period
# [a-z]          | Lowercase English letters
# [a-zA-Z]       | All English letters
# [0-9]          | Numbers
# *              | 0 or more of previous pattern
# +              | 1 or more of previous pattern

# We'll use the SuperSU zip as an example.
#
# Filename: UPDATE-SuperSU-v1.65.zip
# Pattern: ^UPDATE-SuperSU-v[0-9\.]+\.zip$
#
# So, when we go patch a file, if we see "UPDATE-SuperSU-v" at the beginning,
# followed by 1 or more of either numbers or a period and then ".zip", then we
# know it's a SuperSU zip. Of course, a simpler pattern like ^.*SuperSU.*\.zip$
# would work just as well.
filename_regex           = r"^.*SuperSU.*\.zip$"

# This is the name of the ROM, kernel, or other zip file.
file_info.name           = 'Name goes here'

# This is the type of ramdisk. Run the 'list-ramdisks' file in the useful/
# folder to see what choices are available. (It's pretty obvious, you'll see)
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'

# If the zip file you're patching does not have a kernel, set this to false.
file_info.has_boot_image = True

# The zip files are automatically searched for boot images by default and will
# find all boot.img and boot.lok files. If you would like to specify the boot
# image manually, change the variable below. If there are multiple boot images,
# a list can be provided (eg. ['boot1.img', 'boot2.img'])
#file_info.bootimg        = 'boot.img'

# These two lines enable the autopatcher. In most cases, this is sufficient.
file_info.patch          = autopatcher.auto_patch
file_info.extract        = autopatcher.files_to_auto_patch
# If, for whatever reason, the autopatcher doesn't work, uncomment this line,
# copy your patch to patches/ and put the patch here.
#file_info.patch          = 'jflte/AOSP/YourROM.patch'

### Advanced stuff ###

# If you need to customize how the filename is matched, add a function named
# matches(filename) that returns true or false. Note that 'filename' contains
# the path too. For example:
#
# def matches(filename):
#     filename = os.path.split(filename)[1]
#     if ...:
#         return True
#     else:
#         return False

# If you need to customize the FileInfo based on the filename (eg. setting
# the loki parameter based on the filename), add a function named
# get_file_info(filename) that returns the new file FileInfo object. Like
# above, the 'filename' variable contains the path as well. For example:
#
# def get_file_info(filename):
#     filename = os.path.split(filename)[1]
#     if 'ATT' in filename or 'VZW' in filename:
#         file_info.loki = True
#     return file_info
