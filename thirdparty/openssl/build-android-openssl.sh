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

ver="1.0.2d"
name="openssl-${ver}"
tar="${name}.tar.gz"
url="http://www.openssl.org/source/${tar}"

set -e

cd "$(dirname "${0}")"
source ../common.sh

mkdir -p openssl
cd openssl

if [ ! -f "${tar}" ]; then
    wget "${url}"
fi

build_openssl() {
    local arch="${1}"
    local toolchain="${2}"
    local toolchain_ver="${3}"
    local android_api="${4}"

    rm -rf "build_${arch}/" "installed_${arch}/"
    mkdir "build_${arch}/"
    pushd "build_${arch}/"

    tar xvf "../${tar}" --strip=1

    local old_path="${PATH}"
    export PATH="${PATH}:$(get_toolchain_dir "${toolchain}" "${toolchain_ver}")"

    perl -pi -e 's/install: all install_docs install_sw/install: install_sw/g' Makefile.org

    extra_args=(--prefix=/)

    # Same options as AOSP build
    extra_args+=(
        -DL_ENDIAN
        no-camellia
        no-capieng
        no-cast
        no-dtls1
        no-gost
        no-gmp
        no-heartbeats
        no-idea
        no-jpake
        no-md2
        no-mdc2
        no-rc5
        no-rdrand
        no-ripemd
        no-rfc3779
        no-rsax
        no-sctp
        no-seed
        no-sha0
        no-static_engine
        no-whirlpool
        no-zlib
        -DNO_WINDOWS_BRAINDEATH
    )

    # For static build only
    extra_args+=(no-dso)

    # Arch-specific options
    case "${arch}" in
    armeabi-v7a)
        extra_args+=(linux-armv4)
        local openssl_arch=arm
        local openssl_machine=armv7
        ;;
    arm64-v8a)
        extra_args+=(linux-aarch64)
        local openssl_arch=arm64
        local openssl_machine=aarch64
        ;;
    x86)
        extra_args+=(linux-elf)
        local openssl_arch=x86
        local openssl_machine=i686
        ;;
    x86_64)
        extra_args+=(linux-x86_64)
        local openssl_arch=x86_64
        local openssl_machine=x86_64
        ;;
    esac

    # Compiler flags
    local sysroot_dir
    sysroot_dir="$(get_ndk_sysroot "${android_api}" "${arch}")"
    extra_args+=("--sysroot=${sysroot_dir}")

    SYSTEM=android \
    ARCH="${openssl_arch}" \
    MACHINE="${openssl_machine}" \
    RELEASE="3.4.0" \
    CROSS_COMPILE="${toolchain}-" \
    ANDROID_DEV="${sysroot_dir}/usr" \
    ./Configure "${extra_args[@]}"

    make depend -j4
    make all -j4
    make install INSTALL_PREFIX="$(pwd)/../installed_${arch}/"

    export PATH="${old_path}"

    popd
}

build_openssl armeabi-v7a arm-linux-androideabi 4.9 android-21
build_openssl arm64-v8a aarch64-linux-android 4.9 android-21
build_openssl x86 i686-linux-android 4.9 android-21
build_openssl x86_64 x86_64-linux-android 4.9 android-21


outdir="$(mktemp -d)"
curdir="$(pwd)"

cp -r installed_armeabi-v7a/include/ "${outdir}/include/"
rm "${outdir}/include/openssl/opensslconf.h"
cp ../opensslconf.h "${outdir}/include/openssl/"

for arch in armeabi-v7a arm64-v8a x86 x86_64; do
    mkdir "${outdir}/lib_${arch}/"
    cp build_${arch}/lib{crypto,ssl}.a "${outdir}/lib_${arch}/"
    cp installed_${arch}/include/openssl/opensslconf.h \
        "${outdir}/include/openssl/opensslconf_${arch}.h"
done

pushd "${outdir}"
tar jcvf "${curdir}"/../openssl-${ver}_android.tar.bz2 \
    lib_*/*.a include
popd

rm -rf "${outdir}"
