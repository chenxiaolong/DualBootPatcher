if(ANDROID)
    set(MBP_LZO_INCLUDES
        ${THIRD_PARTY_LZO_DIR}/${ANDROID_ABI}/include)
    set(MBP_LZO_LIBRARIES
        ${THIRD_PARTY_LZO_DIR}/${ANDROID_ABI}/lib/liblzo2.a)
else()
    if(MBP_USE_SYSTEM_LZO)
        find_path(MBP_LZO_INCLUDES lzo/lzo1x.h)
        find_library(MBP_LZO_LIBRARIES NAMES lzo2 liblzo2)

        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(LZO DEFAULT_MSG MBP_LZO_LIBRARIES MBP_LZO_INCLUDES)
        if(NOT LZO_FOUND)
            message(FATAL_ERROR "CMAKE_USE_SYSTEM_LZO is ON but LZO is not found!")
        endif()
    else()
        set(ENABLE_STATIC ON CACHE INTERNAL "Build static LZO library.")
        set(ENABLE_SHARED OFF CACHE INTERNAL "Build shared LZO library.")
        set(ENABLE_BINARIES OFF CACHE INTERNAL "Build LZO binaries.")
        set(INSTALL_LIBS OFF CACHE INTERNAL "Install LZO library.")
        set(INSTALL_DOCS OFF CACHE INTERNAL "Install LZO documentation.")
        set(INSTALL_HEADERS OFF CACHE INTERNAL "Install LZO headers.")

        add_subdirectory(external/lzo)

        # Linking shared library to lzo's static library, need -fPIC
        set_target_properties(
            lzo_static
            PROPERTIES
            POSITION_INDEPENDENT_CODE 1
        )

        set(MBP_LZO_INCLUDES "${CMAKE_SOURCE_DIR}/external/lzo/include")
        set(MBP_LZO_LIBRARIES lzo_static)
    endif()
endif()
