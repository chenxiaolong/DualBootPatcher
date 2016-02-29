if(ANDROID)
    set(MBP_LZ4_INCLUDES
        ${THIRD_PARTY_LZ4_DIR}/${ANDROID_ABI}/include)
    set(MBP_LZ4_LIBRARIES
        ${THIRD_PARTY_LZ4_DIR}/${ANDROID_ABI}/lib/liblz4.a)
else()
    if(MBP_USE_SYSTEM_LZ4)
        find_path(MBP_LZ4_INCLUDES lz4.h)
        find_library(MBP_LZ4_LIBRARIES NAMES lz4 liblz4)

        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(LZ4 DEFAULT_MSG MBP_LZ4_LIBRARIES MBP_LZ4_INCLUDES)
        if(NOT LZ4_FOUND)
            message(FATAL_ERROR "CMAKE_USE_SYSTEM_LZ4 is ON but LZ4 is not found!")
        endif()
    else()
        set(BUILD_TOOLS OFF CACHE INTERNAL "Build the command line tools")
        set(BUILD_LIBS ON CACHE INTERNAL "Build the libraries in addition to the tools")
        set(LZ4_INSTALL OFF CACHE INTERNAL "Install the built tools and/or libraries")

        add_subdirectory(external/lz4/cmake_unofficial)

        # Linking shared library to libarchive's static library, need -fPIC
        set_target_properties(
            liblz4
            PROPERTIES
            POSITION_INDEPENDENT_CODE 1
        )

        set(MBP_LZ4_INCLUDES "${CMAKE_SOURCE_DIR}/external/lz4/lib")
        set(MBP_LZ4_LIBRARIES liblz4)
    endif()
endif()
