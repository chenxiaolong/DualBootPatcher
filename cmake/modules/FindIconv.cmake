# Find the iconv include directory and library
#
# ICONV_INCLUDE_DIR - Where to find <iconv.h>
# ICONV_LIBRARIES   - List of iconv libraries
# ICONV_FOUND       - True if iconv found

# Find include directory
find_path(ICONV_INCLUDE_DIR NAMES iconv.h)

# Find library
find_library(ICONV_LIBRARY NAMES iconv libiconv)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ICONV DEFAULT_MSG ICONV_INCLUDE_DIR ICONV_LIBRARY
)

if(ICONV_FOUND)
    set(ICONV_LIBRARIES ${ICONV_LIBRARY})

    add_library(Iconv::Iconv UNKNOWN IMPORTED)
    set_target_properties(
        Iconv::Iconv
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${ICONV_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ICONV_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(ICONV_INCLUDE_DIR ICONV_LIBRARY)
