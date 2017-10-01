# Find the liblog include directory and library
#
# ANDROID_LIBLOG_INCLUDE_DIR - Where to find <log/log.h>
# ANDROID_LIBLOG_LIBRARIES   - List of liblog libraries
# ANDROID_LIBLOG_FOUND       - True if liblog found

# Find include directory
find_path(ANDROID_LIBLOG_INCLUDE_DIR NAMES log/log.h)

# Find library
find_library(ANDROID_LIBLOG_LIBRARY NAMES log liblog)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ANDROID_LIBLOG
    DEFAULT_MSG
    ANDROID_LIBLOG_INCLUDE_DIR
    ANDROID_LIBLOG_LIBRARY
)

if(ANDROID_LIBLOG_FOUND)
    set(ANDROID_LIBLOG_LIBRARIES ${ANDROID_LIBLOG_LIBRARY})

    add_library(AndroidSystemCore::Log UNKNOWN IMPORTED)
    set_target_properties(
        AndroidSystemCore::Log
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${ANDROID_LIBLOG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ANDROID_LIBLOG_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(ANDROID_LIBLOG_INCLUDE_DIR ANDROID_LIBLOG_LIBRARY)
