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

# Chainfire's repo is the same as the upstream repo except that it has commit
# da6e0d7ee142806e5f80ffdc3253535ea0f13159, which contains some build fixes
# for Android.

#url="https://github.com/SELinuxProject/selinux.git"
#branch="master"
#commit="b1bbd3030be095b5e5c49c6f899ed8071fb05f30"
url="https://github.com/Chainfire/selinux.git"
branch="android"
commit="da6e0d7ee142806e5f80ffdc3253535ea0f13159"

curdate="$(date +%Y%m%d)"

set -e

if [ ! -d selinux ]; then
    git clone "${url}" selinux
else
    pushd selinux
    git checkout "${branch}"
    git pull
    popd
fi

pushd selinux/libsepol
git checkout "${commit}"
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

if [ ! -f ../../libsepol_${curdate}_android.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/../../libsepol_${curdate}_android.tar.bz2 \
        lib_*/ include/
    popd
fi

rm -rf "${outdir}"

popd
