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

set(EXFAT_VER "1.2.6-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=fae530acf2a405291282d7d070a50e7b6788fa09d0e0fde0fcb700c6c1a943099751343104023bcb2f97322f0b61bcbed4ae147eec01401b8a9149a0967998a8)
    get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=307ac5e3a5c48d8caa29efd050365aad7615e889235dd11b8530e119a03e7562ce5f43e639bdeffd6b0222030c3448ecccf3fd03b4323e214b7212509430e842)
    get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=deb7d0d6571a0d93f5cb13ab7a7d66b61f52306153a431fc8a73553421ff4918cb582007fbaec7a397a63c38894f3c1f9854e5ceae7cc2b43e0b0a96ae83bf11)
    get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=a61ef02a2a2df3db573dd7f4fca8acad700686337c1866de968a85fd7d686a760eff1c471899670bd655be89059de7a11f3573a70758275afedd147b4bd6e470)

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

set(LIBARCHIVE_VER "3.3.1-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=a9a4f56aed0c29b6a7f99f65e4b810bd5a3439ae04497d81c7197be1d4d8b69edccca37a0a5b5b615927d7512140b1adf75416b2f208d7e3089a54045038eaab)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=44d0c3f68f66e4e3456e58e2f3f62c6752e7e49b9044ad1a0d5e17b136d1722bdcd9966e59c4c75d3bf64b6e8ebc14e9a95fa2c5450fefebf243eee8bf4260b6)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=b7d06cac3a9bf85c82110ef75c115135b1fd4d2ff80cd9dab509454e40d82749361f30452ef1ce46575f4a7e76f0aae7e24b8f5395fffe6334fe5ce6bf2440f2)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=1024ee95c35d57f33e610befcafdc54816d6dcd221f29b4b3364f2437846c4d410fa5c87df0f539e97d373c9e1445ed3a197e9b33a223003b56757518346da89)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=e92409e20e1932a97e18c869e55ab31d0e6728446f2feefd401f1dfac3ecd4182ce036d3d23f7a08f26cc764576be5694eb793fd09b637067715e4c96edc7d1f)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=1ddad68462f8bdd744433c6c9a7c5c3aea274fb79bb946d6eec63a71eed3f9759250b05484b04d98120bd431a81050e7b72278b2c2c534d8f8d787497104f92b)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=444a2c15dce5c8d3ed308114a601df91ec065423d2f387eb52b9d3d5240e040d6dfe8c5a2518e7c80e604cc72a078142723f4666b2f36cca1b4497bbe9c3bd01)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=28dd5f06330ae38c24f5e1e2ca022956641a7972d80c38b05cdd8bc591ad530d67ef1a0fdf647a353310d07402839d1e008d676f8a7cb0c3a90ac264af6593b5)
else()
    set(THIRD_PARTY_LIBARCHIVE_DIR "${MBP_PREBUILTS_BINARY_DIR}/libarchive/${LIBARCHIVE_VER}" PARENT_SCOPE)
    set(THIRD_PARTY_LIBARCHIVE_LOWAPI_DIR "${MBP_PREBUILTS_BINARY_DIR}/libarchive-lowapi/${LIBARCHIVE_VER}" PARENT_SCOPE)
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
else()
    set(THIRD_PARTY_LIBICONV_DIR "${MBP_PREBUILTS_BINARY_DIR}/libiconv/${LIBICONV_VER}" PARENT_SCOPE)
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
else()
    set(THIRD_PARTY_LIBLZMA_DIR "${MBP_PREBUILTS_BINARY_DIR}/liblzma/${LIBLZMA_VER}" PARENT_SCOPE)
endif()

################################################################################
# lzo for Android
################################################################################

set(LZO_VER "2.10-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(lzo ${LZO_VER} armeabi-v7a SHA512=90c6725a711d066a7b90e7631a2b34ac502cf1b1f9680826e094aa867d17c6b373c99872425ca3adbd3c87c35c872781353edbd085642c4a1de87da1c3cde72f)
    get_prebuilt(lzo ${LZO_VER} arm64-v8a   SHA512=ab8616290db4abd5e7c68d82d3f9067143cd0a508cd5b991727ce9699387a4b708bff2dc10c1bf46e99dc9a39d18597855a0b9919f60216cb1ae0f7e0acf0a7d)
    get_prebuilt(lzo ${LZO_VER} x86         SHA512=1f4c7859c90e4eab321a85e016a85dc1e290cb2bc4e4740895c5eca750670b8c62dc1119943f4306394a7580255a62bed855b9c3c882affc826e74df88f196c5)
    get_prebuilt(lzo ${LZO_VER} x86_64      SHA512=4502188785e66e34e8560c2d273eb22e243d8d6e8defeffcc77bf67f7ee88eb73365067a2ec83b3d571edfe1c3c2336c051fbc4f0761bb9143375d34c3ebb9c6)
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

set(JANSSON_VER "2.10-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(jansson ${JANSSON_VER} armeabi-v7a SHA512=06408ef2a73268b1955d9613e7779399cbf5cf15dd29461ad9c81728c835ac017a86154e199be020dea65087f91839693a379c40a6259e98f0b520e05c17d8f2)
    get_prebuilt(jansson ${JANSSON_VER} arm64-v8a   SHA512=994c03b6d4bf4fe5476e26a4c02ae41183f80c8fcd1c04b99457c4ff9a922f07c49edab70b4095128276edadc7aa6dddde3f4ca9b6ef49a05cfe1adad01a1876)
    get_prebuilt(jansson ${JANSSON_VER} x86         SHA512=72c99186f325232c2351362c3331049e254a7a7151b7e2692d667dfb0c822b934d6a6c32cc9de397bf6d35092aa49fdaeb69de639afc8c29d8cda523d69672e0)
    get_prebuilt(jansson ${JANSSON_VER} x86_64      SHA512=47c87efbc55fa0bfcd03e1593e7bc026b53d4b2d4020c1bef515c8776b0c84b93160b874978ca64267beb7828815a1467f8995e54984eda4ec44dc0cf7a2cf42)
else()
    set(THIRD_PARTY_JANSSON_DIR "${MBP_PREBUILTS_BINARY_DIR}/jansson/${JANSSON_VER}" PARENT_SCOPE)
endif()

################################################################################
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "7.1.1_r28-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libsepol ${LIBSEPOL_VER} armeabi-v7a SHA512=2e18d72ca987552355fe97238692e271402c44e893cd0fdae5bb4730dec608c6b34639d5080f2dccbf4cf8abe6f329a9139679b75c4fd1905ad92fc079d262ff)
    get_prebuilt(libsepol ${LIBSEPOL_VER} arm64-v8a   SHA512=dac3da7bff81f6bb363bd93875715860f78e984a66ead4a6610de92f128a8815d603484dc099800d61a2b261e30bd6962a7f6e36075804bd0a981b1b98d74eda)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86         SHA512=d846ca9a48c1b4880aa61598c2dd62bebe9497e8c24720b8e5fb11be4770ec90d3f01a7e60b1d6e19c5b27e89540449f6eb1402b6bd1fdb291d1f03e18d0e307)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86_64      SHA512=3c6f16f22b3d63ca3818809c9c03f9dc13ab6516194d716bb30bd07d38ba0c47d840d02b4dab34cac6d92d5bd50e7c41c70f7c8fb2981875c65f7a46ee2cde9d)
else()
    set(THIRD_PARTY_LIBSEPOL_DIR "${MBP_PREBUILTS_BINARY_DIR}/libsepol/${LIBSEPOL_VER}" PARENT_SCOPE)
endif()

################################################################################
# libpng for Android
################################################################################

set(LIBPNG_VER "7.1.1_r28-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libpng ${LIBPNG_VER} armeabi-v7a SHA512=793d430c17a4bc7db9f57e7c12032c64c648d266d05f7f519f3aa63ca33178e81e012c1c4be4be7dde6baaad61fbe641e48547a9b88643098d22d056607a2dd3)
    get_prebuilt(libpng ${LIBPNG_VER} arm64-v8a   SHA512=b61ec5cd83644bdc0ae1f5562fdb590bbd64cbd4175296bd3ddbe7d11d269031a91cb77c24cc240dd0f9318116c6d9c299796cbaee744585e521d24f5f54c09b)
    get_prebuilt(libpng ${LIBPNG_VER} x86         SHA512=12001cdf6684bfe5ec2f1c19fa85363a1a25480f44724de47ba5530b8ae34d18e81b5c8701a920317c16d53bc2250ae9794aa83d2bd3965df9113db5cc7f9653)
    get_prebuilt(libpng ${LIBPNG_VER} x86_64      SHA512=1cd2e27bfa43c3cce484a2e1ecd1e8fd53cb54d26535e0c5fcd113a2c3ad203087af06f56cc6b66d51a8a4236b9bea0e963cb9d1ab8b6ae8a0f9699b02957b5a)
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

set(LIBDRM_VER "7.1.1_r28-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libdrm ${LIBDRM_VER} armeabi-v7a SHA512=7bc3683546d387bacf706c872566211afb22b01ed479e7eccf8906cbe767e497e20c437e8249835db1308d3b189dc7245b70620edc465014e35d7861d52d4407)
    get_prebuilt(libdrm ${LIBDRM_VER} arm64-v8a   SHA512=9c5d37c5973d9442d2eb1808fb0195936cf91c320a908b7305e64d3b8672d7f22ebe6b2ab20b5a201c648633db667bfef50ca15d416d63e353dca777fc0581bc)
    get_prebuilt(libdrm ${LIBDRM_VER} x86         SHA512=be1cfca18cef6bb8dc8c37036cf578ad9f137ca5c27a6b55f316ed291258145f97126a6ddd387110f2125c3b8cd3af9d7dde0a8f669bc3057b3661133cda8b6d)
    get_prebuilt(libdrm ${LIBDRM_VER} x86_64      SHA512=6a3c99878dfd4a3bd9896fa3f71431c48f2703cef5bce5f448cb44098330cd46795308605027db54668791aba0ef0448b76c1fd3211f891379017374fbf72417)
else()
    set(THIRD_PARTY_LIBDRM_DIR "${MBP_PREBUILTS_BINARY_DIR}/libdrm/${LIBDRM_VER}" PARENT_SCOPE)
endif()

################################################################################
# safe-iop for Android
################################################################################

set(SAFE_IOP_VER "7.1.1_r28-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} armeabi-v7a SHA512=3202b5e9b88c1bee4dd5291f43fcd130f3d2335432b9c2fbaa988afa9456c1490c293d04d7bcf4390eb3643000668167f3a79db8b5ed615020eff020fc2dc095)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} arm64-v8a   SHA512=d3c3d689d4f14ad239178ecb57e4d4d04568793b20ae34470889d617017237228a58576f3954a6b328153632cb4076b92f0d0cab6e095c329cfce417ada02400)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86         SHA512=d4172e8d253c60bde5586e9de3d591885b8c73de46ef3f6ddabab0519828b20701610daade253699943c0e0b7e4d3378f7e7815a5fd6ca59ee202c590254be55)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86_64      SHA512=c823420e85ed4919efb3afa38de855e1393a06d81e8fe293d3ae26c098b859b9f7df7fe6e0847027d5628293af4924b04e708d8e01e4d01705a2bdc223740ea3)
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
