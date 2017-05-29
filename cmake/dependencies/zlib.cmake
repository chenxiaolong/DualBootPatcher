find_package(ZLIB REQUIRED)

set(MBP_ZLIB_INCLUDES ${ZLIB_INCLUDE_DIRS})
set(MBP_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})

if(ANDROID AND ANDROID_UNIFIED_HEADERS
        AND NOT ${ZLIB_INCLUDE_DIR} MATCHES "/sysroot/usr/include$")
    message(FATAL_ERROR "zlib.h was found in legacy directory instead of unified. Please clean your build directory.")
endif()
