if(ANDROID)
    message(STATUS "NOTE: Android actually uses BoringSSL")
    set(MBP_OPENSSL_INCLUDES
        ${THIRD_PARTY_BORINGSSL_DIR}/${ANDROID_ABI}/include)
    set(MBP_OPENSSL_CRYPTO_LIBRARY
        ${THIRD_PARTY_BORINGSSL_DIR}/${ANDROID_ABI}/lib/libcrypto.a)
    set(MBP_OPENSSL_SSL_LIBRARY
        ${THIRD_PARTY_BORINGSSL_DIR}/${ANDROID_ABI}/lib/libssl.a)
else()
    find_package(OpenSSL)
    if(NOT OPENSSL_FOUND)
        message(FATAL_ERROR "OpenSSL is not found!")
    endif()

    set(MBP_OPENSSL_INCLUDES ${OPENSSL_INCLUDE_DIR})
    set(MBP_OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY})
    set(MBP_OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARY})
endif()
