if(ANDROID)
    set(MBP_FUSE_INCLUDES
        ${THIRD_PARTY_FUSE_DIR}/${ANDROID_ABI}/include)
    set(MBP_FUSE_LIBRARIES
        ${THIRD_PARTY_FUSE_DIR}/${ANDROID_ABI}/lib/libfuse.a)
endif()
