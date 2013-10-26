# This file describes the ROM/Kernel/etc with the following information:
#   - Pattern in file name
#   - Patch to use
#   - Type of ramdisk (if any)
#   - Message to display to user
#
# Please copy this file to a new one before editing.

from fileinfo import FileInfo
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

# This is the type of ramdisk. Run the 'list-ramdisks' file in the useful/
# folder to see what choices are available. (It's pretty obvious, you'll see)
file_info.ramdisk        = 'jflte/AOSP/AOSP.def'

# If the zip file you're patching does not have a kernel, set this to false.
file_info.has_boot_image = True

# This is the patch file you generated. Just copy the patch into a subfolder in
# patches/ and put the path here.
file_info.patch          = 'jflte/AOSP/YourROM.patch'

def print_message():
  # This is the message that is shown if the file to be patched is this one.
  print("Detected The Name of Some ROM")

###

def matches(filename):
  if re.search(filename_regex, filename):
    return True
  else:
    return False

def get_file_info():
  return file_info
