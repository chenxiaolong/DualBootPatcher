# Find the LZ4 include directory and library
#
# LZ4_INCLUDE_DIR - Where to find <lz4.h>
# LZ4_LIBRARIES   - List of lz4 libraries
# LZ4_FOUND       - True if lz4 found

# Find include directory
find_path(LZ4_INCLUDE_DIR NAMES lz4.h)

# Find library
find_library(LZ4_LIBRARY NAMES lz4 liblz4)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LZ4 DEFAULT_MSG LZ4_INCLUDE_DIR LZ4_LIBRARY
)

if(LZ4_FOUND)
    set(LZ4_LIBRARIES ${LZ4_LIBRARY})
endif()

mark_as_advanced(LZ4_INCLUDE_DIR LZ4_LIBRARY)
