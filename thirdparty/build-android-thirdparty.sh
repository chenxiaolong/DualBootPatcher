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
linaro_ver=14.08

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

ver_patch="2.7"
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

    CC="${cc}" ./configure \
        --host=${toolchain} \
        --build=${toolchain_host} \
        --prefix="${prefix}"

    CC="${cc}" make

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
# Build valgrind
################################################################################

ver_valgrind="3.10.0"
name_valgrind="valgrind-${ver_valgrind}"
tar_valgrind="${name_valgrind}.tar.bz2"
url_valgrind="http://valgrind.org/downloads/${tar_valgrind}"

push_android() {
    local toolchain="${1}"
    local toolchain_ver="${2}"

    if [ "${toolchain}" = "i686-linux-android" ]; then
        local toolchain_dir="x86"
    else
        local toolchain_dir="${toolchain}"
    fi

    local basedir="${ANDROID_NDK_HOME}/toolchains/${toolchain_dir}-${toolchain_ver}/prebuilt/linux-x86_64/bin"
    android_path="${PATH}"
    android_cc="${CC}"
    android_cxx="${CXX}"
    android_cpp="${CPP}"
    android_ar="${AR}"
    android_as="${AS}"
    android_ld="${LD}"
    android_objcopy="${OBJCOPY}"
    android_objdump="${OBJDUMP}"
    android_ranlib="${RANLIB}"
    android_strip="${STRIP}"

    export PATH="${basedir}:${PATH}"
    export CC="${toolchain}-gcc"
    export CXX="${toolchain}-g++"
    export CPP="${toolchain}-cpp"
    export AR="${toolchain}-ar"
    export AS="${toolchain}-as"
    export LD="${toolchain}-ld"
    export OBJCOPY="${toolchain}-objcopy"
    export OBJDUMP="${toolchain}-objdump"
    export RANLIB="${toolchain}-ranlib"
    export STRIP="${toolchain}-strip"
}

pop_android() {
    export PATH="${android_path}"
    export CC="${android_cc}"
    export CXX="${android_cxx}"
    export CPP="${android_cpp}"
    export AR="${android_ar}"
    export AS="${android_as}"
    export LD="${android_ld}"
    export OBJCOPY="${android_objcopy}"
    export OBJDUMP="${android_objdump}"
    export RANLIB="${android_ranlib}"
    export STRIP="${android_strip}"
}

if [ ! -f "${tar_valgrind}" ]; then
    wget "${url_valgrind}"
fi

build_valgrind() {
    local arch="${1}"
    local toolchain="${2}"
    local toolchain_ver="${3}"
    local android_api="${4}"

    if [ -f output/${arch}/valgrind.tar.7z ]; then
        return
    fi

    push_android "${toolchain}" "${toolchain_ver}"

    mkdir -p output/${arch}/

    if [ -d "${name_valgrind}_${arch}" ]; then
        rm -rf "${name_valgrind}_${arch}"
    fi
    mkdir "${name_valgrind}_${arch}"

    tar xvf "${tar_valgrind}" --strip-components=1 -C "${name_valgrind}_${arch}"

    pushd "${name_valgrind}_${arch}"

    if [ "${arch}" = "armeabi-v7a" ]; then
        local android_arch="arm"
    elif [ "${arch}" = "arm64-v8a" ]; then
        local android_arch="arm64"
    else
        local android_arch="${arch}"
    fi

    local sysroot="--sysroot=${ANDROID_NDK_HOME}/platforms/${android_api}/arch-${android_arch}"
    local cflags="${sysroot}"
    local cxxflags="${sysroot}"
    local cppflags="${sysroot}"

    if [ "${arch}" = "armeabi-v7a" ]; then
        conf_args=('--host=armv7-unknown-linux' '--target=armv7-unknown-linux')
    elif [ "${arch}" = "arm64-v8a" ]; then
        conf_args=('--host=aarch64-unknown-linux' '--enable-only64bit')
    elif [ "${arch}" = "x86" ]; then
        conf_args=('--host=i686-android-linux' '--target=i686-android-linux')
        cflags+=" -fno-pic"
    else
        conf_args=()
    fi

    CFLAGS="${cflags}" \
    CXXFLAGS="${cxxflags}" \
    CPPFLAGS="${cppflags}" \
    ./configure \
        --prefix=/data/local/Inst \
        ${conf_args[@]} \
        --with-tmpdir=/sdcard

    CFLAGS="${cflags}" \
    CXXFLAGS="${cxxflags}" \
    CPPFLAGS="${cppflags}" \
    make -j4

    make install DESTDIR="$(pwd)/valgrind-inst"

    pushd valgrind-inst
    find -mindepth 1 -maxdepth 1 -print0 \
            | tar cv --null -T - \
            | 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -si \
                    ../../output/${arch}/valgrind.tar.7z
    popd

    popd

    pop_android
}

# Does not compile with the android-L platform
build_valgrind armeabi-v7a arm-linux-androideabi 4.9 android-18
# Valgrind 3.10.0 supports aarch64, but not when combined with the NDK compiler
#build_valgrind arm64-v8a aarch64-linux-android 4.9 android-L
# Does not compile with the android-L platform
build_valgrind x86 i686-linux-android 4.9 android-18
# Valgrind does not support Android x86_64
#build_valgrind x86_64 ${toolchain_host} ${temp_prefix_x86_64}


################################################################################
# Build tarballs
################################################################################
curdate="$(date +'%Y%m%d')"
echo "${curdate}" > output/PREBUILTS_VERSION_ANDROID.txt
echo "${curdate}" > output/PREBUILTS_VERSION_ANDROID_VALGRIND.txt

# "cmake -E tar" (CMake 3.0.1) doesn't like xz on Windows, so we'll use bz2
if [ ! -f prebuilts-android-${curdate}.tar.bz2 ]; then
    tar jcvf prebuilts-android-${curdate}.tar.bz2 \
        output/PREBUILTS_VERSION_ANDROID.txt \
        output/*/{patch,{g,s}etf{acl,attr}}
fi

if [ ! -f prebuilts-android-valgrind-${curdate}.tar ]; then
    tar cvf prebuilts-android-valgrind-${curdate}.tar \
        output/PREBUILTS_VERSION_ANDROID_VALGRIND.txt \
        output/*/valgrind.tar.7z
fi
