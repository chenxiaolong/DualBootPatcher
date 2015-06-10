# Maintainer: ant32 <antreimer@gmail.com>
# Contributor: Filip Brcic <brcha@gna.org>
# Contributor: jellysheep <max.mail@dameweb.de>
pkgname=mingw-w64-qt5-base
pkgver=5.4.2
pkgrel=1
pkgdesc="A cross-platform application and UI framework (mingw-w64)"
arch=(i686 x86_64)
url="http://qt-project.org"
license=("custom, FDL, GPL3, LGPL")
depends=(
  mingw-w64-crt
  mingw-w64-zlib
  mingw-w64-libjpeg-turbo
  mingw-w64-libiconv
  mingw-w64-sqlite
  mingw-w64-libpng
  mingw-w64-openssl
  mingw-w64-libdbus
  mingw-w64-pcre
  mingw-w64-harfbuzz
)
groups=(mingw-w64-qt mingw-w64-qt5)
optdepends=(
  "mingw-w64-postgresql-libs: PostgreSQL support"
  "mingw-w64-mariadb-connector-c: MySQL support"
  "qtchooser"
)
makedepends=(mingw-w64-gcc
             mingw-w64-postgresql-libs
             mingw-w64-mariadb-connector-c
             mingw-w64-pkg-config
             mingw-w64-glib2)
options=(!strip !buildflags staticlibs)
_pkgfqn="qtbase-opensource-src-${pkgver}"
source=("http://download.qt-project.org/official_releases/qt/5.4/${pkgver}/submodules/${_pkgfqn}.tar.xz"
        "qt5-merge-static-and-shared-library-trees.patch"
        "qt5-add-angle-support.patch"
        "angle-fix.patch"
        "qt5-use-external-angle-library.patch"
        "qt5-workaround-pkgconfig-install-issue.patch"
        "qt5-qtbase-fix-linking-against-static-pcre.patch"
        "qt5-rename-qtmain-to-qt5main.patch"
        "qt5-dont-build-host-libs-static.patch"
        "qt5-enable-rpath-for-host-tools.patch"
        "qt5-add-compiled-shaders-remove-dxsdk.patch"
        "qt5-dont-add-resource-files-to-qmake-libs.patch"
        "qt5-prevent-debug-library-names-in-pkgconfig-files.patch"
        "qt5-qmake-implib-dll-a.patch"
        "qt5-fix-static-dbus-detection.patch"
        "qt5-fix-static-harfbuzz-detection.patch"
        "qt5-use-win32-g++-mkspecs-profile.patch"
        "qt5-use-system-zlib-in-host-libs.patch"
        "qt-5.1.x-win_flex-replace.patch"
        "fix-opengl-to-many-sections.patch"
        "fix-static-psql-mysql.patch")
md5sums=('67a95eec79ffc4a14f516ad6f3d24c96'
         'f51eb2b176b10af943831d33925c1251'
         '8c6bda3ce8aca368b1f50f57e5f25aa7'
         'af4ac76fb9d1ea383359ea881ffcd8e3'
         '361e456685180a4004de6af76c58ddba'
         '19debeaea610e70691fae3b2a0a94eaa'
         '4fe6523dd1c34398df3aa5a8763530cc'
         'c4d4ca54a2e9096d2af13a266177f6d6'
         '3bd322551924543553a2bf81b4419a09'
         '15e8eaa4f0a9a9990277c79e243770be'
         'bd0ced78eafaf187197d64f9e381c6c5'
         '47b2aa0c6251c24c3c36c86b86a99c4c'
         '6a6bc88f35ac8080869de39bc128ce5b'
         '474de0e31122ecd140ebc19fe3960057'
         '40de3aaf7d713034e06f4eece665b1ba'
         '1a7921cadeb5c3db383dea068519e8d3'
         'f0e068b82ca41ae32c0d72587f070dd8'
         'a265dea62755caf38187114143999224'
         '852519f457e33e0d1e9d2a2df872967d'
         '612a4dfb9f1a3898a1920c28bb999159'
         'd0eb81aef1a21c65813fe4ddabbc4206')
_architectures="x86_64-w64-mingw32 i686-w64-mingw32"

# Helper functions for the split builds
isStatic() {
  [ $pkgname = "mingw-w64-qt5-base-static" ]
}

isOpenGL() {
  [ $pkgname = "mingw-w64-qt5-base-opengl" ]
}

isInternalAngle() {
  [ $pkgname = "mingw-w64-qt5-base-internal-angle" ]
}

isStatic && depends+=("mingw-w64-qt5-base")
! isOpenGL && ! isInternalAngle && depends+=("mingw-w64-angleproject")
isOpenGL || isInternalAngle && provides+=("mingw-w64-qt5-base")
isOpenGL || isInternalAngle && conflicts+=("mingw-w64-qt5-base")
isInternalAngle && conflicts+=("mingw-w64-angleproject")

prepare() {
  cd "${srcdir}/${_pkgfqn}"

  # When building Qt as static library some files have a different content
  # when compared to the static library. Merge those changes manually.
  # This patch also applies some additional changes which are required to make
  # linking against the static version of Qt work without any manual fiddling
  patch -p0 -i "${srcdir}/qt5-merge-static-and-shared-library-trees.patch"

  if ! isOpenGL; then
    # Add support for Angle
    patch -p0 -i "${srcdir}/qt5-add-angle-support.patch"
  fi

  if ! isOpenGL && ! isInternalAngle; then
    # Make sure our external Angle package is used instead of the bundled one
    patch -p0 -i "${srcdir}/qt5-use-external-angle-library.patch"
  fi
  
  # opengl to many sections error
  isOpenGL && patch -p0 -i ../fix-opengl-to-many-sections.patch

  # Make sure the .pc files of the Qt5 modules are installed correctly
  patch -p0 -i "${srcdir}/qt5-workaround-pkgconfig-install-issue.patch"

  # Prevents resource files from being added to the LIBS parameter
  # This solves an issue where the generated pkg-config files contained
  # invalid Libs.private references like .obj/debug/Qt5Cored_resource_res.o
  patch -p1 -i "${srcdir}/qt5-dont-add-resource-files-to-qmake-libs.patch"

  # qmake generates the pkgconfig .pc files two times, once for the
  # release build and once for the debug build (which we're not actually
  # building in this package). For both generations the exact same
  # pkgconfig file name is used. This causes references to the debug
  # build ending up in the .pc files which are unwanted
  # Prevent this from happening by giving the pkgconfig .pc
  # files for the debug build an unique file name
  patch -p1 -i "${srcdir}/qt5-prevent-debug-library-names-in-pkgconfig-files.patch"

  # Fix qmake to create implibs with .dll.a extension for MinGW
  patch -p1 -i "${srcdir}/qt5-qmake-implib-dll-a.patch"

  # As of Qt 5.4.1 the detection of the static DBus and Harfbuzz libraries got broken
  patch -p1 -i ../qt5-fix-static-dbus-detection.patch
  patch -p1 -i ../qt5-fix-static-harfbuzz-detection.patch

  # Patch the win32-g++ mkspecs profile to match our environment
  patch -p0 -i "${srcdir}/qt5-use-win32-g++-mkspecs-profile.patch"

  # The bundled pcre is built as static library by default
  # As we're not using the bundled copy but our own copy
  # we need to do some fiddling to fix compilation issues
  # when trying to build static qmake projects
  patch -p1 -i "${srcdir}/qt5-qtbase-fix-linking-against-static-pcre.patch"

  # Make sure the qtmain (static) library doesn't conflict with the one
  # provided by the mingw-qt (qt4) package. The mkspecs profile is already
  # updated by patch100 to reflect this change
  # https://bugzilla.redhat.com/show_bug.cgi?id=1092465
  patch -p1 -i "${srcdir}/qt5-rename-qtmain-to-qt5main.patch"

  # Upstream always wants the host libraries to be static instead of
  # shared libraries. This causes issues and is against the Fedora
  # packaging guidelines so disable this 'feature'
  patch -p0 -i "${srcdir}/qt5-dont-build-host-libs-static.patch"

  # Build host tools with rpath enabled
  # We have to use rpath here as the library which the
  # various tools depend on (libQt5Bootstrap.so) resides
  # in the folder /usr/${_arch}/lib
  # We can't use the regular %%_libdir for this as we
  # want to avoid conflicts with the native qt5 packages
  patch -p1 -i "${srcdir}/qt5-enable-rpath-for-host-tools.patch"

  # Build host libs with system zlib. This patch cannot be upstreamed as-is
  # due to the other host-libs patches.
  patch -p1 -i "${srcdir}/qt5-use-system-zlib-in-host-libs.patch"

  if ! isInternalAngle; then
    # Make sure the Qt5 build system uses our external ANGLE library
    rm -rf src/3rdparty/angle include/QtANGLE/{EGL,GLES2,KHR}
  fi

  # As well as our external PCRE library and zlib
  rm -rf src/3rdparty/{pcre,zlib}

  if isStatic; then
    patch -p0 -i ../fix-static-psql-mysql.patch
  fi

  if isInternalAngle; then
    # fix make[6]: win_flex: Command not found
    patch -p2 -i "${srcdir}/qt-5.1.x-win_flex-replace.patch"
    # Fix Angle. See https://code.google.com/p/angleproject/issues/detail?id=462
    patch -p0 -i "${srcdir}/angle-fix.patch"
    patch -p0 < "${srcdir}/qt5-add-compiled-shaders-remove-dxsdk.patch"
    find ./src/3rdparty/angle -type f -and \( -name *.h -or -name *.cpp \) | xargs sed -i \
      -e 's_#include <D3Dcompiler.h>_#include <d3dcompiler.h>_' \
      -e 's_#include "libGLESv2/Constants.h"_#include "libGLESv2/constants.h"_'
  fi
}

build() {
  cd "${srcdir}/${_pkgfqn}"

  # Setup flags
  export CFLAGS="-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4"
  export CXXFLAGS="-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4"
  unset LDFLAGS

  for _arch in ${_architectures}; do

    # Phonon is disabled for now because we lack the directx headers
    # The odd paths for the -hostbindir argument are on purpose
    # The qtchooser tool assumes that the tools 'qmake', 'moc' and others
    # are all available in the same folder with these exact file names
    # To prevent conflicts with the mingw-qt (Qt4) package we have
    # to put these tools in a dedicated folder
    qt_configure_args="\
      -xplatform win32-g++ \
      -optimized-qmake \
      -verbose \
      -opensource \
      -confirm-license \
      -force-pkg-config \
      -force-debug-info \
      -audio-backend \
      -system-zlib \
      -system-libpng \
      -system-libjpeg \
      -system-sqlite \
      -no-fontconfig \
      -iconv \
      -openssl \
      -dbus-linked \
      -no-glib \
      -no-gtkstyle \
      -no-icu \
      -release \
      -nomake examples \
      -make tools \
      -hostprefix /usr/${_arch} \
      -hostdatadir /usr/${_arch}/lib/qt \
      -hostbindir /usr/${_arch}/lib/qt/bin \
      -prefix /usr/${_arch} \
      -bindir /usr/${_arch}/bin \
      -archdatadir /usr/${_arch}/lib/qt \
      -datadir /usr/${_arch}/share/qt \
      -docdir /usr/${_arch}/share/doc/qt \
      -examplesdir /usr/${_arch}/share/qt/examples \
      -headerdir /usr/${_arch}/include/qt \
      -libdir /usr/${_arch}/lib \
      -plugindir /usr/${_arch}/lib/qt/plugins \
      -sysconfdir /usr/${_arch}/etc \
      -translationdir /usr/${_arch}/share/qt/translations \
      -device-option CROSS_COMPILE=${_arch}-"

    isOpenGL && qt_configure_args+=' -opengl desktop'
    isStatic && qt_configure_args+=' -opengl no'

    unset PKG_CONFIG_PATH

    if ! isStatic; then
      export QT_LFLAGS_MYSQL="-L/usr/${_arch}/lib -lmysql"
    fi
    # Qt doesn't detect mysql correctly, so use this:
    export QT_CFLAGS_MYSQL="-I/usr/${_arch}/include/mariadb"
    #export QT_LFLAGS_MYSQL_R="-lmariadbclient -lws2_32 -lpthread -lz -lm -lssl -lcrypto"
    # Hardcode MySQL flags into configure (really nice solution :( )
    sed -e "s|^QT_CFLAGS_MYSQL=.*$|QT_CFLAGS_MYSQL=\"${QT_CFLAGS_MYSQL}\"|g" -i "${srcdir}/${_pkgfqn}/configure"
    sed -e "s|^QT_LFLAGS_MYSQL=.*$|QT_LFLAGS_MYSQL=\"${QT_LFLAGS_MYSQL}\"|g" -i "${srcdir}/${_pkgfqn}/configure"
    #sed -e "s|^QT_LFLAGS_MYSQL_R=.*$|QT_LFLAGS_MYSQL_R=\"${QT_LFLAGS_MYSQL_R}\"|g" -i "${srcdir}/${_pkgfqn}/configure"
    qt_configure_args_mysql="-mysql_config /this/file/should/not/exist"

    mkdir ../build-${_arch} && pushd ../build-${_arch}

    if isStatic; then
      ../${_pkgfqn}/configure -static $qt_configure_args $qt_configure_args_mysql
      make
    else
      # The LD_LIBRARY_PATH override is needed because libQt5Bootstrap* are shared
      # libraries which various compiled tools (like moc) use. As the libQt5Bootstrap*
      # libraries aren't installed at this point yet, we have to workaround this
      ../${_pkgfqn}/configure -shared $qt_configure_args $qt_configure_args_mysql
      LD_LIBRARY_PATH=`pwd`/lib make
    fi

    popd
  done
}

package() {
  for _arch in ${_architectures}; do
    cd "$srcdir/${_pkgfqn}"
    make install -C ../build-${_arch} INSTALL_ROOT="${pkgdir}"

    if isStatic; then
      # Drop the qtmain and Qt5Bootstrap static libraries from the static tree as
      # they are already part of the main tree
      rm -f "${pkgdir}/usr/${_arch}/lib/libqt5main"*
      rm -f "${pkgdir}/usr/${_arch}/lib/libQt5Bootstrap"*
      rm -f "${pkgdir}/usr/${_arch}/lib/libQt5OpenGLExtensions"*
      rm -f "${pkgdir}/usr/${_arch}/lib/libQt5PlatformSupport"*

      # we want to keep a couple pri files not found in base
      mv "${pkgdir}/usr/${_arch}/lib/qt/mkspecs/modules/qt_plugin_"*.pri "${pkgdir}/usr/${_arch}"

      # Delete duplicate files that are in the base package
      rm -fR "${pkgdir}/usr/${_arch}/"{include,share}
      rm -fR "${pkgdir}/usr/${_arch}/lib/"{qt/bin,qt/mkspecs,pkgconfig,cmake}

      # move pri files back
      mkdir -p "${pkgdir}/usr/${_arch}/lib/qt/mkspecs/modules"
      mv "${pkgdir}/usr/${_arch}/"*.pri "${pkgdir}/usr/${_arch}/lib/qt/mkspecs/modules"
    else # not static => shared release

      # The .dll's are installed in both bindir and libdir
      # One copy of the .dll's is sufficient
      rm -f "${pkgdir}/usr/${_arch}/lib/"*.dll

      # Add qtchooser support
      [ "${_arch}" =  "i686-w64-mingw32" ] && mingwn='mingw32' || mingwn='mingw64'
      mkdir -p "${pkgdir}/etc/xdg/qtchooser"
      echo "/usr/${_arch}/lib/qt/bin" >  "${pkgdir}/etc/xdg/qtchooser/$mingwn-qt5.conf"
      echo "/usr/${_arch}/lib" >> "${pkgdir}/etc/xdg/qtchooser/$mingwn-qt5.conf"

      # Manually install qmake and other native tools so we don't depend anymore on
      # the version of the native system Qt and also fix issues as illustrated at
      # http://stackoverflow.com/questions/6592931/building-for-windows-under-linux-using-qt-creator
      # Also make sure the tools can be found by CMake
      mkdir -p "${pkgdir}/usr/bin"
      for tool in qmake moc rcc uic qdbuscpp2xml qdbusxml2cpp qdoc syncqt.pl; do
        ln -s ../${_arch}/lib/qt/bin/${tool} "${pkgdir}/usr/bin/${_arch}-${tool}-qt5"
      done
    fi

    # The pkg-config files for Qt5Bootstrap aren't interesting as this particular
    # library only contains native code and not cross-compiled code
    rm -f "${pkgdir}/usr/${_arch}/lib/pkgconfig/Qt5Bootstrap.pc"

    # remove doc
    rm -rf "${pkgdir}/usr/${_arch}/share/doc"

    # And finaly, strip the binaries
    if ! isStatic; then
      strip --strip-unneeded "${pkgdir}/usr/${_arch}/lib/libQt5Bootstrap"{,DBus}.so.${pkgver}
    fi
    
    # keeping prl files for base build since qbs seems to need them.
    if isStatic; then
      rm -f "${pkgdir}/usr/${_arch}/lib"{,/qt/plugins/*}/*.prl
    fi
    find "${pkgdir}/usr/${_arch}" -name "*.exe" -o -name "*.bat" -o -name "*.def" -o -name "*.exp" | xargs -rtl1 rm
    find "${pkgdir}/usr/${_arch}" -name "*.dll" -exec ${_arch}-strip --strip-unneeded {} \;
    find "${pkgdir}/usr/${_arch}" -name "*.a" -o -name "*.dll" | xargs -rtl1 ${_arch}-strip -g
  done
}
