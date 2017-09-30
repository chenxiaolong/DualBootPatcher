if(ANDROID)
    set(ICONV_INCLUDE_DIR
        ${THIRD_PARTY_LIBICONV_DIR}/${ANDROID_ABI}/include)
    set(ICONV_LIBRARY
        ${THIRD_PARTY_LIBICONV_DIR}/${ANDROID_ABI}/lib/libiconv.a)
endif()

find_package(Iconv REQUIRED)
