#!/bin/bash

set -e

source ../compile/env.sh
setup_toolchain

if [ ! -d jsoncpp ]; then
    git clone https://github.com/jacobsa/jsoncpp.git
fi

pushd jsoncpp/
    git fetch
    git checkout 47f1577fd38c3810469049bd5de5956a44954ae6
    python2 amalgamate.py
popd

${CXX} -g -O0 -static \
    -std=c++11 \
    syncdaemon.cpp \
    common.cpp \
    configfile.cpp \
    jsoncpp/dist/jsoncpp.cpp \
    -Ijsoncpp/dist

#${STRIP} a.out
#upx -9 a.out

# /data/local/Inst/bin/valgrind -v --leak-check=full --show-leak-kinds=all --track-origins=yes ./a.out
