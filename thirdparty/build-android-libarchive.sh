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

url='https://github.com/libarchive/libarchive.git'
ver='3.1.2'

if [ ! -d libarchive ]; then
    git clone "${url}" libarchive
else
    pushd libarchive
    git checkout master
    git pull
    popd
fi

pushd libarchive
git checkout v${ver}
git am ../0001-Android-build-fix.patch
popd

mkdir -p libarchive-build
pushd libarchive-build

if [ ! -f android.toolchain.cmake ]; then
    wget 'https://github.com/taka-no-me/android-cmake/raw/master/android.toolchain.cmake'
fi

cmake ../libarchive \
    -DCMAKE_TOOLCHAIN_FILE=android.toolchain.cmake \
    -DANDROID_ABI=armeabi-v7a \
    -DANDROID_NATIVE_API_LEVEL=android-17 \
    -DANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-4.9 \
    -DLIBRARY_OUTPUT_PATH_ROOT=.

make -j4

popd

rm -rf output
mkdir output
pushd output

mkdir lib include

cp --preserve=links ../libarchive-build/libs/armeabi-v7a/* lib/
cp ../libarchive/libarchive/archive.h include/
cp ../libarchive/libarchive/archive_entry.h include/

tar jcvf ../android-prebuilts-libarchive-${ver}.tar.bz2 lib/ include/

popd

rm -rf output