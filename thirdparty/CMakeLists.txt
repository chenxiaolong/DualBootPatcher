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

set(EXFAT_VER "1.2.7-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=f60375270c90430486fd653a1bc59f08822e1864463637542ce36b1beecb2330bce473533004f20498ae8213b338f91f39d2e6649c16343502a2ab1eae6b12c2)
    get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=bf9bde23604e47faea482cea81f14c7901459da35666c862ee40fb5de0c8329578aded2339a3cb7b1559586a5995e9b8a8a06ca7996c16c134c548cd035c99c6)
    get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=d26d41aa3e1ee03163e7b72065a1ad4ff3ada040877afb4d964991f92357f3bc6e704863e811b44d6325d0745c7a5d39ee0415715d5289a79c7f552528e59512)
    get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=6ea70d0ad356be6e3795645b8eaa2d9b5ce845ea28fdf028a1b5f2b363c09bc5453f72fbaa52082bb1436e4a9842f92db1e22e14c7c3a491bc233164df5c85e8)

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

set(LIBARCHIVE_VER "3.3.2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=d856695ce2332c416e24faea4ac579cbf7fc87605a5f0f4f70b17154178aeccb92f8bb8e6581576df019788f01374929ab639eb7663756b86011c443ae793242)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=7a1c6a6aee80c9581c1b824c0c395896e211bb0cc188a097cbd26f4ad02879f3662596fc391b806d85d1c350891f6d958573f768e00e610595e7bc966127d136)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=1a500fa7ad3fbb72a341cf0493d2694c0d51ae6d1de8f2de62f227095200c89d71f4d2d455d2c314e49318a2aa719501d8dd0c97792323d1d39e9f8d91670ce0)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=eb73f6eec0b4af4ae3e4efcde8bf7235cd0cab017935f5c6c379830dadce115512114220673b1bb344bdd925c1901bd10b450b059130cf19b40029c49b19834f)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=2f3ef295acee3186aae37475a238026f57493e9173bf445a2cb11d4dd17c67244f6c81dc1f21cf238dd25a4455c96b4fe6898f0cb22162315ae8a32363a257ee)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=51e31086eb84aea93daec5099c3060e9c35e1689cf6144b1e7000c302d1afd2411cabbcba4c2d655225704fdfeb51fa855547067c3ac2d5e5c55ef5789c5b4ea)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=bf7321187e753b5596ed0f3385b7b76f2f4b4e028a7f4a907593e839451226b1639affcb018c2538c991d8c92af2b02d5035491d0e176335461a38432fb1bb91)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=455c6061df7a9c9625fb7b383e4852908d64b191c2386b19cfd769ccb569b8bce45a884419c0b483273cb2659e0a5ae03506ca3ee900ef478feae586ccacf357)
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

set(LIBLZMA_VER "5.2.3-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(liblzma ${LIBLZMA_VER} armeabi-v7a SHA512=2fd3c5578a6313abc7737d9862c9e2dde35296004439ab7fe41b42a54ee094295a04a83b0a565ac4801c01cbfb050189405163d0ab7a6b6bf48e128e6adb2bb1)
    get_prebuilt(liblzma ${LIBLZMA_VER} arm64-v8a   SHA512=aacf52a9779c448182f0dcb011bbe0b658019d4b8b6a436070b8c57140787a8638a7696c9cbc5a04c0947b2d309d78a7aa98a85d28a67329a34c0fb01018608b)
    get_prebuilt(liblzma ${LIBLZMA_VER} x86         SHA512=c685fe430601bb35609e9159427a98db4d8e9e5b1f414335216366b643b154c978a02815e5c2507cf73f2fcad32956e4dcc243c3fafbe6afd8e4a948e78d9a67)
    get_prebuilt(liblzma ${LIBLZMA_VER} x86_64      SHA512=e48433585641d3caf86d17fd2ecac8fdeacd236705997303a06ba9a45ecdba94f6d9713386e1d5fa1413743cb459222fd37c3334d10e29e88ff90d1915e34092)
elseif(ANDROID)
    set(liblzma_root "${MBP_PREBUILTS_BINARY_DIR}/liblzma/${LIBLZMA_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${liblzma_root}")

    set(LIBLZMA_INCLUDE_DIR "${liblzma_root}/include"       CACHE INTERNAL "")
    set(LIBLZMA_LIBRARY     "${liblzma_root}/lib/liblzma.a" CACHE INTERNAL "")
endif()

################################################################################
# lz4 for Android
################################################################################

set(LZ4_VER "1.8.0-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lz4 ${LZ4_VER} armeabi-v7a SHA512=b3dc0e3eca7f81fb7aa20257f37d2ae873af62981b16554f1d022643fcbfe6e7aaa22864c26be9091acc5449a0bc08039c9e0bd5fed04f10e7060456b807311d)
    get_prebuilt(lz4 ${LZ4_VER} arm64-v8a   SHA512=534a6f6c39bc25f89d328dc937ade807a041f863bcac5653e405d23509357ea3f0bd4ea948b878b69a97925c46d40b943ee5fa63f2ccf4f3e42030eb1dd21d2e)
    get_prebuilt(lz4 ${LZ4_VER} x86         SHA512=b8f491d9bd164f4cf3b114117f42b1a6f28381516e517d059e19036d2e86e6e5522a9e63d9ce778553ef8743c7f9a02a7cdf4b5eb7f171b94954b4be9f364c52)
    get_prebuilt(lz4 ${LZ4_VER} x86_64      SHA512=d29bfa4508136cb4fae29216ca1dd6fbc2fc5d09b6472ffc462f2a782602021ed9c12296cf27f6aa7eefa968130eb232b9aab90c9c6afebe9efe77e321667cdc)
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

set(BORINGSSL_VER "r4885.9770532a-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(boringssl ${BORINGSSL_VER} armeabi-v7a SHA512=78e7bf80cc05abc655fb46e5de369412146537b1ac7bf4fbd21e1b1ca944b888d9710b6e7c3fba8e21a109bc841e6f1216a0159a7fe7cd0166f85b6d5ab44d35)
    get_prebuilt(boringssl ${BORINGSSL_VER} arm64-v8a   SHA512=42b46a1ffb36490fed65380341f899efa5382beb5be098cd35c1e4e93878b438ba5f0f55273a581446451de8ab3beea1d3eb47288c5b8d9ca3bc18cc31b42f89)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86         SHA512=84cac549586f9827f0e0ed8d1070acb575fe5356c35d7272db2a3dbdcc72928d2fc776e61dc6fb11fd2490c40cb63469cd24db54c23e72a33f7b5ca17ad9cbe8)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86_64      SHA512=7ef1e1b4d6009bcba555eff1e996c82af0efbbda54e776ab81440cc741b965bde334bc6d28ff1f2f9f7aafba7c8d6354ff40f410c7b2bad90b8f0765a76ae1b1)
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
