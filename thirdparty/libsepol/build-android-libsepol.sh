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

url="https://github.com/SELinuxProject/selinux.git"
ver="2.3"

set -e

if [ ! -d selinux ]; then
    git clone "${url}" selinux
else
    pushd selinux
    git checkout master
    git pull
    popd
fi

pushd selinux/libsepol
git checkout "libsepol-${ver}"
git am ../../0001-Build-for-Android-targets.patch
git am ../../0001-Build-static-library.patch

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
cp -r include/sepol "${outdir}"/include/

# Static libraries
for arch in "${arches[@]}"; do
    mkdir "${outdir}"/lib_${arch}/
    cp obj/local/${arch}/libsepol.a "${outdir}"/lib_${arch}/
done

if [ ! -f ../../libsepol-${ver}_android.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/../../libsepol-${ver}_android.tar.bz2 \
        lib_*/ include/
    popd
fi

rm -rf "${outdir}"

popd
