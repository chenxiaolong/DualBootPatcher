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

ver="5.2.0"
name="xz-${ver}"
tar="${name}.tar.xz"
url="http://tukaani.org/xz/${tar}"

set -e

if [ ! -f "${tar}" ]; then
    wget "${url}"
fi

if [ ! -d "${name}" ]; then
    tar xvf "${tar}"
fi

pushd "${name}"
patch -p1 -i ../0001-Android-build-system.patch

arches=(armeabi-v7a arm64-v8a x86 x86_64)
ndk-build \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=./Android.mk \
    APP_ABI="${arches[*]}" \
    APP_PLATFORM=android-21 \
    -j4

outdir="$(mktemp -d)"

# Header files
mkdir -p "${outdir}"/include/
cp src/liblzma/api/lzma.h "${outdir}"/include/
cp -r src/liblzma/api/lzma/ "${outdir}"/include/

# Static libraries
for arch in "${arches[@]}"; do
    mkdir "${outdir}"/lib_${arch}/
    cp obj/local/${arch}/liblzma.a "${outdir}"/lib_${arch}/
done

if [ ! -f ../liblzma-${ver}_android.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/../liblzma-${ver}_android.tar.bz2 \
        lib_*/ include/
    popd
fi

rm -rf "${outdir}"

popd
