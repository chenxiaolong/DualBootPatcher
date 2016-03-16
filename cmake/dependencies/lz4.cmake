if(ANDROID)
    set(MBP_LZ4_INCLUDES
        ${THIRD_PARTY_LZ4_DIR}/${ANDROID_ABI}/include)
    set(MBP_LZ4_LIBRARIES
        ${THIRD_PARTY_LZ4_DIR}/${ANDROID_ABI}/lib/liblz4.a)
else()
    find_path(MBP_LZ4_INCLUDES lz4.h)
    find_library(MBP_LZ4_LIBRARIES NAMES lz4 liblz4)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LZ4 DEFAULT_MSG MBP_LZ4_LIBRARIES MBP_LZ4_INCLUDES)
    if(NOT LZ4_FOUND)
        message(FATAL_ERROR "CMAKE_USE_SYSTEM_LZ4 is ON but LZ4 is not found!")
    endif()
endif()
