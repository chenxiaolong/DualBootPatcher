#!/bin/bash

setup_toolchain() {
    export HOST_TOOLCHAIN_PREFIX="x86_64-unknown-linux-gnu"
    export ANDROIDSDK="/opt/android-sdk"
    export ANDROIDNDK="/opt/android-ndk"
    export ANDROIDNDKVER=r9
    export ANDROIDAPI=18

    local CURDIR=$(cd "$(dirname "${BASH_SOURCE}")" && pwd)

    export NDKPLATFORM="${CURDIR}"/gcc

    export TOOLCHAIN_PREFIX=arm-linux-androideabi
    export TOOLCHAIN_VERSION=4.8

    if [ ! -d "${NDKPLATFORM}" ]; then
        ${ANDROIDNDK}/build/tools/make-standalone-toolchain.sh \
            --verbose \
            --platform=android-${ANDROIDAPI} \
            --install-dir="${NDKPLATFORM}" \
            --ndk-dir="${ANDROIDNDK}" \
            --system=linux-x86_64 \
            --toolchain=${TOOLCHAIN_PREFIX}-${TOOLCHAIN_VERSION}
    fi

    export PATH="${NDKPLATFORM}/bin:${ANDROIDSDK}/tools:${PATH}"

    export CC="${TOOLCHAIN_PREFIX}-gcc"
    export CXX="${TOOLCHAIN_PREFIX}-g++"
    export CPP="${TOOLCHAIN_PREFIX}-cpp"
    export AR="${TOOLCHAIN_PREFIX}-ar"
    export AS="${TOOLCHAIN_PREFIX}-as"
    export LD="${TOOLCHAIN_PREFIX}-ld"
    export OBJCOPY="${TOOLCHAIN_PREFIX}-objcopy"
    export OBJDUMP="${TOOLCHAIN_PREFIX}-objdump"
    export RANLIB="${TOOLCHAIN_PREFIX}-ranlib"
    export STRIP="${TOOLCHAIN_PREFIX}-strip"
}
