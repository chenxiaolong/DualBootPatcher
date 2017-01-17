if(${MBP_BUILD_TARGET} STREQUAL android-app)
    # Use API 17 version of the library
    set(LibArchive_INCLUDE_DIR
        ${THIRD_PARTY_LIBARCHIVE_LOWAPI_DIR}/${ANDROID_ABI}/include)
    set(LibArchive_LIBRARY
        ${THIRD_PARTY_LIBARCHIVE_LOWAPI_DIR}/${ANDROID_ABI}/lib/libarchive.a)
elseif(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Use API 21 version of the library
    set(LibArchive_INCLUDE_DIR
        ${THIRD_PARTY_LIBARCHIVE_DIR}/${ANDROID_ABI}/include)
    set(LibArchive_LIBRARY
        ${THIRD_PARTY_LIBARCHIVE_DIR}/${ANDROID_ABI}/lib/libarchive.a)
endif()

find_package(LibArchive REQUIRED)

set(MBP_LIBARCHIVE_INCLUDES ${LibArchive_INCLUDE_DIRS})
set(MBP_LIBARCHIVE_LIBRARIES ${LibArchive_LIBRARIES})
