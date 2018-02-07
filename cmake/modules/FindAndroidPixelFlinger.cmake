# Find the pixelflinger include directory and library
#
# ANDROID_PIXELFLINGER_INCLUDE_DIR - Where to find <pixelflinger/pixelflinger.h>
# ANDROID_PIXELFLINGER_LIBRARIES   - List of pixelflinger libraries
# ANDROID_PIXELFLINGER_FOUND       - True if pixelflinger found

# Find include directory
find_path(ANDROID_PIXELFLINGER_INCLUDE_DIR NAMES pixelflinger/pixelflinger.h)

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

    add_library(AndroidSystemCore::PixelFlinger UNKNOWN IMPORTED)
    set_target_properties(
        AndroidSystemCore::PixelFlinger
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${ANDROID_PIXELFLINGER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ANDROID_PIXELFLINGER_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "AndroidSystemCore::Cutils;AndroidSystemCore::Utils"
    )
endif()

mark_as_advanced(ANDROID_PIXELFLINGER_INCLUDE_DIR ANDROID_PIXELFLINGER_LIBRARY)
