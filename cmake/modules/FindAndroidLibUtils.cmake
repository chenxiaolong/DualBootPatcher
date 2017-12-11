# Find the libutils include directory and library
#
# ANDROID_LIBUTILS_INCLUDE_DIR - Where to find <utils/Log.h>
# ANDROID_LIBUTILS_LIBRARIES   - List of libutils libraries
# ANDROID_LIBUTILS_FOUND       - True if libutils found

# Find include directory
find_path(ANDROID_LIBUTILS_INCLUDE_DIR NAMES utils/Log.h)

# Find library
find_library(ANDROID_LIBUTILS_LIBRARY NAMES utils libutils)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ANDROID_LIBUTILS
    DEFAULT_MSG
    ANDROID_LIBUTILS_INCLUDE_DIR
    ANDROID_LIBUTILS_LIBRARY
)

if(ANDROID_LIBUTILS_FOUND)
    set(ANDROID_LIBUTILS_LIBRARIES ${ANDROID_LIBUTILS_LIBRARY})

    add_library(AndroidSystemCore::Utils UNKNOWN IMPORTED)
    set_target_properties(
        AndroidSystemCore::Utils
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${ANDROID_LIBUTILS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ANDROID_LIBUTILS_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(ANDROID_LIBUTILS_INCLUDE_DIR ANDROID_LIBUTILS_LIBRARY)
