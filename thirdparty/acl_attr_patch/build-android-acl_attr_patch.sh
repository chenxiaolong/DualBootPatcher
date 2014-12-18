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

cd "$(dirname "${0}")"

mkdir -p build
cd build

topdir="$(pwd)"

gcc_ver=4.9
linaro_ver=14.09

url_base="http://releases.linaro.org/${linaro_ver}/components/toolchain/binaries"
name_arm="gcc-linaro-arm-linux-gnueabihf-${gcc_ver}-20${linaro_ver}_linux"
name_aarch64="gcc-linaro-aarch64-linux-gnu-${gcc_ver}-20${linaro_ver}_linux"
tar_arm="${name_arm}.tar.xz"
tar_aarch64="${name_aarch64}.tar.xz"
url_arm="${url_base}/${tar_arm}"
url_aarch64="${url_base}/${tar_aarch64}"
toolchain_arm="arm-linux-gnueabihf"
toolchain_aarch64="aarch64-linux-gnu"
toolchain_host="$(gcc -dumpmachine)"

if [ ! -f "${tar_arm}" ]; then
    wget "${url_arm}"
fi

if [ ! -f "${tar_aarch64}" ]; then
    wget "${url_aarch64}"
fi

if [ ! -d "${name_arm}" ]; then
    tar xvf "${tar_arm}"
fi

if [ ! -d "${name_aarch64}" ]; then
    tar xvf "${tar_aarch64}"
fi

push_linaro() {
    linaro_old_path="${PATH}"
    export PATH="${PATH}:${topdir}/${name_arm}/bin"
    export PATH="${PATH}:${topdir}/${name_aarch64}/bin"
}

pop_linaro() {
    export PATH="${linaro_old_path}"
}

android-strip() {
    local arch="${1}"
    local toolchain="${2}"
    shift
    shift

    if [[ "${arch}" == "x86" ]] || [[ "${arch}" == "x86_64" ]]; then
        strip "${@}"
    else
        ${toolchain}-strip "${@}"
    fi
}

android-upx() {
    local arch="${1}"
    shift

    if [[ "${arch}" != "arm64-v8a" ]]; then
        upx --lzma "${@}"
    fi
}

outdir="$(mktemp -d)"


################################################################################
# Build acl and attr
################################################################################

ver_attr="2.4.47"
ver_acl="2.2.52"
name_attr="attr-${ver_attr}"
name_acl="acl-${ver_acl}"
tar_attr="${name_attr}.src.tar.gz"
tar_acl="${name_acl}.src.tar.gz"
url_attr="http://download.savannah.gnu.org/releases/attr/${tar_attr}"
url_acl="http://download.savannah.gnu.org/releases/acl/${tar_acl}"

if [ ! -f "${tar_attr}" ]; then
    wget "${url_attr}"
fi

if [ ! -f "${tar_acl}" ]; then
    wget "${url_acl}"
fi

if [ ! -d "${name_attr}" ]; then
    tar xvf "${tar_attr}"
fi

if [ ! -d "${name_acl}" ]; then
    tar xvf "${tar_acl}"
fi

build_attr() {
    local arch="${1}"
    local toolchain="${2}"

    push_linaro

    rm -rf build_attr_${arch}/
    cp -r ${name_attr} build_attr_${arch}
    pushd build_attr_${arch}/

    if [ "${arch}" = "x86" ]; then
        local cc="${toolchain}-gcc -m32"
    else
        local cc="${toolchain}-gcc"
    fi

    CC="${cc}" ./configure \
        --host=${toolchain} \
        --build=${toolchain_host} \
        --prefix="$(pwd)/../inst_attr_${arch}" \
        --enable-shared \
        --enable-static \
        --disable-lib64 \
        --disable-gettext

    CC="${cc}" make

    make install-dev install-lib

    mkdir -p "${outdir}"/${arch}/

    for tool in getfattr setfattr; do
        pushd ${tool}
        ${cc} \
            -static \
            -o "${outdir}"/${arch}/${tool} \
            ${tool}.o \
            ../libmisc/.libs/libmisc.a \
            ../libattr/.libs/libattr.a
        popd
    done

    popd

    android-strip ${arch} ${toolchain} "${outdir}"/${arch}/{get,set}fattr
    android-upx ${arch} "${outdir}"/${arch}/{get,set}fattr

    pop_linaro
}

build_acl() {
    local arch="${1}"
    local toolchain="${2}"

    local attr_cflags="-I$(pwd)/inst_attr_${arch}/include"
    local attr_ldflags="-L$(pwd)/inst_attr_${arch}/lib"

    push_linaro

    rm -rf build_acl_${arch}/
    cp -r ${name_acl} build_acl_${arch}
    pushd build_acl_${arch}/

    if [ "${arch}" = "x86" ]; then
        local cc="${toolchain}-gcc -m32"
    else
        local cc="${toolchain}-gcc"
    fi

    CC="${cc}" CFLAGS="${attr_cflags}" LDFLAGS="${attr_ldflags}" ./configure \
        --host=${toolchain} \
        --build=${toolchain_host} \
        --prefix="$(pwd)/../inst_acl_${arch}" \
        --disable-shared \
        --enable-static \
        --disable-lib64 \
        --disable-gettext

    CC="${cc}" CFLAGS="${attr_cflags}" LDFLAGS="${attr_ldflags}" make

    mkdir -p "${outdir}"/${arch}/

    pushd getfacl
    ${cc} \
        ${x86_flags} ${attr_cflags} ${attr_ldflags} \
        -static \
        -o "${outdir}"/${arch}/getfacl \
        getfacl.o \
        user_group.o \
        ../libmisc/.libs/libmisc.a \
        ../libacl/.libs/libacl.a \
        -lattr
    popd

    pushd setfacl
    ${cc} \
        ${x86_flags} ${attr_cflags} ${attr_ldflags} \
        -static \
        -o "${outdir}"/${arch}/setfacl \
        setfacl.o \
        do_set.o \
        sequence.o \
        parse.o \
        ../libmisc/.libs/libmisc.a \
        ../libacl/.libs/libacl.a \
        -lattr
    popd

    popd

    android-strip ${arch} ${toolchain} "${outdir}"/${arch}/{get,set}facl
    android-upx ${arch} "${outdir}"/${arch}/{get,set}facl

    pop_linaro
}

build_attr armeabi-v7a ${toolchain_arm}
build_attr arm64-v8a ${toolchain_aarch64}
build_attr x86 ${toolchain_host}
build_attr x86_64 ${toolchain_host}
build_acl armeabi-v7a ${toolchain_arm}
build_acl arm64-v8a ${toolchain_aarch64}
build_acl x86 ${toolchain_host}
build_acl x86_64 ${toolchain_host}


################################################################################
# Build GNU patch
################################################################################

ver_patch="2.7.1"
name_patch="patch-${ver_patch}"
tar_patch="${name_patch}.tar.xz"
url_patch="ftp://ftp.gnu.org/gnu/patch/${tar_patch}"

if [ ! -f "${tar_patch}" ]; then
    wget "${url_patch}"
fi

if [ ! -d "${name_patch}" ]; then
    tar xvf "${tar_patch}"
fi

build_patch() {
    local arch="${1}"
    local toolchain="${2}"

    push_linaro

    rm -rf build_patch_${arch}/
    mkdir build_patch_${arch}/
    pushd build_patch_${arch}/

    if [ "${arch}" = "x86" ]; then
        local cc="${toolchain}-gcc -m32"
    else
        local cc="${toolchain}-gcc"
    fi

    CC="${cc} -static" ../${name_patch}/configure \
        --host=${toolchain} \
        --build=${toolchain_host}

    CC="${cc} -static" make

    mkdir -p "${outdir}"/${arch}/
    cp src/patch "${outdir}"/${arch}/patch

    popd

    android-strip ${arch} ${toolchain} "${outdir}"/${arch}/patch
    android-upx ${arch} "${outdir}"/${arch}/patch

    pop_linaro
}

build_patch armeabi-v7a ${toolchain_arm}
build_patch arm64-v8a ${toolchain_aarch64}
build_patch x86 ${toolchain_host}
build_patch x86_64 ${toolchain_host}


################################################################################
# Build tarballs
################################################################################
curdir="$(pwd)"

pushd "${outdir}"
tar jcvf "${curdir}"/../attr-${ver_attr}_android.tar.bz2 \
    */{g,s}etfattr

tar jcvf "${curdir}"/../acl-${ver_acl}_android.tar.bz2 \
    */{g,s}etfacl

tar jcvf "${curdir}"/../patch-${ver_patch}_android.tar.bz2 \
    */patch
popd

rm -rf "${outdir}"
