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

set -e

url='https://github.com/MysticTreeGames/Boost-for-Android.git'
commit='8fd5469a492c621b604c4e85231b328f3a62c487'
version='1.53.0'

if [ ! -d Boost-for-Android ]; then
    git clone "${url}" Boost-for-Android
else
    pushd Boost-for-Android
    git checkout master
    git pull
    popd
fi

pushd Boost-for-Android

git checkout "${commit}"
git am ../0001-Allow-build-directory-to-be-changed.patch

build() {
    ./build-android.sh \
        --progress \
        --with-libraries=filesystem,regex,system \
        --boost="${version}"
}

if [ ! -d build_armeabi-v7a/lib/ ]; then
    BUILD_DIR=build_armeabi-v7a build --toolchain=arm-linux-androideabi-4.9
fi

if [ ! -d build_arm64-v8a/lib/ ]; then
    BUILD_DIR=build_arm64-v8a build --toolchain=aarch64-linux-android-4.9
fi
  
if [ ! -d build_x86/lib/ ]; then
    BUILD_DIR=build_x86 build --toolchain=x86-4.9
fi

if [ ! -d build_x86_64/lib/ ]; then
    BUILD_DIR=build_x86_64 build --toolchain=x86_64-4.9
fi

outdir="$(mktemp -d)"

mkdir "${outdir}"/include/

# The headers are the same for each architecture
cp -r build_armeabi-v7a/include/boost-*/boost/ "${outdir}"/include/

cp -r build_armeabi-v7a/lib/ "${outdir}"/lib_armeabi-v7a/
cp -r build_arm64-v8a/lib/ "${outdir}"/lib_arm64-v8a/
cp -r build_x86/lib/ "${outdir}"/lib_x86/
cp -r build_x86_64/lib/ "${outdir}"/lib_x86_64/

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}"/../boost-${version}_android.tar.bz2 \
    include/ lib_*/
popd

popd
