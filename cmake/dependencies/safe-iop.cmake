if(ANDROID)
    set(MBP_SAFE_IOP_INCLUDES
        ${THIRD_PARTY_SAFE_IOP_DIR}/${ANDROID_ABI}/include)
    set(MBP_SAFE_IOP_LIBRARIES
        ${THIRD_PARTY_SAFE_IOP_DIR}/${ANDROID_ABI}/lib/libsafe_iop.a)
endif()
