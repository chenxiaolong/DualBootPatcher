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

set(LIBARCHIVE_VER "3.3.1-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=95a704e0145b623882b477b0dba1415d37f850105d0526d3b6f73e345781aadd189fcaaafb40f8d835b078dced6316c84fdbf1527501241250200c9130fae88f)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=ac8f260c88930ac58901531c61681ba1c2cf51826b716dffa0394d057437fe0854930bf6d2593b1d690ef525c57bfc1e22b31e90c7342f48d0911ad7368fa597)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=6cb1de2d77cf164c1ef26a29587d361c4a183af9e13fe4ce6b72c77c19556b447c2faa10482e9d657c95a63f3d4e8a723592d79ca8c8f0c54afda5c7b89d3d4f)
    get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=5b199df7af0e9c103ee0968ffa01b9f1971a0cacf4c80a802e64b948b41e8a4c80f1555b422cf85d6403e05db88ca929b1c8a531e78ed19181348a1f9053798c)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=2b2c7ae240d148e6a3e04648a65a81db35455338b4c474320f6881652d0ddc435f19a3d16b1f5e0609da25970c9bc233b5e3a2b833e41c5672e6a25fddfc5d5f)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=5b0091cd7b737c7a0eef4efaec20c5ec0129dbf230336ded9a4a14a15f7c31e0472807b34823c9d9df99e966269593586f560846f793156675ce815690ec4e89)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=6b5911cb6e277c866d747b74d89a0e78fe1c090bcc4ac061c0d2d6903888d375c0cb31bdc9576b97df212e1d8223306742a8dd741c4b14650d1d66b3a36b7fdd)
    get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=e52029e66b4f05f1e275ec0e4471c636af48ad18d80e89bf39797d1d925a6591ca046fe947c717b19bf4ae8d8630fd3b0156ce1bb4adc10ab09aee29d1a87951)
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

set(FREETYPE2_VER "7.1.1_r28-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(freetype2 ${FREETYPE2_VER} armeabi-v7a SHA512=35a25bc2ba2f6b25db6bd603f38797be8c5ab4a40e9c261e6e3e9647fe73e6a6d1ffe219e9b2432f5449b0de0a6a0442a7add6de2746ec43bb3ae618cb3c9778)
    get_prebuilt(freetype2 ${FREETYPE2_VER} arm64-v8a   SHA512=6b6d7f5b9df89adf8d696d6b133e5003b186952d923a3db1031b643add0945e25eb312e8b378a7f878a955d353ffd90040ebd1b1ee9cb660aa8a3b81c0996d33)
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86         SHA512=96797a3c2e20e7c5455721d55090d45960ae61c381786647db2ef7757713e54e7130a2735af2b2d9ee34058c8cbcbe1d51c1927915df9a34070a776325ac832a)
    get_prebuilt(freetype2 ${FREETYPE2_VER} x86_64      SHA512=0de6b6735f4b9c2b2e6de5eb622752ca089c2629ea03f1396ec49397de517fe3ba55ae4c955cfc80f5227de226f2742f2360b28b5bcae437e63725cc03927afd)
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

set(ANDROID_SYSTEM_CORE_VER "7.1.1_r28-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} armeabi-v7a SHA512=d8799859df657420516002eceb7e2e6f340f569db7028d5df2923650ba42f37fd9770fe15c329686cf06033235b80cf8365fe0f2c3e9ed1a48962f33ae550092)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} arm64-v8a   SHA512=24be099388ebe69bc7fec2570d18f8a2dc5b9f4241d306a6d35ffb56cc1f698b55802847bc3cf73197c720056e424521ac42bc401b5040c90ac5ce594c4f7405)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86         SHA512=d13a4916eb0fe674f8ea267c5d3b2e01bab6675f12cddd9a37b46e409b98e4708474e9f5fbf1f25a8af4f6cf9fb8e6031b39311fa55b41573a080e5b368be32a)
    get_prebuilt(android-system-core ${ANDROID_SYSTEM_CORE_VER} x86_64      SHA512=899656a0adc8de35e2cfc3762c4bebec07cc44afa77aa58f714301385ba2aedb4d37f04849cb03507e7b7b833c6e47f37e8c75866f89be84daf4df4c85d991f2)
else()
    set(THIRD_PARTY_ANDROID_SYSTEM_CORE_DIR "${MBP_PREBUILTS_BINARY_DIR}/android-system-core/${ANDROID_SYSTEM_CORE_VER}" PARENT_SCOPE)
endif()

################################################################################
# boringssl for Android
################################################################################

set(BORINGSSL_VER "r3955.bbfe603-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(boringssl ${BORINGSSL_VER} armeabi-v7a SHA512=472b281a4bc380bf96acc392333410b89d3102c20d4a30e150a4c295bcec386f288c6aa1469f59e54e588d33c3d130905560b3b9bb701370b5bb3226523430d6)
    get_prebuilt(boringssl ${BORINGSSL_VER} arm64-v8a   SHA512=e96210ddeb12b4d90c6baac6eeb122485fd84f9ba984100a0f8e978c1eab383b59dc11aef35d4acb7e3388c2214aedc8b891acb3ed1bc429a1361d0907eb8049)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86         SHA512=118ba17765bc8168a1ab62642f6707354f39d12e09768ede2fd63c1d370d89f5594ed045756d57eb0a9b031da45f5234eff0dc51f0de99365145758e63c71129)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86_64      SHA512=f335b1c6c49e83ffefdc4e41fcad7ab83e49346eeacfffa0b240831aa25c08204201711a9309aa45f8530ad3545b6808c68def27432c07317dc4ae5601213c04)
else()
    set(THIRD_PARTY_BORINGSSL_DIR "${MBP_PREBUILTS_BINARY_DIR}/boringssl/${BORINGSSL_VER}" PARENT_SCOPE)
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
        https://dbp.noobdev.io/misc/prebuilts/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
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
