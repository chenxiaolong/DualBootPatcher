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
# libxml2 for Android
################################################################################

set(LIBXML2_VER "android-5.0.0_r6")

file(
    DOWNLOAD
    ${URL_BASE}/libxml2_${LIBXML2_VER}.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libxml2_${LIBXML2_VER}.tar.bz2
    EXPECTED_MD5 83181acfdac6089b1d44d77b242cb4b5
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/libxml2)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/libxml2
    )

    message(STATUS "Extracting libxml2 for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/libxml2_${LIBXML2_VER}.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/libxml2
    )
endif()


################################################################################
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.1.2")

file(
    DOWNLOAD
    ${URL_BASE}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    EXPECTED_MD5 bcb32910936b37e4d5802fbc9319be5e
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

set(LIBLZMA_VER "5.2.0")

file(
    DOWNLOAD
    ${URL_BASE}/liblzma-${LIBLZMA_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/liblzma-${LIBLZMA_VER}_android.tar.bz2
    EXPECTED_MD5 eab12196d906ee4ecfa447ea435b4fe7
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

set(LIBSEPOL_VER "2.3")

file(
    DOWNLOAD
    ${URL_BASE}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libsepol-${LIBSEPOL_VER}_android.tar.bz2
    EXPECTED_MD5 73e618c7c075936e67544c332edb7055
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
# jna for Android
################################################################################

if(BUILD_ANDROID)
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
# e2fsprogs for Android
################################################################################

#set(E2FSPROGS_VER "android-5.0.2_r1")

#file(
#    DOWNLOAD
#    ${URL_BASE}/e2fsprogs-${E2FSPROGS_VER}_android.tar.bz2
#    ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs-${E2FSPROGS_VER}_android.tar.bz2
#    EXPECTED_MD5 ae67c29df2f4752b8524fe19e0b6e9ea
#    SHOW_PROGRESS
#)

#if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs)
#    execute_process(
#        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs
#    )
#
#    message(STATUS "Extracting e2fsprogs for Android ...")
#    execute_process(
#        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs-${E2FSPROGS_VER}_android.tar.bz2
#        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs
#    )
#endif()

#install(
#    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs/
#    DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
#    COMPONENT Libraries
#)


################################################################################
# GNU patch for Windows
################################################################################

if(WIN32)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cygwin
    )

    set(CYGWIN_VER "1.7.34-001")
    set(CYGWIN_TAR "cygwin-${CYGWIN_VER}.tar.xz")

    file(
        DOWNLOAD
        http://mirrors.kernel.org/sourceware/cygwin/x86/release/cygwin/${CYGWIN_TAR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
        EXPECTED_MD5 d99e04108035fe41626192f98748f03c
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
