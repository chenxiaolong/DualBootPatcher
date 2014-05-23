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

import sys

last_line_length = 0


def same_line(msg):
    global last_line_length
    print('\r' + (' ' * last_line_length), end="")
    last_line_length = len(msg)
    print('\r' + msg, end="")

    # For Python 2
    sys.stdout.flush()


###


def add_task(msg):
    pass


def set_task(msg):
    pass


# Details about task
def details(msg):
    same_line(msg)


def clear():
    same_line("")


# Miscellaneous info
def info(msg):
    print(msg)


# Failed to run command
def command_error(output="", error=""):
    print("--- ERROR BEGIN ---")

    if (output):
        print("--- STDOUT BEGIN ---")
        print(output)
        print("--- STDOUT END ---")
    else:
        print("No stdout output")

    if (error):
        print("--- STDERR BEGIN ---")
        print(error)
        print("--- STDERR END ---")
    else:
        print("No stderr output")

    print("--- ERROR END ---")


# Process failed
def failed(msg):
    clear()
    print(msg, file=sys.stderr)


# Process succeeded
def succeeded(msg):
    clear()
    print(msg, file=sys.stderr)


# Set maximum progress
def max_progress(num):
    pass


# Increment progress counter
def progress():
    pass
