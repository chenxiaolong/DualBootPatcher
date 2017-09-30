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

add_library(LibArchive::LibArchive UNKNOWN IMPORTED)
set_target_properties(
    LibArchive::LibArchive
    PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${LibArchive_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${LibArchive_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "LibLZMA::LibLZMA;LZ4::LZ4;ZLIB::ZLIB"
)
