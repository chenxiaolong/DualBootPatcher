if(ANDROID)
    if(${MBP_BUILD_TARGET} STREQUAL android-system)
        set(CMAKE_FIND_LIBRARY_SUFFIXES_OLD ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif()

    find_package(ZLIB)
    if(NOT ZLIB_FOUND)
        message(FATAL_ERROR "zlib is included with the Android NDK, but it was not found!")
    endif()

    set(MBP_ZLIB_INCLUDES ${ZLIB_INCLUDE_DIR})
    set(MBP_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})

    if(${MBP_BUILD_TARGET} STREQUAL android-system)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_OLD})
        unset(CMAKE_FIND_LIBRARY_SUFFIXES_OLD)
    endif()
else()
    if(MBP_USE_SYSTEM_ZLIB)
        find_package(ZLIB)
        if(NOT ZLIB_FOUND)
            message(FATAL_ERROR "MBP_USE_SYSTEM_ZLIB is ON but zlib is not found!")
        endif()

        set(MBP_ZLIB_INCLUDES ${ZLIB_INCLUDE_DIR})
        set(MBP_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})
    else()
        set(SKIP_INSTALL_ALL ON CACHE INTERNAL "Disable zlib install")
        set(ZLIB_ENABLE_TESTS OFF CACHE INTERNAL "Disable zlib tests")

        add_subdirectory(external/zlib)

        # Linking shared library to zlib's static library, need -fPIC
        set_target_properties(
            zlibstatic
            PROPERTIES
            POSITION_INDEPENDENT_CODE 1
        )

        # Don't build the shared library or other binaries
        set(ZLIB_DISABLE zlib example example64 minigzip minigzip64)
        foreach(ZLIB_TARGET ${ZLIB_DISABLE})
            if(TARGET ${ZLIB_TARGET})
                set_target_properties(
                    ${ZLIB_TARGET}
                    PROPERTIES
                    EXCLUDE_FROM_ALL 1
                    EXCLUDE_FROM_DEFAULT_BUILD 1
                )
            endif()
        endforeach()

        set(MBP_ZLIB_INCLUDES
            ${CMAKE_SOURCE_DIR}/external/zlib
            ${CMAKE_BINARY_DIR}/external/zlib)
        set(MBP_ZLIB_LIBRARIES zlibstatic)
    endif()
endif()
