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

set(EXFAT_VER "1.2.8-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=1881c13e25674f0bbd4237c7984c11350a8c7fca15ea103227b8f526892fa1d6b3cbd57fdf2a033627c7a2f7d41231f29396ffbb88d02b3e4278b213dd832891)
    get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=6f449e102e3c073b6ec2cb99ccf4b3cfbc931b9b53008e1cf3f0ca366093f26d999463c5c2e19b49638679c78ae926f8c0dd834aa243fffdc9d724f39efc55ce)
    get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=bc83d33920b7ac4728b0e5ad0bd3e002a493a7a3469e348fe552deb095e29d50c65eb4894b0a281346526e84c2bb4577ab1f1dfca9f7bdd4ddd62bea3298a80b)
    get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=f612a063267811e5d614dbc83910adf0ace9aa872b309f49177b92f1c217c48f0f069ab79465fa6d25dae3b451e99f460ee166a64e961f1b51406ed677477180)

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

set(LIBARCHIVE_VER "3.3.2-4")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=24a952d7986ad68ab3d1d702a84681477bbe1c2a5de011be99b5dcf7035677f7fb8006fa1226bdedb822a774188d7899aa18f8bfff61bca211899e1ec5921a50)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=44c0d9c08d791fddfb24b751695b1f5c88406dc5b3a179f1b4049e0eb42ba6175e53e8b6d6a457b465232ffc415982dcbbb5eed246ba20ab581cdf26e53dfd7d)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=1af895fad7bace24c9ffbca5e39aeb101c2b949271679f25f0ccabdd184d569ace38640e7053c3db825561077460c422fc257a08e6d5f3e53cafd86e4deed79f)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=d675a6bc0efe915e59c895ffb2eab2e643fa524f5b18971e95bb3ffa18bd7693cd0bde758c91b75816f236544057220600e0233659a3aa83d55c3831dc250e34)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=d0d4684140c01b09fb1e8563028231c3f29b921fee1dff5db8fced2d7ebfe4951badeb02d08e6280bd77b0d6a3ea6a57bfe112964d1890c025510895f5478926)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=8b23c600573b3a7bd8c2f5c08cacf95d3d14fa18d48f9652fa15bd76f7284f5dbd8fa94c5c8150c6242e74d4cc493bbfc59950033e9f22c5d3c2f1c533f7d08d)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=538da58a4e1acb6cbb912054b8c6d48af04a171ecde80055d7227ae29d017de82b521ddbb9f067bf3cc56dc731405dfaf2e73bdadfbb92b82d923446f8369e97)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=1e269d1cf73b8398b381df673a76c89b23d75f7344bad708913ec98d48312742ca9b368380eaf58f07e456dd3d783876fee30f3bd6a88232b7650e5da287bfe5)
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

set(LZ4_VER "1.8.2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lz4 ${LZ4_VER} armeabi-v7a SHA512=2ad7ffc1a1c94c7d304722bb2e5f1c9b668986a849193e36bec8ba2f58ec35fe6c8b750287fa4404284a7099d15578df879bd5b84f78524ac6b5023fe1a78900)
    get_prebuilt(lz4 ${LZ4_VER} arm64-v8a   SHA512=38e156af25ff155dd7c02318bc47b2290c17fe37e635bf68fd8be2d592c3d53ccf9c79d71e0b322e8c0afb9f92e173da8ae4c2534add53c67d9df1e0b9fd4bc4)
    get_prebuilt(lz4 ${LZ4_VER} x86         SHA512=7d43d6e50eaf3840de2252bdc65134ce3b5022104f01087920271ca6e27881e5e91f6404afea2f364bb8f8ced2b62cf11df7f36761534a2673ff70a1b7e59554)
    get_prebuilt(lz4 ${LZ4_VER} x86_64      SHA512=aaf77816d72c50553e1f7092e545e4fc7b8d133f847b8d57eaf3c50a2283c473904f64fc79c0de7225c29c21d77a5040e11b00b1a0645b35c3a4a9f1e1d43810)
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

set(PROCPS_NG_VER "3.3.15-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} armeabi-v7a SHA512=5b19abafb5a806bdd82666289a0315126689d5c8a6ef44b84a68ca652a55d34ec7a4e8e0628fed6f9a6f84ec25d9fcf2749bbbcffc7cd02d790cfed99ac28030)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} arm64-v8a   SHA512=50f3af6e4de8dc63ed079f491a5706604cfc16eea09283628b69a578a081e4e6415d42cffb7c899a47f998d5c113ad30972d438554815d5f3afd98cdc118861e)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} x86         SHA512=825c3a17b8844d311c3753d173856fe389a56ffb9e37d2771e86819261e4d36d1699a5b846c7a31f5202344e9359831896d134114c19d4669b30559014586fca)
    get_prebuilt(procps-ng ${PROCPS_NG_VER} x86_64      SHA512=629a3556096751ca6bd30aa336acd7459d4806b3df5fa8bf4daf8d1858aa4d13b91a2e86db1d15be3ff1624e16da2e4b397ae6a7d6b7ea8d7f3fbc726db9ccc9)
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
