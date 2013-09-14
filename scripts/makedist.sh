#!/bin/bash

VERSION="1.0"
MINGW_PREFIX=i486-mingw32-

set -e

if [ "${#}" -lt 1 ]; then
  echo "Usage: ${0} [path to Android source code]"
  exit 1
fi

ANDROID_DIR="${1}/system/core/"
CURDIR="$(cd "$(dirname "${0}")" && cd ..  && pwd)"
cd "${CURDIR}"

build_windows() {
  pushd "${ANDROID_DIR}"
  ${MINGW_PREFIX}gcc -static -Iinclude \
    mkbootimg/mkbootimg.c \
    libmincrypt/sha.c \
    -o "${TARGETDIR}/binaries/mkbootimg.exe"

  ${MINGW_PREFIX}gcc -static -Iinclude \
    mkbootimg/unpackbootimg.c \
    -o "${TARGETDIR}/binaries/unpackbootimg.exe"

  strip "${TARGETDIR}"/binaries/{mkbootimg,unpackbootimg}.exe
  popd

  wget 'https://downloads.sourceforge.net/project/gnuwin32/patch/2.5.9-7/patch-2.5.9-7-bin.zip'
  unzip -p patch-2.5.9-7-bin.zip bin/patch.exe > \
    "${TARGETDIR}/binaries/patch.exe"
  rm -v patch-2.5.9-7-bin.zip
}

build_linux() {
  pushd "${ANDROID_DIR}"
  gcc -m32 -static -Iinclude \
    mkbootimg/mkbootimg.c \
    libmincrypt/sha.c \
    -o "${TARGETDIR}/binaries/mkbootimg"

  gcc -m32 -static -Iinclude \
    mkbootimg/unpackbootimg.c \
    -o "${TARGETDIR}/binaries/unpackbootimg"

  strip "${TARGETDIR}"/binaries/{mkbootimg,unpackbootimg}
  popd
}

compress_ramdisks() {
  for i in ramdisks/*; do
    if [ -d "${i}" ]; then
      pushd "${i}"
      find . | cpio -o -H newc | gzip > \
        "${TARGETDIR}/ramdisks/$(basename "${i}").cpio.gz"
      popd
    fi
  done
}

TARGETNAME="DualBootPatcher-${VERSION}"
TARGETDIR="${CURDIR}/${TARGETNAME}"
rm -rf "${TARGETDIR}" "${TARGETNAME}.zip"
mkdir -p "${TARGETDIR}/binaries" "${TARGETDIR}/ramdisks"

# Build and copy stuff into target directory
build_windows
build_linux
compress_ramdisks

cp -rt "${TARGETDIR}" \
  $(git ls-tree --name-only --full-tree HEAD | grep -v .gitignore)

# Remove unneeded files
find "${TARGETDIR}/ramdisks/" -mindepth 1 -maxdepth 1 -type d | xargs rm -rf

# Create zip
zip -r ${TARGETNAME}.zip ${TARGETNAME}/
echo "Successfully created: ${CURDIR}/${TARGETNAME}.zip"
