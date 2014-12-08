################################################################################
# getfacl, setfacl, getfattr, setfattr, and patch for Android
################################################################################

set(PREBUILTS_VER "20141123")

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00149/V91/prebuilts-android-${PREBUILTS_VER}.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-${PREBUILTS_VER}.tar.bz2
    EXPECTED_MD5 881af483d2d58fa3d5f5b84b6f335c65
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/output/PREBUILTS_VERSION_ANDROID.txt)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android
    )

    message(STATUS "Extracting Android prebuilt binaries ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-${PREBUILTS_VER}.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android
    )
endif()

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/output/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
    COMPONENT Libraries
    FILES_MATCHING
    PATTERN getfattr
    PATTERN setfattr
    PATTERN getfacl
    PATTERN setfacl
    PATTERN patch
)


################################################################################
# valgrind for Android
################################################################################

#set(VALGRIND_VER "20140913")
#
#file(
#    DOWNLOAD
#    http://fs1.d-h.st/download/00140/33H/prebuilts-android-valgrind-${VALGRIND_VER}.tar
#    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-valgrind-${VALGRIND_VER}.tar
#    EXPECTED_MD5 235011c66908e631c0fa40fac13be9fa
#    SHOW_PROGRESS
#)
#
#if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/output/PREBUILTS_VERSION_ANDROID_VALGRIND.txt)
#    message(STATUS "Extracting valgrind for Android ...")
#    execute_process(
#        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-valgrind-${VALGRIND_VER}.tar
#        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android
#    )
#endif()


################################################################################
# busybox for Android
################################################################################

set(BUSYBOX_VER "20140928")

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00142/syi/prebuilts-android-busybox-${BUSYBOX_VER}.tar
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-busybox-${BUSYBOX_VER}.tar
    EXPECTED_MD5 6c418ec985a4d93e4afbe3194bb5534e
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/output/PREBUILTS_VERSION_ANDROID_BUSYBOX.txt)
    message(STATUS "Extracting busybox for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-busybox-${BUSYBOX_VER}.tar
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android
    )
endif()

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/android/output/armeabi-v7a/busybox-static
    DESTINATION ${DATA_INSTALL_DIR}/binaries/
)


################################################################################
# boost for Android
################################################################################

if(BUILD_ANDROID)
    set(BOOST_VER "1.57.0")

    file(
        DOWNLOAD
        http://fs1.d-h.st/download/00148/zQG/boost-${BOOST_VER}_android.tar.bz2
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
endif()


################################################################################
# libxml2 for Android
################################################################################

if(BUILD_ANDROID)
    set(LIBXML2_VER "android-5.0.0_r6")

    file(
        DOWNLOAD
        http://fs1.d-h.st/download/00148/9K1/libxml2_${LIBXML2_VER}.tar.bz2
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
endif()


################################################################################
# libarchive for Android
################################################################################

if(BUILD_ANDROID)
    set(LIBARCHIVE_VER "3.1.2")

    file(
        DOWNLOAD
        http://fs1.d-h.st/download/00148/rKB/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
        ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
        EXPECTED_MD5 0e3e3065853dceef8a0827b8915dfe95
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
endif()


################################################################################
# jansson for Android
################################################################################

set(JANSSON_VER "2.7")

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00152/KDP/jansson_v${JANSSON_VER}.tar.bz2
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
