if(ANDROID)
    set(SAFE_IOP_INCLUDE_DIR
        ${THIRD_PARTY_SAFE_IOP_DIR}/${ANDROID_ABI}/include)
    set(SAFE_IOP_LIBRARY
        ${THIRD_PARTY_SAFE_IOP_DIR}/${ANDROID_ABI}/lib/libsafe_iop.a)
endif()

find_package(SafeIop REQUIRED)
