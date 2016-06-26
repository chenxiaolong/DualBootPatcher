# All of the files here are built from the PKGBUILD files in thirdparty/*/PKGBUILD.
# All of the files are signed with my GPG key and correct hashes are provided
# at https://snapshots.noobdev.io/repo/prebuilts, but if you don't trust me or
# my binaries, you can use your own builds builds by copying your own prebuilts
# to thirdparty/prebuilts and updating the checksums here. It is not necessary
# to upload the files to a server as CMake will not download the files if the
# checksums match.

set(URL_BASE "https://snapshots.noobdev.io/repo/prebuilts")

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
# fuse-exfat for Android
################################################################################

set(EXFAT_VER "1.2.3-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=a7b9a11ef770d362866e779d2ade7f6118b9318227f175e0b6fb1d22540e0680c0c89b2e7b00b064df6fdb809e06ee5743b946f9cc83e5bebafc0e8e666a7071)
    get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=b4a66a9da003151eb4d71baa73ebfbed801ed31352c268e28d421cb21c6603b9277253bfcb233c999bdd54d499c358bd798040782fa5b70c8df5f5868fcebcb4)
    get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=8007dc82e7532442df63d0fa7c82bd3c6da282ff2aee844d988ab9f8cf1268c9e71010dc2c0cb7ccb3b79ea7206ad18b343be8b4f0e253d70d48dddb0874952e)
    get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=27796496bcba29563fd61e580fd1e63bbe22fe805d074bdab6a812e9991b9e19681ab33e2c5d7ed67588e786f8084bd6cbe772d5362371803d3b1bbcfbc8b337)

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

set(LIBARCHIVE_VER "3.1.2.r599.g567b374-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=41dd2ad89235487a0340ac165fe64dda24e7527f6b8dab4174668f58bf1d67c2acaf242ec2f4049d3b6ca2cc373cf282fbb08846b259cfa31c749bb5125f1fbf)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=b4275049a0fe681ec32343fea136a368663484ee5b89312fc1d85a9f38b527dcf09bc935413a061cfc5fb1451c81f4ea5a98f1a03d0fe870c9b35fd39c134050)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=36bae8d2af41e97532d1b20b7d50f5fa38c1cf18a5cd309a91f5fd33346efda3fd4dc63185a8e6fe4039c7c7a986826f356e5a9eb1879cd7e551805c59e2337a)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=e00f00d660de88e30d6d918e5c46f6a2816ac2eb7da51c8e55f4603df6fef7628f985c7691c487656d9cc9c3b69504faa917b4c4171c65407418c39492fbffdd)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=c2c090cde24b797425a6d7a44a86cfa08ebe9a6be537bbd9343cc3d33a42f36e39c173d53c54cd6591f2c08d870676513a386e29f31f5aa556d69dfd81da0118)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=34305c516c757e88813418842e37afec9fd41d7a6130be756fef23f369642a24d31b7258a96fe3d31d73791d86a8b9825ce4f8db30446fc5360393cf542e99a9)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=983231de624ab6e207fe11d78ac257ec01622ab75cb96480811646d562d5fdc04446beac743a3e71309b5ce4bc8bd2bc980808f13ac2b284e6483ea635416bf8)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=a4d240f7aec7aef643b294e5e90f5218fe11f7fde028aefd257da234b0e563a939ff8b578616dafb9ee0840836cc2a3bd47657b855bff2f4dae6c806173d27b4)
else()
    set(THIRD_PARTY_LIBARCHIVE_DIR "${MBP_PREBUILTS_BINARY_DIR}/libarchive/${LIBARCHIVE_VER}" PARENT_SCOPE)
    set(THIRD_PARTY_LIBARCHIVE_LOWAPI_DIR "${MBP_PREBUILTS_BINARY_DIR}/libarchive-lowapi/${LIBARCHIVE_VER}" PARENT_SCOPE)
endif()

################################################################################
# libiconv for Android
################################################################################

set(LIBICONV_VER "1.14-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libiconv ${LIBICONV_VER} armeabi-v7a SHA512=d4bc7e39a7205ff166a1aa07b93ceb4fd7b6f7f2be26dc8917dd5753034470a78e9a18139613461a4e3b565b43eeb479c735566f058966f71dae19cf16d5a208)
    get_prebuilt(libiconv ${LIBICONV_VER} arm64-v8a   SHA512=b2a03adda6bfcb18bd14422b1e950f383bf030b77680e06114220666034bfb074d26a0d9c2067df3bc8b1b64cdbb9fa5b6c4cc83d0cfa11e66121352809d6756)
    get_prebuilt(libiconv ${LIBICONV_VER} x86         SHA512=827d284e74787091f1403f8bb9a61ee2e0ab4930eb1da24a0ad564caadf413ac7e2d3501489b2c35cca2a0978fa347e0cbf7bf76b42a31098d31fffba80a9ca9)
    get_prebuilt(libiconv ${LIBICONV_VER} x86_64      SHA512=004b629ce9ffa43cd88c23d87b927c6d80dcf4de8b750cd40532e00100e1e13387c0982f2af3c3a3593ed32b83aba66b7a7849bc08d1f210e556a192c2948615)
else()
    set(THIRD_PARTY_LIBICONV_DIR "${MBP_PREBUILTS_BINARY_DIR}/libiconv/${LIBICONV_VER}" PARENT_SCOPE)
endif()

###############################################################################
# liblzma for Android
################################################################################

set(LIBLZMA_VER "5.2.2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(liblzma ${LIBLZMA_VER} armeabi-v7a SHA512=0bec5f8bc3e1484b830e2c57b041e916687ea4035769074e5576f6682a531d285538fbf7a5a85c88c654209d8e9e8b14ad80e78a3d9c29aa5d304eecf247edc8)
    get_prebuilt(liblzma ${LIBLZMA_VER} arm64-v8a   SHA512=6b66601dba6c419f7462a331df7dc71cc13356168a0c6a9c0cb9eb38d46af5aae6f7128838c3db80167b8b318200c6f6ec5b37b0f1ef23a668ba1512a1325417)
    get_prebuilt(liblzma ${LIBLZMA_VER} x86         SHA512=2a4008a8f89a0bce25a7c5c293733cafdee54effda563c6a92f75cb607d65abc47e9c7e900ed2f026991af80a2120faeebf44ad8993521993f24cf9f44bd839b)
    get_prebuilt(liblzma ${LIBLZMA_VER} x86_64      SHA512=c2f5607f913dd4b76abeef6536e108f15dc5daec779f85f3d22588d76950cf7c188e7cda465d0007e5720050667d37a1a4e08e5f0766ee81db26785b9035f52f)
else()
    set(THIRD_PARTY_LIBLZMA_DIR "${MBP_PREBUILTS_BINARY_DIR}/liblzma/${LIBLZMA_VER}" PARENT_SCOPE)
endif()

################################################################################
# lzo for Android
################################################################################

set(LZO_VER "2.09-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lzo ${LZO_VER} armeabi-v7a SHA512=baa6537ca4ba80c508f39bb64e3404c5ccb71c31c67b28501bbc4a24272b558a0f59fbb79573c6a261bffebfb1aeff174f383b75626752393c5093959b0c8415)
    get_prebuilt(lzo ${LZO_VER} arm64-v8a   SHA512=83a44742fbe960d94f46b03ed3eb3786985db34608e151a9a00310c4a884e9ede553322175ad22806bf364f3ec8665565a38a6bfe83cb01a67eb0e777eb7c09f)
    get_prebuilt(lzo ${LZO_VER} x86         SHA512=2bef29b8413410c6426a9d4ad103f0040e6e1d6b018b8ea7aedf730e79624e12d2da8ab68486c1eb1de23db0b96d8388213128c240a53e4fda4a1fec20650c70)
    get_prebuilt(lzo ${LZO_VER} x86_64      SHA512=d04a86dd68f190115f220019c0e2f315f78b7c098246052137a2b3fe94759c0015010ba4242f19002fc919ee3687946ae15e77d83e9d516feb110b0dbc6cfb60)
else()
    set(THIRD_PARTY_LZO_DIR "${MBP_PREBUILTS_BINARY_DIR}/lzo/${LZO_VER}" PARENT_SCOPE)
endif()

################################################################################
# lz4 for Android
################################################################################

set(LZ4_VER "131-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lz4 ${LZ4_VER} armeabi-v7a SHA512=e3806e50e89527de9b4061a35ac6ef4be372ec4321a53a19a343ad5c5a29059582d0cdc48baffbd2dfd39332f116b1ea8d32d4662174ad0fdacebaac133edb58)
    get_prebuilt(lz4 ${LZ4_VER} arm64-v8a   SHA512=262925a98d27ca0afd5dc579b7eca522ae6d15bed80294f34a6ed9b58ce51e321f755260a5f4723af26addf4856e52a8fb490db82b7a1e309dca50e6656bc5fb)
    get_prebuilt(lz4 ${LZ4_VER} x86         SHA512=4047ddb609ff1b228c8075c70347fab91119e3d5324cfe03eb616ebb6238f3ac9b77b6fdfad79eedb3f0faf81fb1ddd0a7dfd1e0d9012059fa2bed025ac8e5a1)
    get_prebuilt(lz4 ${LZ4_VER} x86_64      SHA512=51129cf86be6c5f0b50262be15f3113fc08123df00e12d3474d5fb38c95ea5d0b6ddf5c83ca752d076ddc56920901174bbf2e9a29a3b8072eac3d60259f51334)
else()
    set(THIRD_PARTY_LZ4_DIR "${MBP_PREBUILTS_BINARY_DIR}/lz4/${LZ4_VER}" PARENT_SCOPE)
endif()

################################################################################
# jansson for Android
################################################################################

set(JANSSON_VER "2.7-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(jansson ${JANSSON_VER} armeabi-v7a SHA512=c7812e9b7f61a24857c8fc03d1e1b59bc0a5a4c85ca93552378844aa874aefeebec25b6a108143a778970ec9097ca7832f6181ad0f07bc593cf4ecc8de4f38c7)
    get_prebuilt(jansson ${JANSSON_VER} arm64-v8a   SHA512=8ae200c9028373b974da39afc0569ddd633bc273cb0b3c138de29d0152678c36d754c5ffdba94552e3165640f4c0bb80cb5255fc1744e194cac4fb864206db22)
    get_prebuilt(jansson ${JANSSON_VER} x86         SHA512=00739bbd7d27045fda4a51118c1559fd279cc3a16a425d1f91cfa898a5c5d63bb246fde3fb9e0d1eb57bea547c512cdc75be94093166940176348aecbe249ef9)
    get_prebuilt(jansson ${JANSSON_VER} x86_64      SHA512=9a6759dc9444026c9b4370f61ea7b46f0f4c35dcb2b9aa16666241b0743ee44f0829fc57be25528ca14a08ce596b13d9beaf28cf3108f9a84b5296106bf1d407)
else()
    set(THIRD_PARTY_JANSSON_DIR "${MBP_PREBUILTS_BINARY_DIR}/jansson/${JANSSON_VER}" PARENT_SCOPE)
endif()

################################################################################
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "2.5.r1468.e93899c-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libsepol ${LIBSEPOL_VER} armeabi-v7a SHA512=9ecba7c8ed990fed8b3a9753673ada21f233f224c4df6d2873b21f4298e2a86f4f571727ae01e32d3999aefd66b4f7762642f1320087c2bed5954cbf5e107ba2)
    get_prebuilt(libsepol ${LIBSEPOL_VER} arm64-v8a   SHA512=fd0f661818c704d8f0d28eef9190775e0604c65a64f2d0bbccdac918472393c65d14760437614eb23af174b68ec549343f04ee9d63a531cb28d537887363d0b1)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86         SHA512=f987eb915cc5e2917199a94908fbb1cb87cf9eed84af2db8e15afddcd0dd2c1a495f8605f6129c48f5818f10bebbf5320090c1d482d39c1c74a09b43b96342cb)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86_64      SHA512=ff064e46281ce1458e174525bedfe610cd2ffe9e3b4c581f5201d48ce6ee57337e1b50937d94c9e156d985f15e79abba3bde92e4511372c1fca51233a63fa242)
else()
    set(THIRD_PARTY_LIBSEPOL_DIR "${MBP_PREBUILTS_BINARY_DIR}/libsepol/${LIBSEPOL_VER}" PARENT_SCOPE)
endif()

################################################################################
# libpng for Android
################################################################################

set(LIBPNG_VER "6.0.1_r43-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libpng ${LIBPNG_VER} armeabi-v7a SHA512=c5baec277537952b9458525463a00f51d9e18684053e225632b8f07ea98030e38909d1c29a8a56df58a7f746d8fc2fb412bbc234d2eaa4c33392e6733477c4e0)
    get_prebuilt(libpng ${LIBPNG_VER} arm64-v8a   SHA512=42beba8b70cfe30e3a9222b5acfd9d6519a51d1032245d8fd98605f8bff5814e12d4dd8cfceb8eaaa99b53540f1b907c86accf799b254a12359b7287d9ff3b94)
    get_prebuilt(libpng ${LIBPNG_VER} x86         SHA512=0ac4f9f87897d9678c84e5961957af3b66068eaaf0c986ef58a3ccdfa7a1eb706080039ab2d3d98cd4720da9ec148c03f83e88240e32a0b003bfed479f040ddb)
    get_prebuilt(libpng ${LIBPNG_VER} x86_64      SHA512=0c42204d4d3834108b4820ec4a57d48f237373c9ca8bf67a79ad477994af147e01ff851f0855bae2138f7d8a60493bda42daaa6ca35c607a854c1c01c1a80d38)
else()
    set(THIRD_PARTY_LIBPNG_DIR "${MBP_PREBUILTS_BINARY_DIR}/libpng/${LIBPNG_VER}" PARENT_SCOPE)
endif()

################################################################################
# freetype2 for Android
################################################################################

set(FREETYPE2_VER "6.0.1_r43-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(freetype2 ${FREETYPE2_VER} armeabi-v7a SHA512=13b3eeaeb463d1d831164b3ca095d8ff37f3773227fcc0c0338cc64b7481235227a7a76af0529acdcf36e024c1d6667dc0ff94f00eb1e358162d5ad75b17dff0)
    get_prebuilt(freetype2 ${FREETYPE2_VER} arm64-v8a   SHA512=b93c90348161582fef6745d9ace50b233b96260801814d51a04563b9dc2ba562400704a5aac04c216913002d97bb0fe8e93f006ab11bf68bab90f68eadd37812)
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86         SHA512=249ffc06e171e684d7ee29c1cfc4eee26df342eae3375b913d602761645e9c460f5be9feb39ac642c877a87b9ab6abfb0997752e6d838cba413e9a49e06b8c2e)
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86_64      SHA512=33822c1a426d7858f0ab806645f1d82cd74214a39be46ff98e45d9ac23d70fbc163de25f5cedd374d0d614f7e6f492b56edbbd4a843bf3ade697f7fae097325d)
else()
    set(THIRD_PARTY_FREETYPE2_DIR "${MBP_PREBUILTS_BINARY_DIR}/freetype2/${FREETYPE2_VER}" PARENT_SCOPE)
endif()

################################################################################
# libdrm for Android
################################################################################

set(LIBDRM_VER "6.0.1_r43-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libdrm ${LIBDRM_VER} armeabi-v7a SHA512=c1aa21aca556d6185477df50ab0659cadad613d033f37153f4c5a361410befc1c3a5d8683465ed6b66a56f9d30b40f08ab2b899c837f2723d93f20145c5a29c7)
    get_prebuilt(libdrm ${LIBDRM_VER} arm64-v8a   SHA512=3d8b2048bb09dd2d40e23683d0c2a2585ec0d3e0f25ec220171fc8131507d9d3c63c441412d7e537b2a1f81cbd8db38486f9561cb59912506018af3bed1a86e0)
    get_prebuilt(libdrm ${LIBDRM_VER} x86         SHA512=dfb3ccb53acc9923cddb90b400cb7f2d85318469f84c54c375c6a9cada4c070da8caaaad3617c27b15de240652265ec269d2148c686fe11b281f5d9bd076c0ee)
    get_prebuilt(libdrm ${LIBDRM_VER} x86_64      SHA512=9a2e7181662283a2d32f7f048dd5b2b91c260bff38e505ef04236866a19ea89246cbca6ff010a94f6054c32fea64720e5da1d14ab6ae5f0c7e75f6d72cd90b7d)
else()
    set(THIRD_PARTY_LIBDRM_DIR "${MBP_PREBUILTS_BINARY_DIR}/libdrm/${LIBDRM_VER}" PARENT_SCOPE)
endif()

################################################################################
# safe-iop for Android
################################################################################

set(SAFE_IOP_VER "6.0.1_r43-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} armeabi-v7a SHA512=cf4705a7928f4ae70e92d94f47bbc7d01bb9a9eef707cfade62c6d7198be559325bcea528bcc67b6e93654e69e8e76c015981df55d30a712fa0ec8f4de4edd9b)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} arm64-v8a   SHA512=b6d2bc283a359f7b97ff34eb8c3d1b74081ec7e8f51abf0f56eb98af0507da9ac73ae83e078eca7ed1935b0ebafd27b02914c4dd68730f7b72f475dd4050c689)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86         SHA512=5a75f2ac7f816fee629c697f0bca6a1b14e6b306b1a45260afff6740dc3b96370d2894a4eb90811880536b015b49c1affa5d349b1afa3cc00a870b3d7e0b5711)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86_64      SHA512=f46430623bb0146ff85c30cb696a29348a0330e5434b5486626ca6504c1a99cb970087725dcd04f6c164129c0772c180e6aa81abef043b077725d93f2d46ed61)
else()
    set(THIRD_PARTY_SAFE_IOP_DIR "${MBP_PREBUILTS_BINARY_DIR}/safe-iop/${SAFE_IOP_VER}" PARENT_SCOPE)
endif()

################################################################################
# AOSP system/core libraries
################################################################################

set(ANDROID_SYSTEM_CORE_VER "6.0.1_r43-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} armeabi-v7a SHA512=3c811dadad95f5a9470820dbfe824e3836812bdc75b4f65d5cd992e142a109d6701b21ef3be2fd00a9a6b6dd8216014e0d311c0c8c35747b074d73d8738c55c7)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} arm64-v8a   SHA512=eabef3ace7ec3841afecf7062790ae999d3d30e9720d271e3d194de93b457874d587de4d13d5f526134e0167decd362b98be727fd71b71d3e1438e1b6aa63f17)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86         SHA512=de6ce6fd560d82551841cd5102cb657cc758e767fa6f796b9141cbcab1bd13158c2afe147e81d18d2bfb1decc9977e79442a1e2f2f455ef8dc9fdbf45c476f1b)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86_64      SHA512=96d14d34c579517e2604e4defd1e8246a3c171635667465fd6780207b8f4f7da1744b105d8357636e4ecde4977b530d76a22402d005037e38d7fd37a13664332)
else()
    set(THIRD_PARTY_ANDROID_SYSTEM_CORE_DIR "${MBP_PREBUILTS_BINARY_DIR}/android-system-core/${ANDROID_SYSTEM_CORE_VER}" PARENT_SCOPE)
endif()

################################################################################
# boringssl for Android
################################################################################

set(BORINGSSL_VER "r2486.a2d4c0c-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(boringssl ${BORINGSSL_VER} armeabi-v7a SHA512=3e765d4153718eb332a78af2f933c8fc77740e7c7f029221132eaf52bb1743c55e98df69a263d0374685ee8e1d804b7b2af211cfbbe76c1e0400e75e64786c88)
    get_prebuilt(boringssl ${BORINGSSL_VER} arm64-v8a   SHA512=5fc9e4834ccbd869b56187477e553fa318d9e9489eae5b759cd8141075209df5f06919491012b6766a4e7427db107ab4437e6557656fbb215ff67173fae4cd88)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86         SHA512=72695bed3ed19f779f6dda21bcd604a5e24861e7bda103b2db1d7d807081ff25a201ee3fbfcfec40c006d8a72967ee4e3c7af88a31be8eef857b251c1c8f24df)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86_64      SHA512=646d1c9a9ca358ddb6d1382aa09a35036a31b6825b66964630e5b71f242aa5447585c55192b2e8d76ea5b15e58067b11739506c0d0d4695955338c1ec3424a4d)
else()
    set(THIRD_PARTY_BORINGSSL_DIR "${MBP_PREBUILTS_BINARY_DIR}/boringssl/${BORINGSSL_VER}" PARENT_SCOPE)
endif()

################################################################################
# FUSE for Android
################################################################################

set(FUSE_VER "cm_13.0_r990.42f00b6-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(fuse ${FUSE_VER} armeabi-v7a SHA512=b72ab9a3a553921fb97eba8393fd608ae142f065e752bfe072b4050466b4ae9a52392fbf32890dd56db6743931360fbbd9efed2f0e37580c4c881fa482cd704d)
    get_prebuilt(fuse ${FUSE_VER} arm64-v8a   SHA512=50fb86ed55a258e038dcd6ef2791a9d7570aecc8e49997bbb4c3a67b955cc2473e5800adffa0761405cafd8de6ceb4e758b813a9db88be161d39dd9b162cf6ad)
    get_prebuilt(fuse ${FUSE_VER} x86         SHA512=7506a27e44e510dc2f868b91c9494609fec91f1cf834390b66aa58ae9257beb898f65f9c1b37b886f948d80d65feb0b153197f22bced5dbae199ba00020c7d6e)
    get_prebuilt(fuse ${FUSE_VER} x86_64      SHA512=66912456e71687c8c37d3c193ac6a075248f2245628bc553e94edd1f39b655a7da6b8c6e284c9dee0fcc7a959f3c19ecf38bc42f76713f50d29498e81c15c0c7)
else()
    set(THIRD_PARTY_FUSE_DIR "${MBP_PREBUILTS_BINARY_DIR}/fuse/${FUSE_VER}" PARENT_SCOPE)
endif()

################################################################################
# procps-ng for Android
################################################################################

# TODO: TO BE REMOVED SOON!

set(PROCPS_NG_VER "3.3.10")

if(MBP_TOP_LEVEL_BUILD)
    file(
        DOWNLOAD
        https://snapshots.noobdev.io/misc/prebuilts/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
        ${MBP_PREBUILTS_DIR}/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
        EXPECTED_HASH MD5=38489848300b3ac68297142ae943cfb5
        EXPECTED_HASH SHA512=3eb49e02b0372669f4683aa4003f97a285ba2106ed2e847c022f9db2c5a92b8be3611f1409d5fcb75c0789b03df8faeeb453e3625d26c28f2c4c842daedc1e77
        TLS_VERIFY ON
        SHOW_PROGRESS
    )

    if(NOT EXISTS ${MBP_PREBUILTS_BINARY_DIR}/procps-ng)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory ${MBP_PREBUILTS_BINARY_DIR}/procps-ng
        )

        message(STATUS "Extracting procps-ng for Android ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${MBP_PREBUILTS_DIR}/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
            WORKING_DIRECTORY ${MBP_PREBUILTS_BINARY_DIR}/procps-ng
        )
    endif()
else()
    set(THIRD_PARTY_PROCPS_NG_DIR "${MBP_PREBUILTS_BINARY_DIR}/procps-ng" PARENT_SCOPE)
endif()

################################################################################
# jna for Android
################################################################################

# PARENT_SCOPE because we need this in Android_GUI/build.gradle.in
set(JNA_VER "4.2.1-1")
set(JNA_VER "4.2.1-1" PARENT_SCOPE)

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(jna ${JNA_VER} all SHA512=28fb507df36870da6592513a64437c80ba5c582d563e95f108f3033973622e0f326fd4a146fa20934b60ecc82130ae99da4777f1555b5c9d8ddd48f32033e911)
endif()

################################################################################
# e2fsprogs for Android
################################################################################

set(E2FSPROGS_VER "5.1.1_r9-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(e2fsprogs ${E2FSPROGS_VER} armeabi-v7a SHA512=610b2e98b8106217e5c36f092fe6226f631929f12de536800d9b505345783e4e421e0fbd9d4fd71cfbddf542fbe0cd3fb5853b315b240bdc2c6c36316d744571)
    get_prebuilt(e2fsprogs ${E2FSPROGS_VER} arm64-v8a   SHA512=55422570a954fae83265c6c2204334b78069b85b8544a2d1ca365ea517372c1a45d8b13da2492df5832e89be6769ee2294c0d265251c82399c65fae101adeb75)
    get_prebuilt(e2fsprogs ${E2FSPROGS_VER} x86         SHA512=05741d24206fd457a40d609239cbf45f04aa4ec1116bd2cee497407d5275a3262aadf6e643c3128b81b775a9270cfda76f1073a2cba3a3a54b6fad3d83164345)
    get_prebuilt(e2fsprogs ${E2FSPROGS_VER} x86_64      SHA512=5523d9b3ece096decc481193a6633df6febe987909374cc36accde9bbb9afb64d38217dd9ae68d1147959f5c19746c8b70c8b8553683b958200a4357743a0086)
endif()

################################################################################
