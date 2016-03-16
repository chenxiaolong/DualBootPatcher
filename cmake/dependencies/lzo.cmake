if(ANDROID)
    set(MBP_LZO_INCLUDES
        ${THIRD_PARTY_LZO_DIR}/${ANDROID_ABI}/include)
    set(MBP_LZO_LIBRARIES
        ${THIRD_PARTY_LZO_DIR}/${ANDROID_ABI}/lib/liblzo2.a)
else()
    find_path(MBP_LZO_INCLUDES lzo/lzo1x.h)
    find_library(MBP_LZO_LIBRARIES NAMES lzo2 liblzo2)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LZO DEFAULT_MSG MBP_LZO_LIBRARIES MBP_LZO_INCLUDES)
    if(NOT LZO_FOUND)
        message(FATAL_ERROR "LZO is not found!")
    endif()
endif()
