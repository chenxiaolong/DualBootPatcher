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
    export PATH="${PATH}:$(pwd)/${name_arm}/bin"
    export PATH="${PATH}:$(pwd)/${name_aarch64}/bin"
}

pop_linaro() {
    export PATH="${linaro_old_path}"
}

# Output and temp dirs

temp_prefix_arm="$(pwd)/temp_prefix_arm"
temp_prefix_aarch64="$(pwd)/temp_prefix_aarch64"
temp_prefix_x86="$(pwd)/temp_prefix_x86"
temp_prefix_x86_64="$(pwd)/temp_prefix_x86_64"

mkdir -p output


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

build_attr() {
    local arch="${1}"
    local toolchain="${2}"
    local prefix="${3}"

    if [ -f output/${arch}/getfattr ] && [ -f output/${arch}/setfattr ]; then
        return
    fi

    push_linaro

    mkdir -p output/${arch}/

    if [ -d "${name_attr}_${arch}" ]; then
        rm -rf "${name_attr}_${arch}"
    fi
    mkdir "${name_attr}_${arch}"

    tar xvf "${tar_attr}" --strip-components=1 -C "${name_attr}_${arch}"

    pushd "${name_attr}_${arch}"

    if [ "${arch}" = "x86" ]; then
        local cc="${toolchain}-gcc -m32"
    else
        local cc="${toolchain}-gcc"
    fi

    CC="${cc}" ./configure \
        --host=${toolchain} \
        --build=${toolchain_host} \
        --prefix="${prefix}" \
        --enable-shared \
        --enable-static \
        --disable-lib64 \
        --disable-gettext

    CC="${cc}" make

    make install-dev
    make install-lib

    for tool in getfattr setfattr; do
        pushd ${tool}
        ${cc} \
            -static \
            -o ../../output/${arch}/${tool} \
            ${tool}.o \
            ../libmisc/.libs/libmisc.a \
            ../libattr/.libs/libattr.a
        popd
    done

    popd

    if [ "${arch}" = "x86" ] || [ "${arch}" = "x86_64" ]; then
        strip output/${arch}/{get,set}fattr
    else
        ${toolchain}-strip output/${arch}/{get,set}fattr
    fi

    if [ "${arch}" != "arm64-v8a" ]; then
        upx --lzma output/${arch}/{get,set}fattr
    fi

    pop_linaro
}

build_acl() {
    local arch="${1}"
    local toolchain="${2}"
    local prefix="${3}"

    local attr_cflags="-I${prefix}/include"
    local attr_ldflags="-L${prefix}/lib"

    if [ -f output/${arch}/getfacl ] && [ -f output/${arch}/setfacl ]; then
        return
    fi

    push_linaro

    mkdir -p output/${arch}/

    if [ -d "${name_acl}_${arch}" ]; then
        rm -rf "${name_acl}_${arch}"
    fi
    mkdir "${name_acl}_${arch}"

    tar xvf "${tar_acl}" --strip-components=1 -C "${name_acl}_${arch}"

    pushd "${name_acl}_${arch}"

    if [ "${arch}" = "x86" ]; then
        local cc="${toolchain}-gcc -m32"
    else
        local cc="${toolchain}-gcc"
    fi

    CC="${cc}" CFLAGS="${attr_cflags}" LDFLAGS="${attr_ldflags}" ./configure \
        --host=${toolchain} \
        --build=${toolchain_host} \
        --prefix="${prefix}" \
        --disable-shared \
        --enable-static \
        --disable-lib64 \
        --disable-gettext

    CC="${cc}" CFLAGS="${attr_cflags}" LDFLAGS="${attr_ldflags}" make

    pushd getfacl
    ${cc} \
        ${x86_flags} ${attr_cflags} ${attr_ldflags} \
        -static \
        -o ../../output/${arch}/getfacl \
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
        -o ../../output/${arch}/setfacl \
        setfacl.o \
        do_set.o \
        sequence.o \
        parse.o \
        ../libmisc/.libs/libmisc.a \
        ../libacl/.libs/libacl.a \
        -lattr
    popd

    popd

    if [ "${arch}" = "x86" ] || [ "${arch}" = "x86_64" ]; then
        strip output/${arch}/{get,set}facl
    else
        ${toolchain}-strip output/${arch}/{get,set}facl
    fi

    if [ "${arch}" != "arm64-v8a" ]; then
        upx --lzma output/${arch}/{get,set}facl
    fi

    pop_linaro
}

build_attr armeabi-v7a ${toolchain_arm} ${temp_prefix_arm}
build_attr arm64-v8a ${toolchain_aarch64} ${temp_prefix_aarch64}
build_attr x86 ${toolchain_host} ${temp_prefix_x86}
build_attr x86_64 ${toolchain_host} ${temp_prefix_x86_64}
build_acl armeabi-v7a ${toolchain_arm} ${temp_prefix_arm}
build_acl arm64-v8a ${toolchain_aarch64} ${temp_prefix_aarch64}
build_acl x86 ${toolchain_host} ${temp_prefix_x86}
build_acl x86_64 ${toolchain_host} ${temp_prefix_x86_64}


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

build_patch() {
    local arch="${1}"
    local toolchain="${2}"
    local prefix="${3}"

    if [ -f output/${arch}/patch ]; then
        return
    fi

    push_linaro

    mkdir -p output/${arch}/

    if [ -d "${name_patch}_${arch}" ]; then
        rm -rf "${name_patch}_${arch}"
    fi
    mkdir "${name_patch}_${arch}"

    tar xvf "${tar_patch}" --strip-components=1 -C "${name_patch}_${arch}"

    pushd "${name_patch}_${arch}"

    if [ "${arch}" = "x86" ]; then
        local cc="${toolchain}-gcc -m32"
    else
        local cc="${toolchain}-gcc"
    fi

    CC="${cc} -static" ./configure \
        --host=${toolchain} \
        --build=${toolchain_host} \
        --prefix="${prefix}"

    CC="${cc} -static" make

    cp src/patch ../output/${arch}/patch

    popd

    if [ "${arch}" = "x86" ] || [ "${arch}" = "x86_64" ]; then
        strip output/${arch}/patch
    else
        ${toolchain}-strip output/${arch}/patch
    fi

    if [ "${arch}" != "arm64-v8a" ]; then
        upx --lzma output/${arch}/patch
    fi

    pop_linaro
}

build_patch armeabi-v7a ${toolchain_arm} ${temp_prefix_arm}
build_patch arm64-v8a ${toolchain_aarch64} ${temp_prefix_aarch64}
build_patch x86 ${toolchain_host} ${temp_prefix_x86}
build_patch x86_64 ${toolchain_host} ${temp_prefix_x86_64}


################################################################################
# Build tarballs
################################################################################
curdate="$(date +'%Y%m%d')"
echo "${curdate}" > output/PREBUILTS_VERSION_ANDROID.txt

# "cmake -E tar" (CMake 3.0.1) doesn't like xz on Windows, so we'll use bz2
if [ ! -f prebuilts-android-${curdate}.tar.bz2 ]; then
    tar jcvf prebuilts-android-${curdate}.tar.bz2 \
        output/PREBUILTS_VERSION_ANDROID.txt \
        output/*/{patch,{g,s}etf{acl,attr}}
fi
