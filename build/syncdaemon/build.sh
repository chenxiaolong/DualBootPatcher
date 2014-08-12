#!/bin/bash

set -e

cd "$(dirname "${0}")"

source ../compile/env.sh

export ANDROID_NDK_HOME="$(get_conf builder android-ndk)"
export PATH="${PATH}:${ANDROID_NDK_HOME}"

if [ ! -d jsoncpp ]; then
    git clone https://github.com/open-source-parsers/jsoncpp.git
fi

pushd jsoncpp/
    git fetch
    git checkout 60f778b9fcd1c59908293a6145dffd369d4f6daf
    python2 amalgamate.py
popd

ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk clean
ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=Application.mk

# Static analyzer
if false; then
    cppcheck \
        --enable=all \
        --inconclusive \
        common.cpp \
        configfile.cpp \
        syncdaemon.cpp \
        -Ijsoncpp/dist \
        --force
fi

# x86 build
if false; then
    g++ \
        -DVERSION=\"$(get_conf builder version)\" \
        -std=c++11 \
        common.cpp \
        configfile.cpp \
        syncdaemon.cpp \
        jsoncpp/dist/jsoncpp.cpp \
        -osyncdaemon_x86 \
        -Ijsoncpp/dist \
        -Wall \
        -Wextra \
        -pedantic
fi

cp libs/armeabi-v7a/syncdaemon .

#if [[ "${buildtype}" == release ]] && which upx >/dev/null; then
#    upx --lzma syncdaemon
#fi

# /data/local/Inst/bin/valgrind -v --leak-check=full --show-leak-kinds=all --track-origins=yes ./a.out
