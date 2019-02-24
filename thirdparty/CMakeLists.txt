# All of the files here are built from the PKGBUILD files in thirdparty/*/PKGBUILD.
# All of the files are signed with my GPG key and correct hashes are provided
# at https://dbp.noobdev.io/repo/prebuilts, but if you don't trust me or
# my binaries, you can use your own builds builds by copying your own prebuilts
# to thirdparty/prebuilts and updating the checksums here. It is not necessary
# to upload the files to a server as CMake will not download the files if the
# checksums match.

set(URL_BASE "https://dbp.noobdev.io/repo/prebuilts")

set(MBP_PREBUILTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/prebuilts"
    CACHE PATH "Prebuilts download directory")
set(MBP_PREBUILTS_BINARY_DIR "${CMAKE_BINARY_DIR}/thirdparty/prebuilts"
    CACHE PATH "Extracted prebuilts directory")

set(MBP_PREBUILTS_DIR "${PREBUILTS_DIR}" PARENT_SCOPE)

macro(arch_to_abi arch abi_out)
    if("${arch}" STREQUAL "armv7")
        set("${abi_out}" "armeabi-v7a")
    elseif("${arch}" STREQUAL "aarch64")
        set("${abi_out}" "arm64-v8a")
    elseif("${arch}" STREQUAL "x86")
        set("${abi_out}" "x86")
    elseif("${arch}" STREQUAL "x86_64")
        set("${abi_out}" "x86_64")
    elseif("${arch}" STREQUAL "any")
        set("${abi_out}" "all")
    else()
        message(FATAL_ERROR "Invalid arch: ${arch}")
    endif()
endmacro()

macro(abi_to_arch abi arch_out)
    if("${abi}" STREQUAL "armeabi-v7a")
        set("${arch_out}" "armv7")
    elseif("${abi}" STREQUAL "arm64-v8a")
        set("${arch_out}" "aarch64")
    elseif("${abi}" STREQUAL "x86")
        set("${arch_out}" "x86")
    elseif("${abi}" STREQUAL "x86_64")
        set("${arch_out}" "x86_64")
    elseif("${abi}" STREQUAL "all")
        set("${arch_out}" "any")
    else()
        message(FATAL_ERROR "Invalid ABI: ${abi}")
    endif()
endmacro()

macro(get_prebuilt pkgname pkgver abi hash)
    abi_to_arch(${abi} arch)

    # Download prebuilt archive to prebuilts directory
    file(
        DOWNLOAD
        ${URL_BASE}/${pkgname}-${pkgver}-${arch}.pkg.tar.xz
        ${MBP_PREBUILTS_DIR}/${pkgname}-${pkgver}-${arch}.pkg.tar.xz
        EXPECTED_HASH ${hash}
        TLS_VERIFY ON
        SHOW_PROGRESS
    )

    # Extract the archive
    if(NOT EXISTS ${MBP_PREBUILTS_BINARY_DIR}/${pkgname}/${pkgver}/${abi}/.PKGINFO)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory ${MBP_PREBUILTS_BINARY_DIR}/${pkgname}/${pkgver}/${abi}
        )

        message(STATUS "Extracting ${pkgname}-${pkgver} (${abi}) ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${MBP_PREBUILTS_DIR}/${pkgname}-${pkgver}-${arch}.pkg.tar.xz
            WORKING_DIRECTORY ${MBP_PREBUILTS_BINARY_DIR}/${pkgname}/${pkgver}/${abi}
        )
    endif()
endmacro()


execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MBP_PREBUILTS_BINARY_DIR}
)

################################################################################
if(MBP_TOP_LEVEL_BUILD)
    message(STATUS "Locking prebuilts directory: ${MBP_PREBUILTS_DIR}")
    file(LOCK ${MBP_PREBUILTS_DIR} DIRECTORY)
endif()
################################################################################

################################################################################
# fuse-exfat for Android
################################################################################

set(EXFAT_VER "1.3.0-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=abeba669aa0a2ab789a4eed802a66e403c6ce17a0f55cc453da779721877231e0a8e42feea1255886e8e8ca42aa424e6f93afbaf1f5eecdbc1ac2dc334cee7ae)
    get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=774869398d6cdd7f6b3bc9d24d1895a7786b17434726bd926696dec1f9a98857e260d6bde27f4ce23881f354b7d92afc5650b1e3cd4b6bd5d6b5474101837883)
    get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=dbde59601b6dedfee9d3628342376717375598097242ab6e7ba08214003dd426beb0ecfb9bd52838d5e6e009417ce700a684e7afe72aaba2e76b0ac2ee34c8b1)
    get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=fea9f2f98e4d71de3af8485c45f29aa03c212204332115f4c79086530ec5b1c784dfaa00c3a535a0a80e667ac74a4ae9600558c21a37767fd853330d653e5dd9)

    foreach(abi armeabi-v7a arm64-v8a x86 x86_64)
        add_sign_files_target(
            sign_prebuilt_exfat_${abi}
            ${MBP_PREBUILTS_BINARY_DIR}/exfat/${EXFAT_VER}/${abi}/bin/mount.exfat
        )

        install(
            FILES ${MBP_PREBUILTS_BINARY_DIR}/exfat/${EXFAT_VER}/${abi}/bin/mount.exfat
                  ${MBP_PREBUILTS_BINARY_DIR}/exfat/${EXFAT_VER}/${abi}/bin/mount.exfat.sig
            DESTINATION ${DATA_INSTALL_DIR}/binaries/android/${abi}/
            COMPONENT Libraries
        )
    endforeach()
endif()

################################################################################
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.3.3-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=5c3b979d9478cb28a4d4a685ef869a6e605af90984760ff55ff308e5e36bebaa98df11dc5ce8c220a3df1cb703fa805d139daf42e9af3872b45dfa8722656af0)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=5d5c3cb041373e139a5f2db37e94eadc9e6dcf9d1305177745ee6792e114da259926eb478a75368f3e0f213816d7a8ac26a1404a844e4419d7c561c047b681b0)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=e83a33ba6878afee65b44f1d122bd9bd472de6f458e52a1e16291ad18dd22f157eba45957341ab3f7c58dc8339261dde33f5bbd9dd20a68a5afd0e9bbedc202d)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=67a6e55c2cceb53bddff4711acfd8f14b41b5dfc1957a2503e17515222e81ad0f1eb45f26bb70dc409ef1c56139f7a939cc6e531dca99d34eabaa02b479b2611)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=13e2bf0af005efff29b1eed10b42784e60d99e7b7b56e61cf3e05df8c8e5d6efa7b37d2752463208dd466ac33f684dccc63b6198c12940bbd45a1d1da87768ea)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=adcab6a11685259144b24fcd27500f629546819f682cfb6065b8fad61f384a9c82c7a2994bb4fdc12896c4a76e12fd6366bb7b541c62995ad506db0b15a4d517)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=49673f4e69edd2f5d1bb1cb68bdc51e5f7c2115ba55f50319bc5c809ea25649e2fe8c6b5a36293bcdca706b8a2ea2b6f3e5c21e142b5326bad0f6938db7ab2d1)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=44e6dde2b2e1864457a583766e404f22290730bbdf03a9f41dadd7fc32ec55c8d875248a0c25f214fa94ef1b4bbb2a1b508197384f3487d1e80683fe3b06258c)
elseif(ANDROID)
    if(${MBP_BUILD_TARGET} STREQUAL android-app)
        set(libarchive_root "${MBP_PREBUILTS_BINARY_DIR}/libarchive-lowapi/${LIBARCHIVE_VER}/${ANDROID_ABI}")
    else()
        set(libarchive_root "${MBP_PREBUILTS_BINARY_DIR}/libarchive/${LIBARCHIVE_VER}/${ANDROID_ABI}")
    endif()
    list(APPEND CMAKE_FIND_ROOT_PATH "${libarchive_root}")

    set(LibArchive_INCLUDE_DIR "${libarchive_root}/include"          CACHE INTERNAL "")
    set(LibArchive_LIBRARY     "${libarchive_root}/lib/libarchive.a" CACHE INTERNAL "")
endif()

###############################################################################
# liblzma for Android
################################################################################

set(LIBLZMA_VER "5.2.4-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(liblzma ${LIBLZMA_VER} armeabi-v7a SHA512=486f0390e37e873fbafa965f1f6dc4363c0aa7f131f21191c888ef191decf68a077f199748b4208e56e0d7f0b0415aad500cb187577ae717ccc239abeb045db9)
    get_prebuilt(liblzma ${LIBLZMA_VER} arm64-v8a   SHA512=fc84f2fd6288c8b7f793a3d79d39e9dde1a29117da13f70d9955a7787d369346d1f6ac6615da4d7487fe9e9441a990b1bbecee78a120096e9c60e6e30d90c7f1)
    get_prebuilt(liblzma ${LIBLZMA_VER} x86         SHA512=e37174e6fcdfd26d23f79233a56d3b46455c60d58360b50e0bc4a268bcd43792bc1d964da6c8416129c8b6e0298bae1f4d63e2aad7320c6c6d2322916cbf4ac6)
    get_prebuilt(liblzma ${LIBLZMA_VER} x86_64      SHA512=6d4b93ed1ede309df14057165e95b309e34aa4a510d8227fbd8e76a380a2f262f5ebe34d142f3730b88fbb31b7259fc235ef1d7241862dd915e77bbb419c49bd)
elseif(ANDROID)
    set(liblzma_root "${MBP_PREBUILTS_BINARY_DIR}/liblzma/${LIBLZMA_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${liblzma_root}")

    set(LIBLZMA_INCLUDE_DIR "${liblzma_root}/include"       CACHE INTERNAL "")
    set(LIBLZMA_LIBRARY     "${liblzma_root}/lib/liblzma.a" CACHE INTERNAL "")
endif()

################################################################################
# lz4 for Android
################################################################################

set(LZ4_VER "1.8.3-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lz4 ${LZ4_VER} armeabi-v7a SHA512=2e2dad80466f75c7e8cb27f002d2bace39bfb3cdcb68e100656ab76d7091d6dcb18a783b81b89698c88e7b1e9016e5b1b42b8dc8e54f0363da1a6249b0c5887e)
    get_prebuilt(lz4 ${LZ4_VER} arm64-v8a   SHA512=86ef9257ffa3cb4b2db57e284df26aa73b99994a275452bca1cec4b00bcbbd875df1c21bc099be1047faddca0103c57fa6d517093549674d0fc0b9a236d721f2)
    get_prebuilt(lz4 ${LZ4_VER} x86         SHA512=ff04c0b432a8033176e0f8d2054031882723ee681075e2aea0377041b85a64d6795ff868dc899ee54e856d98ebb68cfd3231b38fa2599de6642de8d5314f931d)
    get_prebuilt(lz4 ${LZ4_VER} x86_64      SHA512=17bd0e0d6eea82e46a0d0f2a33ca3d5bfebf59fa3d7d6b042b75f39eaccc9d1ae5cbbaa349846770a5a9462440ecda13fa981c073c9ac24817805f0b6ebdbe0f)
elseif(ANDROID)
    set(lz4_root "${MBP_PREBUILTS_BINARY_DIR}/lz4/${LZ4_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${lz4_root}")

    set(LZ4_INCLUDE_DIR "${lz4_root}/include"      CACHE INTERNAL "")
    set(LZ4_LIBRARY     "${lz4_root}/lib/liblz4.a" CACHE INTERNAL "")
endif()

################################################################################
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "8.1.0_r2-3")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libsepol ${LIBSEPOL_VER} armeabi-v7a SHA512=3c913394e7baadd642aa43cdf66016cb19e5bb5931148653672ee32de77977710efe0df8fbe41a9c1b7cc8d75460e9dca3bc6afc8a2a8f82b26c93953fec4fa3)
    get_prebuilt(libsepol ${LIBSEPOL_VER} arm64-v8a   SHA512=6b7b4dca60f797904870bda421570009a37922b4e3632d2d268faa9188aceb3f9e37ee1f4f81ee89c533d70b58c53db84068725b664b50b5fedb1d359465659e)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86         SHA512=a16d521cf86c9077e0b83720c7def782fafcd0bd7237a969a092d8083072f7b94a9f52961a5600be918c95dc49b0e79d04ddb757d442f3494e3be4db40080590)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86_64      SHA512=62b52352b1017d64297e0bbaa557c73a884aedebaa5fa8c6c9bfaeb2fb5dcf79f3925e6ace6deb9919b4a46668852a299d49c4833e8850a87bf906da1b6df923)
elseif(ANDROID)
    set(libsepol_root "${MBP_PREBUILTS_BINARY_DIR}/libsepol/${LIBSEPOL_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libsepol_root}")

    set(LIBSEPOL_INCLUDE_DIR "${libsepol_root}/include"        CACHE INTERNAL "")
    set(LIBSEPOL_LIBRARY     "${libsepol_root}/lib/libsepol.a" CACHE INTERNAL "")
endif()

################################################################################
# boringssl for Android
################################################################################

set(BORINGSSL_VER "r5501.dc3322053-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(boringssl ${BORINGSSL_VER} armeabi-v7a SHA512=f0f3b1ab6df3d762b94a289e6d26a21b7bf4f5e5dda7279f34b6f4309ccd1c2926bef92c6b97c92c1b8317f14562427d5cccf847cad430ceccc5e23c1bd69398)
    get_prebuilt(boringssl ${BORINGSSL_VER} arm64-v8a   SHA512=f3bbe3d3db1141ea63ad079c96b102d32ff90a3396de62843383a859069d20a5fec3f66f21fc7d9688754c4fabfb54372c31352bf9e39e700957a78a52bd20d1)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86         SHA512=df034552e70a55a0c575cdd2a989c5e530e88f31c9d05886064f319403cebd91e9f6512cfaaff37d78abafb6eba727360bd06a4ee3a59624f17d9a1df41fb586)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86_64      SHA512=f5a4d2dc49fe8b2bc2ee7515c68d706d0e20ae2bd6bcfd61f6fcc83e829aff2836be2e972ded15623243e41af55671400ca781d4953c84b8f291e953c5e14512)
elseif(ANDROID)
    set(boringssl_root "${MBP_PREBUILTS_BINARY_DIR}/boringssl/${BORINGSSL_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${boringssl_root}")

    set(OPENSSL_INCLUDE_DIR    "${boringssl_root}/include"         CACHE INTERNAL "")
    set(OPENSSL_CRYPTO_LIBRARY "${boringssl_root}/lib/libcrypto.a" CACHE INTERNAL "")
    set(OPENSSL_SSL_LIBRARY    "${boringssl_root}/lib/libssl.a"    CACHE INTERNAL "")
endif()

################################################################################
# FUSE for Android
################################################################################

set(FUSE_VER "cm_14.1_r1023.3ecfa58-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(fuse ${FUSE_VER} armeabi-v7a SHA512=d9b7c43945471e9ec75e5ea31c39a9f5bb6ddb4b4c079c60b7375fa875c97b95b354c6eb9ba2eee0837806151a3688e446ecfbfa9d059ca36d0496571362f170)
    get_prebuilt(fuse ${FUSE_VER} arm64-v8a   SHA512=f75435a8696067711b50959458180fbeb5e50e39de91b521411fa93ea184172c98b9029c11f9c9d01683e4d13e798bba7322c62e5ca3d19686e24f47eaf1fa71)
    get_prebuilt(fuse ${FUSE_VER} x86         SHA512=9c91b9eddf12400f5fab8859bd0fc728db318caaf75ce493c309a0f8b48879c80ed519ff6d1c6c7900a76a3c2db5240dce44c5a232c9982c7bf6b0309c7061be)
    get_prebuilt(fuse ${FUSE_VER} x86_64      SHA512=55228d906f8f414203e32b923f298e390270b99d0fc039fabf480354657586d4704ffb420b7a163a7224cf2fc7cfab4ca3726ef346a1d66a6f0740a11495f6bf)
elseif(ANDROID)
    set(fuse_root "${MBP_PREBUILTS_BINARY_DIR}/fuse/${FUSE_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${fuse_root}")

    set(FUSE_INCLUDE_DIR "${fuse_root}/include"       CACHE INTERNAL "")
    set(FUSE_LIBRARY     "${fuse_root}/lib/libfuse.a" CACHE INTERNAL "")
endif()

################################################################################
# qt5 for Android
################################################################################

set(QT5_VER "5.12.1-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(qt5 ${QT5_VER} armeabi-v7a SHA512=081818d7f7e9f59d6fbee70419a7fa088290fe3a6180cd8b7459f3959db632e58203b67b8802031c2efcfcee25029031c1019ea883a06d4fff541844087656e3)
    get_prebuilt(qt5 ${QT5_VER} arm64-v8a   SHA512=4f2d8e786d2f7201b4b3402fc276547d0b701014a747139966696305948c224b03884ec9f7134c7f38619212d51360e3e35b6fe5ef64aad62f646c82d1c016a0)
    get_prebuilt(qt5 ${QT5_VER} x86         SHA512=2b5044d11facf0c5d9c17586141799b72e0908d4b8edf55e5612dccbcf943e4af21de87ed9eb7eeaf99b18251ac312c117414c1d48a6d10559e23e8fcb5199da)
    get_prebuilt(qt5 ${QT5_VER} x86_64      SHA512=a1d7db8fe3dc1fce0bbf068db1ee78cbef7f2d8c9c6ac31d4e76ba79abe76f7dbda9af7607a86e7480be3e6a3f8be7b05f8edc7a0b46bc679614b6aba1231bd4)
elseif(ANDROID)
    set(qt5_root "${MBP_PREBUILTS_BINARY_DIR}/qt5/${QT5_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${qt5_root}")
endif()

################################################################################
# test-runner-image
################################################################################

if(MBP_ENABLE_QEMU)
    set(TEST_RUNNER_IMAGE_VER "1.0-1")

    if(MBP_TOP_LEVEL_BUILD)
        get_prebuilt(test-runner-image ${TEST_RUNNER_IMAGE_VER} armeabi-v7a SHA512=d7ff0445b06e75feaaf8995ec90c2d8e307edacac5d4db5c9b6a5caeb3df2b5a82af369b41ad699ce2b0af629ded213ebae84c495250f7f1f8c35844bef0f23b)
        get_prebuilt(test-runner-image ${TEST_RUNNER_IMAGE_VER} arm64-v8a   SHA512=017034706f3851268442a37c9cb1509fbf69656c544110c2673fe59e721ff76a12e20d0557ca32edc57f8bdb9b9c60e499cde1279311eb7fa81c4a5840f26c74)
        get_prebuilt(test-runner-image ${TEST_RUNNER_IMAGE_VER} x86         SHA512=939adf3f2988a620f176bfcc648a1396c5159f01cdd76141b27d4d652cd20f9341eabdf4291bc5884d176bf5ca8994762fe7f930e801d22e0f3cb1bb1c9f32a5)
        get_prebuilt(test-runner-image ${TEST_RUNNER_IMAGE_VER} x86_64      SHA512=14f3aab1f8709fe1379eb347b03ea49f90950792ae09979bc1b6c5408d14dd505a126100334b472b4302d6fb6bacd01082ccf17fc1e46f7fbd462403d2412d1c)
    elseif(ANDROID)
        set(test_runner_image_root "${MBP_PREBUILTS_BINARY_DIR}/test-runner-image/${TEST_RUNNER_IMAGE_VER}/${ANDROID_ABI}")

        set(TEST_RUNNER_IMAGE_DIR "${test_runner_image_root}" CACHE INTERNAL "")
    endif()
endif()

################################################################################
# AROMA for Android
################################################################################

set(AROMA_VER "2.70RC2")

if(MBP_TOP_LEVEL_BUILD)
    file(
        DOWNLOAD
        #"http://forum.xda-developers.com/devdb/project/dl/?id=286&task=get"
        "https://dbp.noobdev.io/mirror/aroma-${AROMA_VER}.zip"
        ${MBP_PREBUILTS_DIR}/aroma-${AROMA_VER}.zip
        EXPECTED_HASH MD5=a77c4993803db28d53cd7e6a37ec73b5
        EXPECTED_HASH SHA512=44abff7bd536908ae8cde9a17e1fb334b59561e115f54b23bf910e1f7920b6f35ab078d3353db65c3526e25c0be27cd592470145063cafd4e05418e4bece775f
        TLS_VERIFY ON
        SHOW_PROGRESS
    )

    set(THIRD_PARTY_AROMA_FILE "${MBP_PREBUILTS_DIR}/aroma-${AROMA_VER}.zip" PARENT_SCOPE)
endif()

################################################################################
if(MBP_TOP_LEVEL_BUILD)
    message(STATUS "Unlocking prebuilts directory: ${MBP_PREBUILTS_DIR}")
    file(LOCK ${MBP_PREBUILTS_DIR} DIRECTORY RELEASE)
endif()
################################################################################

# Ensure parent scope can see the changes to CMAKE_FIND_ROOT_PATH
set(CMAKE_FIND_ROOT_PATH "${CMAKE_FIND_ROOT_PATH}" PARENT_SCOPE)
