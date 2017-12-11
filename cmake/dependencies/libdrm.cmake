if(ANDROID)
    set(LIBDRM_INCLUDE_DIR
        ${THIRD_PARTY_LIBDRM_DIR}/${ANDROID_ABI}/include)
    set(LIBDRM_LIBRARY
        ${THIRD_PARTY_LIBDRM_DIR}/${ANDROID_ABI}/lib/libdrm.a)
endif()

find_package(LibDrm REQUIRED)
