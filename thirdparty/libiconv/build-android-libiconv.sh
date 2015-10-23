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

ver="1.14"
rel=1
name="libiconv-${ver}"
tar="${name}.tar.gz"
url="http://ftp.gnu.org/gnu/libiconv/${tar}"

set -e

cd "$(dirname "${0}")"

mkdir -p libiconv
cd libiconv

if [ ! -f "${tar}" ]; then
    wget "${url}"
fi

if [ ! -d "${name}" ]; then
    tar xvf "${tar}"
fi

pushd "${name}"
patch -p1 -i ../../0001-Android-doesn-t-have-langinfo.h.patch
patch -p1 -i ../../0002-Add-generated-headers-for-Android.patch
patch -p1 -i ../../0003-Add-Android.mk.patch

arches=(armeabi-v7a arm64-v8a x86 x86_64)
ndk-build \
    NDK_PROJECT_PATH=. \
    NDK_TOOLCHAIN_VERSION=4.9 \
    APP_BUILD_SCRIPT=Android.mk \
    APP_ABI="${arches[*]}" \
    APP_PLATFORM=android-21 \
    -j4

outdir="$(mktemp -d)"

# Copy static libraries
for arch in "${arches[@]}"; do
    mkdir "${outdir}/lib_${arch}/"
    cp "obj/local/${arch}/libiconv.a" "${outdir}/lib_${arch}/"
done

# Copy headers
mkdir "${outdir}"/include/
cp include/iconv.h.inst "${outdir}"/include/iconv.h
cp libcharset/include/localcharset.h.inst "${outdir}"/include/localcharset.h

tar jcvf ../../libiconv-${ver}-${rel}_android.tar.bz2 -C "${outdir}" \
    "${arches[@]/#/lib_}" include \

rm -rf "${outdir}"

popd
