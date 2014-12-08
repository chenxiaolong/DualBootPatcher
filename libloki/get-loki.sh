#!/bin/bash

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

set -e

cd "$(dirname "${0}")"

if [ ! -d loki ]; then
    git clone https://github.com/djrbliss/loki.git
fi

pushd loki/
git fetch
git checkout 14c37b73ad6401c952a39183f246a2d045aba9ae
git am ../0001-Add-loki-JNI-shared-library.patch
popd

rm -rf jni/
mkdir jni/
cp loki/*.{c,h,mk} loki/LICENSE.txt jni/
