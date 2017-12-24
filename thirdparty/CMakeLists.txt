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
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "8.1.0_r2-2")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(libsepol ${LIBSEPOL_VER} armeabi-v7a SHA512=10869a22e56b8b58624fef45849fac29d386ecb738e5733900ef26244f6747662e65dad40fb3bed845cc1d30f98a441d71da03a6edf837f4a6bcc904f662bd7c)
    get_prebuilt(libsepol ${LIBSEPOL_VER} arm64-v8a   SHA512=fc3229d925d973165bea2dba1f892ef8b4f97811bd8b6b266be533e0ca36e07c519c72f41b85eb4a053eb0719f6a234dd2f1ecd234ad8b18a3bf06f5e8856815)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86         SHA512=4be6f8592836cebb965c22f29a2028cfd07a1a5467d3aa781d0c9c08d058b2e80eac79e92946a9d1b6e0d314d68d76c421a7997fff7e956fc3b2259823cf0156)
    get_prebuilt(libsepol ${LIBSEPOL_VER} x86_64      SHA512=6384a4ef8d44c122064049fc8668a02c97e8bacf9c39bdbdb5fa4f9397485becfbb6799d63a3490c65a362b2a41fc41020d23b31f144efb0125133434098952e)
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

set(SAFE_IOP_VER "8.1.0_r2-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} armeabi-v7a SHA512=420ed2ad3734c0424166802b5db626393aab45dbaaa3482d318555d97c97a62a6a2221b80123eab9ff2338d10333efa7c408a52560d84153a4eb6b28d1ee2270)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} arm64-v8a   SHA512=97688da51630653470e6a8cc5dbc23c1bb5f6bb6866ab9fd6726c4062f77c14b28f0723f33d28c5e493b33449fd28305332245fdfe22c6ab6c86df24cdd18f19)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86         SHA512=f846ed837ac644bfa6d2cbf5dcef86126a8b1ce02b3f740f028e37b25d882c6b0d28018450796796ceef5426155d2c990e9ee1903f7cd3b8c9760247a6dbffbe)
    get_prebuilt(safe-iop ${SAFE_IOP_VER} x86_64      SHA512=4b6cf93ac93b575bb0c0c909643f844fda52adff491ff27631f4698348c11e751b5ade69a58d9247734c3c5c2e7092b82ae6d6d04ddd3d011979dddb0329a691)
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

set(BORINGSSL_VER "r4872.0a54e9984-1")

if(MBP_TOP_LEVEL_BUILD)
    get_prebuilt(boringssl ${BORINGSSL_VER} armeabi-v7a SHA512=3cb0278e7363c9766e9dfe41599a5a36480c7612ebf80672b75d5cc75d7cb63c7026686b19a07cbf678a82b7d919ce8f468e555ed7dfb1b5b71e59dbac76a516)
    get_prebuilt(boringssl ${BORINGSSL_VER} arm64-v8a   SHA512=d24c8bc1ca17a9050869a62567af894c0e433209967d16adfd0516ac69f8513c7ab351aa7d11216fd85aced7e413bff697d89d7c50f84055647b3757bcaffc9f)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86         SHA512=a0134cf29d07925ea0fe959f186df1ce50041f5e15ebb6f16cd61c374d284b4d94885bbdff2613a329e2d5717af767cd76bd112a20cc2bde6fd863933cfb05f2)
    get_prebuilt(boringssl ${BORINGSSL_VER} x86_64      SHA512=cbe301865a0e2603450c0a571cf8a757890aafb3cabfb14a5da03e97c7069cecf29626de396bfa4c75d5919dd2b72fa0099b689b3f195853c2ccf6c134353be6)
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
