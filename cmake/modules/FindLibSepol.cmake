# Find the libsepol include directory and library
#
# LIBSEPOL_INCLUDE_DIR - Where to find <sepol/sepol.h>
# LIBSEPOL_LIBRARIES   - List of libsepol libraries
# LIBSEPOL_FOUND       - True if libsepol found

# Find include directory
find_path(LIBSEPOL_INCLUDE_DIR NAMES sepol/sepol.h)

# Find library
find_library(LIBSEPOL_LIBRARY NAMES sepol libsepol)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LIBSEPOL DEFAULT_MSG LIBSEPOL_INCLUDE_DIR LIBSEPOL_LIBRARY
)

if(LIBSEPOL_FOUND)
    set(LIBSEPOL_LIBRARIES ${LIBSEPOL_LIBRARY})

    add_library(LibSepol::LibSepol UNKNOWN IMPORTED)
    set_target_properties(
        LibSepol::LibSepol
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${LIBSEPOL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSEPOL_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(LIBSEPOL_INCLUDE_DIR LIBSEPOL_LIBRARY)
