# Find the libdrm include directory and library
#
# LIBDRM_INCLUDE_DIR - Where to find <libdrm/drm.h>
# LIBDRM_LIBRARIES   - List of libdrm libraries
# LIBDRM_FOUND       - True if libdrm found

# Find include directory
find_path(LIBDRM_INCLUDE_DIR NAMES drm/drm.h)

# Find library
find_library(LIBDRM_LIBRARY NAMES drm libdrm)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LIBDRM DEFAULT_MSG LIBDRM_INCLUDE_DIR LIBDRM_LIBRARY
)

if(LIBDRM_FOUND)
    set(LIBDRM_LIBRARIES ${LIBDRM_LIBRARY})

    add_library(LibDrm::LibDrm UNKNOWN IMPORTED)
    set_target_properties(
        LibDrm::LibDrm
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${LIBDRM_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INCLUDE_DIR};${LIBDRM_INCLUDE_DIR}/drm"
    )
endif()

mark_as_advanced(LIBDRM_INCLUDE_DIR LIBDRM_LIBRARY)
