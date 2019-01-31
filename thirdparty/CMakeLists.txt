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
# libpng for Android
################################################################################

set(LIBPNG_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libpng ${LIBPNG_VER} armeabi-v7a SHA512=d7e3a1d01b06db858e0fa76248db06a7e7e40f5c9eb9df8a484f0102c4bf637cecbdfbb96d5aef29cc52cb1804eb134631d0e1c4dbb444c0585af8270f5bfc20)
    get_prebuilt(libpng ${LIBPNG_VER} arm64-v8a   SHA512=b53337eb0e552bc0aeeeffdabd99216ab441bac17ece16542a24c36504d6c12953e95f3c7c3a1b0a10146fa1c91f3e0c7fbe9781e967a9c20f975c1b00758642)
    get_prebuilt(libpng ${LIBPNG_VER} x86         SHA512=187b79724027d65e46973e516559532b8216aeb02b886ca7932d074c1365dc0b83cab75139616a8a657a873a4dc5ae2fbe36e4d406d85b51fc43b4662760f9e3)
    get_prebuilt(libpng ${LIBPNG_VER} x86_64      SHA512=70bfd7733dc5f1fb5886a7d6cbe3bca57795ed1047ffaa9eb446dde5250bb8e0bcfb6804c35b48d49d0341a3b6662154ba34ea74934b0ca453ae2931032e29db)
elseif(ANDROID)
    set(libpng_root "${MBP_PREBUILTS_BINARY_DIR}/libpng/${LIBPNG_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libpng_root}")

    set(PNG_PNG_INCLUDE_DIR "${libpng_root}/include"      CACHE INTERNAL "")
    set(PNG_LIBRARY         "${libpng_root}/lib/libpng.a" CACHE INTERNAL "")
endif()

################################################################################
# freetype2 for Android
################################################################################

set(FREETYPE2_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(freetype2 ${FREETYPE2_VER} armeabi-v7a SHA512=144d8f54786d909bf9b73d6841fd25494e09ba725863fdb707df2286860c6c31b2594663e1b24cf385f757f04e9bc10cec819e24e3be6adedf77281513888d51)
    get_prebuilt(freetype2 ${FREETYPE2_VER} arm64-v8a   SHA512=323331521674881876a75f13d23b5cba557b8d95cd39b9558e5368cbdd20f8fea53a5d153c2d32db197614d7061eff635b2e0f577ad8a0a937fb19e02c6560cb )
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86         SHA512=43a506a1bf40ec67809abf579cde3ea9b40458c1e00a549be2b7e76c26e90230e02c76abd31e2b42ef99a728e547b55772d3e9089385af98441ae8357b1a523b)
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86_64      SHA512=290b6afa6f1ef0a5da253d650faa93e788b503c7cc1385297705634c566d5543990350b5828068ba5c326c62d40f6c747cf64a88812b0024a647967ce15665ea)
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

set(LIBDRM_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libdrm ${LIBDRM_VER} armeabi-v7a SHA512=58f854e894955e44659c99fb59f17c00fc631fe6e89a31243beb51840813e3207f3417cc582ddb59a03ec465013782d21595f51d243385640ba81ad43b85838a)
    get_prebuilt(libdrm ${LIBDRM_VER} arm64-v8a   SHA512=fbf7022ddca22345a004ba733edf5129e5af4ee35a6b95e3305a51aea834af138e63e5ba640d13d0a0212ac1308bf36f61ee3064484f35a1e91277eb37554bd5)
    get_prebuilt(libdrm ${LIBDRM_VER} x86         SHA512=ec738a7ca9c208eea70083a823c2b739261b65e213fd025d8df5a436dbc55f000cb95abc8ac0ffdc5b219d2c32c409c6d46019d976bc291b6a42f4aab313d3e3)
    get_prebuilt(libdrm ${LIBDRM_VER} x86_64      SHA512=dfbcb7caba8d27cf66d9010e67a555b8fe722b00b97d63578c94433a8e6f74016cb7890b30c72f34e03573b13501cfa2b375066f03a4c80a7ac84556f69a7d2f)
elseif(ANDROID)
    set(libdrm_root "${MBP_PREBUILTS_BINARY_DIR}/libdrm/${LIBDRM_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${libdrm_root}")

    set(LIBDRM_INCLUDE_DIR "${libdrm_root}/include"      CACHE INTERNAL "")
    set(LIBDRM_LIBRARY     "${libdrm_root}/lib/libdrm.a" CACHE INTERNAL "")
endif()

################################################################################
# safe-iop for Android
################################################################################

set(SAFE_IOP_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} armeabi-v7a SHA512=6332d33ce3dae5e5a00e998cfc686778f25802f1852015db2a2438a1a8cd1d4712548b13b36d6e3194405d64a0e02878c5ff12d16e69138f25e97342fe521323)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} arm64-v8a   SHA512=4cf43213649fafc166f64540ac30c5c4679082d55946f110b45910fe2eeed6a304626cd674ee2aa0ac7accf87066d6243a6bd7b4b2282e3becdb85367457e84c)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86         SHA512=407def1bbf213798a92bb44cdee9e3100f1b77b927184b8ca0d1b14c1b967956b87f59c67d9ef84fedfe4cdafb73615e242de37b656fe3dcdebc34c6eceeedc5)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86_64      SHA512=2be8d010913d81a902cca40721398647fe8fea5011213e4f0eebaa1f06abf40b2defe72e47a6ed8cc145b1a59f870cb967b9a084cf2fad65f90f7d3a6c369e3c)
elseif(ANDROID)
    set(safe_iop_root "${MBP_PREBUILTS_BINARY_DIR}/safe-iop/${SAFE_IOP_VER}/${ANDROID_ABI}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${safe_iop_root}")

    set(SAFE_IOP_INCLUDE_DIR "${safe_iop_root}/include"           CACHE INTERNAL "")
    set(SAFE_IOP_LIBRARY     "${safe_iop_root}/lib/libsafe_iop.a" CACHE INTERNAL "")
endif()

################################################################################
# AOSP system/core libraries
################################################################################

set(ANDROID_SYSTEM_CORE_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} armeabi-v7a SHA512=71997359175ccd1b6885c0ee1e4fda00f21e6c7185fc1941c5fa5ecb018fd0fbc329063d01672af2868342ce689874c87aebba1c9fedd4b7fdd09d634db58d79)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} arm64-v8a   SHA512=3a1acd557e3d6bb111b565d43b19e917488b6ab733056112ab8d9bf56d7dd4dbb7e08e59493d207ff008ddcaeff841f1b8ec82568c0093523b0934d7a2633bdd)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86         SHA512=9f44f98e7ea48980b6a4e44a471c9f73581816bb8120923c7caac5753a42f91c8bf255a6a8b98c4b19c7ed93f2c8d5c4cb81048bfec3a5175b8452152e0a50e3)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86_64      SHA512=c0ee56e5a5bc05f7bfeab2ca794631dae50b6289e1987ace4041c81afc18c328213955f2dee34434df7a34f33163b8adf8b3f6ef987a2caeef7a729ffef025a3)
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
