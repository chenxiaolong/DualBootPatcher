if(ANDROID)
    set(PROCPS_INCLUDE_DIR
        ${THIRD_PARTY_PROCPS_NG_DIR}/include)
    set(PROCPS_LIBRARY
        ${THIRD_PARTY_PROCPS_NG_DIR}/lib_${ANDROID_ABI}/libprocps.a)
endif()

find_package(Procps REQUIRED)
