if(ANDROID)
    set(PNG_PNG_INCLUDE_DIR
        ${THIRD_PARTY_LIBPNG_DIR}/${ANDROID_ABI}/include)
    set(PNG_LIBRARY
        ${THIRD_PARTY_LIBPNG_DIR}/${ANDROID_ABI}/lib/libpng.a)
endif()

find_package(PNG REQUIRED)
