# Find the libcutils include directory and library
#
# ANDROID_LIBCUTILS_INCLUDE_DIR - Where to find <cutils/log.h>
# ANDROID_LIBCUTILS_LIBRARIES   - List of libcutils libraries
# ANDROID_LIBCUTILS_FOUND       - True if libcutils found

# Find include directory
find_path(ANDORID_LIBCUTILS_INCLUDE_DIR NAMES cutils/log.h)

# Find library
find_library(ANDROID_LIBCUTILS_LIBRARY NAMES cutils libcutils)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ANDROID_LIBCUTILS
    DEFAULT_MSG
    ANDROID_LIBCUTILS_INCLUDE_DIR
    ANDROID_LIBCUTILS_LIBRARY
)

if(ANDROID_LIBCUTILS_FOUND)
    set(ANDROID_LIBCUTILS_LIBRARIES ${ANDROID_LIBCUTILS_LIBRARY})
endif()

mark_as_advanced(ANDROID_LIBCUTILS_INCLUDE_DIR ANDROID_LIBCUTILS_LIBRARY)
