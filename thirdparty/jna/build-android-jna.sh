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

url='https://github.com/twall/jna.git'
ver='4.1.0'

mkdir -p jna/
cd jna/

if [ ! -d jna ]; then
    git clone "${url}" jna
else
    pushd jna
    git reset --hard HEAD
    git checkout master
    git pull
    popd
fi

pushd jna
git checkout ${ver}
git am ../../0001-Fix-Android-build.patch
git am ../../0001-Exclude-libjnidispatch-for-non-Android-OS-s.patch

# Remove unneeded libs for other OS's and architectures
rm \
    {lib/native,dist}/darwin.jar \
    {lib/native,dist}/freebsd-x86.jar \
    {lib/native,dist}/freebsd-x86-64.jar \
    {lib/native,dist}/linux-arm.jar \
    {lib/native,dist}/linux-x86.jar \
    {lib/native,dist}/linux-x86-64.jar \
    {lib/native,dist}/openbsd-x86.jar \
    {lib/native,dist}/openbsd-x86-64.jar \
    {lib/native,dist}/sunos-sparc.jar \
    {lib/native,dist}/sunos-sparcv9.jar \
    {lib/native,dist}/sunos-x86.jar \
    {lib/native,dist}/sunos-x86-64.jar \
    {lib/native,dist}/w32ce-arm.jar \
    {lib/native,dist}/win32-x86.jar \
    {lib/native,dist}/win32-x86-64.jar

ant -Dos.prefix=android-arm native
ant -Dos.prefix=android-arm64 native
ant -Dos.prefix=android-x86 native
# Fails to build right now
#ant -Dos.prefix=android-x86_64 native

ant dist
popd

outdir="$(mktemp -d)"

#mkdir -p "${outdir}"/lib/{armeabi-v7a,arm64-v8a,x86,x86_64}/
mkdir -p "${outdir}"/lib/{armeabi-v7a,arm64-v8a,x86}/

cp jna/dist/jna.jar "${outdir}"
cp jna/build/native-android-arm/libjnidispatch.so "${outdir}"/lib/armeabi-v7a/
cp jna/build/native-android-arm64/libjnidispatch.so "${outdir}"/lib/arm64-v8a/
cp jna/build/native-android-x86/libjnidispatch.so "${outdir}"/lib/x86/
#cp jna/build/native-android-x86_64/libjnidispatch.so "${outdir}"/lib/x86_64/

curdir="$(pwd)"
pushd "${outdir}"
tar jcvf "${curdir}"/../jna-${ver}_android.tar.bz2 \
    lib/*/libjnidispatch.so jna.jar
popd

rm -rf "${outdir}"
