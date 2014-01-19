#!/bin/bash

set -e

if [ "${#}" -lt 1 ]; then
  echo "Usage: ${0} [device] [release]"
  exit
fi

DEFAULT_DEVICE=${1}

RELEASE=false
if [ "x${2}" = "xrelease" ]; then
  RELEASE=true
fi

CURDIR="$(cd "$(dirname "${0}")" && cd ..  && pwd)"
BUILDDIR="${CURDIR}/build"
DISTDIR="${CURDIR}/dist"
cd "${CURDIR}"

. "${BUILDDIR}/config.sh"
. "${BUILDDIR}/common.sh"
. "${BUILDDIR}/python.sh"
. "${BUILDDIR}/binaries.sh"
. "${BUILDDIR}/android-gui.sh"
. "${BUILDDIR}/qt-gui.sh"


# Build PC version
TARGETNAME="DualBootPatcher-${VERSION}-${DEFAULT_DEVICE}"
TARGETDIR="${DISTDIR}/${TARGETNAME}"
TARGETFILE="${DISTDIR}/${TARGETNAME}.zip"
rm -rf "${TARGETDIR}" "${TARGETFILE}"
mkdir -p "${TARGETDIR}"

# Build and copy stuff into target directory
create_python_windows
build_windows

cp -rt "${TARGETDIR}" \
  $(git ls-tree --name-only --full-tree HEAD | grep -v -e .gitignore -e Android_GUI)

# Set default device
sed -i "s/@DEFAULT_DEVICE@/${DEFAULT_DEVICE}/g" "${TARGETDIR}/defaults.conf"

pushd "${DISTDIR}"
zip -r "${TARGETFILE}" "${TARGETNAME}/"
popd

rm -r "${TARGETDIR}"


# Android version
TARGETNAME="DualBootPatcherAndroid-${VERSION}"
TARGETDIR="${DISTDIR}/${TARGETNAME}"
TARGETFILE="${DISTDIR}/${TARGETNAME}.tar.xz"
rm -rf "${TARGETDIR}" "${TARGETFILE}"
mkdir -p "${TARGETDIR}"

create_python_android
build_android

cp -rt "${TARGETDIR}" \
  $(git ls-tree --name-only --full-tree HEAD | grep -v -e .gitignore -e Android_GUI)

# Set default device
sed -i "s/@DEFAULT_DEVICE@/${DEFAULT_DEVICE}/g" "${TARGETDIR}/defaults.conf"

# Remove PC stuff from Android tar
find "${TARGETDIR}" -name '*.bat' -delete

pushd "${DISTDIR}"
tar Jcvf ${TARGETFILE} ${TARGETNAME}/
popd

# Android app
ANDROIDGUI=${CURDIR}/Android_GUI/

rm -r "${ANDROIDGUI}/assets/"
mkdir "${ANDROIDGUI}/assets/"
mv "${TARGETFILE}" "${ANDROIDGUI}/assets/"
cp "${CURDIR}/ramdisks/busybox-static" "${ANDROIDGUI}/assets/tar"
rm -f "${DISTDIR}"/${TARGETNAME}-${DEFAULT_DEVICE}*.apk

if [ "x${RELEASE}" = "xtrue" ]; then
  build_android_app release
else
  build_android_app
fi

rm -r "${TARGETDIR}"


echo
echo "Done."
