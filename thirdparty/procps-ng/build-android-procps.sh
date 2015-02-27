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

gcc_ver=4.9

ver="3.3.10"
name="procps-ng-${ver}"
tar="${name}.tar.xz"
url="http://downloads.sourceforge.net/project/procps-ng/Production/${tar}"

set -e

cd "$(dirname "${0}")"
source ../common.sh

mkdir -p procps-ng
cd procps-ng

if [ ! -f "${tar}" ]; then
    wget "${url}"
fi

if [ ! -d "${name}" ]; then
    tar xvf "${tar}"
fi

pushd "${name}"
sed -i '/AC_FUNC_MALLOC/d' configure.ac
sed -i '/AC_FUNC_REALLOC/d' configure.ac
autoreconf -vfi
popd

build_procps() {
    local arch="${1}"
    local toolchain="${2}"
    local toolchain_ver="${3}"
    local android_api="${4}"

    rm -rf build_${arch}/
    mkdir build_${arch}/
    pushd build_${arch}/

    local sysroot="--sysroot=$(get_ndk_sysroot ${android_api} ${arch})"
    local cflags="${sysroot}"
    local cxxflags="${sysroot}"
    local cppflags="${sysroot}"

    push_android_env "${toolchain}" "${toolchain_ver}"

    CFLAGS="${cflags}" CXXFLAGS="${cxxflags}" CPPFLAGS="${cppflags}" \
    ../${name}/configure \
        --target=${toolchain} \
        --host=${toolchain} \
        --prefix=/usr \
        --without-ncurses

    CFLAGS="${cflags}" CXXFLAGS="${cxxflags}" CPPFLAGS="${cppflags}" \
    make -C proc

    make -C proc install DESTDIR="$(pwd)/../install_${arch}"

    pop_android_env

    popd
}

build_procps armeabi-v7a arm-linux-androideabi 4.9 android-21
build_procps arm64-v8a aarch64-linux-android 4.9 android-21
build_procps x86 i686-linux-android 4.9 android-21
build_procps x86_64 x86_64-linux-android 4.9 android-21


outdir="$(mktemp -d)"
curdir="$(pwd)"

for arch in armeabi-v7a arm64-v8a x86 x86_64; do
    cp -r install_${arch}/usr/lib "${outdir}/lib_${arch}/"
done

cp -r install_armeabi-v7a/usr/include "${outdir}/include/"

pushd "${outdir}"
tar jcvf "${curdir}"/../procps-ng-${ver}_android.tar.bz2 \
    lib_*/libprocps.a include
popd

rm -rf "${outdir}"
