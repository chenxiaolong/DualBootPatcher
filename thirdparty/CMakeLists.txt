# All of the files here are built from the scripts thirdparty/*/build_android_*.sh
# If you don't trust me or my binaries, you can use your own builds by copying
# the tarballs to thirdparty/ and changing the md5sums here (CMake will not
# download anything if the files exist and the md5sums match)

set(URL_BASE "https://dl.dropboxusercontent.com/u/486665/MultiBootPatcher/prebuilts")

################################################################################
# patch for Android
################################################################################

set(PATCH_VER "2.7.1")

file(
    DOWNLOAD
    ${URL_BASE}/patch-${PATCH_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/patch-${PATCH_VER}_android.tar.bz2
    EXPECTED_MD5 1ff85704c4906d86c52090a990f9062c
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/patch)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/patch
    )

    message(STATUS "Extracting patch for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/patch-${PATCH_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/patch
    )
endif()

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/patch/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
    COMPONENT Libraries
    FILES_MATCHING
    PATTERN patch
)


################################################################################
# boost for Android
################################################################################

set(BOOST_VER "1.57.0")

file(
    DOWNLOAD
    ${URL_BASE}/boost-${BOOST_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/boost-${BOOST_VER}_android.tar.bz2
    EXPECTED_MD5 ab35effea3638135b618d0d8de298471
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/boost)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/boost
    )

    message(STATUS "Extracting boost for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/boost-${BOOST_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/boost
    )
endif()


################################################################################
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.1.2.r443.g687ac8d")

file(
    DOWNLOAD
    ${URL_BASE}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    EXPECTED_MD5 507f280567b2bded29039123861784e1
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/libarchive)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/libarchive
    )

    message(STATUS "Extracting libarchive for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/libarchive
    )
endif()


################################################################################
# liblzma for Android
################################################################################

set(LIBLZMA_VER "5.2.1")

file(
    DOWNLOAD
    ${URL_BASE}/liblzma-${LIBLZMA_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/liblzma-${LIBLZMA_VER}_android.tar.bz2
    EXPECTED_MD5 43e6c040f4e1684e06537dc8864681af
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/liblzma)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/liblzma
    )

    message(STATUS "Extracting liblzma for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/liblzma-${LIBLZMA_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/liblzma
    )
endif()


################################################################################
# liblz4 for Android
################################################################################

set(LIBLZ4_VER "r127")

file(
    DOWNLOAD
    ${URL_BASE}/liblz4-${LIBLZ4_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/liblz4-${LIBLZ4_VER}_android.tar.bz2
    EXPECTED_MD5 7120178f62782fb0908c609e1a2a667b
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/liblz4)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/liblz4
    )

    message(STATUS "Extracting liblz4 for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/liblz4-${LIBLZ4_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/liblz4
    )
endif()


################################################################################
# jansson for Android
################################################################################

set(JANSSON_VER "2.7")

file(
    DOWNLOAD
    ${URL_BASE}/jansson_v${JANSSON_VER}.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/jansson_v${JANSSON_VER}.tar.bz2
    EXPECTED_MD5 fbc60a91f4d9a8a25a978a085c3a6661
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/jansson)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/jansson
    )

    message(STATUS "Extracting jansson for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/jansson_v${JANSSON_VER}.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/jansson
    )
endif()


################################################################################
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "2.3-98d4973")

file(
    DOWNLOAD
    ${URL_BASE}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
    EXPECTED_MD5 9c39f359cfba0b32caca521da2bcdfbc
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/libsepol)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/libsepol
    )

    message(STATUS "Extracting libsepol for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/libsepol
    )
endif()


################################################################################
# libloki for Android
################################################################################

set(LIBLOKI_VER "r87.38680b6")

file(
    DOWNLOAD
    ${URL_BASE}/libloki-${LIBLOKI_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libloki-${LIBLOKI_VER}_android.tar.bz2
    EXPECTED_MD5 a6131ac0cfecdf0720f9a2e2beae448a
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/libloki)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/libloki
    )

    message(STATUS "Extracting libloki for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/libloki-${LIBLOKI_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/libloki
    )
endif()


################################################################################
# procps-ng for Android
################################################################################

set(PROCPS_NG_VER "3.3.10")

file(
    DOWNLOAD
    ${URL_BASE}/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
    EXPECTED_MD5 38489848300b3ac68297142ae943cfb5
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/procps-ng)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/procps-ng
    )

    message(STATUS "Extracting procps-ng for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/procps-ng
    )
endif()


################################################################################
# jna for Android
################################################################################

if(MBP_BUILD_ANDROID)
    set(JNA_VER "4.1.0")

    file(
        DOWNLOAD
        ${URL_BASE}/jna-${JNA_VER}_android.tar.bz2
        ${CMAKE_CURRENT_SOURCE_DIR}/jna-${JNA_VER}_android.tar.bz2
        EXPECTED_MD5 ab22d3d2ff7f6cacd5b35a1fde79bd29
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/jna)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/jna
        )

        message(STATUS "Extracting jna for Android ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/jna-${JNA_VER}_android.tar.bz2
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/jna
        )
    endif()
endif()


################################################################################
# busybox unzip for Android
################################################################################

set(UNZIP_VER "cm-12.0")

file(
    DOWNLOAD
    ${URL_BASE}/unzip-${UNZIP_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/unzip-${UNZIP_VER}_android.tar.bz2
    EXPECTED_MD5 bce798d56defb33138df4ec936c64c67
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/unzip)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/unzip
    )

    message(STATUS "Extracting unzip for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/unzip-${UNZIP_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/unzip
    )
endif()

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/unzip/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
    COMPONENT Libraries
)


################################################################################
# GNU patch for Windows
################################################################################

if(WIN32)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cygwin
    )

    set(CYGWIN_VER "1.7.34-005")
    set(CYGWIN_TAR "cygwin-${CYGWIN_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/cygwin/${CYGWIN_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
        EXPECTED_MD5 750c465312a7ab5837882349babfc106
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygwin1.dll)
        message(STATUS "Extracting cygwin for Windows ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
        )
    endif()

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygwin1.dll
        DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
        COMPONENT Libraries
    )


    set(LIBINTL8_VER "0.18.3.2-2")
    set(LIBINTL8_TAR "libintl8-${LIBINTL8_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/gettext/libintl8/${LIBINTL8_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${LIBINTL8_TAR}
        EXPECTED_MD5 48db59bcca1d1437b5facb2997165777
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygintl-8.dll)
        message(STATUS "Extracting libintl for Windows ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${LIBINTL8_TAR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
        )
    endif()

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygintl-8.dll
        DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
        COMPONENT Libraries
    )


    set(LIBICONV2_VER "1.14-2")
    set(LIBICONV2_TAR "libiconv2-${LIBICONV2_VER}.tar.bz2")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/libiconv/libiconv2/${LIBICONV2_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${LIBICONV2_TAR}
        EXPECTED_MD5 a78340002d86f7f586de40887e213944
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygiconv-2.dll)
        message(STATUS "Extracting libiconv for Windows ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${LIBICONV2_TAR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
        )
    endif()

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygiconv-2.dll
        DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
        COMPONENT Libraries
    )


    set(PATCH_VER "2.7.1-1")
    set(PATCH_TAR "patch-${PATCH_VER}.tar.bz2")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/patch/${PATCH_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${PATCH_TAR}
        EXPECTED_MD5 73428366ed536bd7881754a51d61762d
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/patch.exe)
        message(STATUS "Extracting GNU patch for Windows ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${PATCH_TAR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
        )
    endif()

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/patch.exe
        DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
        RENAME hctap.exe
        COMPONENT Libraries
    )
endif()
