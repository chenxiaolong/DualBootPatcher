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

current_progress = 0


def add_task(msg):
    print("ADDTASK:%s" % msg)


def set_task(msg):
    print("SETTASK:%s" % msg)
    print("SETDETAILS:")


# Details about task
def details(msg):
    print("SETDETAILS:" + msg)


def clear():
    print("SETDETAILS:")


# Miscellaneous info
def info(msg):
    print("SETDETAILS:" + msg)


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
    print("EXITFAIL:" + msg, file=sys.stderr)


# Process succeeded
def succeeded(msg):
    print("EXITSUCCESS:" + msg, file=sys.stderr)


# Set maximum progress
def max_progress(num):
    global current_progress
    print("SETMAXPROGRESS:" + str(num))
    current_progress = 0


# Increment progress counter
def progress():
    global current_progress
    current_progress += 1
    print("SETPROGRESS:" + str(current_progress))
