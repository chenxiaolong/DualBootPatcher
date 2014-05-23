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

import multiboot.debug as debug

import subprocess


def run_command(command,
                stdin_data=None,
                cwd=None,
                universal_newlines=True):
    debug.debug("Running command: " + str(command))

    try:
        process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=cwd,
            universal_newlines=universal_newlines
        )
        output, error = process.communicate(input=stdin_data)

        exit_code = process.returncode

        return (exit_code, output, error)

    except:
        raise Exception("Failed to run command: \"%s\"" % ' '.join(command))
