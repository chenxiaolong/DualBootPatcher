#!/bin/bash

VERSION="1.17.3"
MINGW_PREFIX=i486-mingw32-

set -e

if [ "${#}" -lt 1 ]; then
  echo "Usage: ${0} [path to Android source code]"
  exit 1
fi

ANDROID_DIR="${1}/system/core/"
CURDIR="$(cd "$(dirname "${0}")" && cd ..  && pwd)"
cd "${CURDIR}"

create_portable_python() {
  local URL="http://ftp.osuosl.org/pub/portablepython/v3.2/PortablePython_3.2.5.1.exe"
  local MD5SUM="5ba055a057ce4fe1950a0f1f7ebae323"
  if [ ! -f windowsbinaries/pythonportable.exe ] || \
    ! md5sum windowsbinaries/pythonportable.exe | grep -q ${MD5SUM}; then
    mkdir -p windowsbinaries
    if which axel >/dev/null; then
      axel -an10 "${URL}" -o windowsbinaries/pythonportable.exe
    else
      wget "${URL}" -O windowsbinaries/pythonportable.exe
    fi
  fi

  rm -rf pythonportable/

  local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
  pushd "${TEMPDIR}"
  7z x ../windowsbinaries/pythonportable.exe \*/App

  # Weird filename encoding in NSIS installer
  COUNTER=0
  for i in *; do
    mv "${i}" ${COUNTER}
    let COUNTER+=1
  done

  FOUND=""
  for i in *; do
    if [ -f "${i}/App/python.exe" ]; then
      FOUND="${i}"
      break
    fi
  done
  if [ -z "${FOUND}" ]; then
    echo "No python.exe found"
    exit 1
  fi
  find . -mindepth 1 -maxdepth 1 ! -name "${FOUND}" | xargs rm -rf

  mv "${FOUND}/App/" ../pythonportable/

  popd

  rm -rf "${TEMPDIR}"

  pushd pythonportable/

  # Don't add unneeded files to the zip
  rm -r DLLs
  rm -r Doc
  rm -r include
  rm -r libs
  rm -r locale
  rm -r Scripts
  rm -r tcl
  rm -r Tools
  rm NEWS.txt
  rm PyScripter.*
  rm python-3.2.5.msi
  rm README.txt
  rm remserver.py
  rm -rf Lib/__pycache__
  rm -r Lib/{concurrent,ctypes,curses}
  rm -r Lib/{dbm,distutils}
  rm -r Lib/email
  rm -r Lib/{html,http}
  rm -r Lib/{idlelib,importlib}
  rm -r Lib/json
  rm -r Lib/{lib2to3,logging}
  rm -r Lib/{msilib,multiprocessing}
  rm -r Lib/pydoc_data
  rm -r Lib/{site-packages,sqlite3}
  rm -r Lib/{test,tkinter,turtledemo}
  rm -r Lib/wsgiref
  rm -r Lib/{unittest,urllib}
  rm -r Lib/{xml,xmlrpc}

  rm Lib/{__future__.py,__phello__.foo.py,_compat_pickle.py,_dummy_thread.py,_markupbase.py,_osx_support.py,_pyio.py,_strptime.py,_threading_local.py}
  rm Lib/{aifc.py,antigravity.py,argparse.py,ast.py}
  rm Lib/{base64.py,bdb.py,binhex.py}
  rm Lib/{calendar.py,cgi.py,cgitb.py,chunk.py,cmd.py,code.py,codeop.py,colorsys.py,compileall.py,configparser.py,contextlib.py,cProfile.py,csv.py}
  rm Lib/{datetime.py,decimal.py,difflib.py,dis.py,doctest.py,dummy_threading.py}
  rm Lib/{filecmp.py,fileinput.py,formatter.py,fractions.py,ftplib.py}
  rm Lib/{getopt.py,getpass.py,gettext.py,glob.py}
  rm Lib/hmac.py
  rm Lib/{imaplib.py,imghdr.py,inspect.py}
  rm Lib/{macpath.py,macurl2path.py,mailbox.py,mailcap.py,mimetypes.py,modulefinder.py}
  rm Lib/{netrc.py,nntplib.py,nturl2path.py,numbers.py}
  rm Lib/{opcode.py,optparse.py,os2emxpath.py}
  rm Lib/{pdb.py,pickle.py,pickletools.py,pipes.py,pkgutil.py,platform.py,plistlib.py,poplib.py,ppp.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
  rm Lib/{queue.py,quopri.py}
  rm Lib/{rlcompleter.py,runpy.py}
  rm Lib/{sched.py,shelve.py,shlex.py,smtpd.py,smtplib.py,sndhdr.py,socket.py,socketserver.py,ssl.py,string.py,stringprep.py,sunau.py,symbol.py,symtable.py}
  rm Lib/{tabnanny.py,telnetlib.py,textwrap.py,this.py,timeit.py,tty.py,turtle.py}
  rm Lib/{uu.py,uuid.py}
  rm Lib/{wave.py,webbrowser.py,wsgiref.egg-info}
  rm Lib/xdrlib.py

  popd
}

build_windows() {
  pushd "${ANDROID_DIR}"
  cp mkbootimg/mkbootimg.c mkbootimg/mkbootimg.windows.c
  cp mkbootimg/unpackbootimg.c mkbootimg/unpackbootimg.windows.c
  patch -p1 -i "${CURDIR}/patches/0001-I-hate-Windows-with-a-passion.patch"

  ${MINGW_PREFIX}gcc -static -Iinclude \
    mkbootimg/mkbootimg.windows.c \
    libmincrypt/sha.c \
    -o "${TARGETDIR}/binaries/mkbootimg.exe"

  ${MINGW_PREFIX}gcc -static -Iinclude \
    mkbootimg/unpackbootimg.windows.c \
    -o "${TARGETDIR}/binaries/unpackbootimg.exe"

  strip "${TARGETDIR}"/binaries/{mkbootimg,unpackbootimg}.exe
  rm mkbootimg/mkbootimg.windows.c mkbootimg/unpackbootimg.windows.c
  popd

  mkdir -p windowsbinaries
  pushd windowsbinaries

  local URLBASE="http://downloads.sourceforge.net/project/mingw/MSYS"

  if [ ! -f patch-2.6.1-1-msys-1.0.13-bin.tar.lzma ]; then
    wget "${URLBASE}/Extension/patch/patch-2.6.1-1/patch-2.6.1-1-msys-1.0.13-bin.tar.lzma"
  fi

  if [ ! -f msysCORE-1.0.18-1-msys-1.0.18-bin.tar.lzma ]; then
    wget "${URLBASE}/Base/msys-core/msys-1.0.18-1/msysCORE-1.0.18-1-msys-1.0.18-bin.tar.lzma"
  fi

  if [ ! -f libintl-0.18.1.1-1-msys-1.0.17-dll-8.tar.lzma ]; then
    wget "${URLBASE}/Base/gettext/gettext-0.18.1.1-1/libintl-0.18.1.1-1-msys-1.0.17-dll-8.tar.lzma"
  fi

  if [ ! -f libiconv-1.14-1-msys-1.0.17-dll-2.tar.lzma ]; then
    wget "${URLBASE}/Base/libiconv/libiconv-1.14-1/libiconv-1.14-1-msys-1.0.17-dll-2.tar.lzma"
  fi

  if [ ! -f liblzma-5.0.3-1-msys-1.0.17-dll-5.tar.lzma ]; then
    wget "${URLBASE}/Base/xz/xz-5.0.3-1/liblzma-5.0.3-1-msys-1.0.17-dll-5.tar.lzma"
  fi

  if [ ! -f xz-5.0.3-1-msys-1.0.17-bin.tar.lzma ]; then
    wget "${URLBASE}/Base/xz/xz-5.0.3-1/xz-5.0.3-1-msys-1.0.17-bin.tar.lzma"
  fi

  tar Jxvf patch-2.6.1-1-msys-1.0.13-bin.tar.lzma bin/patch.exe \
    --to-stdout > "${TARGETDIR}/binaries/hctap.exe"
  tar Jxvf msysCORE-1.0.18-1-msys-1.0.18-bin.tar.lzma bin/msys-1.0.dll \
    --to-stdout > "${TARGETDIR}/binaries/msys-1.0.dll"
  tar Jxvf libintl-0.18.1.1-1-msys-1.0.17-dll-8.tar.lzma bin/msys-intl-8.dll \
    --to-stdout > "${TARGETDIR}/binaries/msys-intl-8.dll"
  tar Jxvf libiconv-1.14-1-msys-1.0.17-dll-2.tar.lzma bin/msys-iconv-2.dll \
    --to-stdout > "${TARGETDIR}/binaries/msys-iconv-2.dll"
  tar Jxvf liblzma-5.0.3-1-msys-1.0.17-dll-5.tar.lzma bin/msys-lzma-5.dll \
    --to-stdout > "${TARGETDIR}/binaries/msys-lzma-5.dll"
  tar Jxvf xz-5.0.3-1-msys-1.0.17-bin.tar.lzma bin/xz.exe \
    --to-stdout > "${TARGETDIR}/binaries/xz.exe"

  chmod +x "${TARGETDIR}"/binaries/*.{exe,dll}
  popd
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
      find . | cpio -o -H newc > \
        "${TARGETDIR}/ramdisks/$(basename "${i}").cpio"
      popd
    fi
  done

  pushd "${TARGETDIR}/ramdisks/"
  tar cvf - *.cpio | xz -9 > ramdisks.tar.xz
  rm *.cpio
  popd
}

TARGETNAME="DualBootPatcher-${VERSION}"
TARGETDIR="${CURDIR}/${TARGETNAME}"
rm -rf "${TARGETDIR}" "${TARGETNAME}.zip"
mkdir -p "${TARGETDIR}/binaries" "${TARGETDIR}/ramdisks"

# Build and copy stuff into target directory
create_portable_python
build_windows
build_linux
compress_ramdisks

cp -rt "${TARGETDIR}" \
  $(git ls-tree --name-only --full-tree HEAD | grep -v .gitignore) pythonportable

# Remove unneeded files
rm -r pythonportable/
rm "${TARGETDIR}/patches/0001-I-hate-Windows-with-a-passion.patch"
find "${TARGETDIR}/ramdisks/" -mindepth 1 -maxdepth 1 -type d | xargs rm -rf

# Create zip
zip -r ${TARGETNAME}.zip ${TARGETNAME}/
echo "Successfully created: ${CURDIR}/${TARGETNAME}.zip"
