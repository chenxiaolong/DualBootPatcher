#!/bin/bash

get_conf() {
    local CURDIR=$(cd "$(dirname "${BASH_SOURCE}")" && pwd)
    local conf="${CURDIR}/../build.conf"
    local conf_custom="${CURDIR}/../build.custom.conf"

    python3 -c "
import configparser
import os

config = configparser.ConfigParser()
config.read('${conf}')

if os.path.exists('${conf_custom}'):
    config.read('${conf_custom}')

print(config['$1']['$2'])
"
}

setup_toolchain() {
    export HOST_TOOLCHAIN_PREFIX="$(get_conf builder host-toolchain)"
    export ANDROIDSDK="$(get_conf builder android-sdk)"
    export ANDROIDNDK="$(get_conf builder android-ndk)"
    export ANDROIDNDKVER="$(get_conf builder android-ndk-ver)"
    export ANDROIDAPI="$(get_conf builder android-api)"

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
