#!/bin/bash

set -e

if [ "${#}" -lt 1 ]; then
  echo "Usage: ${0} <device> [release|debug|none]"
  exit
fi

DEFAULT_DEVICE=${1}

BUILDTYPE="none"
if [[ ! -z "${2}" ]]; then
  BUILDTYPE="${2}"
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
. "${BUILDDIR}/shortcuts.sh"


COPY=(
  'README.jflte.txt'
  'defaults.conf'
  'patch-file.sh'
  'patches/'
  'patchinfo/'
  'ramdisks/'
  'scripts/'
  'useful/'
)


# Build PC version
TARGETNAME="DualBootPatcher-${VERSION}-${DEFAULT_DEVICE}"
TARGETDIR="${DISTDIR}/${TARGETNAME}"
if [ "x${BUILDTYPE}" = "xci" ]; then
    TARGETFILE="${DISTDIR}/${TARGETNAME}.7z"
else
    TARGETFILE="${DISTDIR}/${TARGETNAME}.zip"
fi
rm -rf "${TARGETDIR}" "${DISTDIR}"/${TARGETNAME}.{7z,zip}
mkdir -p "${TARGETDIR}"

# Build and copy stuff into target directory
create_python_windows
create_pyqt_windows
create_binaries_windows
create_shortcuts_windows
create_shortcuts_linux

cp -rt "${TARGETDIR}" ${COPY[@]}

cp Android_GUI/ic_launcher-web.png "${TARGETDIR}/scripts/icon.png"

# Set default device
sed -i "s/@DEFAULT_DEVICE@/${DEFAULT_DEVICE}/g" "${TARGETDIR}/defaults.conf"

pushd "${DISTDIR}"
if [ "x${BUILDTYPE}" = "xci" ]; then
    7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on \
        "${TARGETFILE}" "${TARGETNAME}/"
else
    zip -r "${TARGETFILE}" "${TARGETNAME}/"
fi
popd

rm -r "${TARGETDIR}"


if [ "x${BUILDTYPE}" != "xnone" ]; then
    # Android version
    TARGETNAME="DualBootPatcherAndroid-${VERSION}"
    TARGETDIR="${DISTDIR}/${TARGETNAME}"
    TARGETFILE="${DISTDIR}/${TARGETNAME}.tar.xz"
    rm -rf "${TARGETDIR}" "${TARGETFILE}"
    mkdir -p "${TARGETDIR}"

    create_python_android
    create_binaries_android

    cp -rt "${TARGETDIR}" ${COPY[@]}

    # Set default device
    sed -i "s/@DEFAULT_DEVICE@/${DEFAULT_DEVICE}/g" "${TARGETDIR}/defaults.conf"

    # Remove PC stuff from Android tar
    find "${TARGETDIR}" -name '*.bat' -delete

    pushd "${DISTDIR}"
    tar Jcvf ${TARGETFILE} ${TARGETNAME}/
    popd

    # Android app
    ANDROIDGUI=${CURDIR}/Android_GUI/

    rm -rf "${ANDROIDGUI}/assets/"
    mkdir "${ANDROIDGUI}/assets/"
    mv "${TARGETFILE}" "${ANDROIDGUI}/assets/"
    cp "${CURDIR}/ramdisks/busybox-static" "${ANDROIDGUI}/assets/tar"
    rm -f "${DISTDIR}"/${TARGETNAME}-${DEFAULT_DEVICE}*.apk

    if [ "x${BUILDTYPE}" = "xrelease" ]; then
        build_android_app release
    else
        build_android_app
    fi

    rm -r "${TARGETDIR}"
fi


echo
echo "Done."
