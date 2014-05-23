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

def add_task(msg):
    pass


def set_task(msg):
    pass


# Details about task
def details(msg):
    pass


def clear():
    pass


# Miscellaneous info
def info(msg):
    pass


# Failed to run command
def command_error(output="", error=""):
    pass


# Process failed
def failed(msg):
    pass


# Process succeeded
def succeeded(msg):
    pass


# Set maximum progress
def max_progress(num):
    pass


# Increment progress counter
def progress():
    pass
