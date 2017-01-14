# Find the jansson include directory and library
#
# JANSSON_INCLUDE_DIR - Where to find <jansson.h>
# JANSSON_LIBRARIES   - List of jansson libraries
# JANSSON_FOUND       - True if jansson found

# Find include directory
find_path(JANSSON_INCLUDE_DIR NAMES jansson.h)

# Find library
find_library(JANSSON_LIBRARY NAMES jansson libjansson)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    JANSSON DEFAULT_MSG JANSSON_INCLUDE_DIR JANSSON_LIBRARY
)

if(JANSSON_FOUND)
    set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
endif()

mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARY)
