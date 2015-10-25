# All of the files here are built from the PKGBUILD files in thirdparty/*/PKGBUILD.
# All of the files are signed with my GPG key and correct hashes are provided
# at https://snapshots.noobdev.io/repo/prebuilts, but if you don't trust me or
# my binaries, you can use your own builds builds by copying your own prebuilts
# to thirdparty/prebuilts and updating the checksums here. It is not necessary
# to upload the files to a server as CMake will not download the files if the
# checksums match.

set(URL_BASE "https://snapshots.noobdev.io/repo/prebuilts")
set(PREBUILTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/prebuilts")
set(EXTRACT_DIR "${CMAKE_CURRENT_BINARY_DIR}/android")

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
        ${PREBUILTS_DIR}/${pkgname}-${pkgver}-${arch}.pkg.tar.xz
        EXPECTED_HASH ${hash}
        TLS_VERIFY ON
        SHOW_PROGRESS
    )

    # Extract the archive
    if(NOT EXISTS ${EXTRACT_DIR}/${pkgname}/${pkgver}/${abi}/.PKGINFO)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTRACT_DIR}/${pkgname}/${pkgver}/${abi}
        )

        message(STATUS "Extracting ${pkgname}-${pkgver} (${abi}) ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf ${PREBUILTS_DIR}/${pkgname}-${pkgver}-${arch}.pkg.tar.xz
            WORKING_DIRECTORY ${EXTRACT_DIR}/${pkgname}/${pkgver}/${abi}
        )
    endif()
endmacro()


execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PREBUILTS_DIR}
)





################################################################################
# fuse-exfat for Android
################################################################################

set(EXFAT_VER "1.0.0-1")

get_prebuilt(exfat ${EXFAT_VER} armeabi-v7a SHA512=fafa1127112b420037164807babb5e0cf55b17e6ec4f9f7cd2acdc0ab287ec87fd4ec59364fc11b0d4b1a71ae73ada063ffa558a7f96f6e20a544ecfaed79d36)
get_prebuilt(exfat ${EXFAT_VER} arm64-v8a   SHA512=9017d583a8e9657a6c53bbc85d32fc8450b09aa9fd4e99af51667373bec1879f3136eee7d3a88853060a2d6c76183c7983ecc207db1d6ebc96afc1e48013b3e9)
get_prebuilt(exfat ${EXFAT_VER} x86         SHA512=7a758c46ae55c55d59994c1b96362450a0a616825f4405be236a8f9be223d1249c9567ce1709e23404c587a0dad6b6b936b757ef4bff14ecdb7d32287e278c89)
get_prebuilt(exfat ${EXFAT_VER} x86_64      SHA512=4822fff7bc6c69e6eb0bd8b2d5b0bcd7231ef47771fe85e089c9ccc94ae213550ebe740ac7de60d45ee02bbdc58923a42da7e514784a46d38e9560138c6fb549)

foreach(abi armeabi-v7a arm64-v8a x86 x86_64)
    install(
        FILES ${EXTRACT_DIR}/exfat/${EXFAT_VER}/${abi}/bin/mount.exfat
        DESTINATION ${DATA_INSTALL_DIR}/binaries/android/${abi}/
        COMPONENT Libraries
    )
endforeach()

################################################################################
# libarchive for Android
################################################################################

set(LIBARCHIVE_VER "3.1.2.r599.g567b374-1")

get_prebuilt(libarchive ${LIBARCHIVE_VER} armeabi-v7a SHA512=7ad6cda99dd6ccaa82b2297eab7a527dab7c59f6909a2f9be82c816d1f732da9258255ddf26d60e865ed1758418c56200074abf46cd311b6107b75eb4077d9a8)
get_prebuilt(libarchive ${LIBARCHIVE_VER} arm64-v8a   SHA512=67b7b07e43a124804c65d50c17f949a345258d47fc324671c39541176bfd2b7d0fc98eb374e2ebceabaddc21a93849eeefbe3071b0d3a68da236b22f8f57a26d)
get_prebuilt(libarchive ${LIBARCHIVE_VER} x86         SHA512=3548d223d1189b9287314d90cf7b400b4f8b15eb305a177f95a9bfa920a743f50b1b1bdaeb2f0ce2272845e9f9702604e3e5771cb236172ffbcbfd712a3fc4aa)
get_prebuilt(libarchive ${LIBARCHIVE_VER} x86_64      SHA512=a34ce8c78339c716b845d385f8b86f8a1e1811dab07bbe233597bb9d19c399c96719757ed35c121e1b138d35a52ba6e760cf550f390d9b92e0c2f9669be1dac1)
get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} armeabi-v7a SHA512=219998ad1cc94ee38fda69989946169c4e22d0905d40ffebe6aebc2f07b9959d0fcab0256f664d6eae6e6b78ce2e10c4752f508bcb31a9de66adc78d46660233)
get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} arm64-v8a   SHA512=e4f1fa1f1f52b8c03886e0ed6b2e8dc2aae1ca31bad81b0919edc7756504322272c51f852b6db4a6a40bea087fa90acfb6f37bc9002932869643a53250d21cdc)
get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86         SHA512=ae47b020d27b0c6e2a75759e0447a69ada6746164841c19a60122d0f9b1cfe67b8af19c2aba1a603817cdade9dab3da8b1409cc6f0115dcfb45b7b0850357f2e)
get_prebuilt(libarchive-lowapi ${LIBARCHIVE_VER} x86_64      SHA512=edb924ce4e364dc5e204eb0b48d1628227d1dfc9c7f27c4d3ec3b4f758dd2a2d160bd3d08859dc6e95862d50c789fd1f58466927bb50f6403cacb118b6168f32)

################################################################################
# libiconv for Android
################################################################################

set(LIBICONV_VER "1.14-1")

get_prebuilt(libiconv ${LIBICONV_VER} armeabi-v7a SHA512=d4bc7e39a7205ff166a1aa07b93ceb4fd7b6f7f2be26dc8917dd5753034470a78e9a18139613461a4e3b565b43eeb479c735566f058966f71dae19cf16d5a208)
get_prebuilt(libiconv ${LIBICONV_VER} arm64-v8a   SHA512=b2a03adda6bfcb18bd14422b1e950f383bf030b77680e06114220666034bfb074d26a0d9c2067df3bc8b1b64cdbb9fa5b6c4cc83d0cfa11e66121352809d6756)
get_prebuilt(libiconv ${LIBICONV_VER} x86         SHA512=827d284e74787091f1403f8bb9a61ee2e0ab4930eb1da24a0ad564caadf413ac7e2d3501489b2c35cca2a0978fa347e0cbf7bf76b42a31098d31fffba80a9ca9)
get_prebuilt(libiconv ${LIBICONV_VER} x86_64      SHA512=004b629ce9ffa43cd88c23d87b927c6d80dcf4de8b750cd40532e00100e1e13387c0982f2af3c3a3593ed32b83aba66b7a7849bc08d1f210e556a192c2948615)

###############################################################################
# liblzma for Android
################################################################################

set(LIBLZMA_VER "5.2.2-1")

get_prebuilt(liblzma ${LIBLZMA_VER} armeabi-v7a SHA512=0bec5f8bc3e1484b830e2c57b041e916687ea4035769074e5576f6682a531d285538fbf7a5a85c88c654209d8e9e8b14ad80e78a3d9c29aa5d304eecf247edc8)
get_prebuilt(liblzma ${LIBLZMA_VER} arm64-v8a   SHA512=6b66601dba6c419f7462a331df7dc71cc13356168a0c6a9c0cb9eb38d46af5aae6f7128838c3db80167b8b318200c6f6ec5b37b0f1ef23a668ba1512a1325417)
get_prebuilt(liblzma ${LIBLZMA_VER} x86         SHA512=2a4008a8f89a0bce25a7c5c293733cafdee54effda563c6a92f75cb607d65abc47e9c7e900ed2f026991af80a2120faeebf44ad8993521993f24cf9f44bd839b)
get_prebuilt(liblzma ${LIBLZMA_VER} x86_64      SHA512=c2f5607f913dd4b76abeef6536e108f15dc5daec779f85f3d22588d76950cf7c188e7cda465d0007e5720050667d37a1a4e08e5f0766ee81db26785b9035f52f)

################################################################################
# lzo for Android
################################################################################

set(LZO_VER "2.09-1")

get_prebuilt(lzo ${LZO_VER} armeabi-v7a SHA512=baa6537ca4ba80c508f39bb64e3404c5ccb71c31c67b28501bbc4a24272b558a0f59fbb79573c6a261bffebfb1aeff174f383b75626752393c5093959b0c8415)
get_prebuilt(lzo ${LZO_VER} arm64-v8a   SHA512=83a44742fbe960d94f46b03ed3eb3786985db34608e151a9a00310c4a884e9ede553322175ad22806bf364f3ec8665565a38a6bfe83cb01a67eb0e777eb7c09f)
get_prebuilt(lzo ${LZO_VER} x86         SHA512=2bef29b8413410c6426a9d4ad103f0040e6e1d6b018b8ea7aedf730e79624e12d2da8ab68486c1eb1de23db0b96d8388213128c240a53e4fda4a1fec20650c70)
get_prebuilt(lzo ${LZO_VER} x86_64      SHA512=d04a86dd68f190115f220019c0e2f315f78b7c098246052137a2b3fe94759c0015010ba4242f19002fc919ee3687946ae15e77d83e9d516feb110b0dbc6cfb60)

################################################################################
# lz4 for Android
################################################################################

set(LZ4_VER "131-1")

get_prebuilt(lz4 ${LZ4_VER} armeabi-v7a SHA512=e3806e50e89527de9b4061a35ac6ef4be372ec4321a53a19a343ad5c5a29059582d0cdc48baffbd2dfd39332f116b1ea8d32d4662174ad0fdacebaac133edb58)
get_prebuilt(lz4 ${LZ4_VER} arm64-v8a   SHA512=262925a98d27ca0afd5dc579b7eca522ae6d15bed80294f34a6ed9b58ce51e321f755260a5f4723af26addf4856e52a8fb490db82b7a1e309dca50e6656bc5fb)
get_prebuilt(lz4 ${LZ4_VER} x86         SHA512=4047ddb609ff1b228c8075c70347fab91119e3d5324cfe03eb616ebb6238f3ac9b77b6fdfad79eedb3f0faf81fb1ddd0a7dfd1e0d9012059fa2bed025ac8e5a1)
get_prebuilt(lz4 ${LZ4_VER} x86_64      SHA512=51129cf86be6c5f0b50262be15f3113fc08123df00e12d3474d5fb38c95ea5d0b6ddf5c83ca752d076ddc56920901174bbf2e9a29a3b8072eac3d60259f51334)

################################################################################
# jansson for Android
################################################################################

set(JANSSON_VER "2.7-1")

get_prebuilt(jansson ${JANSSON_VER} armeabi-v7a SHA512=c7812e9b7f61a24857c8fc03d1e1b59bc0a5a4c85ca93552378844aa874aefeebec25b6a108143a778970ec9097ca7832f6181ad0f07bc593cf4ecc8de4f38c7)
get_prebuilt(jansson ${JANSSON_VER} arm64-v8a   SHA512=8ae200c9028373b974da39afc0569ddd633bc273cb0b3c138de29d0152678c36d754c5ffdba94552e3165640f4c0bb80cb5255fc1744e194cac4fb864206db22)
get_prebuilt(jansson ${JANSSON_VER} x86         SHA512=00739bbd7d27045fda4a51118c1559fd279cc3a16a425d1f91cfa898a5c5d63bb246fde3fb9e0d1eb57bea547c512cdc75be94093166940176348aecbe249ef9)
get_prebuilt(jansson ${JANSSON_VER} x86_64      SHA512=9a6759dc9444026c9b4370f61ea7b46f0f4c35dcb2b9aa16666241b0743ee44f0829fc57be25528ca14a08ce596b13d9beaf28cf3108f9a84b5296106bf1d407)

################################################################################
# libsepol for Android
################################################################################

set(LIBSEPOL_VER "2.4.r1240.674256b-1")

get_prebuilt(libsepol ${LIBSEPOL_VER} armeabi-v7a SHA512=61ec95038e7922924a4240eac6ae795bcd4ece96eeb88e8792154c3c7906c086c7ad3d572f09b5531bf3e247b2e2585209d12e00b0436808c1d1b08552c3e86c)
get_prebuilt(libsepol ${LIBSEPOL_VER} arm64-v8a   SHA512=f00a33c6a57e8e6b0971c905f5f90a7cd57c77bfe7745f78471cfb9776f35f1954f24b6c9f248e1afb01f1ae68477a34cab6e3801846a39d5da172102d3cae8f)
get_prebuilt(libsepol ${LIBSEPOL_VER} x86         SHA512=9dab02513969e9950e2d5ee30ff8b2cc44c4f2c08b82f2ddf5d2040333b56c67adfb8a4143ed774f39c64b84f52ca32f080a8f10ffc2624d8f7752b6574b8de3)
get_prebuilt(libsepol ${LIBSEPOL_VER} x86_64      SHA512=96ea69c1e0c677a739d0c2d620666f4225e5ef2751525edb8c00c921f2687d47a1caf026b3bb7f1088c8dba94cc327b539082b62257f891d3a62b801458760f4)

################################################################################
# openssl for Android
################################################################################

set(OPENSSL_VER "1.0.2d-1")

get_prebuilt(openssl ${OPENSSL_VER} armeabi-v7a SHA512=a2b0e64d156dd9d361a66f5009ac616bbe4890449ba8fac9c603e1e4741eaa52b6f0208fc6dc675f918fd979e4cda720667792cf15c5c7797ed11bf60dbe9d96)
get_prebuilt(openssl ${OPENSSL_VER} arm64-v8a   SHA512=73bf3af333d16e843ea36f93a03e885ce02de5aef980038bd1bc491e24c431a08b8cc5d32dac9c1d0379505dab05c424cfa47c101dca19d6bc939042b96bc0d1)
get_prebuilt(openssl ${OPENSSL_VER} x86         SHA512=a9e10a0bd19c6c7f7b01547685807b2610e9250b34fe1c1b4f51b94f0e2b42b3b58b01fb756012a205b5a163ca9cb628152d8e7306c85c756c338ecc063f63d5)
get_prebuilt(openssl ${OPENSSL_VER} x86_64      SHA512=c2d2362274ff72234e8922383115a6466a7f91281c4a8cada7c12c1f4473864dc76179a2c140cfeed0d22ac21a88a268494fec497af620a19030007c3c026305)

################################################################################
# procps-ng for Android
################################################################################

# TODO: TO BE REMOVED SOON!

set(PROCPS_NG_VER "3.3.10")

file(
    DOWNLOAD
    https://snapshots.noobdev.io/misc/prebuilts/procps-ng-${PROCPS_NG_VER}_android.tar.bz2
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

set(JNA_VER "4.2.1-1")

get_prebuilt(jna ${JNA_VER} all SHA512=28fb507df36870da6592513a64437c80ba5c582d563e95f108f3033973622e0f326fd4a146fa20934b60ecc82130ae99da4777f1555b5c9d8ddd48f32033e911)

################################################################################
# e2fsprogs for Android
################################################################################

set(E2FSPROGS_VER "5.1.1_r9-1")

get_prebuilt(e2fsprogs ${E2FSPROGS_VER} armeabi-v7a SHA512=610b2e98b8106217e5c36f092fe6226f631929f12de536800d9b505345783e4e421e0fbd9d4fd71cfbddf542fbe0cd3fb5853b315b240bdc2c6c36316d744571)
get_prebuilt(e2fsprogs ${E2FSPROGS_VER} arm64-v8a   SHA512=55422570a954fae83265c6c2204334b78069b85b8544a2d1ca365ea517372c1a45d8b13da2492df5832e89be6769ee2294c0d265251c82399c65fae101adeb75)
get_prebuilt(e2fsprogs ${E2FSPROGS_VER} x86         SHA512=05741d24206fd457a40d609239cbf45f04aa4ec1116bd2cee497407d5275a3262aadf6e643c3128b81b775a9270cfda76f1073a2cba3a3a54b6fad3d83164345)
get_prebuilt(e2fsprogs ${E2FSPROGS_VER} x86_64      SHA512=5523d9b3ece096decc481193a6633df6febe987909374cc36accde9bbb9afb64d38217dd9ae68d1147959f5c19746c8b70c8b8553683b958200a4357743a0086)

foreach(abi armeabi-v7a arm64-v8a x86 x86_64)
    install(
        FILES ${EXTRACT_DIR}/e2fsprogs/${E2FSPROGS_VER}/${abi}/bin/e2fsck
              ${EXTRACT_DIR}/e2fsprogs/${E2FSPROGS_VER}/${abi}/bin/resize2fs
        DESTINATION ${DATA_INSTALL_DIR}/binaries/android/${abi}/
        COMPONENT Libraries
    )
endforeach()

################################################################################

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    @ONLY
)