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

if [ ! -f ../boost-${version}_armeabi-v7a.tar.bz2 ]; then
    BUILD_DIR=build_armeabi-v7a build --toolchain=arm-linux-androideabi-4.9
    pushd build_armeabi-v7a
    tar jcvf ../../boost-${version}_armeabi-v7a.tar.bz2 include/ lib/
    popd
fi

if [ ! -f ../boost-${version}_arm64-v8a.tar.bz2 ]; then
    BUILD_DIR=build_arm64-v8a build --toolchain=aarch64-linux-android-4.9
    pushd build_arm64-v8a
    tar jcvf ../../boost-${version}_arm64-v8a.tar.bz2 include/ lib/
    popd
fi
  
if [ ! -f ../boost-${version}_x86.tar.bz2 ]; then
    BUILD_DIR=build_x86 build --toolchain=x86-4.9
    pushd build_x86
    tar jcvf ../../boost-${version}_x86.tar.bz2 include/ lib/
    popd
fi

if [ ! -f ../boost-${version}_x86_64.tar.bz2 ]; then
    BUILD_DIR=build_x86_64 build --toolchain=x86_64-4.9
    pushd build_x86_64
    tar jcvf ../../boost-${version}_x86_64.tar.bz2 include/ lib/
    popd
fi

popd