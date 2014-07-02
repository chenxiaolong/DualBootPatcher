#!/bin/bash

set -e

source env.sh
setup_toolchain

VALGRIND_VER=3.9.0

export NDK=${ANDROIDNDK}
export NDKROOT=${ANDROIDNDK}

export HWKIND=nexus_s
export CFLAGS="-DANDROID_HARDWARE_${HWKIND}"
export CXXFLAGS="${CFLAGS}"

CURDIR="$(cd "$(dirname "${0}")" && pwd)"
BUILDDIR="${CURDIR}/build/"
mkdir -p "${BUILDDIR}"

pushd "${BUILDDIR}"

if [ ! -f valgrind-${VALGRIND_VER}.tar.bz2 ]; then
    wget "http://valgrind.org/downloads/valgrind-${VALGRIND_VER}.tar.bz2"
fi

rm -rf valgrind-${VALGRIND_VER}/
tar jxvf valgrind-${VALGRIND_VER}.tar.bz2

pushd valgrind-${VALGRIND_VER}/
./configure \
    --prefix=/data/local/Inst \
    --host=armv7-unknown-linux \
    --target=armv7-unknown-linux \
    --with-tmpdir=/sdcard 

make -j4
make install DESTDIR=$(pwd)/../../valgrind

popd
popd
