# Find the pixelflinger include directory and library
#
# ANDROID_PIXELFLINGER_INCLUDE_DIR - Where to find <pixelflinger/pixelflinger.h>
# ANDROID_PIXELFLINGER_LIBRARIES   - List of pixelflinger libraries
# ANDROID_PIXELFLINGER_FOUND       - True if pixelflinger found

# Find include directory
find_path(ANDORID_PIXELFLINGER_INCLUDE_DIR NAMES pixelflinger/pixelflinger.h)

# Find library
find_library(ANDROID_PIXELFLINGER_LIBRARY NAMES pixelflinger libpixelflinger)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ANDROID_PIXELFLINGER
    DEFAULT_MSG
    ANDROID_PIXELFLINGER_INCLUDE_DIR
    ANDROID_PIXELFLINGER_LIBRARY
)

if(ANDROID_PIXELFLINGER_FOUND)
    set(ANDROID_PIXELFLINGER_LIBRARIES ${ANDROID_PIXELFLINGER_LIBRARY})
endif()

mark_as_advanced(ANDROID_PIXELFLINGER_INCLUDE_DIR ANDROID_PIXELFLINGER_LIBRARY)
