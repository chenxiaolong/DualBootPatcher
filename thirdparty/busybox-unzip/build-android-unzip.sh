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

url='https://github.com/CyanogenMod/android_external_busybox.git'
branch='cm-12.0'
commit='60283c6026060e3793a32439c78553feb7eb93f3'

if [ ! -d busybox/ ]; then
    git clone "${url}" busybox
else
    pushd busybox
    git checkout "${branch}"
    git pull
    popd
fi

pushd busybox
git checkout "${commit}"
git am ../0001-Build-only-the-unzip-applet.patch

arches=(armeabi-v7a arm64-v8a x86 x86_64)
args=(
    NDK_PROJECT_PATH=.
    NDK_TOOLCHAIN_VERSION=4.9
    APP_BUILD_SCRIPT=Android.ndk.mk
    APP_ABI="${arches[@]}"
    APP_PLATFORM=android-21
)

ndk-build "${args[@]}" busybox_prepare
ndk-build "${args[@]}"

outdir="$(mktemp -d)"
cp -a libs/. "${outdir}"
rm "${outdir}"/*/busybox_shared
rename busybox_static unzip "${outdir}"/*/busybox_static

if [ ! -f ../unzip-${branch}_android.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/../unzip-${branch}_android.tar.bz2 \
        "${arches[@]}"
    popd
fi

rm -rf "${outdir}"

popd
