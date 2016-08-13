if(ANDROID)
    set(MBP_JANSSON_INCLUDES
        ${THIRD_PARTY_JANSSON_DIR}/${ANDROID_ABI}/include)
    set(MBP_JANSSON_LIBRARIES
        ${THIRD_PARTY_JANSSON_DIR}/${ANDROID_ABI}/lib/libjansson.a)
else()
    find_path(MBP_JANSSON_INCLUDES jansson.h)
    find_library(MBP_JANSSON_LIBRARIES NAMES jansson libjansson)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(
        JANSSON DEFAULT_MSG MBP_JANSSON_LIBRARIES MBP_JANSSON_INCLUDES
    )
    if(NOT JANSSON_FOUND)
        message(FATAL_ERROR "jansson library was not found")
    endif()
endif()
