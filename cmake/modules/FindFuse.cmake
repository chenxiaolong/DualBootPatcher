# Find the fuse include directory and library
#
# FUSE_INCLUDE_DIR - Where to find <fuse.h>
# FUSE_LIBRARIES   - List of fuse libraries
# FUSE_FOUND       - True if fuse found

# Find include directory
find_path(FUSE_INCLUDE_DIR NAMES fuse.h)

# Find library
find_library(FUSE_LIBRARY NAMES fuse libfuse)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    FUSE DEFAULT_MSG FUSE_INCLUDE_DIR FUSE_LIBRARY
)

if(FUSE_FOUND)
    set(FUSE_LIBRARIES ${FUSE_LIBRARY})

    add_library(Fuse::Fuse UNKNOWN IMPORTED)
    set_target_properties(
        Fuse::Fuse
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${FUSE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FUSE_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(FUSE_INCLUDE_DIR FUSE_LIBRARY)
