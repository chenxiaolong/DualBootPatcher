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

set(EXFAT_VER "1.2.8-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=66a9bcb02c3d759e0297382d993ed82637f7d0e1c056bdd39ea81574edc517e80fff38692966c08f221f656c3a41e2b6cdf8750eb4a019e57e29ec4e1ad6fa7c)
    get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=0f0615ea4fe1716d1ec24c77ef55404374a932d963d9349caaf30868909f6704dc8142b9272d6bb10f9f46e6cd4ccfcaab09eac63235497eeb946e192b0da62c)
    get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=03290cdafbe1b570f655241a27a66e579e47d880f595adb0f578287cb5d8a6b56fb3d68569efdf5722afb71a21622673ca82c4aa3dead15a0294546167814db2)
    get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=36e280594185a865e16a6aae7366497a079f27bc9851a664e181f012d2aa39d31ac9b79fff8fca681386da1696be0e5937161409abaadf1ee3258598c39905ab)

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

set(LIBARCHIVE_VER "3.3.2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=7b239264b78361e3c2eb8438970419f2365aeb9ac68fa5d721544fe40aacd7e400b67bc4d86a535d70ed7a603f5c455c1b6c2ce292d2ea36cbc3ac8c3f20b926)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=835f7f4fa46c40b07d42d0c7a6db3459e3f3d6602e5f4f45475e57e90ff6d2ba3e3608506958e0302451e5ae5da3e9a794d1dd70c53d6a0ffcefcfdb5b94ddb7)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=8f51786bcab3f28a928ac0ec917081199f235b48c69b491f947e068c57538edb204965b943ed9b33158cc01d353fc5de21f10638b42deb529f7bbfebe27c53bd)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=73befec4363447699aaf2225fbb5b08ea9d887dd76e9431e2311f61ee5bed5be61e76f39629e5fcfa779c26b808581762bf130e96d8e47569371db356212e267)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=914fe4d2b6db3e622a115f205442b57ed09a2859adfc65d3d64708c11dcc5945af1015d083c915f28180b8702931bdb368088e4cf46953ee6bdea80cc3840138)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=dac57f85bd4cab85ad9c59ba86f2bf3fbac0ba9089224de85189f1084fb675728896ba002fbd4ed5f99904c9ab7ff7ca6dfe10da3b7cfbdf43ca12edc8a2dd32)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=107bfa9fb8054b9bde493d46a8b16bb0b7e2571108aea0d39413fb7765c692aa8e6a5bf7762c818933f87e976152f90e0ba175030805ddf907d32ce8b142a549)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=0efc4cd5461d2000ac1dcac205c9fbc75a40d2d58d54c0a3fa6915da87d34b41f4cc3aa1b2cc36189b9c46c6ea92baf88e56039a5eaf34e8fab0012afd3de0f3)
elseif(ANDROID)
    if(${MBP_BUILD_TARGET} STREQUAL android-app)
        # Use API 17 version of the library
        set(libarchive_root "${MBP_PREBUILTS_BINARY_DIR}/libarchive-lowapi/${LIBARCHIVE_VER}/${ANDROID_ABI}")
    else()
        # Use API 21 version of the library
        set(libarchive_root "${MBP_PREBUILTS_BINARY_DIR}/libarchive/${LIBARCHIVE_VER}/${ANDROID_ABI}")
    endif()
    list(APPEND CMAKE_FIND_ROOT_PATH "${libarchive_root}")

    set(LibArchive_INCLUDE_DIR "${libarchive_root}/include"          CACHE INTERNAL "")
    set(LibArchive_LIBRARY     "${libarchive_root}/lib/libarchive.a" CACHE INTERNAL "")
endif()

################################################################################
# libiconv for Android
################################################################################

set(LIBICONV_VER "1.15-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libiconv ${LIBICONV_VER} armeabi-v7a SHA512=408d375cdedce1c3545a2b1e29c3d86780f9fe726eee5b23348540ee032230680affcd9c6685fc8227d578819e046e2544b35896a4d4248d1d59b5f46399c286)
    get_prebuilt(libiconv ${LIBICONV_VER} arm64-v8a   SHA512=ce1229780ecd372d014ef8a876eba0ec3942f494cd2532985961c827b92ae9853f99e8bd06566bac218619e384bfe7edcb9eaf4da8efbc159d6b5763cbd56255)
    get_prebuilt(libiconv ${LIBICONV_VER} x86         SHA512=6fff503771eab5e021eaf8bbc7952d0d4d1d358f9f7a24ebb2e8e72613d51be04eee0824a10b5271dc32f00f1432b0320758678b4462c7b056ebb543f9ca8556)
    get_prebuilt(libiconv ${LIBICONV_VER} x86_64      SHA512=5da1a7282e9cae8bd9d73efcec61163c740a1d04e513f24445b03b3ef1bab97b4fa2fc5bed245519c0a62ab521484e07eca0ea09696dd7922184380cdb596ec3)
elseif(ANDROID)
    set(libiconv_root "${MBP_PREBUILTS_BINARY_DIR}/libiconv/${LIBICONV_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libiconv_root}")

    set(ICONV_INCLUDE_DIR "${libiconv_root}/include"        CACHE INTERNAL "")
    set(ICONV_LIBRARY     "${libiconv_root}/lib/libiconv.a" CACHE INTERNAL "")
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

set(LZ4_VER "1.8.2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lz4 ${LZ4_VER} armeabi-v7a SHA512=54a0dc9a4f09cd952156ba7ba17fb60b450262b6ef900b6be94bdf4e9acd5b8676f8087c1b79c9a8a176661e6766e61e672677ddf874f246f74440ff87b96d6f)
    get_prebuilt(lz4 ${LZ4_VER} arm64-v8a   SHA512=c6700378d31860c7704fb88265f2a5bd076f57b528f6acc436c8bd360bd1f307e056cfef245c73eb0e07ddf67c286d8af259f0ac66d1394f843b2cc0931565e4)
    get_prebuilt(lz4 ${LZ4_VER} x86         SHA512=51d71fd538c7f21373592bd3e6957484768aaa74112fbb296df2bb885d2f92d05f3319e2b0cbe62eceb365b05fbfe32f763dc5000b6d9a7ac4927e4422d31aa0)
    get_prebuilt(lz4 ${LZ4_VER} x86_64      SHA512=86448cce689ca3b0eb69c157a7020d1197a4e11592dbb53eded858cc18ed1c34041f4a9e8c52df18906a838f310816977fdb88190b6cdd4b239da47345e63149)
elseif(ANDROID)
    set(lz4_root "${MBP_PREBUILTS_BINARY_DIR}/lz4/${LZ4_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${lz4_root}")

    set(LZ4_INCLUDE_DIR "${lz4_root}/include"      CACHE INTERNAL "")
    set(LZ4_LIBRARY     "${lz4_root}/lib/liblz4.a" CACHE INTERNAL "")
endif()

################################################################################
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libsepol ${LIBSEPOL_VER} armeabi-v7a SHA512=10869a22e56b8b58624fef45849fac29d386ecb738e5733900ef26244f6747662e65dad40fb3bed845cc1d30f98a441d71da03a6edf837f4a6bcc904f662bd7c)
    get_prebuilt(libsepol ${LIBSEPOL_VER} arm64-v8a   SHA512=fc3229d925d973165bea2dba1f892ef8b4f97811bd8b6b266be533e0ca36e07c519c72f41b85eb4a053eb0719f6a234dd2f1ecd234ad8b18a3bf06f5e8856815)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86         SHA512=4be6f8592836cebb965c22f29a2028cfd07a1a5467d3aa781d0c9c08d058b2e80eac79e92946a9d1b6e0d314d68d76c421a7997fff7e956fc3b2259823cf0156)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86_64      SHA512=6384a4ef8d44c122064049fc8668a02c97e8bacf9c39bdbdb5fa4f9397485becfbb6799d63a3490c65a362b2a41fc41020d23b31f144efb0125133434098952e)
elseif(ANDROID)
    set(libsepol_root "${MBP_PREBUILTS_BINARY_DIR}/libsepol/${LIBSEPOL_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libsepol_root}")

    set(LIBSEPOL_INCLUDE_DIR "${libsepol_root}/include"        CACHE INTERNAL "")
    set(LIBSEPOL_LIBRARY     "${libsepol_root}/lib/libsepol.a" CACHE INTERNAL "")
endif()

################################################################################
# libpng for Android
################################################################################

set(LIBPNG_VER "8.1.0_r2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libpng ${LIBPNG_VER} armeabi-v7a SHA512=0f1a34e27b39cb54aea366c9ba496ccec5faca4dcdb03ba75e91b5e8cadb4f6c88a6feb48e00df136561df360e0a7ce5540a300223f7377b3fc93df1c8f07958)
    get_prebuilt(libpng ${LIBPNG_VER} arm64-v8a   SHA512=fb5110df17096581e0b2faf6d1ff3e6abfe080380a1d87877c3fee854b2ac4719440107e3b6f5eb9a390c226ead8f726765e2062cd098c3fc3ac22443d1506f9)
    get_prebuilt(libpng ${LIBPNG_VER} x86         SHA512=b3fff96d96e43d6776b61007146e54138cc41c25b10ed14e2b75ff82ac07c244f66d34d2097f6708d5ebf08d8eae7acb5105131d303208d4e1f5023b6410ad7e)
    get_prebuilt(libpng ${LIBPNG_VER} x86_64      SHA512=7551c14005976f505c301f6470ca777f5505d9e1e99e2d7457917e31855e04fc3cf7f6f1d1ec8fb1c64b71bfdf11d9c035aa40046a2e61d67676949ea0d93f39)
elseif(ANDROID)
    set(libpng_root "${MBP_PREBUILTS_BINARY_DIR}/libpng/${LIBPNG_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libpng_root}")

    set(PNG_PNG_INCLUDE_DIR "${libpng_root}/include"      CACHE INTERNAL "")
    set(PNG_LIBRARY         "${libpng_root}/lib/libpng.a" CACHE INTERNAL "")
endif()

################################################################################
# freetype2 for Android
################################################################################

set(FREETYPE2_VER "8.1.0_r2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(freetype2 ${FREETYPE2_VER} armeabi-v7a SHA512=af1277a2c4422640e8de4c0406e590b06a18227cfe35799f3939e65fb50c08de3376e58175583771b802e5a03bacd90fb2bf5a13c1369296084ab6f8c8240696)
    get_prebuilt(freetype2 ${FREETYPE2_VER} arm64-v8a   SHA512=bfba808fd55e7ccf5ce4ca88ff496fc2a3a6aa6c5f68079a2a3b5ec88133aa0f053cbfe200f3e76d3ecaafe2ca4ce3a5d70d795c25bdb17233a163dde878b47e )
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86         SHA512=3897a468cb66f8b47325d45cbe197a7feb34e3d8297db6ae1f601ade112cb78d8056ca68d0bd98af2a1facd714de414e747d561e9ca4ffb615d2ccec00a81d7b)
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86_64      SHA512=3bbe49a9ff35b1965d36f7747c04568458f7030fb23f0684c9ff0cf704ec9510a17aa2c0fc7d71ec3bb43b7736d703e9ceffeeb8404a795ec3abe079eca04d57)
elseif(ANDROID)
    set(freetype_root "${MBP_PREBUILTS_BINARY_DIR}/freetype2/${FREETYPE2_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${freetype_root}")

    set(FREETYPE_INCLUDE_DIR_freetype2 "${freetype_root}/include"      CACHE INTERNAL "")
    set(FREETYPE_INCLUDE_DIR_ft2build  "${freetype_root}/include"      CACHE INTERNAL "")
    # AOSP calls the library libft2 instead of libfreetype for some reason
    set(FREETYPE_LIBRARY               "${freetype_root}/lib/libft2.a" CACHE INTERNAL "")
endif()

################################################################################
# libdrm for Android
################################################################################

set(LIBDRM_VER "8.1.0_r2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libdrm ${LIBDRM_VER} armeabi-v7a SHA512=ddc784d7a4289ca86fe98fb524a1a1fa5d087b38dd2bd94f7eb3bc5396cc39237cd0cac681a6b7fc14af27828d9723ddc0181fce0cf1db732bb54ccb42cd33d3)
    get_prebuilt(libdrm ${LIBDRM_VER} arm64-v8a   SHA512=71e310ba96825699907c97d8dd2d0b738bd54f080c4838e965e322ad6329c36439efbc9594e519593c3590e7b0ea4d2a256603d4d3c1e114b14c5b5098b9b2eb)
    get_prebuilt(libdrm ${LIBDRM_VER} x86         SHA512=f045f2c33a0bd287eb2ddf3f1ddc375c061db4ec9149300e38e2783f72de795716219213f9f3368860893e69ca1384cba1ffdecc5b25ab04c3e3b1ce0e7ec49d)
    get_prebuilt(libdrm ${LIBDRM_VER} x86_64      SHA512=715a2cc39f7dd057764bff715ce1f60509bda182780c1c63a941d1ea1a3729ace097ef79370174da1e7f89ed2c217338979e57bcafca9e7ace2c01bed9c60542)
elseif(ANDROID)
    set(libdrm_root "${MBP_PREBUILTS_BINARY_DIR}/libdrm/${LIBDRM_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libdrm_root}")

    set(LIBDRM_INCLUDE_DIR "${libdrm_root}/include"      CACHE INTERNAL "")
    set(LIBDRM_LIBRARY     "${libdrm_root}/lib/libdrm.a" CACHE INTERNAL "")
endif()

################################################################################
# safe-iop for Android
################################################################################

set(SAFE_IOP_VER "8.1.0_r2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} armeabi-v7a SHA512=420ed2ad3734c0424166802b5db626393aab45dbaaa3482d318555d97c97a62a6a2221b80123eab9ff2338d10333efa7c408a52560d84153a4eb6b28d1ee2270)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} arm64-v8a   SHA512=97688da51630653470e6a8cc5dbc23c1bb5f6bb6866ab9fd6726c4062f77c14b28f0723f33d28c5e493b33449fd28305332245fdfe22c6ab6c86df24cdd18f19)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86         SHA512=f846ed837ac644bfa6d2cbf5dcef86126a8b1ce02b3f740f028e37b25d882c6b0d28018450796796ceef5426155d2c990e9ee1903f7cd3b8c9760247a6dbffbe)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86_64      SHA512=4b6cf93ac93b575bb0c0c909643f844fda52adff491ff27631f4698348c11e751b5ade69a58d9247734c3c5c2e7092b82ae6d6d04ddd3d011979dddb0329a691)
elseif(ANDROID)
    set(safe_iop_root "${MBP_PREBUILTS_BINARY_DIR}/safe-iop/${SAFE_IOP_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${safe_iop_root}")

    set(SAFE_IOP_INCLUDE_DIR "${safe_iop_root}/include"           CACHE INTERNAL "")
    set(SAFE_IOP_LIBRARY     "${safe_iop_root}/lib/libsafe_iop.a" CACHE INTERNAL "")
endif()

################################################################################
# AOSP system/core libraries
################################################################################

set(ANDROID_SYSTEM_CORE_VER "8.1.0_r2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} armeabi-v7a SHA512=970ab64fd94a20df0f597c11322475a150bd1b89f1f2115b14213e571d53637e9ca66e78f9612e6c5228a0c80f9b6e6de6307b03cef7fd9ab35b23ffaa8b8b6d)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} arm64-v8a   SHA512=70c0d1e878c21b83c83c1cc02dc4d1e63c03ec68659bd7dd79379d5a6dc24c084307673ed16d36c0d7d4c3d85bcb5f9a3cc4d6a08bea0d0ced03c68cb4c035b6)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86         SHA512=a1d36212b478a82ea7784510fef0a370c407db45c587aa54e41e524fb13f9a9ffc84082e74b72442424ec1994ce430cf356a16d28abd875cc7ea8fddd9bc74c9)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86_64      SHA512=4bfc8c00ded97f37a9c3ce64f6517f48b8291aacbc80ecb18b906b8366d77086d2f679d8032f16f133d222e8d88a6dda0c6f9215349f71bf88530fec406ed581)
elseif(ANDROID)
    set(android_system_core_root "${MBP_PREBUILTS_BINARY_DIR}/android-system-core/${ANDROID_SYSTEM_CORE_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${android_system_core_root}")

    set(ANDROID_LIBCUTILS_INCLUDE_DIR    "${android_system_core_root}/include"               CACHE INTERNAL "")
    set(ANDROID_LIBCUTILS_LIBRARY        "${android_system_core_root}/lib/libcutils.a"       CACHE INTERNAL "")
    set(ANDROID_LIBLOG_INCLUDE_DIR       "${android_system_core_root}/include"               CACHE INTERNAL "")
    set(ANDROID_LIBLOG_LIBRARY           "${android_system_core_root}/lib/liblog.a"          CACHE INTERNAL "")
    set(ANDROID_LIBUTILS_INCLUDE_DIR     "${android_system_core_root}/include"               CACHE INTERNAL "")
    set(ANDROID_LIBUTILS_LIBRARY         "${android_system_core_root}/lib/libutils.a"        CACHE INTERNAL "")
    set(ANDROID_PIXELFLINGER_INCLUDE_DIR "${android_system_core_root}/include"               CACHE INTERNAL "")
    set(ANDROID_PIXELFLINGER_LIBRARY     "${android_system_core_root}/lib/libpixelflinger.a" CACHE INTERNAL "")
endif()

################################################################################
# boringssl for Android
################################################################################

set(BORINGSSL_VER "r5241.982279b36-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(boringssl ${BORINGSSL_VER} armeabi-v7a SHA512=4d1440e1c113fce11b5d4defb8d0bacb20f7c6d127ee2fb739c62f8bf19ae57687e7c02cb8c5261aad4c13faa52cd06b2b232113be662ecbe1d93b6344750517)
    get_prebuilt(boringssl ${BORINGSSL_VER} arm64-v8a   SHA512=d30a3d9fa0d329e42d3f05b69f9a1551789b40dbf5e73a5c76a23ba078cb9b436f00bb7d21ffc470b31eb91fdf45e4b6ce8d607dc94bbce2f14f5e2752400677)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86         SHA512=62029e03af18fe4233db23b17f9c5641d701ea5ea2ea60000d3055bddfc3496fca5b91fa612d7ead0a92416e14194c8b4d0b30f18fe206b2e66d0877adb9b2e1)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86_64      SHA512=6fd872db4296f1ce26dbbfe2226eb16310c670310650d16a06829ec101e3cbb5a26f0ba0561dc9573d696450ee12bda991a25e01635b6173f7e9e3037776ddb0)
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
# procps-ng for Android
################################################################################

set(PROCPS_NG_VER "3.3.12-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} armeabi-v7a SHA512=b9ad536720a8832eefe2a6a685f1ab9b5897c455569ff6c080b0345a82e217dc550144d6a0351801e31e215e1682e0aa6b4cb806101ffb57e4a29a3ac41db4f0)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} arm64-v8a   SHA512=43c46843d7f1aad136806562f79b6772d5f20cf8ff58d3f77e5d0b9c1b34f49a86c5180c1f6df79e0a5cbcca62c76372e58b01770112780a40cae9c71b773fab)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} x86         SHA512=f8ad8cff112b2e9165fc1bf9df876abc8180a4f79c626413955c394c3f18c656310008eace93e00df5d46aac0db0f3cfb672290486cd8ff0f4b3205900e672be)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} x86_64      SHA512=fbfc19f9b7ee14f68d7d19a946eb0e229831fc8ff79484ad95d56e0c86b10ed99801be1f5f91b777c6d7341f62e57eb56dc5435314d9cf5b7ac2bee49bf1eb69)
elseif(ANDROID)
    set(procps_root "${MBP_PREBUILTS_BINARY_DIR}/procps-ng/${PROCPS_NG_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${procps_root}")

    set(PROCPS_INCLUDE_DIR "${procps_root}/include"         CACHE INTERNAL "")
    set(PROCPS_LIBRARY     "${procps_root}/lib/libprocps.a" CACHE INTERNAL "")
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
