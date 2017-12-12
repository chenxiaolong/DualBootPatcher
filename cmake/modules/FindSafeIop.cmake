# Find the safe-iop include directory and library
#
# SAFE_IOP_INCLUDE_DIR - Where to find <safe_iop.h>
# SAFE_IOP_LIBRARIES   - List of safe-iop libraries
# SAFE_IOP_FOUND       - True if safe-iop found

# Find include directory
find_path(SAFE_IOP_INCLUDE_DIR NAMES safe_iop.h)

# Find library
find_library(SAFE_IOP_LIBRARY NAMES safe_iop libsafe_iop)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    SAFE_IOP DEFAULT_MSG SAFE_IOP_INCLUDE_DIR SAFE_IOP_LIBRARY
)

if(SAFE_IOP_FOUND)
    set(SAFE_IOP_LIBRARIES ${SAFE_IOP_LIBRARY})

    add_library(SafeIop::SafeIop UNKNOWN IMPORTED)
    set_target_properties(
        SafeIop::SafeIop
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${SAFE_IOP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SAFE_IOP_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(SAFE_IOP_INCLUDE_DIR SAFE_IOP_LIBRARY)
