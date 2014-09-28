#!/bin/bash

cm="https://github.com/CyanogenMod"
aosp="https://android.googlesource.com"

cm_ver=cm-11.0
aosp_ver=android-4.4.4_r2

clone=(
    build::${cm}/android_build::${cm_ver}
    bionic::${cm}/android_bionic::${cm_ver}
    external/busybox::${cm}/android_external_busybox::${cm_ver}
    external/libselinux::${cm}/android_external_libselinux::${cm_ver}
    external/libsepol::${cm}/android_external_libsepol::${cm_ver}
    prebuilts/gcc/linux-x86/arm/arm-eabi-4.7::${aosp}/platform/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7::${aosp_ver}
    prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.7::${aosp}/platform/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.7::${aosp_ver}
    system/core::${cm}/android_system_core::${cm_ver}
)

set -e
cd "$(dirname "${0}")"

mkdir -p cm-busybox/
pushd cm-busybox/

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
git am ../../0001-Don-t-fail-when-JDK-isn-t-installed.patch
git am ../../0002-Use-Python-2.patch
popd

. build/envsetup.sh
lunch aosp_arm-eng
make -j8 static_busybox WITHOUT_LIBCOMPILER_RT=true

popd

mkdir -p output/armeabi-v7a/
cp cm-busybox/out/target/product/generic/utilities/busybox \
    output/armeabi-v7a/busybox-static


################################################################################
# Build tarballs
################################################################################
curdate="$(date +'%Y%m%d')"
echo "${curdate}" > output/PREBUILTS_VERSION_ANDROID_BUSYBOX.txt

# "cmake -E tar" (CMake 3.0.1) doesn't like xz on Windows, so we'll use bz2
if [ ! -f prebuilts-android-busybox-${curdate}.tar.bz2 ]; then
    tar jcvf prebuilts-android-busybox-${curdate}.tar.bz2 \
        output/PREBUILTS_VERSION_ANDROID_BUSYBOX.txt \
        output/*/busybox-static
fi
