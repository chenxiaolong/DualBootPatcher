# Find the libutils include directory and library
#
# ANDROID_LIBUTILS_INCLUDE_DIR - Where to find <utils/Log.h>
# ANDROID_LIBUTILS_LIBRARIES   - List of libutils libraries
# ANDROID_LIBUTILS_FOUND       - True if libutils found

# Find include directory
find_path(ANDORID_LIBUTILS_INCLUDE_DIR NAMES utils/Log.h)

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
endif()

mark_as_advanced(ANDROID_LIBUTILS_INCLUDE_DIR ANDROID_LIBUTILS_LIBRARY)
