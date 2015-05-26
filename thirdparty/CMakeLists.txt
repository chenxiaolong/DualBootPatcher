# All of the files here are built from the scripts thirdparty/*/build_android_*.sh
# If you don't trust me or my binaries, you can use your own builds by copying
# the tarballs to thirdparty/ and changing the md5sums here (CMake will not
# download anything if the files exist and the md5sums match)

set(URL_BASE "https://dl.dropboxusercontent.com/u/486665/MultiBootPatcher/prebuilts")

################################################################################
# patch for Android
################################################################################

set(PATCH_VER "2.7.5")

file(
    DOWNLOAD
    ${URL_BASE}/patch-${PATCH_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/patch-${PATCH_VER}_android.tar.bz2
    EXPECTED_MD5 42fc4fa332029f62076d6ab6a535f591
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
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.1.2.r538.gb6173a2")

file(
    DOWNLOAD
    ${URL_BASE}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    EXPECTED_MD5 ee6cf9505250679451b0ca88f59989e8
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

set(LIBLZ4_VER "r128")

file(
    DOWNLOAD
    ${URL_BASE}/liblz4-${LIBLZ4_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/liblz4-${LIBLZ4_VER}_android.tar.bz2
    EXPECTED_MD5 4dc6cd165ac73c4594a341735b9f63a1
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

set(LIBSEPOL_VER "2.3-674256b")

file(
    DOWNLOAD
    ${URL_BASE}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
    EXPECTED_MD5 b0d59444f3aaafe919d8e7059f0fd5ad
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
# GNU patch for Windows
################################################################################

if(WIN32)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cygwin
    )

    set(CYGWIN_VER "2.0.2-1")
    set(CYGWIN_TAR "cygwin-${CYGWIN_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/cygwin/${CYGWIN_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
        EXPECTED_MD5 f587b4538fb1045fb5fabd5a6bee9201
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


    set(LIBATTR_VER "2.4.46-1")
    set(LIBATTR_TAR "libattr1-${LIBATTR_VER}.tar.bz2")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/attr/libattr1/${LIBATTR_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${LIBATTR_TAR}
        EXPECTED_MD5 2c60a11de348a3a694492cd482adfb7e
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygattr-1.dll)
        message(STATUS "Extracting libattr for Windows ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${LIBATTR_TAR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
        )
    endif()

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygattr-1.dll
        DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
        COMPONENT Libraries
    )


    set(LIBINTL8_VER "0.19.4-1")
    set(LIBINTL8_TAR "libintl8-${LIBINTL8_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/gettext/libintl8/${LIBINTL8_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${LIBINTL8_TAR}
        EXPECTED_MD5 7020560199ede8aaeec6cff1b535e27a
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


    set(LIBICONV2_VER "1.14-3")
    set(LIBICONV2_TAR "libiconv2-${LIBICONV2_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/libiconv/libiconv2/${LIBICONV2_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${LIBICONV2_TAR}
        EXPECTED_MD5 d9df418d5cc50dfcbfad971282d6fe1f
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


    set(PATCH_VER "2.7.4-1")
    set(PATCH_TAR "patch-${PATCH_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/patch/${PATCH_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${PATCH_TAR}
        EXPECTED_MD5 b652c0c0815e74b195cd29892515ffa0
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
