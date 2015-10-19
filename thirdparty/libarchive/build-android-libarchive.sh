#!/bin/bash

# Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

rel=2

xz_ver='5.2.1'
lzo_ver='2.09'
lz4_ver='r128'
libiconv_ver='1.14-1'

url='https://github.com/libarchive/libarchive.git'
commit='567b37436642c344ecae1208f9d20885864986d1'

if [ ! -f ../liblzma/liblzma-${xz_ver}_android.tar.bz2 ]; then
    echo "Please run thirdparty/liblzma/build-android-liblzma.sh first"
    exit 1
fi

if [ ! -f ../lzo/liblzo2-${lzo_ver}_android.tar.bz2 ]; then
    echo "Please run thirdparty/lzo/build-android-lzo.sh first"
    exit 1
fi

if [ ! -f ../lz4/liblz4-${lz4_ver}_android.tar.bz2 ]; then
    echo "Please run thirdparty/lz4/build-android-lz4.sh first"
    exit 1
fi

if [ ! -f ../libiconv/libiconv-${libiconv_ver}_android.tar.bz2 ]; then
    echo "Please run thirdparty/libiconv/build-android-libiconv first"
    exit 1
fi

mkdir -p liblzma/
tar xvf ../liblzma/liblzma-${xz_ver}_android.tar.bz2 -C liblzma/

mkdir -p liblzo2/
tar xvf ../lzo/liblzo2-${lzo_ver}_android.tar.bz2 -C liblzo2/

mkdir -p liblz4/
tar xvf ../lz4/liblz4-${lz4_ver}_android.tar.bz2 -C liblz4/

mkdir -p libiconv/
tar xvf ../libiconv/libiconv-${libiconv_ver}_android.tar.bz2 -C libiconv/

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
git checkout ${commit}
ver=$(git describe --long | sed -E "s/^v//g;s/([^-]*-g)/r\1/;s/-/./g")
git am ../../0001-Android-build-fix.patch
git am ../../0002-Change-statfs.f_flag-statfs.f_flags.patch
git am ../../0003-Force-UTF-8-as-the-default-charset-on-Android-since-.patch
popd

if [ ! -f android.toolchain.cmake.orig ]; then
    wget 'https://github.com/taka-no-me/android-cmake/raw/master/android.toolchain.cmake' \
        -O android.toolchain.cmake.orig
fi

# Hack to allow us to specify liblzma and lz4's path
sed '/[^A-Z_]CMAKE_FIND_ROOT_PATH[^A-Z_]/ s/)/"${LIBLZMA_PREFIX_PATH}" "${LIBLZO2_PREFIX_PATH}" "${LIBLZ4_PREFIX_PATH}" "${LIBICONV_PREFIX_PATH}")/g' \
    < android.toolchain.cmake.orig \
    > android.toolchain.cmake

build() {
    local abi="${1}"
    local api="${2}"
    local toolchain="${3}"

    rm -rf "build_${abi}"
    mkdir -p "build_${abi}"
    pushd "build_${abi}"

    rm -rf liblzma
    mkdir liblzma
    ln -s "$(pwd)/../../liblzma/include" liblzma/include
    ln -s "$(pwd)/../../liblzma/lib_${abi}" liblzma/lib

    rm -rf liblzo2
    mkdir liblzo2
    ln -s "$(pwd)/../../liblzo2/include" liblzo2/include
    ln -s "$(pwd)/../../liblzo2/lib_${abi}" liblzo2/lib

    rm -rf liblz4
    mkdir liblz4
    ln -s "$(pwd)/../../liblz4/include" liblz4/include
    ln -s "$(pwd)/../../liblz4/lib_${abi}" liblz4/lib

    rm -rf libiconv
    mkdir libiconv
    ln -s "$(pwd)/../../libiconv/include" libiconv/include
    ln -s "$(pwd)/../../libiconv/lib_${abi}" libiconv/lib

    cmake ../libarchive \
        -DENABLE_TAR=OFF \
        -DENABLE_CPIO=OFF \
        -DENABLE_CAT=OFF \
        -DENABLE_TEST=OFF \
        -DENABLE_ICONV=OFF `# At least until we need it` \
        -DLIBLZMA_PREFIX_PATH="$(pwd)/liblzma" \
        -DLIBLZO2_PREFIX_PATH="$(pwd)/liblzo2" \
        -DLIBLZ4_PREFIX_PATH="$(pwd)/liblz4" \
        -DLIBICONV_PREFIX_PATH="$(pwd)/libiconv" \
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
tar jcvf "${curdir}/../libarchive-${ver}-${rel}_android.tar.bz2" \
    lib_*/libarchive.a include/
popd

rm -rf "${outdir}"
