#!/bin/bash

# Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

url="https://android.googlesource.com/platform/external/libxml2"
ver='android-5.0.0_r6'

set -e
cd "$(dirname "${0}")"

if [ ! -d libxml2 ]; then
    git clone "${url}" libxml2
else
    pushd libxml2
    git checkout master
    git pull
    popd
fi

pushd libxml2
git checkout "${ver}"
git am ../0001-libxml2-Don-t-build-with-icu-support.patch
git am ../0002-libxml2-Remove-BUILD_HOST_STATIC_LIBRARY.patch

arches=(armeabi-v7a arm64-v8a x86 x86_64)
ndk-build \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=./Android.mk \
    APP_ABI="${arches[*]}" \
    -j4

outdir="$(mktemp -d)"

# Header files
mkdir -p "${outdir}"/include/libxml2/
cp -r include/libxml/ "${outdir}"/include/libxml2/

# Static libraries
for arch in "${arches[@]}"; do
    mkdir "${outdir}"/lib_${arch}/
    cp obj/local/${arch}/libxml2.a "${outdir}"/lib_${arch}/
done

if [ ! -f ../libxml2_${ver}.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/../libxml2_${ver}.tar.bz2 lib_*/ include/
    popd
fi

rm -rf "${outdir}"

popd

