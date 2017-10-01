if(ANDROID)
    set(LZ4_INCLUDE_DIR
        ${THIRD_PARTY_LZ4_DIR}/${ANDROID_ABI}/include)
    set(LZ4_LIBRARY
        ${THIRD_PARTY_LZ4_DIR}/${ANDROID_ABI}/lib/liblz4.a)
endif()

find_package(LZ4 REQUIRED)
