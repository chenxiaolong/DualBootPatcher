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

gcc_ver=4.9

ver="3.10.1"
name="valgrind-${ver}"
tar="${name}.tar.bz2"
url="http://valgrind.org/downloads/${tar}"

set -e

cd "$(dirname "${0}")"
source ../common.sh

mkdir -p valgrind
cd valgrind

if [ ! -f "${tar}" ]; then
    wget "${url}"
fi

if [ ! -d "${name}" ]; then
    tar xvf "${tar}"
fi

build_valgrind() {
    local arch="${1}"
    local toolchain="${2}"
    local toolchain_ver="${3}"
    local android_api="${4}"

    if [ -f valgrind-${ver}_${arch}.tar.7z ]; then
        return
    fi

    rm -rf build_${arch}/
    mkdir build_${arch}/
    pushd build_${arch}/

    local sysroot="--sysroot=$(get_ndk_sysroot ${android_api} ${arch})"
    local cflags="${sysroot}"
    local cxxflags="${sysroot}"
    local cppflags="${sysroot}"

    if [ "${arch}" = "armeabi-v7a" ]; then
        local conf_args=('--host=armv7-unknown-linux' 
                         '--target=armv7-unknown-linux')
    elif [ "${arch}" = "arm64-v8a" ]; then
        local conf_args=('--host=aarch64-unknown-linux'
                         '--enable-only64bit')
    elif [ "${arch}" = "x86" ]; then
        local conf_args=('--host=i686-android-linux'
                         '--target=i686-android-linux')
        cflags+=" -fno-pic"
    else
        local conf_args=()
    fi

    push_android_env "${toolchain}" "${toolchain_ver}"

    CFLAGS="${cflags}" CXXFLAGS="${cxxflags}" CPPFLAGS="${cppflags}" \
    ../${name}/configure \
        --prefix=/data/local/Inst \
        ${conf_args[@]} \
        --with-tmpdir=/sdcard

    CFLAGS="${cflags}" CXXFLAGS="${cxxflags}" CPPFLAGS="${cppflags}" \
    make -j4

    make install DESTDIR="$(pwd)/valgrind-inst"

    pop_android_env

    pushd valgrind-inst
    find -mindepth 1 -maxdepth 1 -print0 \
            | tar cv --null -T - \
            | 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -si \
                    ../../../valgrind-${ver}_${arch}.tar.7z
    popd

    popd
}

# Does not compile with the android-21 platform
build_valgrind armeabi-v7a arm-linux-androideabi 4.9 android-18

# Valgrind 3.10.1 supports aarch64, but not when combined with the NDK compiler
#build_valgrind arm64-v8a aarch64-linux-android 4.9 android-21

# Does not compile with the android-21 platform
build_valgrind x86 i686-linux-android 4.9 android-18

# Valgrind does not support Android x86_64
#build_valgrind x86_64 x86_64-linux-android 4.9 android-21
