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

xz_ver='5.2.0'

url='https://github.com/libarchive/libarchive.git'
ver='3.1.2'

if [ ! -f ../liblzma/liblzma-${xz_ver}_android.tar.bz2 ]; then
    echo "Please run thirdparty/liblzma/build-android-liblzma.sh first"
    exit 1
fi

mkdir -p liblzma/
tar xvf ../liblzma/liblzma-${xz_ver}_android.tar.bz2 -C liblzma/

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
    # Hack to allow us to specify liblzma's path
    sed -i '/[^A-Z_]CMAKE_FIND_ROOT_PATH[^A-Z_]/ s/)/"${LIBLZMA_PREFIX_PATH}")/g' android.toolchain.cmake
fi

build() {
    local abi="${1}"
    local api="${2}"
    local toolchain="${3}"

    mkdir -p "build_${abi}"
    pushd "build_${abi}"

    rm -rf liblzma
    mkdir liblzma
    ln -s "$(pwd)/../../liblzma/include" liblzma/include
    ln -s "$(pwd)/../../liblzma/lib_${abi}" liblzma/lib

    cmake ../libarchive \
        -DLIBLZMA_PREFIX_PATH="$(pwd)/liblzma" \
        -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake \
        -DANDROID_ABI="${abi}" \
        -DANDROID_NATIVE_API_LEVEL="${api}" \
        -DANDROID_TOOLCHAIN_NAME="${toolchain}" \
        -DLIBRARY_OUTPUT_PATH_ROOT=.

    make -j4

    popd
}

build armeabi-v7a android-21 arm-linux-androideabi-4.9
build arm64-v8a android-21 aarch64-linux-android-4.9
build x86 android-21 x86-4.9
build x86_64 android-21 x86_64-4.9


outdir="$(mktemp -d)"

mkdir "${outdir}"/include/

cp libarchive/libarchive/archive.h "${outdir}"/include/
cp libarchive/libarchive/archive_entry.h "${outdir}"/include/
for abi in armeabi-v7a arm64-v8a x86 x86_64; do
    cp -a build_${abi}/libs/${abi}/ "${outdir}"/lib_${abi}/
done

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}"/../libarchive-${ver}_android.tar.bz2 \
    lib_*/libarchive.a include/
popd

rm -rf "${outdir}"
