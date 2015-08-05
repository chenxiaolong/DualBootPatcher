#!/bin/bash

# Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

url="https://github.com/chenxiaolong/lzo.git"
ver="2.09"

set -e

mkdir -p lzo
cd lzo

if [ ! -d lzo ]; then
    git clone "${url}" lzo
else
    pushd lzo
    git checkout master
    git pull
    popd
fi

pushd lzo
git checkout "v${ver}"
popd

if [ ! -f android.toolchain.cmake.orig ]; then
    wget 'https://github.com/taka-no-me/android-cmake/raw/master/android.toolchain.cmake' \
        -O android.toolchain.cmake
fi

build() {
    local abi="${1}"
    local api="${2}"
    local toolchain="${3}"

    mkdir -p "build_${abi}"
    pushd "build_${abi}"
  
    cmake ../lzo \
        -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake \
        -DANDROID_ABI="${abi}" \
        -DANDROID_NATIVE_API_LEVEL="${api}" \
        -DANDROID_TOOLCHAIN_NAME="${toolchain}" \
        -DLIBRARY_OUTPUT_PATH_ROOT=.
  
    make -j4
  
    popd
}

build armeabi-v7a android-17 arm-linux-androideabi-4.9
build arm64-v8a android-21 aarch64-linux-android-4.9
build x86 android-17 x86-4.9
build x86_64 android-21 x86_64-4.9


outdir="$(mktemp -d)"

mkdir -p "${outdir}"/include/
cp -r lzo/include/lzo/ "${outdir}"/include/
for abi in armeabi-v7a arm64-v8a x86 x86_64; do
    cp -a build_${abi}/libs/${abi}/ "${outdir}"/lib_${abi}/
done

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}"/../liblzo2-${ver}_android.tar.bz2 lib_*/ include/
popd

rm -rf "${outdir}"
