#!/bin/bash

# Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

set -e

icon_file=icon.png
icon_sizes=(256 64 48 40 32 24 20 16)
icon_files=()

for size in "${icon_sizes[@]}"; do
    convert -resize ${size}x${size} ${icon_file} output-${size}.ico
    icon_files+=(output-${size}.ico)
done

convert "${icon_files[@]}" icon.ico
rm "${icon_files[@]}"
