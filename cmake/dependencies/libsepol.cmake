if(ANDROID)
    set(LIBSEPOL_INCLUDE_DIR
        ${THIRD_PARTY_LIBSEPOL_DIR}/${ANDROID_ABI}/include)
    set(LIBSEPOL_LIBRARY
        ${THIRD_PARTY_LIBSEPOL_DIR}/${ANDROID_ABI}/lib/libsepol.a)
endif()

find_package(LibSepol REQUIRED)
