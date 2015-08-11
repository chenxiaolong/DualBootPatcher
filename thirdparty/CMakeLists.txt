# All of the files here are built from the scripts thirdparty/*/build_android_*.sh
# If you don't trust me or my binaries, you can use your own builds by copying
# the tarballs to thirdparty/ and changing the md5sums here (CMake will not
# download anything if the files exist and the md5sums match)

set(URL_BASE "https://dl.dropboxusercontent.com/u/486665/MultiBootPatcher/prebuilts")

################################################################################
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.1.2.r538.ge3ad411_r1")

file(
    DOWNLOAD
    ${URL_BASE}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    EXPECTED_MD5 04d34e1326212134be46275be8306e81
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
# liblzo2 for Android
################################################################################

set(LIBLZO2_VER "2.09")

file(
    DOWNLOAD
    ${URL_BASE}/liblzo2-${LIBLZO2_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/liblzo2-${LIBLZO2_VER}_android.tar.bz2
    EXPECTED_MD5 45fafd07f61da8589fb4594b5a125d47
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/liblzo2)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/liblzo2
    )

    message(STATUS "Extracting liblzo2 for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/liblzo2-${LIBLZO2_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/liblzo2
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
