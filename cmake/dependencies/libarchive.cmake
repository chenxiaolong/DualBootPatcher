if(ANDROID)
    set(MBP_LIBARCHIVE_INCLUDES
        ${THIRD_PARTY_LIBARCHIVE_DIR}/${ANDROID_ABI}/include)
    set(MBP_LIBARCHIVE_LIBRARIES
        ${THIRD_PARTY_LIBARCHIVE_DIR}/${ANDROID_ABI}/lib/libarchive.a)

    set(MBP_LIBARCHIVE_LOWAPI_INCLUDES
        ${THIRD_PARTY_LIBARCHIVE_LOWAPI_DIR}/${ANDROID_ABI}/include)
    set(MBP_LIBARCHIVE_LOWAPI_LIBRARIES
        ${THIRD_PARTY_LIBARCHIVE_LOWAPI_DIR}/${ANDROID_ABI}/lib/libarchive.a)
else()
    if(MBP_USE_SYSTEM_LIBARCHIVE)
        find_package(LibArchive)
        if(NOT LibArchive_FOUND)
            message(FATAL_ERROR "MBP_USE_SYSTEM_LIBARCHIVE is ON but LibArchive is not found!")
        endif()

        set(MBP_LIBARCHIVE_INCLUDES ${LibArchive_INCLUDE_DIRS})
        set(MBP_LIBARCHIVE_LIBRARIES ${LibArchive_LIBRARIES})
    else()
        set(ENABLE_NETTLE OFF CACHE INTERNAL "Enable use of Nettle")
        set(ENABLE_OPENSSL OFF CACHE INTERNAL "Enable use of OpenSSL")
        set(ENABLE_LZMA ON CACHE INTERNAL "Enable the use of the system found LZMA library if found")
        set(ENABLE_ZLIB ON CACHE INTERNAL "Enable the use of the system found ZLIB library if found")
        set(ENABLE_BZip2 OFF CACHE INTERNAL "Enable the use of the system found BZip2 library if found")
        set(ENABLE_LIBXML2 OFF CACHE INTERNAL "Enable the use of the system found libxml2 library if found")
        set(ENABLE_EXPAT OFF CACHE INTERNAL "Enable the use of the system found EXPAT library if found")
        set(ENABLE_PCREPOSIX OFF CACHE INTERNAL "Enable the use of the system found PCREPOSIX library if found")
        set(ENABLE_LibGCC OFF CACHE INTERNAL "Enable the use of the system found LibGCC library if found")
        set(ENABLE_XATTR OFF CACHE INTERNAL "Enable extended attribute support")
        set(ENABLE_ACL OFF CACHE INTERNAL "Enable ACL support")
        set(ENABLE_ICONV OFF CACHE INTERNAL "Enable iconv support")
        set(ENABLE_TAR OFF CACHE INTERNAL "Enable tar building")
        set(ENABLE_CPIO OFF CACHE INTERNAL "Enable cpio building")
        set(ENABLE_CAT OFF CACHE INTERNAL "Enable cat building")
        set(ENABLE_TEST OFF CACHE INTERNAL "Enable unit and regression tests")
        set(ENABLE_INSTALL OFF CACHE INTERNAL "Enable installing of libraries")

        set(ZLIB_INCLUDE_DIR ${MBP_ZLIB_INCLUDES})
        set(ZLIB_LIBRARY ${MBP_ZLIB_LIBRARIES})
        set(LZMA_INCLUDE_DIR ${MBP_LIBLZMA_INCLUDES})
        set(LZMA_LIBRARY ${MBP_LIBLZMA_LIBRARIES})
        set(LZ4_INCLUDE_DIR ${MBP_LZ4_INCLUDES})
        set(LZ4_LIBRARY ${MBP_LZ4_LIBRARIES})
        set(LZO2_INCLUDE_DIR ${MBP_LZO_INCLUDES})
        set(LZO2_LIBRARY ${MBP_LZO_LIBRARIES})

        add_definitions(-DLIBARCHIVE_STATIC)

        add_subdirectory(external/libarchive)

        # Linking shared library to libarchive's static library, need -fPIC
        set_target_properties(
            archive_static
            PROPERTIES
            POSITION_INDEPENDENT_CODE 1
        )
        # Don't build the shared library
        set_target_properties(
            archive
            PROPERTIES
            EXCLUDE_FROM_ALL 1
            EXCLUDE_FROM_DEFAULT_BUILD 1
        )

        set(MBP_LIBARCHIVE_INCLUDES ${CMAKE_SOURCE_DIR}/external/libarchive/libarchive)
        set(MBP_LIBARCHIVE_LIBRARIES archive_static)
    endif()
endif()
