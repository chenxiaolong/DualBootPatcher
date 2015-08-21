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

url_fuse="https://github.com/CyanogenMod/android_external_fuse.git"
url_exfat="https://github.com/CyanogenMod/android_external_exfat.git"
commit_fuse='81b1cc26b41b25afd2d8e09dce1bad4969016066'
commit_exfat='82c66279aea954faa06d224e66bce5f30331b4c0'
ver=''
rel=1

set -e

mkdir -p build/
cd build/

if [ ! -d fuse ]; then
    git clone "${url_fuse}" fuse
else
    pushd fuse
    git fetch
    git checkout "${commit_fuse}"
    popd
fi

if [ ! -d exfat ]; then
    git clone "${url_exfat}" exfat
else
    pushd exfat
    git fetch
    git checkout "${commit_exfat}"
    popd
fi

major=$(sed -nr 's/^.*EXFAT_VERSION_MAJOR\s+(.*)$/\1/p' exfat/libexfat/version.h)
minor=$(sed -nr 's/^.*EXFAT_VERSION_MINOR\s+(.*)$/\1/p' exfat/libexfat/version.h)
patch=$(sed -nr 's/^.*EXFAT_VERSION_PATCH\s+(.*)$/\1/p' exfat/libexfat/version.h)
ver=${major}.${minor}.${patch}

cp ../Android.mk .

arches=(armeabi-v7a arm64-v8a x86 x86_64)
ndk-build \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=./Android.mk \
    APP_ABI="${arches[*]}" \
    APP_PLATFORM=android-21 \
    -j4

outdir="$(mktemp -d)"

for arch in "${arches[@]}"; do
    mkdir "${outdir}/${arch}/"
    cp "libs/${arch}/mount.exfat_static" "${outdir}/${arch}/mount.exfat"
done

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}/../exfat-${ver}-${rel}_android.tar.bz2" \
    --xform 's/\.\///g' \
    ./*/mount.exfat
popd

rm -rf "${outdir}"
