#!/bin/bash

VERSION="3.1.0"
MINGW_PREFIX=i486-mingw32-
ANDROID_NDK=/opt/android-ndk

set -e

RELEASE=false
if [ "x${1}" = "xrelease" ]; then
  RELEASE=true
fi

CURDIR="$(cd "$(dirname "${0}")" && cd ..  && pwd)"
cd "${CURDIR}"

download_mkbootimg() {
  rm -rf sources/mkbootimg
  mkdir -p sources/mkbootimg
  pushd sources/mkbootimg

  mkdir mkbootimg
  pushd mkbootimg
  wget https://github.com/CyanogenMod/android_system_core/raw/cm-11.0/mkbootimg/mkbootimg.c
  wget https://github.com/CyanogenMod/android_system_core/raw/cm-11.0/mkbootimg/unpackbootimg.c
  wget https://github.com/CyanogenMod/android_system_core/raw/cm-11.0/mkbootimg/bootimg.h
  popd

  mkdir libmincrypt
  pushd libmincrypt
  wget https://github.com/CyanogenMod/android_system_core/raw/cm-11.0/libmincrypt/sha.c
  popd

  mkdir -p include/mincrypt
  pushd include/mincrypt
  wget https://github.com/CyanogenMod/android_system_core/raw/cm-11.0/include/mincrypt/sha.h
  wget https://github.com/CyanogenMod/android_system_core/raw/cm-11.0/include/mincrypt/hash-internal.h
  popd

  popd
}

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

  rm Lib/{__phello__.foo.py,_compat_pickle.py,_dummy_thread.py,_markupbase.py,_osx_support.py,_pyio.py,_strptime.py,_threading_local.py}
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

create_portable_android() {
  local URL="http://fs1.d-h.st/download/00079/91N/python-install-eeba91c.tar.xz"
  local MD5SUM="869aacce52cac8febe0533905203b316"
  if [ ! -f androidbinaries/python-install.tar.xz ] || \
    ! md5sum androidbinaries/python-install.tar.xz | grep -q ${MD5SUM}; then
    mkdir -p androidbinaries
    if which axel >/dev/null; then
      axel -an10 "${URL}" -o androidbinaries/python-install.tar.xz
    else
      wget "${URL}" -O androidbinaries/python-install.tar.xz
    fi
  fi

  rm -rf pythonportable/

  local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
  pushd "${TEMPDIR}"
  tar Jxvf ../androidbinaries/python-install.tar.xz
  mv python-install ../pythonportable/
  popd

  rm -rf "${TEMPDIR}"

  pushd pythonportable/

  # Don't add unneeded files to the zip
  find bin -type f ! -name python -delete

  local LIB=lib/python2.7

  rm -r ${LIB}/bsddb
  rm -r ${LIB}/{compiler,config,ctypes,curses}
  rm -r ${LIB}/distutils
  rm -r ${LIB}/email
  rm -r ${LIB}/hotshot
  rm -r ${LIB}/{idlelib,importlib}
  rm -r ${LIB}/json
  rm -r ${LIB}/{lib-tk,lib2to3,logging}
  rm -r ${LIB}/multiprocessing
  rm -r ${LIB}/{plat-linux3,pydoc_data}
  rm -r ${LIB}/{site-packages,sqlite3}
  rm -r ${LIB}/wsgiref
  rm -r ${LIB}/unittest
  rm -r ${LIB}/xml

  rm ${LIB}/lib-dynload/{_csv.so,_heapq.so,_hotshot.so,_json.so,_lsprof.so,_testcapi.so,audioop.so,grp.so,mmap.so,resource.so,syslog.so,termios.so}

  rm ${LIB}/{__phello__.foo.py,_LWPCookieJar.py,_MozillaCookieJar.py,_pyio.py,_strptime.py,_threading_local.py}
  rm ${LIB}/{aifc.py,antigravity.py,anydbm.py,argparse.py,ast.py,atexit.py,audiodev.py}
  rm ${LIB}/{base64.py,BaseHTTPServer.py,Bastion.py,bdb.py,binhex.py}
  rm ${LIB}/{calendar.py,cgi.py,CGIHTTPServer.py,cgitb.py,chunk.py,cmd.py,code.py,codeop.py,commands.py,colorsys.py,compileall.py,ConfigParser.py,contextlib.py,Cookie.py,cookielib.py,cProfile.py,csv.py}
  rm ${LIB}/{dbhash.py,decimal.py,difflib.py,dircache.py,dis.py,doctest.py,DocXMLRPCServer.py,dumbdbm.py,dummy_thread.py,dummy_threading.py}
  rm ${LIB}/{filecmp.py,fileinput.py,formatter.py,fpformat.py,fractions.py,ftplib.py}
  rm ${LIB}/{getopt.py,getpass.py,gettext.py,glob.py}
  rm ${LIB}/{hmac.py,htmlentitydefs.py,htmllib.py,HTMLParser.py,httplib.py}
  rm ${LIB}/{ihooks.py,imaplib.py,imghdr.py,imputil.py,inspect.py}
  rm ${LIB}/{macpath.py,macurl2path.py,mailbox.py,mailcap.py,markupbase.py,md5.py,mhlib.py,mimetools.py,mimetypes.py,MimeWriter.py,mimify.py,modulefinder.py,multifile.py,mutex.py}
  rm ${LIB}/{netrc.py,new.py,nntplib.py,nturl2path.py,numbers.py}
  rm ${LIB}/{opcode.py,optparse.py,os2emxpath.py}
  rm ${LIB}/{pdb.doc,pdb.py,pickletools.py,pipes.py,pkgutil.py,plistlib.py,popen2.py,poplib.py,posixfile.py,pprint.py,profile.py,pstats.py,pty.py,py_compile.py,pyclbr.py,pydoc.py}
  rm ${LIB}/{Queue.py,quopri.py}
  rm ${LIB}/{rexec.py,rfc822.py,rlcompleter.py,robotparser.py,runpy.py}
  rm ${LIB}/{sched.py,sets.py,sgmllib.py,sha.py,shelve.py,shlex.py,SimpleHTTPServer.py,SimpleXMLRPCServer.py,smtpd.py,smtplib.py,sndhdr.py,socket.py,SocketServer.py,sre.py,ssl.py,statvfs.py,StringIO.py,stringold.py,stringprep.py,sunau.py,sunaudio.py,symbol.py,symtable.py}
  rm ${LIB}/{tabnanny.py,telnetlib.py,textwrap.py,this.py,timeit.py,toaiff.py,tty.py}
  rm ${LIB}/{urllib.py,urllib2.py,urlparse.py,user.py,UserList.py,UserString.py,uu.py,uuid.py}
  rm ${LIB}/{wave.py,webbrowser.py,whichdb.py,wsgiref.egg-info}
  rm ${LIB}/{xdrlib.py,xmllib.py,xmlrpclib.py}

  popd
}

build_windows() {
  local TD="${TARGETDIR}/binaries/windows/"
  mkdir -p "${TD}"

  pushd sources/mkbootimg
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

  # diff
  if [ ! -f diffutils-3.2-1.tar.bz2 ]; then
    wget "${URLBASE}/diffutils/diffutils-3.2-1.tar.bz2"
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
  tar jxvf diffutils-3.2-1.tar.bz2 usr/bin/diff.exe \
    --to-stdout > "${TD}/diff.exe"

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

  pushd sources/mkbootimg
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

build_android() {
  local TD="${TARGETDIR}/binaries/android/"
  mkdir -p "${TD}"

  local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
  ${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh \
    --verbose \
    --platform=android-18 \
    --install-dir=${TEMPDIR} \
    --ndk-dir=${ANDROID_NDK} \
    --system=linux-x86_64

  pushd androidbinaries

  if [ ! -f patch-2.7.tar.xz ]; then
    wget 'ftp://ftp.gnu.org/gnu/patch/patch-2.7.tar.xz'
  fi

  tar Jxvf patch-2.7.tar.xz
  cd patch-2.7
  PATH="${TEMPDIR}/bin:${PATH}" ./configure --host=arm-linux-androideabi
  PATH="${TEMPDIR}/bin:${PATH}" make
  cp src/patch "${TD}"
  cd ..
  rm -r patch-2.7

  popd

  pushd sources/mkbootimg
  ${TEMPDIR}/bin/arm-linux-androideabi-gcc \
    -static -Iinclude \
    --sysroot=${TEMPDIR}/sysroot \
    mkbootimg/mkbootimg.c \
    libmincrypt/sha.c \
    -o "${TD}/mkbootimg"

  ${TEMPDIR}/bin/arm-linux-androideabi-gcc \
    -static -Iinclude \
    --sysroot=${TEMPDIR}/sysroot \
    mkbootimg/unpackbootimg.c \
    -o "${TD}/unpackbootimg"

  ${TEMPDIR}/arm-linux-androideabi/bin/strip "${TD}"/{mkbootimg,unpackbootimg,patch}
  popd

  rm -r ${TEMPDIR}

  cp "${CURDIR}/ramdisks/busybox-static" "${TD}/"
  ln -s busybox-static "${TD}/cpio"
}

build_android_app() {
  pushd "${ANDROIDGUI}"

  sed "s/@VERSION@/${VERSION}/g" < AndroidManifest.xml.in > AndroidManifest.xml

  if [ "x${1}" = "xrelease" ]; then
    ./gradlew assembleRelease
    mv build/apk/Android_GUI-release-unsigned.apk \
      "${CURDIR}/${ANDROIDTARGETNAME}-signed.apk"
  else
    ./gradlew assembleDebug
    mv build/apk/Android_GUI-debug-unaligned.apk \
      "${CURDIR}/${ANDROIDTARGETNAME}-debug.apk"
  fi

  popd
}

TARGETNAME="DualBootPatcher-${VERSION}"
TARGETDIR="${CURDIR}/${TARGETNAME}"
rm -rf "${TARGETDIR}" "${TARGETNAME}.zip"
mkdir -p "${TARGETDIR}/binaries" "${TARGETDIR}/ramdisks"

# Build and copy stuff into target directory
download_mkbootimg
create_portable_python
build_windows
build_mac
build_linux
build_android

mv pythonportable/ "${TARGETDIR}"
cp -rt "${TARGETDIR}" \
  $(git ls-tree --name-only --full-tree HEAD | grep -v -e .gitignore -e Android_GUI)

# Remove unneeded files
rm "${TARGETDIR}/patches/0001-I-hate-Windows-with-a-passion.patch"

# Android stuff
ANDROIDTARGETNAME="DualBootPatcherAndroid-${VERSION}"
ANDROIDTARGETDIR="${CURDIR}/${ANDROIDTARGETNAME}"
rm -rf "${ANDROIDTARGETDIR}" "${ANDROIDTARGETNAME}.zip"
cp -r ${TARGETNAME}/ ${ANDROIDTARGETNAME}/

# Remove Android stuff from PC zip
rm -r "${TARGETDIR}/binaries/android/"

# Remove PC stuff from Android tar
pushd "${ANDROIDTARGETNAME}"

rm -r binaries/{linux,osx,windows}
rm -r pythonportable
find -name '*.bat' -delete

popd

create_portable_android
mv pythonportable/ "${ANDROIDTARGETDIR}"

# Create zip
zip -r ${TARGETNAME}.zip ${TARGETNAME}/
#zip -r ${ANDROIDTARGETNAME}.zip ${ANDROIDTARGETNAME}/
tar Jcvf ${ANDROIDTARGETNAME}.tar.xz ${ANDROIDTARGETNAME}/

# Android app
ANDROIDGUI=${CURDIR}/Android_GUI/
rm -r "${ANDROIDGUI}/assets/"
mkdir "${ANDROIDGUI}/assets/"
mv ${ANDROIDTARGETNAME}.tar.xz "${ANDROIDGUI}/assets/"
cp ramdisks/busybox-static "${ANDROIDGUI}/assets/tar"
rm -f ${ANDROIDTARGETNAME}-*.apk
if [ "x${RELEASE}" = "xtrue" ]; then
  build_android_app release
else
  build_android_app
fi

echo
echo "Done."
