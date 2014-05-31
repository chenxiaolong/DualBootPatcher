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

import os
import re
import sys

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

import multiboot.cmd as cmd

if len(sys.argv) < 3:
    print("Not enough arguments")
    print("")
    print("Linux/Mac: Did you run this script with two parameters?")
    print("")
    print("Windows: Did you drag two folders onto this script?")
    sys.exit(1)

if os.name == "posix":
    diff = "diff"
elif os.name == "nt":
    diff = os.path.join(OS.binariesdir, "diff.exe")
else:
    print("Unsupported operating system")
    sys.exit(1)

original = sys.argv[1]
modified = sys.argv[2]

exists = True
if not os.path.exists(original):
    print("The original path, %s, does not exist" % original)
    exists = False

if not os.path.exists(modified):
    print("The modified path, %s, does not exist" % modified)
    exists = False

if not exists:
    sys.exit(1)

print("You want to compare the following two directories:")
print("")
print("Original: " + original)
print("Modified: " + modified)
print("")
choice = input("Is this correct? (y = yes, n = no, s = swap) ")

proceed = False
if choice == '' or choice[:1] == 'y' or choice[:1] == 'Y':
    proceed = True

elif choice[:1] == 's' or choice[:1] == 'S':
    original, modified = modified, original
    proceed = True

if not proceed:
    sys.exit(0)

exit_status, output, error = cmd.run_command(
    [diff, '-N', '-r', '-u', original, modified]
)

print("\n")

if len(output) == 0:
    print('The directories are identical. '
          'Did you compare the right directories?')
    sys.exit(1)

filtered = []
parentdir = os.path.realpath(os.path.join(original, '..'))
parentdir2 = os.path.realpath(os.path.join(modified, '..'))

if not os.path.samefile(parentdir, parentdir2):
    print('WARNING: The two directories do not share the '
          'same parent directory')

for line in output.split('\n'):
    if line:
        # Regex's from
        # http://mail.kde.org/pipermail/kompare-devel/2006-August/000148.html
        if re.search(r"--- ([^\t]+)(?:\t([^\t]+)(?:\t?)(.*))?", line):
            m = re.search(r"--- ([^\t]+)(?:\t([^\t]+)(?:\t?)(.*))?", line)
            filtered.append("--- %s" % os.path.relpath(
                m.group(1), parentdir).replace("\\", "/"))

        elif re.search(r"\+\+\+ ([^\t]+)(?:\t([^\t]+)(?:\t?)(.*))?", line):
            m = re.search(r"\+\+\+ ([^\t]+)(?:\t([^\t]+)(?:\t?)(.*))?", line)
            filtered.append("+++ %s" % os.path.relpath(
                m.group(1), parentdir).replace("\\", "/"))

        elif line[:1] == '+' or line[:1] == '-' \
                or line[:1] == '@' or line[:1] == ' ':
            filtered.append(line)

patch = os.path.join(parentdir, 'NewFilePatch.patch')

f = open(patch, 'wb')
f.write('\n'.join(filtered).encode("UTF-8"))
f.write('\n'.encode("UTF-8"))
f.close()

print("The patch was created as:")
print("")
print("  " + patch)
print("")
print("Please copy it to the correct subdirectory under patches/")
