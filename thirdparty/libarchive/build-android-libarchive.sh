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

mkdir -p libarchive/
cd libarchive/

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
git am ../../0001-Android-build-fix.patch
git am ../../0001-Change-statfs.f_flag-statfs.f_flags.patch
popd

if [ ! -f android.toolchain.cmake ]; then
    wget 'https://github.com/taka-no-me/android-cmake/raw/master/android.toolchain.cmake'
fi

build() {
    local abi="${1}"
    local api="${2}"
    local toolchain="${3}"

    cmake ../libarchive \
        -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake \
        -DANDROID_ABI="${abi}" \
        -DANDROID_NATIVE_API_LEVEL="${api}" \
        -DANDROID_TOOLCHAIN_NAME="${toolchain}" \
        -DLIBRARY_OUTPUT_PATH_ROOT=.

    make -j4
}

if [ ! -f build_armeabi-v7a/libs/armeabi-v7a/libarchive.a ]; then
    mkdir -p build_armeabi-v7a
    pushd build_armeabi-v7a
    build armeabi-v7a android-17 arm-linux-androideabi-4.9
    popd
fi

if [ ! -f build_arm64-v8a/libs/arm64-v8a/libarchive.a ]; then
    mkdir -p build_arm64-v8a
    pushd build_arm64-v8a
    build arm64-v8a android-21 aarch64-linux-android-4.9
    popd
fi

if [ ! -f build_x86/libs/x86/libarchive.a ]; then
    mkdir -p build_x86
    pushd build_x86
    build x86 android-17 x86-4.9
    popd
fi

if [ ! -f build_x86_64/libs/x86_64/libarchive.a ]; then
    mkdir -p build_x86_64
    pushd build_x86_64
    build x86_64 android-21 x86_64-4.9
    popd
fi


outdir="$(mktemp -d)"

mkdir "${outdir}"/include/
mkdir "${outdir}"/lib_{armeabi-v7a,arm64-v8a,x86,x86_64}/

cp libarchive/libarchive/archive.h "${outdir}"/include/
cp libarchive/libarchive/archive_entry.h "${outdir}"/include/
cp --preserve=links build_armeabi-v7a/libs/armeabi-v7a/* "${outdir}"/lib_armeabi-v7a/
cp --preserve=links build_arm64-v8a/libs/arm64-v8a/* "${outdir}"/lib_arm64-v8a/
cp --preserve=links build_x86/libs/x86/* "${outdir}"/lib_x86/
cp --preserve=links build_x86_64/libs/x86_64/* "${outdir}"/lib_x86_64/

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}"/../libarchive-${ver}_android.tar.bz2 \
    lib_*/libarchive.a include/
popd

rm -rf "${outdir}"
