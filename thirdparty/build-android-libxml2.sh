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

aosp="https://android.googlesource.com"

aosp_ver=android-5.0.0_r6

clone=(
    build::${aosp}/platform/build::${aosp_ver}
    bionic::${aosp}/platform/bionic::${aosp_ver}
    external/icu::${aosp}/platform/external/icu::${aosp_ver}
    external/libxml2::${aosp}/platform/external/libxml2::${aosp_ver}
    external/stlport::${aosp}/platform/external/stlport::${aosp_ver}
    prebuilts/gcc/linux-x86/arm/arm-eabi-4.8::${aosp}/platform/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8::${aosp_ver}
    prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.8::${aosp}/platform/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.8::${aosp_ver}
)

set -e
cd "$(dirname "${0}")"

mkdir -p aosp-libxml2/
pushd aosp-libxml2/

for i in "${clone[@]}"; do
    dir="$(awk -F:: '{print $1}' <<< ${i})"
    repo="$(awk -F:: '{print $2}' <<< ${i})"
    revision="$(awk -F:: '{print $3}' <<< ${i})"

    if [ ! -d "${dir}" ]; then
        git clone "${repo}" "${dir}"
    else
        pushd "${dir}"
        if git show-ref --verify --quiet refs/heads/${revision}; then
            git reset --hard "origin/${revision}"
            git pull
        fi
        popd
    fi

    pushd "${dir}"
    ${revision:+git checkout "${revision}"}
    popd
done

cp build/core/root.mk Makefile

pushd build
git am ../../0001-Don-t-fail-when-GNU-make-isn-t-desired-version.patch
git am ../../0002-Use-Python-2-AOSP.patch
git am ../../0003-Remove-all-Java-checks-AOSP.patch
popd

. build/envsetup.sh
lunch aosp_arm-eng
make -j8 libxml2 WITHOUT_LIBCOMPILER_RT=true

popd

outdir="$(mktemp -d)"

# Static library
mkdir -p "${outdir}"/lib_armeabi-v7a/
cp aosp-libxml2/out/target/product/generic/obj/STATIC_LIBRARIES/libxml2_intermediates/libxml2.a \
    "${outdir}"/lib_armeabi-v7a/

# Header files
mkdir -p "${outdir}"/include/libxml2/
cp -r aosp-libxml2/external/libxml2/include/libxml/ \
    "${outdir}"/include/libxml2/


################################################################################
# Build tarballs
################################################################################

# "cmake -E tar" (CMake 3.0.1) doesn't like xz on Windows, so we'll use bz2
if [ ! -f libxml2_${aosp_ver}.tar.bz2 ]; then
    curdir="$(pwd)"
    pushd "${outdir}"
    tar jcvf "${curdir}"/libxml2_${aosp_ver}.tar.bz2 \
        lib_*/ include/
    popd
fi

rm -rf "${outdir}"
