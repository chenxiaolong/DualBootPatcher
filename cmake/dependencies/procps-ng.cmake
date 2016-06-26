if(ANDROID)
    set(MBP_PROCPS_NG_INCLUDES
        ${THIRD_PARTY_PROCPS_NG_DIR}/include)
    set(MBP_PROCPS_NG_LIBRARIES
        ${THIRD_PARTY_PROCPS_NG_DIR}/lib_${ANDROID_ABI}/libprocps.a)
endif()
