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

set -e

url='https://android.googlesource.com/platform/external/e2fsprogs'
tag='android-5.0.2_r1'

if [ ! -d e2fsprogs/ ]; then
    git clone "${url}" e2fsprogs
else
    pushd e2fsprogs
    git checkout master
    git pull
    popd
fi

pushd e2fsprogs
git checkout "${tag}"
git am ../0001-Fixes.patch

arches=(armeabi-v7a arm64-v8a x86 x86_64)
ndk-build \
    NDK_PROJECT_PATH=. \
    NDK_TOOLCHAIN_VERSION=4.9 \
    APP_BUILD_SCRIPT=Android.mk \
    APP_ABI="${arches[*]}" \
    APP_PLATFORM=android-21 \
    -j4

outdir="$(mktemp -d)"
cp -a libs/. "${outdir}"

if [ ! -f ../e2fsprogs-${tag}_android.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/../e2fsprogs-${tag}_android.tar.bz2 \
        "${arches[@]}"
    popd
fi

rm -rf "${outdir}"

popd
