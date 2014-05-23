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

import sys

listener = None
current_progress = 0


def add_task(task):
    listener.trigger_add_task.emit(task)


def set_task(task):
    listener.trigger_set_task.emit(task)
    clear()


# Details about task
def details(msg):
    listener.trigger_details.emit(msg)


def clear():
    listener.trigger_clear.emit()


# Miscellaneous info
def info(msg):
    listener.trigger_info.emit(msg)


# Failed to run command
def command_error(output="", error=""):
    listener.trigger_command_error.emit(output, error)


# Process failed
def failed(msg):
    listener.trigger_failed.emit(msg)


# Process succeeded
def succeeded(msg):
    listener.trigger_succeeded.emit(msg)


# Set maximum progress
def max_progress(num):
    global current_progress
    listener.trigger_max_progress.emit(num)
    current_progress = 0


# Increment progress counter
def progress():
    global current_progress
    current_progress += 1
    listener.trigger_set_progress.emit(current_progress)
