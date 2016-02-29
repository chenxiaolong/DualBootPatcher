if(ANDROID)
    set(MBP_LIBLZMA_INCLUDES
        ${THIRD_PARTY_LIBLZMA_DIR}/${ANDROID_ABI}/include)
    set(MBP_LIBLZMA_LIBRARIES
        ${THIRD_PARTY_LIBLZMA_DIR}/${ANDROID_ABI}/lib/liblzma.a)
else()
    if(MBP_USE_SYSTEM_LIBLZMA)
        find_package(LibLZMA)
        if(NOT LIBLZMA_FOUND)
            message(FATAL_ERROR "CMAKE_USE_SYSTEM_LIBLZMA is ON but LibLZMA is not found!")
        endif()

        set(MBP_LIBLZMA_INCLUDES ${LIBLZMA_INCLUDE_DIRS})
        set(MBP_LIBLZMA_LIBRARIES ${LIBLZMA_LIBRARIES})
    else()
        add_subdirectory(external/xz)

        # Linking shared library to libarchive's static library, need -fPIC
        set_target_properties(
            lzma_static
            PROPERTIES
            POSITION_INDEPENDENT_CODE 1
        )

        # Don't build the shared library
        set_target_properties(
            lzma
            PROPERTIES
            EXCLUDE_FROM_ALL 1
            EXCLUDE_FROM_DEFAULT_BUILD 1
        )

        set(MBP_LIBLZMA_INCLUDES
            "${CMAKE_SOURCE_DIR}/external/xz/src/liblzma/api")
        set(MBP_LIBLZMA_LIBRARIES lzma_static)
    endif()
endif()
