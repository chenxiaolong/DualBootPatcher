#!/bin/bash

VERSION="2.0betaXX"
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
  rm Lib/{pdb.py,pickle.py,pickletools.py,pipes.py,pkgutil.py,plistlib.py,poplib.py,ppp.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
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
  local TD="${TARGETDIR}/binaries/windows/"
  mkdir -p "${TD}"

  pushd "${ANDROID_DIR}"
  cp mkbootimg/mkbootimg.c mkbootimg/mkbootimg.windows.c
  cp mkbootimg/unpackbootimg.c mkbootimg/unpackbootimg.windows.c
  patch -p1 -i "${CURDIR}/patches/0001-I-hate-Windows-with-a-passion.patch"

  ${MINGW_PREFIX}gcc -static -Iinclude \
    mkbootimg/mkbootimg.windows.c \
    libmincrypt/sha.c \
    -o "${TD}/mkbootimg.exe"

  ${MINGW_PREFIX}gcc -static -Iinclude \
    mkbootimg/unpackbootimg.windows.c \
    -o "${TD}/unpackbootimg.exe"

  strip "${TD}"/{mkbootimg,unpackbootimg}.exe
  rm mkbootimg/mkbootimg.windows.c mkbootimg/unpackbootimg.windows.c
  popd

  mkdir -p windowsbinaries
  pushd windowsbinaries

  # Our mini-Cygwin :)
  local URLBASE="http://mirrors.kernel.org/sourceware/cygwin/x86/release"

  # cygwin library
  if [ ! -f cygwin-1.7.25-1.tar.bz2 ]; then
    wget "${URLBASE}/cygwin/cygwin-1.7.25-1.tar.bz2"
  fi

  # libintl
  if [ ! -f libintl8-0.18.1.1-2.tar.bz2 ]; then
    wget "${URLBASE}/gettext/libintl8/libintl8-0.18.1.1-2.tar.bz2"
  fi

  # libiconv
  if [ ! -f libiconv2-1.14-2.tar.bz2 ]; then
    wget "${URLBASE}/libiconv/libiconv2/libiconv2-1.14-2.tar.bz2"
  fi

  # libreadline
  if [ ! -f libreadline7-6.1.2-3.tar.bz2 ]; then
    wget "${URLBASE}/readline/libreadline7/libreadline7-6.1.2-3.tar.bz2"
  fi

  # libgcc
  if [ ! -f libgcc1-4.7.3-1.tar.bz2 ]; then
    wget "${URLBASE}/gcc/libgcc1/libgcc1-4.7.3-1.tar.bz2"
  fi

  # libncursesw
  if [ ! -f libncursesw10-5.7-18.tar.bz2 ]; then
    wget "${URLBASE}/ncursesw/libncursesw10/libncursesw10-5.7-18.tar.bz2"
  fi

  # cpio
  if [ ! -f cpio-2.11-2.tar.bz2 ]; then
    wget "${URLBASE}/cpio/cpio-2.11-2.tar.bz2"
  fi

  # patch
  if [ ! -f patch-2.7.1-1.tar.bz2 ]; then
    wget "${URLBASE}/patch/patch-2.7.1-1.tar.bz2"
  fi

  # bash
  if [ ! -f bash-4.1.10-4.tar.bz2 ]; then
    wget "${URLBASE}/bash/bash-4.1.10-4.tar.bz2"
  fi

  tar jxvf cygwin-1.7.25-1.tar.bz2 usr/bin/cygwin1.dll \
    --to-stdout > "${TD}/cygwin1.dll"
  tar jxvf libintl8-0.18.1.1-2.tar.bz2 usr/bin/cygintl-8.dll \
    --to-stdout > "${TD}/cygintl-8.dll"
  tar jxvf libiconv2-1.14-2.tar.bz2 usr/bin/cygiconv-2.dll \
    --to-stdout > "${TD}/cygiconv-2.dll"
  tar jxvf libreadline7-6.1.2-3.tar.bz2 usr/bin/cygreadline7.dll \
    --to-stdout > "${TD}/cygreadline7.dll"
  tar jxvf libgcc1-4.7.3-1.tar.bz2 usr/bin/cyggcc_s-1.dll \
    --to-stdout > "${TD}/cyggcc_s-1.dll"
  tar jxvf libncursesw10-5.7-18.tar.bz2 usr/bin/cygncursesw-10.dll \
    --to-stdout > "${TD}/cygncursesw-10.dll"
  tar jxvf cpio-2.11-2.tar.bz2 usr/bin/cpio.exe \
    --to-stdout > "${TD}/cpio.exe"
  tar jxvf patch-2.7.1-1.tar.bz2 usr/bin/patch.exe \
    --to-stdout > "${TD}/hctap.exe"
  tar jxvf bash-4.1.10-4.tar.bz2 usr/bin/bash.exe \
    --to-stdout > "${TD}/bash.exe"

  chmod +x "${TD}"/*.{exe,dll}
  popd
}

build_mac() {
  local TD="${TARGETDIR}/binaries/osx/"
  mkdir -p "${TD}"

  mkdir -p macbinaries
  pushd macbinaries

  # Can't cross compile from Linux, unfortunately
  # Thanks to munchy_cool for the binaries!
  if [ ! -f bootimg_osx.tar.gz ]; then
    wget 'http://fs1.d-h.st/download/00076/dBZ/bootimg_osx.tar.gz'
  fi
  tar zxvf bootimg_osx.tar.gz -C "${TD}"

  popd

  find "${TD}" -type f -exec chmod +x {} \+
}

build_linux() {
  local TD="${TARGETDIR}/binaries/linux/"
  mkdir -p "${TD}"

  pushd "${ANDROID_DIR}"
  gcc -m32 -static -Iinclude \
    mkbootimg/mkbootimg.c \
    libmincrypt/sha.c \
    -o "${TD}/mkbootimg"

  gcc -m32 -static -Iinclude \
    mkbootimg/unpackbootimg.c \
    -o "${TD}/unpackbootimg"

  strip "${TD}"/{mkbootimg,unpackbootimg}
  popd
}

compress_ramdisks() {
  for i in $(find ramdisks -type d -name '*.dualboot'); do
    if [ -d "${i}" ]; then
      pushd "${i}"
      mkdir -p "${TARGETDIR}/$(dirname "${i}")"
      find . | cpio -o -H newc > \
        "${TARGETDIR}/${i}.cpio"
      popd
    fi
  done
}

TARGETNAME="DualBootPatcher-${VERSION}"
TARGETDIR="${CURDIR}/${TARGETNAME}"
rm -rf "${TARGETDIR}" "${TARGETNAME}.zip"
mkdir -p "${TARGETDIR}/binaries" "${TARGETDIR}/ramdisks"

# Build and copy stuff into target directory
create_portable_python
build_windows
build_mac
build_linux
compress_ramdisks

cp -rt "${TARGETDIR}" \
  $(git ls-tree --name-only --full-tree HEAD | grep -v .gitignore) pythonportable

# Remove unneeded files
rm -r pythonportable/
rm "${TARGETDIR}/patches/0001-I-hate-Windows-with-a-passion.patch"

# Create zip
zip -r ${TARGETNAME}.zip ${TARGETNAME}/
echo "Successfully created: ${CURDIR}/${TARGETNAME}.zip"
