if(ANDROID)
    message(STATUS "NOTE: Android actually uses BoringSSL")

    set(OPENSSL_INCLUDE_DIR
        ${THIRD_PARTY_BORINGSSL_DIR}/${ANDROID_ABI}/include)
    set(OPENSSL_CRYPTO_LIBRARY
        ${THIRD_PARTY_BORINGSSL_DIR}/${ANDROID_ABI}/lib/libcrypto.a)
    set(OPENSSL_SSL_LIBRARY
        ${THIRD_PARTY_BORINGSSL_DIR}/${ANDROID_ABI}/lib/libssl.a)
endif()

# The version checks in CMake <3.7 do not work with the BoringSSL headers
if(NOT ANDROID OR NOT CMAKE_VERSION VERSION_LESS 3.7)
    find_package(OpenSSL REQUIRED)
endif()
