# All of the files here are built from the scripts thirdparty/*/build_android_*.sh
# If you don't trust me or my binaries, you can use your own builds by copying
# the tarballs to thirdparty/ and changing the md5sums here (CMake will not
# download anything if the files exist and the md5sums match)

set(URL_BASE "https://dl.dropboxusercontent.com/u/486665/MultiBootPatcher/prebuilts")


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    @ONLY
)


################################################################################
# fuse-exfat for Android
################################################################################

set(EXFAT_VER "1.0.0-1")

file(
    DOWNLOAD
    ${URL_BASE}/exfat-${EXFAT_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/exfat-${EXFAT_VER}_android.tar.bz2
    EXPECTED_HASH MD5=285afc88bd425a3fa0880c4c0cafaa4b
    EXPECTED_HASH SHA512=8e06fb49ea4bbe822dac0bb9e2caf4e06b0ebeb5d4499cf5935e7c6412538b5c26a6384fce59c567c14fb10c271e0ef9c8c36edc96c3c62dedd6b55fd785ffc5
    TLS_VERIFY ON
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/exfat)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/exfat
    )

    message(STATUS "Extracting fuse-exfat for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/exfat-${EXFAT_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/exfat
    )
endif()

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/exfat/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
    COMPONENT Libraries
    FILES_MATCHING
    PATTERN mount.exfat
)


################################################################################
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.1.2.r538.ge3ad411_r1")

file(
    DOWNLOAD
    ${URL_BASE}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/libarchive-${LIBARCHIVE_VER}_android.tar.bz2
    EXPECTED_HASH MD5=04d34e1326212134be46275be8306e81
    EXPECTED_HASH SHA512=b94e72d45edffe7e482f2c5466638fc5bdef43cff28ccbd22fe534d786ad925e9b5cc61eb1cf01cc73d8259637f718b9badf31f99146111c3c56b941dba917ec
    TLS_VERIFY ON
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
    EXPECTED_HASH MD5=43e6c040f4e1684e06537dc8864681af
    EXPECTED_HASH SHA512=4cf6ab03ac814abea2a4e7f881e77e3b12b19bdfe419d743d72060d3e2f6cdffaeff927affe09ab1cb6165e5f19f8712803ec74f4216ed0bec295400e313798e
    TLS_VERIFY ON
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
    EXPECTED_HASH MD5=45fafd07f61da8589fb4594b5a125d47
    EXPECTED_HASH SHA512=6482d1d0d69fdf2a59e710f1a939a429a0c19d414eb66eee51943f94f5ff6e32236686085cf8f9f1b22ac304077ad4629d7dee89a6a6224d5098bdf6bd4da9df
    TLS_VERIFY ON
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
    EXPECTED_HASH MD5=4dc6cd165ac73c4594a341735b9f63a1
    EXPECTED_HASH SHA512=94e686b80e66d35d473ab2ec9f617c2b943887ff22d807f129c3d0672b8b039370561cf7070a7e8a35399bfbe6a3e90501b8bf28d99596df6a175d9ee47633ad
    TLS_VERIFY ON
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
    EXPECTED_HASH MD5=fbc60a91f4d9a8a25a978a085c3a6661
    EXPECTED_HASH SHA512=b9d1e827e3617cd03310f973e97a13cde0b1ae9b99e0cd39f628aa2a33ca7964a73d0fd3804cea012fe22600009bad166defe8c9eb6ca05c56a0125db2368f15
    TLS_VERIFY ON
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
    EXPECTED_HASH MD5=b0d59444f3aaafe919d8e7059f0fd5ad
    EXPECTED_HASH SHA512=dedef4ade145c762ecd39725d5ffc0cf9a418108d66f34000bace9ef5851db82adb1cb674d6bfb910bb28a0e5a0dfd66c159c523d83649acdeeab359bfa7d2a1
    TLS_VERIFY ON
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
# openssl for Android
################################################################################

set(OPENSSL_VER "1.0.2d")

file(
    DOWNLOAD
    ${URL_BASE}/openssl-${OPENSSL_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/openssl-${OPENSSL_VER}_android.tar.bz2
    EXPECTED_HASH MD5=fd016c31bb28ba5a322be043a92af503
    EXPECTED_HASH SHA512=b7b64ed7e3d5da72e9d89d47e0ae6150ed1938c1209eb7a2733168a630018da7dd6deffc89b0726cab02568304084fb7e5b4d058f6f9a7b938a5494cbc3ebecc
    TLS_VERIFY ON
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/openssl)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/openssl
    )

    message(STATUS "Extracting openssl for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/openssl-${OPENSSL_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/openssl
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
    EXPECTED_HASH MD5=38489848300b3ac68297142ae943cfb5
    EXPECTED_HASH SHA512=3eb49e02b0372669f4683aa4003f97a285ba2106ed2e847c022f9db2c5a92b8be3611f1409d5fcb75c0789b03df8faeeb453e3625d26c28f2c4c842daedc1e77
    TLS_VERIFY ON
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
        EXPECTED_HASH MD5=ab22d3d2ff7f6cacd5b35a1fde79bd29
        EXPECTED_HASH SHA512=b10c39120188246139a189258dd5f1a8e586f989df946c61404415bab926b246596224133f3d077d619c56691b9a1720a75851f33564f914e8f29936f6c09702
        TLS_VERIFY ON
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
# e2fsprogs for Android
################################################################################

set(E2FSPROGS_VER "android-5.1.1_r9")

file(
    DOWNLOAD
    ${URL_BASE}/e2fsprogs-${E2FSPROGS_VER}_android.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs-${E2FSPROGS_VER}_android.tar.bz2
    EXPECTED_HASH MD5=d261cd5ac27c3b2cd16d57bcc1758fd9
    EXPECTED_HASH SHA512=e02927b89d5a1ffa865c38b924c59485ce91008420119ce8c527fe5c11b88976b357e6f6f2a51da39cb8400427bf17581f2d57a5a208c7fbb69c08e44208e201
    TLS_VERIFY ON
    SHOW_PROGRESS
)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs
    )

    message(STATUS "Extracting e2fsprogs for Android ...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/e2fsprogs-${E2FSPROGS_VER}_android.tar.bz2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs
    )
endif()

# install(
#     DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/e2fsprogs/
#     DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
#     COMPONENT Libraries
#     FILES_MATCHING
#     PATTERN e2fsck
#     PATTERN resize2fs
# )
