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

ver="1.2.8"
name="zlib-${ver}"
tar="${name}.tar.xz"
url="http://zlib.net/${tar}"

mkdir -p zlib
cd zlib

if [ ! -f "${tar}" ]; then
    wget "${url}"
fi

rm -rf "${name}"
tar xvf "${tar}"

pushd "${name}"
patch -p1 -i ../../0001-Android-build-support.patch
ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk
popd

add_miniunzip() {
    local script="${1}"
    local abi="${2}"

    echo "BEGIN_${abi}" >> "${script}"
    base64 "${name}/libs/${abi}/miniunzip" >> "${script}"
    echo "END_${abi}" >> "${script}"
}

cp ../unzip.in ../unzip

add_miniunzip ../unzip armeabi-v7a
add_miniunzip ../unzip arm64-v8a
add_miniunzip ../unzip x86
add_miniunzip ../unzip x86_64

rm -f ../unzip.xz
xz -9 ../unzip
base64 ../unzip.xz > ../unzip.xz.base64
