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

set -e

ver=1.58.0

name="boost_${ver//./_}"
tarball="${name}.tar.bz2"
url="http://downloads.sourceforge.net/project/boost/boost/${ver}/${tarball}"

mkdir -p boost
cd boost

if [ ! -f "${tarball}" ]; then
    wget "${url}" -O "${tarball}"
fi

if [ ! -d "${name}" ]; then
    tar xvf "${tarball}"
fi

build() {
    local abi="${1}"
    local toolchain="${2}"

    if [[ "${toolchain}" == "x86-4.9" ]]; then
        toolchain_prefix=i686-linux-android-
    elif [[ "${toolchain}" == "x86_64-4.9" ]]; then
        toolchain_prefix=x86_64-linux-android-
    else
        toolchain_prefix="${toolchain%-*}-"
    fi

    local toolchain_dir=${ANDROID_NDK_HOME}/toolchains/${toolchain}/prebuilt/linux-x86_64/bin

    rm -rf bin.v2/libs

    cp ../../user-config_${abi}.jam tools/build/src/user-config.jam

    PATH="${toolchain_dir}:${PATH}" \
    NO_BZIP2=1                      \
    ./b2 -q                         \
        `# --debug-configuration `  \
        target-os=android           \
        toolset=gcc-android         \
        link=static                 \
        threading=multi             \
        --layout=tagged             \
        --prefix="../build_${abi}"  \
        --with-filesystem           \
        --with-system               \
        install

    rm tools/build/src/user-config.jam
}

pushd "${name}"

if [ ! -f bjam ]; then
    ./bootstrap.sh
fi

if [ ! -d ../build_armeabi-v7a/ ]; then
    build armeabi-v7a arm-linux-androideabi-4.9
fi

if [ ! -d ../build_arm64-v8a ]; then
    build arm64-v8a aarch64-linux-android-4.9
fi

if [ ! -d ../build_x86 ]; then
    build x86 x86-4.9
fi

if [ ! -d ../build_x86_64 ]; then
    build x86_64 x86_64-4.9
fi

popd

outdir="$(mktemp -d)"

mkdir "${outdir}"/include/

# The headers are the same for each architecture
cp -r build_armeabi-v7a/include/boost/ "${outdir}"/include/

cp -r build_armeabi-v7a/lib/ "${outdir}"/lib_armeabi-v7a/
cp -r build_arm64-v8a/lib/ "${outdir}"/lib_arm64-v8a/
cp -r build_x86/lib/ "${outdir}"/lib_x86/
cp -r build_x86_64/lib/ "${outdir}"/lib_x86_64/

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}"/../boost-${ver}_android.tar.bz2 include/ lib_*/
popd
