# Find the procps include directory and library
#
# PROCPS_INCLUDE_DIR - Where to find <proc/procps.h>
# PROCPS_LIBRARIES   - List of procps libraries
# PROCPS_FOUND       - True if procps found

# Find include directory
find_path(PROCPS_INCLUDE_DIR NAMES proc/procps.h)

# Find library
find_library(PROCPS_LIBRARY NAMES procps libprocps)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    PROCPS DEFAULT_MSG PROCPS_INCLUDE_DIR PROCPS_LIBRARY
)

if(PROCPS_FOUND)
    set(PROCPS_LIBRARIES ${PROCPS_LIBRARY})

    add_library(Procps::Procps UNKNOWN IMPORTED)
    set_target_properties(
        Procps::Procps
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${PROCPS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PROCPS_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(PROCPS_INCLUDE_DIR PROCPS_LIBRARY)
