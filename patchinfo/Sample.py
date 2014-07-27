# This file describes the ROM/Kernel/etc with the following information:
#   - Pattern in file name
#   - Patch to use
#   - Type of ramdisk (if any)
#   - Message to display to user
#
# Please copy this file to a new one before editing.

from multiboot.autopatchers.standard import StandardPatcher
from multiboot.patchinfo import PatchInfo
import re

patchinfo = PatchInfo()

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
patchinfo.matches        = r"^.*SuperSU.*\.zip$"

# This is the name of the ROM, kernel, or other zip file.
patchinfo.name           = 'Name goes here'

# This is the type of ramdisk. Run the 'list-ramdisks' file in the useful/
# folder to see what choices are available. (It's pretty obvious, you'll see)
patchinfo.ramdisk        = 'jflte/AOSP/AOSP.def'

# If the zip file you're patching does not have a kernel, set this to false.
patchinfo.has_boot_image = True

# The zip files are automatically searched for boot images by default and will
# find all boot.img and boot.lok files. If you would like to specify the boot
# image manually, change the variable below. If there are multiple boot images,
# a list can be provided (eg. ['boot1.img', 'boot2.img'])
#patchinfo.bootimg        = 'boot.img'

# This line enables the autopatcher. In most cases, this is sufficient.
patchinfo.autopatchers   = [StandardPatcher]
# If, for whatever reason, the autopatcher doesn't work, uncomment this line,
# copy your patch to patches/ and put the patch here.
#patchinfo.autopatchers   = ['jflte/AOSP/YourROM.patch']

### Advanced stuff ###

# If you need to customize how the filename is matched, add a function named
# matches(filename) that returns true or false and set patchinfo.matches to it.
# Note that 'filename' contains the path too. For example:
#
# def matches(filename):
#     filename = os.path.split(filename)[1]
#     if ...:
#         return True
#     else:
#         return False
#
# patchinfo.matches = matches

# If you need to customize the PatchInfo based on the filename, add a function
# with parameters (patchinfo, filename) and set patchinfo.on_filename_set to
# it. Like above, the 'filename' variable contains the path. For example:
#
# def on_filename_set(patchinfo, filename):
#     filename = os.path.split(filename)[1]
#     if '1.0' in filename:
#         # do something ...
#         patchinfo.bootimg = 'kernel/blah.img'
#
# patchinfo.on_filename_set = on_filename_set
